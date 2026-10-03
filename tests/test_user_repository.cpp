#include <catch2/catch_test_macros.hpp>

#include "DbTestFixture.h"
#include "../src/models/UserRepository.h"

using namespace auctionhub;

TEST_CASE("UserRepository: create and find by email", "[db][user]")
{
    REQUIRE_TEST_DB_CONNECTED();

    const QString email = "test@example.com";
    const QString hash = models::UserRepository::hashPassword("secret123");

    const qint64 id = models::UserRepository::create(
        email, hash, "seller", true);

    REQUIRE(id > 0);

    const auto user = models::UserRepository::findByEmail(email);
    REQUIRE(user.has_value());
    REQUIRE(user->id == id);
    REQUIRE(user->email == email);
    REQUIRE(user->role == "seller");
    REQUIRE(user->ageConfirmed == true);
}

TEST_CASE("UserRepository: create with duplicate email fails", "[db][user]")
{
    REQUIRE_TEST_DB_CONNECTED();

    const QString email = "duplicate@example.com";
    const QString hash = models::UserRepository::hashPassword("secret");

    const qint64 id1 = models::UserRepository::create(
        email, hash, "seller", true);
    REQUIRE(id1 > 0);

    const qint64 id2 = models::UserRepository::create(
        email, hash, "bidder", true);
    REQUIRE(id2 == -2);  // emailExists → -2
}

TEST_CASE("UserRepository: findById returns nullopt for unknown id", "[db][user]")
{
    REQUIRE_TEST_DB_CONNECTED();

    const auto user = models::UserRepository::findById(99999);
    REQUIRE_FALSE(user.has_value());
}

TEST_CASE("UserRepository: verifyPassword", "[db][user]")
{
    REQUIRE_TEST_DB_CONNECTED();

    const QString email = "verify@example.com";
    const QString password = "secret123";
    const QString hash = models::UserRepository::hashPassword(password);

    models::UserRepository::create(email, hash, "seller", true);

    REQUIRE(models::UserRepository::verifyPassword(email, password).has_value());
    REQUIRE_FALSE(models::UserRepository::verifyPassword(email, "wrong").has_value());
    REQUIRE_FALSE(models::UserRepository::verifyPassword("nobody@example.com", "secret").has_value());
}

TEST_CASE("UserRepository: emailExists", "[db][user]")
{
    REQUIRE_TEST_DB_CONNECTED();

    const QString email = "exists@example.com";
    REQUIRE_FALSE(models::UserRepository::emailExists(email));

    models::UserRepository::create(email, "hash", "seller", true);
    REQUIRE(models::UserRepository::emailExists(email));
}
