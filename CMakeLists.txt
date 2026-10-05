cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# C++ 设置：分层后的业务与驱动层都是 C++（工具链里已带 -fno-exceptions -fno-rtti）
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME STM32_LEARN)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for C, C++ and ASM languages
enable_language(C CXX ASM)

# 分层（依赖方向单向向下）：App -> Bsp -> Drv -> HAL
#   App/  业务逻辑（硬件无关）
#   Bsp/  本板专有：对象图与接线、板上自检
#   Drv/  通用驱动：可跨板复用；HAL 封装（GPIO/时间/中断）
add_executable(${CMAKE_PROJECT_NAME}
        # --- 应用层 App：业务与演示 ---
        App/app_main.cpp
        App/app_demo.cpp
        App/app_bomb.cpp
        # --- 板级层 Bsp：本板对象图与自检 ---
        Bsp/board.cpp
        Bsp/wiretest.cpp
        # --- 驱动层 Drv：通用驱动 ---
        Drv/led.cpp
        Drv/motor.cpp
        Drv/buzzer.cpp
        Drv/light.cpp
        Drv/bsp_oled.c          # SSD1306 驱动保留为 C（单实例、纯过程式，无 C++ 收益）
        # --- 头文件（IDE 里便于索引，不参与编译） ---
        App/app_main.h
        App/app_demo.hpp
        App/app_bomb.hpp
        Bsp/board.hpp
        Bsp/wiretest.hpp
        Drv/gpio.hpp
        Drv/time.hpp
        Drv/irq.hpp
        Drv/led.hpp
        Drv/motor.hpp
        Drv/buzzer.hpp
        Drv/light.hpp
        Drv/bsp_oled.h
        Drv/bsp_oledfont.h)

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
)

# Add include paths（每一层一个目录，头文件按裸文件名包含）
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    App      # 应用层
    Bsp      # 板级层
    Drv      # 驱动层
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
