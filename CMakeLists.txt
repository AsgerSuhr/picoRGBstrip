# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH "/home/asger/Documents/pico/pico-sdk")

# set(PICO_DEOPTIMIZED_DEBUG 1) 

set(PICO_BOARD pico_w CACHE STRING "Board type")
set(PICO_DEOPTIMIZED_DEBUG 1)

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(HA_mqtt C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(${PROJECT_NAME} main.c lib/apa102.c)

pico_set_program_name(${PROJECT_NAME} "HA_mqtt")
pico_set_program_version(${PROJECT_NAME} "0.1")

pico_enable_stdio_uart(${PROJECT_NAME} 1)
pico_enable_stdio_usb(${PROJECT_NAME} 0)

link_libraries(
    pico_stdlib
    hardware_pio
  )

add_subdirectory(lib)

# Add the standard library to the build
# target_link_libraries(${PROJECT_NAME}
#         pico_stdlib)

# Add the standard include files to the build
target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts or any other standard includes, if required
)

# Add any user requested libraries
target_link_libraries(${PROJECT_NAME}
        lib
        hardware_adc
        pico_cyw43_arch_lwip_threadsafe_background
        pico_lwip_mbedtls
        pico_mbedtls
        pico_lwip_mqtt
        tinyusb_device
        tinyusb_board
        pico_unique_id_headers
        )

pico_add_extra_outputs(${PROJECT_NAME})
