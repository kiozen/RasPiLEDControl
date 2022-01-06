#include "animation.hpp"

#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <openssl/md5.h>

#include "controller.hpp"


constexpr const char* kAnimationPath = "/home/pi";

Animation::Animation(asio::io_context& io, Controller& parent)
    : Power(module_e::animation, parent)
    , Log("animation")
    , controller_(parent)
    , timer_(io)
{
    for(const auto& p : std::filesystem::directory_iterator(kAnimationPath))
    {
        const std::filesystem::path& path = p.path();

        if(path.extension() != ".json")
        {
            continue;
        }

        try
        {
            std::ifstream file(path);
            nlohmann::json animation;
            file >> animation;



            const std::string& name = animation["name"];
            const std::string& description = animation["description"];
            const std::string& content = name + description;

            unsigned char buffer[MD5_DIGEST_LENGTH];
            MD5((unsigned char*) content.c_str(), content.size(), buffer);

            std::string hash;
            for (std::size_t i = 0; i < MD5_DIGEST_LENGTH; ++i)
            {
                hash += "0123456789ABCDEF"[buffer[i] / 16];
                hash += "0123456789ABCDEF"[buffer[i] % 16];
            }

            animations_[hash] = {name, description, path};

            D(fmt::format("Found animation '{} {}'.", animation["name"], hash));
        }
        catch(const nlohmann::json::exception& e)
        {
            E(fmt::format("Parsing animation {} failed: {}", path.c_str(), e.what()));
        }
    }
}

Animation::~Animation()
{
}

bool Animation::SwitchOn()
{
    if(animation_.empty())
    {
        D("animation is empty");
        return false;
    }

    if(controller_.Clear() != WS2811_SUCCESS)
    {
        D("clear controller failed");
        return false;
    }

    index_ = 0;
    timer_.expires_at( std::chrono::steady_clock::now() + std::chrono::milliseconds(100));
    timer_.async_wait([this](const asio::error_code& error){
        OnAnimate(error);
    });

    return true;
}

void Animation::SwitchOff()
{
    timer_.cancel();
}


nlohmann::json Animation::GetAnimationInfo() const
{
    std::vector<nlohmann::json> infos;

    for( const auto& animation : animations_)
    {
        auto [hash, info] = animation;
        nlohmann::json json;
        json["name"] = info.name;
        json["description"] = info.desc;
        json["hash"] = hash;

        infos.push_back(json);
    }

    return nlohmann::json(infos);
}

void Animation::SetAnimation(const std::string& hash)
{
    if(animations_.count(hash) == 0)
    {
        animation_.clear();
        index_ = -1;
        return;
    }

    LoadAnimation(animations_[hash].path.c_str());
}

void Animation::LoadAnimation(const std::string& filename)
{
    I(fmt::format("Load animation {}", filename));
    std::ifstream ifs(filename);
    const auto& json_ = nlohmann::json::parse(ifs);
    animation_ = json_["data"].get<animation_t>();
    index_ = 0;
}

void Animation::OnAnimate(const asio::error_code& error)
{
    if(error)
    {
        E(fmt::format("Cyclic loop failed: {}", error.message()));
        controller_.SetPowerAnimation(false);
        return;
    }

    if (index_ == animation_.size())
    {
        controller_.SetPowerAnimation(false);
        return;
    }
    const auto& [time, matrix] = animation_[index_++];
    controller_.Render(matrix);
    timer_.expires_at( timer_.expiry() + std::chrono::milliseconds(time));
    timer_.async_wait([this](const asio::error_code& error){
        OnAnimate(error);
    });
}
