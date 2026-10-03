cmake_minimum_required(VERSION 3.21)

project(AuctionHub
    VERSION 0.1.0
    DESCRIPTION "AuctionHub — приложение для проведения интернет-аукционов"
    LANGUAGES CXX
)

# --- Стандарт C++ ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- compile_commands.json для clangd / clang-tidy ---
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Тип сборки по умолчанию ---
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# --- Qt6 ---
find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Network
    Sql
    HttpServer
)
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.2
)

FetchContent_MakeAvailable(Catch2)

qt_standard_project_setup()

# --- Исполняемый файл сервера ---
add_subdirectory(src)

# --- Исполняемый файл сервера ---
qt_add_executable(auctionhub_server
    src/main.cpp
)

target_include_directories(auctionhub_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(auctionhub_server PRIVATE
    auctionhub_core
)

set_target_properties(auctionhub_server PROPERTIES
    WIN32_EXECUTABLE OFF
    MACOSX_BUNDLE OFF
)

# --- Тесты ---
enable_testing()
add_subdirectory(tests)
