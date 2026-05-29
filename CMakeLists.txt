cmake_minimum_required(VERSION 3.20)
project(libdeobf VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# --- Capstone (disassembler) ---
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(CAPSTONE capstone)
endif()
if(NOT CAPSTONE_FOUND)
    find_path(CAPSTONE_INCLUDE_DIRS capstone/capstone.h
        HINTS $ENV{CAPSTONE_DIR}/include
              ${CMAKE_SOURCE_DIR}/../external/capstone/include)
    find_library(CAPSTONE_LIBRARIES capstone
        HINTS $ENV{CAPSTONE_DIR}/lib
              ${CMAKE_SOURCE_DIR}/../external/capstone/lib)
    if(CAPSTONE_INCLUDE_DIRS AND CAPSTONE_LIBRARIES)
        set(CAPSTONE_FOUND TRUE)
    endif()
endif()

# FetchContent fallback: auto-download capstone if not found
if(NOT CAPSTONE_FOUND)
    message(STATUS "Capstone not found locally — fetching via FetchContent...")
    set(CAPSTONE_BUILD_STATIC ON CACHE BOOL "" FORCE)
    set(CAPSTONE_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    set(CAPSTONE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(CAPSTONE_BUILD_CSTOOL OFF CACHE BOOL "" FORCE)
    set(CAPSTONE_ARCHITECTURE_DEFAULT OFF CACHE BOOL "" FORCE)
    set(CAPSTONE_X86_SUPPORT ON CACHE BOOL "" FORCE)

    FetchContent_Declare(capstone
        GIT_REPOSITORY https://github.com/capstone-engine/capstone.git
        GIT_TAG        5.0.6
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(capstone)
    set(CAPSTONE_FOUND TRUE)
    set(CAPSTONE_FETCHED TRUE)
endif()

# --- Core library ---
file(GLOB_RECURSE LIB_SOURCES
    src/ir/*.cpp
    src/passes/*.cpp
    src/lifter/*.cpp
    src/memory/*.cpp
    src/eval/*.cpp
    src/lower/*.cpp
)

add_library(deobf STATIC ${LIB_SOURCES})
target_include_directories(deobf PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(CAPSTONE_FOUND)
    if(CAPSTONE_FETCHED)
        target_link_libraries(deobf PUBLIC capstone)
    else()
        target_include_directories(deobf PRIVATE ${CAPSTONE_INCLUDE_DIRS})
        target_link_libraries(deobf PRIVATE ${CAPSTONE_LIBRARIES})
    endif()
    target_compile_definitions(deobf PUBLIC DEOBF_HAS_CAPSTONE=1)
else()
    message(WARNING "Capstone not found — lifter will not compile. "
                    "Set CAPSTONE_DIR or install capstone.")
endif()

# --- CLI tool ---
add_executable(dehex_cli tools/cli_main.cpp)
target_link_libraries(dehex_cli PRIVATE deobf)

# --- pybind11 bindings ---
option(DEOBF_BUILD_PYTHON "Build Python bindings" OFF)
if(DEOBF_BUILD_PYTHON)
    find_package(pybind11 QUIET)
    if(pybind11_FOUND)
        pybind11_add_module(pydeobf bindings/python/pydeobf.cpp)
        target_link_libraries(pydeobf PRIVATE deobf)
    else()
        message(WARNING "pybind11 not found — Python bindings disabled.")
    endif()
endif()

# --- Tests ---
option(DEOBF_BUILD_TESTS "Build unit tests" ON)
if(DEOBF_BUILD_TESTS)
    enable_testing()
    file(GLOB TEST_SOURCES tests/*.cpp)
    foreach(TEST_SRC ${TEST_SOURCES})
        get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)
        add_executable(${TEST_NAME} ${TEST_SRC})
        target_link_libraries(${TEST_NAME} PRIVATE deobf)
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    endforeach()
endif()
