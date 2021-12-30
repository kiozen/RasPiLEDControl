#ifndef SRC_LOG_HPP
#define SRC_LOG_HPP

#include <string>

class Log
{
protected:
    Log(const std::string& tag);

    void E(const std::string& msg);
    void I(const std::string& msg);
    void D(const std::string& msg);

private:
    void print(const std::string& level, const std::string& msg);
    std::string tag_;
};

#endif // SRC_LOG_HPP
