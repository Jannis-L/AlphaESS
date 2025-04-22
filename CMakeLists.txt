# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(AlphaESS C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()



set(WIZNET_DIR ${CMAKE_SOURCE_DIR}/libraries/ioLibrary_Driver)
add_subdirectory(${CMAKE_SOURCE_DIR}/libraries)
set(PORT_DIR ${CMAKE_SOURCE_DIR}/port)
add_subdirectory(${PORT_DIR})



add_executable(AlphaESS
    src/alphaESS.c
    src/httpClient.c
    src/main.c
)



pico_set_program_name(AlphaESS "AlphaESS")
pico_set_program_version(AlphaESS "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(AlphaESS 1)
pico_enable_stdio_usb(AlphaESS 0)

# Add the standard library to the build
target_link_libraries(AlphaESS PRIVATE
    pico_stdlib
    pico_mbedtls
    hardware_spi
    hardware_dma
    ETHERNET_FILES
    IOLIBRARY_FILES
    DHCP_FILES
    DNS_FILES
    SNTP_FILES
)

# Add the standard include files to the build
target_include_directories(AlphaESS PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

pico_add_extra_outputs(AlphaESS)

