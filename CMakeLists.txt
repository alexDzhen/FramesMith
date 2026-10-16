cmake_minimum_required(VERSION 3.20)

project(framesmith
    VERSION 1.0.0
    DESCRIPTION "Builds Ethernet frames from operator-chosen fields"
    LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(FRAMESMITH_BUILD_TESTS "Build the unit tests" ON)
option(FRAMESMITH_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

add_library(framesmith_warnings INTERFACE)
if(MSVC)
    target_compile_options(framesmith_warnings INTERFACE /W4 /utf-8)
    target_compile_definitions(framesmith_warnings INTERFACE _CRT_SECURE_NO_WARNINGS)
    if(FRAMESMITH_WARNINGS_AS_ERRORS)
        target_compile_options(framesmith_warnings INTERFACE /WX)
    endif()
else()
    target_compile_options(framesmith_warnings INTERFACE
        -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wstrict-prototypes)
    if(FRAMESMITH_WARNINGS_AS_ERRORS)
        target_compile_options(framesmith_warnings INTERFACE -Werror)
    endif()
endif()

# The portable part: frame assembly, parsing, dump, pcap, CLI.
add_library(framesmith_core STATIC
    src/cli.c
    src/frame.c
    src/hexdump.c
    src/hexparse.c
    src/mac_fwd.c
    src/pcap.c)
add_library(framesmith::core ALIAS framesmith_core)
target_include_directories(framesmith_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(framesmith_core PRIVATE framesmith_warnings)

# The one platform-specific piece: sending a frame on the wire.
if(WIN32)
    target_sources(framesmith_core PRIVATE src/windows/sender_windows.c)
    target_compile_definitions(framesmith_core PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_sources(framesmith_core PRIVATE src/linux/sender_linux.c)
else()
    message(FATAL_ERROR "framesmith supports Linux and Windows; ${CMAKE_SYSTEM_NAME} has no sender backend yet")
endif()

add_executable(framesmith app/main.c)
target_compile_definitions(framesmith PRIVATE FRAMESMITH_VERSION="${PROJECT_VERSION}")
target_link_libraries(framesmith PRIVATE framesmith::core framesmith_warnings)
if(MINGW)
    target_link_options(framesmith PRIVATE -static)
endif()

if(FRAMESMITH_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

include(GNUInstallDirs)
install(TARGETS framesmith RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES README.md LICENSE DESTINATION ${CMAKE_INSTALL_DOCDIR})

string(TOLOWER "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}" FRAMESMITH_PLATFORM)
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${FRAMESMITH_PLATFORM}")
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE)
if(WIN32)
    set(CPACK_GENERATOR ZIP)
else()
    set(CPACK_GENERATOR TGZ)
endif()
include(CPack)
