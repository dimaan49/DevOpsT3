# --- Ядро приложения: всё, кроме main.cpp ---

add_library(auctionhub_core STATIC
    db/Database.cpp
    models/UserRepository.cpp
    models/AuctionRepository.cpp
    models/LotRepository.cpp
    models/BidRepository.cpp
    models/RevokedTokenRepository.cpp
	models/ReviewRepository.cpp
    server/jwt.cpp
    server/auth.cpp
    handlers/UserHandler.cpp
    handlers/AuctionHandler.cpp
    handlers/BidHandler.cpp
    handlers/AuthHandler.cpp
	handlers/ReviewHandler.cpp
)

target_include_directories(auctionhub_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(auctionhub_core PUBLIC
    Qt6::Core
    Qt6::Network
    Qt6::Sql
    Qt6::HttpServer
)
