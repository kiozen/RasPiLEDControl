find_package(ASIO REQUIRED)
find_package(WS2811 REQUIRED)
find_package(FMT REQUIRED)


STRING(TOLOWER ${PROJECT_NAME} APPLICATION_NAME)

set(SRC
    main.cpp
    controller.cpp
)

set(HDR
    controller.hpp
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
)
