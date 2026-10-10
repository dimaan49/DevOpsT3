#include <catch2/catch_test_macros.hpp>

#include "DbTestFixture.h"
#include "models/UserRepository.h"
#include "models/AuctionRepository.h"
#include "models/LotRepository.h"

using namespace auctionhub;

namespace {

qint64 createSeller()
{
    const QString hash = models::UserRepository::hashPassword("secret");
    return models::UserRepository::create("seller@test.local", hash, "seller", true);
}

qint64 createAuction(qint64 sellerId)
{
    return models::AuctionRepository::create(
        sellerId, "Test auction", "Description", 100.0, 1000.0);
}

} // namespace

TEST_CASE("AuctionRepository: create and find", "[db][auction]")
{
    REQUIRE_TEST_DB_CONNECTED();

    const qint64 sellerId = createSeller();
    const qint64 auctionId = createAuction(sellerId);

    REQUIRE(auctionId > 0);

    const auto auction = models::AuctionRepository::findById(auctionId);
    REQUIRE(auction.has_value());
    REQUIRE(auction->sellerId == sellerId);
    REQUIRE(auction->title == "Test auction");
    REQUIRE(auction->step == 100.0);
    REQUIRE(auction->startPrice == 1000.0);
    REQUIRE(auction->status == "draft");
}

TEST_CASE("AuctionRepository: updateStatus", "[db][auction]")
{
    REQUIRE_TEST_DB_CONNECTED();

    const qint64 sellerId = createSeller();
    const qint64 auctionId = createAuction(sellerId);

    REQUIRE(models::AuctionRepository::updateStatus(auctionId, "active"));

    const auto auction = models::AuctionRepository::findById(auctionId);
    REQUIRE(auction->status == "active");
}

TEST_CASE("LotRepository: create and find by auction", "[db][lot]")
{
    REQUIRE_TEST_DB_CONNECTED();

    const qint64 sellerId = createSeller();
    const qint64 auctionId = createAuction(sellerId);

    const qint64 lotId = models::LotRepository::create(
        auctionId, "Test lot", "Description", 500.0);
    REQUIRE(lotId > 0);

    const auto lot = models::LotRepository::findById(lotId);
    REQUIRE(lot.has_value());
    REQUIRE(lot->auctionId == auctionId);
    REQUIRE(lot->startPrice == 500.0);

    const auto lots = models::LotRepository::findByAuction(auctionId);
    REQUIRE(lots.size() == 1);
    REQUIRE(lots[0].id == lotId);
}

TEST_CASE("AuctionRepository: cannot update status of missing auction", "[db][auction][boundary]")
{
    REQUIRE_TEST_DB_CONNECTED();

    REQUIRE_FALSE(models::AuctionRepository::updateStatus(99999, "active"));
}

TEST_CASE("AuctionRepository: invalid status rejected by CHECK", "[db][auction][boundary]")
{
    REQUIRE_TEST_DB_CONNECTED();

    const QString hash = models::UserRepository::hashPassword("secret");
    const qint64 sellerId = models::UserRepository::create("s2@test.local", hash, "seller", true);
    const qint64 auctionId = models::AuctionRepository::create(
        sellerId, "A", "", 100.0, 1000.0);

    REQUIRE(models::AuctionRepository::findById(auctionId).has_value());

    // CHECK (status IN (...)) не пропустит невалидный статус
    REQUIRE_FALSE(models::AuctionRepository::updateStatus(auctionId, "invalid_status"));

    // Проверяем, что статус не изменился
    const auto auction = models::AuctionRepository::findById(auctionId);
    REQUIRE(auction->status == "draft");
}

TEST_CASE("AuctionRepository: create with zero step fails", "[db][auction][boundary]")
{
    REQUIRE_TEST_DB_CONNECTED();

    const QString hash = models::UserRepository::hashPassword("secret");
    const qint64 sellerId = models::UserRepository::create("s3@test.local", hash, "seller", true);

    // В AuctionRepository::create есть проверка step <= 0.0 → return -1
    REQUIRE(models::AuctionRepository::create(sellerId, "A", "", 0.0, 1000.0) == -1);
    REQUIRE(models::AuctionRepository::create(sellerId, "A", "", -100.0, 1000.0) == -1);
}

TEST_CASE("AuctionRepository: create with zero start_price fails", "[db][auction][boundary]")
{
    REQUIRE_TEST_DB_CONNECTED();

    const QString hash = models::UserRepository::hashPassword("secret");
    const qint64 sellerId = models::UserRepository::create("s4@test.local", hash, "seller", true);

    REQUIRE(models::AuctionRepository::create(sellerId, "A", "", 100.0, 0.0) == -1);
    REQUIRE(models::AuctionRepository::create(sellerId, "A", "", 100.0, -100.0) == -1);
}

TEST_CASE("LotRepository: create with zero start_price fails", "[db][lot][boundary]")
{
    REQUIRE_TEST_DB_CONNECTED();

    const QString hash = models::UserRepository::hashPassword("secret");
    const qint64 sellerId = models::UserRepository::create("s5@test.local", hash, "seller", true);
    const qint64 auctionId = models::AuctionRepository::create(sellerId, "A", "", 100.0, 1000.0);

    REQUIRE(models::LotRepository::create(auctionId, "L", "", 0.0) == -1);
    REQUIRE(models::LotRepository::create(auctionId, "L", "", -50.0) == -1);
}


TEST_CASE("AuctionRepository: search by title substring", "[db][auction]")
{
    REQUIRE_TEST_DB_CONNECTED();

    const QString hash = models::UserRepository::hashPassword("secret");
    const qint64 sellerId = models::UserRepository::create("s-srch@test.local", hash, "seller", true);

    models::AuctionRepository::create(sellerId, "Antique watch", "", 100.0, 1000.0);
    models::AuctionRepository::create(sellerId, "Modern watch", "", 100.0, 1000.0);
    models::AuctionRepository::create(sellerId, "Old painting", "", 100.0, 1000.0);

    const auto watches = models::AuctionRepository::search("watch");
    REQUIRE(watches.size() == 2);

    const auto antique = models::AuctionRepository::search("antique");
    REQUIRE(antique.size() == 1);
    REQUIRE(antique[0].title == "Antique watch");

    const auto nothing = models::AuctionRepository::search("xyz");
    REQUIRE(nothing.empty());
}

TEST_CASE("AuctionRepository: search is case-insensitive", "[db][auction]")
{
    REQUIRE_TEST_DB_CONNECTED();

    const QString hash = models::UserRepository::hashPassword("secret");
    const qint64 sellerId = models::UserRepository::create("s-srch2@test.local", hash, "seller", true);

    models::AuctionRepository::create(sellerId, "Antique Watch", "", 100.0, 1000.0);

    REQUIRE(models::AuctionRepository::search("watch").size() == 1);
    REQUIRE(models::AuctionRepository::search("WATCH").size() == 1);
    REQUIRE(models::AuctionRepository::search("WaTcH").size() == 1);
}

TEST_CASE("AuctionRepository: empty search returns all", "[db][auction]")
{
    REQUIRE_TEST_DB_CONNECTED();

    const QString hash = models::UserRepository::hashPassword("secret");
    const qint64 sellerId = models::UserRepository::create("s-srch3@test.local", hash, "seller", true);

    models::AuctionRepository::create(sellerId, "A", "", 100.0, 1000.0);
    models::AuctionRepository::create(sellerId, "B", "", 100.0, 1000.0);

    const auto all = models::AuctionRepository::search("");
    REQUIRE(all.size() == 2);
}
