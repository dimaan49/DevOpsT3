#include <catch2/catch_test_macros.hpp>

#include "../src/models/UserRepository.h"

TEST_CASE("Password hashing is deterministic", "[password]")
{
    const QString password = "secret123";
    const QString hash1 = auctionhub::models::UserRepository::hashPassword(password);
    const QString hash2 = auctionhub::models::UserRepository::hashPassword(password);

    REQUIRE(hash1 == hash2);
    REQUIRE_FALSE(hash1.isEmpty());
}

TEST_CASE("Different passwords produce different hashes", "[password]")
{
    const QString hash1 = auctionhub::models::UserRepository::hashPassword("secret123");
    const QString hash2 = auctionhub::models::UserRepository::hashPassword("secret124");

    REQUIRE(hash1 != hash2);
}

TEST_CASE("Hash is SHA-256 hex (64 chars)", "[password]")
{
    const QString hash = auctionhub::models::UserRepository::hashPassword("secret123");

    REQUIRE(hash.length() == 64);
    for (QChar c : hash) {
        REQUIRE(((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')));
    }
}

TEST_CASE("Known hash value matches", "[password]")
{
    // echo -n "secret123" | sha256sum
    const QString expected =
        "fcf730b6d95236ecd3c9fc2d92d7b6b2bb061514961aec041d6c7a7192f592e4";

    // Замените на реальное значение:
    // echo -n "secret123" | sha256sum
    const QString hash = auctionhub::models::UserRepository::hashPassword("secret123");
    REQUIRE(hash == expected);

    // TODO: подставить реальное значение после проверки
}
