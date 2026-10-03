add_executable(auctionhub_tests
    test_smoke.cpp
    test_jwt.cpp
    test_password.cpp
    test_user_repository.cpp
    test_auction_repository.cpp
    test_bid_repository.cpp
)

target_link_libraries(auctionhub_tests PRIVATE
    auctionhub_core
    Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(auctionhub_tests)

target_include_directories(auctionhub_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)
