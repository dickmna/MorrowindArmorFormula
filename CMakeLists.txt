cmake_minimum_required(VERSION 3.24)

project(
    MorrowindArmorFormula
    VERSION 1.0.3
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(CommonLibSSE CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(rapidcsv CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(xbyak CONFIG REQUIRED)

set(headers
    src/ArmorFormula.h
    src/Hooks.h
    src/PCH.h
    src/Settings.h
)

set(sources
    src/ArmorFormula.cpp
    src/Hooks.cpp
    src/PCH.cpp
    src/Plugin.cpp
    src/Settings.cpp
)

add_library(${PROJECT_NAME} SHARED ${headers} ${sources})

target_precompile_headers(${PROJECT_NAME} PRIVATE src/PCH.h)
target_include_directories(${PROJECT_NAME} PRIVATE src)
target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
)
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        CommonLibSSE::CommonLibSSE
        fmt::fmt-header-only
        rapidcsv::rapidcsv
        spdlog::spdlog_header_only
        xbyak::xbyak
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${headers} ${sources})

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION "Data/SKSE/Plugins"
    LIBRARY DESTINATION "Data/SKSE/Plugins"
)

install(
    FILES package/Data/SKSE/Plugins/MorrowindArmorFormula.toml
    DESTINATION "Data/SKSE/Plugins"
)
