set(WS2811_FOUND FALSE)

if(TARGET WS2811::WS2811)
  set(WS2811_FOUND TRUE)
else()
  find_library(WS2811_LIB ws2811 PATHS /usr/lib /usr/local/lib)
  find_file(WS2811_INCLUDE ws2811/ws2811.h PATHS ${CMAKE_SYSROOT}/usr/include/ ${CMAKE_SYSROOT}/usr/local/include/)
  if(NOT WS2811_LIB OR NOT WS2811_INCLUDE)
    message(FATAL "WS2811 Library not found. Install it from https://github.com/jgarff/rpi_ws281x.")
  else()
    message(STATUS "WS2811 found ${WS2811_LIB}")
    add_library(WS2811::WS2811 INTERFACE IMPORTED GLOBAL)
    set_target_properties(WS2811::WS2811 PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SYSROOT}/usr/include"
        INTERFACE_LINK_LIBRARIES "ws2811"
    )
    set(WS2811_FOUND TRUE)
  endif()
endif()
