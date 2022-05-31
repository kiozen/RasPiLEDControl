find_package(ASIO REQUIRED)
find_package(WS2811 REQUIRED)
find_package(FMT REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(PalSigslot REQUIRED)


STRING(TOLOWER ${PROJECT_NAME} APPLICATION_NAME)

set(SRC
    alarm.cpp
    alarm.hpp
    animation.cpp
    animation.hpp
    controller.cpp
    controller.hpp
    fadeout.cpp
    fadeout.hpp
    i_module.cpp
    i_module.hpp
    light.cpp
    light.hpp
    log.cpp
    log.hpp
    main.cpp
    power.cpp
    power.hpp
    session.cpp
    session.hpp
    ws2811_control.cpp
    ws2811_control.hpp
)

set(PROJECT_SOURCES
    ${SRC}
)

add_executable(${APPLICATION_NAME}
    ${PROJECT_SOURCES}
)

target_compile_definitions(${APPLICATION_NAME} PUBLIC
    -DVER_MAJOR=${PROJECT_VERSION_MAJOR}
    -DVER_MINOR=${PROJECT_VERSION_MINOR}
    -DVER_STEP=${PROJECT_VERSION_PATCH}
    -DAPPLICATION_NAME=${PROJECT_NAME}
)

target_link_libraries(${APPLICATION_NAME}
    PUBLIC
    ASIO::ASIO
    WS2811::WS2811
    fmt::fmt
    OpenSSL::Crypto
    Pal::Sigslot
    stdc++fs
)
