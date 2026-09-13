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
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Network
    Sql
    HttpServer
)

qt_standard_project_setup()

# --- Исполняемый файл сервера ---
qt_add_executable(auctionhub_server
    src/main.cpp
	src/db/Database.h
	src/db/Database.cpp
    src/models/User.h
    src/models/UserRepository.h
    src/models/UserRepository.cpp
    src/models/Auction.h
    src/models/AuctionRepository.h
    src/models/AuctionRepository.cpp
    src/models/Lot.h
    src/models/LotRepository.h
    src/models/LotRepository.cpp
    src/models/Bid.h
    src/models/BidRepository.h
    src/models/BItRepository.cpp
    src/server/ApiResponse.h
    src/handlers/UserHandler.h
    src/handlers/UserHandler.cpp
    src/handlers/AuctionHandler.h
    src/handlers/AuctionHandler.cpp
    src/handlers/BidHandler.h
    src/handlers/BidHandler.cpp
    src/server/jwt.h
    src/server/jwt.cpp
    src/server/auth.h
    src/server/auth.cpp
    src/models/RevokedTokenRepository.h
    src/models/RevokedTokenRepository.cpp
    src/handlers/AuthHandler.h
    src/handlers/AuthHandler.cpp
)

target_include_directories(auctionhub_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(auctionhub_server PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Sql
    Qt6::HttpServer
)

set_target_properties(auctionhub_server PROPERTIES
    WIN32_EXECUTABLE OFF
    MACOSX_BUNDLE OFF
)
