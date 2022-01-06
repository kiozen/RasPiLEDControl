find_package(ASIO REQUIRED)
find_package(WS2811 REQUIRED)
find_package(FMT REQUIRED)
find_package(OpenSSL REQUIRED)


STRING(TOLOWER ${PROJECT_NAME} APPLICATION_NAME)

set(SRC
    main.cpp
    log.cpp
    controller.cpp
    session.cpp
    light.cpp
    alarm.cpp
    animation.cpp
    power.cpp
)

set(HDR
    log.hpp
    controller.hpp
    session.hpp
    light.hpp
    alarm.hpp
    animation.hpp
    power.hpp
)

set(PROJECT_SOURCES
    ${SRC}
    ${HDR}
)

add_executable(${APPLICATION_NAME}
    ${PROJECT_SOURCES}
)

target_link_libraries(${APPLICATION_NAME}
    PUBLIC
    ASIO::ASIO
    WS2811::WS2811
    fmt::fmt
    stdc++fs
    OpenSSL::Crypto
)
