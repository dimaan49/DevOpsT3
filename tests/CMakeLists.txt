# --- Тесты ---

add_executable(auctionhub_tests
    test_smoke.cpp
	test_jwt.cpp
	test_password.cpp
)

target_link_libraries(auctionhub_tests PRIVATE
    auctionhub_core
    Catch2::Catch2WithMain
)

# Регистрируем тесты в CTest
include(CTest)
include(Catch)
catch_discover_tests(auctionhub_tests)
