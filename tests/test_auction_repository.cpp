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
