#include <catch2/catch_test_macros.hpp>

#include "DbTestFixture.h"
#include "../src/models/UserRepository.h"
#include "../src/models/AuctionRepository.h"
#include "../src/models/LotRepository.h"
#include "../src/models/BidRepository.h"

using namespace auctionhub;
using models::BidValidationResult;

namespace {

struct Fixture {
    qint64 sellerId;
    qint64 bidderId;
    qint64 auctionId;
    qint64 lotId;

    Fixture() {
        const QString hash = models::UserRepository::hashPassword("secret");
        sellerId = models::UserRepository::create("seller@bid.test", hash, "seller", true);
        bidderId = models::UserRepository::create("bidder@bid.test", hash, "bidder", true);
        auctionId = models::AuctionRepository::create(sellerId, "A", "", 100.0, 1000.0);
        models::AuctionRepository::updateStatus(auctionId, "active");
        lotId = models::LotRepository::create(auctionId, "L", "", 1000.0);
    }
};

} // namespace

TEST_CASE("BidRepository: first bid must be start_price + step", "[db][bid]")
{
    REQUIRE_TEST_DB_CONNECTED();
    Fixture f;

    // Ставок нет → current_price = start_price = 1000
    REQUIRE(models::BidRepository::currentPrice(f.lotId) == 1000.0);

    // 1100 = 1000 + 100 (шаг), ok
    REQUIRE(models::BidRepository::validate(f.lotId, 1100.0) == BidValidationResult::Ok);
}

TEST_CASE("BidRepository: bid equal to current fails", "[db][bid]")
{
    REQUIRE_TEST_DB_CONNECTED();
    Fixture f;

    REQUIRE(models::BidRepository::validate(f.lotId, 1000.0) == BidValidationResult::TooLow);
}

TEST_CASE("BidRepository: bid not multiple of step fails", "[db][bid]")
{
    REQUIRE_TEST_DB_CONNECTED();
    Fixture f;

    // 1050 = 1000 + 50 → не кратно 100
    REQUIRE(models::BidRepository::validate(f.lotId, 1050.0) == BidValidationResult::NotMultipleOfStep);
}

TEST_CASE("BidRepository: bid on inactive auction fails", "[db][bid]")
{
    REQUIRE_TEST_DB_CONNECTED();

    const QString hash = models::UserRepository::hashPassword("secret");
    const qint64 sellerId = models::UserRepository::create("s@x.test", hash, "seller", true);
    const qint64 auctionId = models::AuctionRepository::create(sellerId, "A", "", 100.0, 1000.0);
    // статус draft, не active
    const qint64 lotId = models::LotRepository::create(auctionId, "L", "", 1000.0);

    REQUIRE(models::BidRepository::validate(lotId, 1100.0) == BidValidationResult::AuctionNotActive);
}

TEST_CASE("BidRepository: bid on unknown lot fails", "[db][bid]")
{
    REQUIRE_TEST_DB_CONNECTED();

    REQUIRE(models::BidRepository::validate(99999, 1000.0) == BidValidationResult::LotNotFound);
}

TEST_CASE("BidRepository: place bid and check current price", "[db][bid]")
{
    REQUIRE_TEST_DB_CONNECTED();
    Fixture f;

    const qint64 bidId = models::BidRepository::place(f.lotId, f.bidderId, 1100.0);
    REQUIRE(bidId > 0);

    REQUIRE(models::BidRepository::currentPrice(f.lotId) == 1100.0);

    // Следующая ставка: 1100 + 100 = 1200
    REQUIRE(models::BidRepository::validate(f.lotId, 1200.0) == BidValidationResult::Ok);
    // 1150 — не кратна
    REQUIRE(models::BidRepository::validate(f.lotId, 1150.0) == BidValidationResult::NotMultipleOfStep);
    // 1100 — не выше текущей
    REQUIRE(models::BidRepository::validate(f.lotId, 1100.0) == BidValidationResult::TooLow);
}
TEST_CASE("BidRepository: boundary — exactly current + step", "[db][bid][boundary]")
{
    REQUIRE_TEST_DB_CONNECTED();
    Fixture f;

    // current = 1000, step = 100. Ровно 1100 — принимается.
    REQUIRE(models::BidRepository::validate(f.lotId, 1100.0) == BidValidationResult::Ok);
}

TEST_CASE("BidRepository: boundary — current + step - 1", "[db][bid][boundary]")
{
    REQUIRE_TEST_DB_CONNECTED();
    Fixture f;

    // 1099 = 1000 + 99, не кратно шагу
    REQUIRE(models::BidRepository::validate(f.lotId, 1099.0) == BidValidationResult::NotMultipleOfStep);
}

TEST_CASE("BidRepository: boundary — current - 1", "[db][bid][boundary]")
{
    REQUIRE_TEST_DB_CONNECTED();
    Fixture f;

    // 999 < 1000
    REQUIRE(models::BidRepository::validate(f.lotId, 999.0) == BidValidationResult::TooLow);
}

TEST_CASE("BidRepository: zero amount is too low", "[db][bid][boundary]")
{
    REQUIRE_TEST_DB_CONNECTED();
    Fixture f;

    REQUIRE(models::BidRepository::validate(f.lotId, 0.0) == BidValidationResult::TooLow);
}

TEST_CASE("BidRepository: negative amount is too low", "[db][bid][boundary]")
{
    REQUIRE_TEST_DB_CONNECTED();
    Fixture f;

    REQUIRE(models::BidRepository::validate(f.lotId, -100.0) == BidValidationResult::TooLow);
}

TEST_CASE("BidRepository: large multiple of step is ok", "[db][bid][boundary]")
{
    REQUIRE_TEST_DB_CONNECTED();
    Fixture f;

    // 1000 + 1000*10 = 11000, разница 10000 кратна 100
    REQUIRE(models::BidRepository::validate(f.lotId, 11000.0) == BidValidationResult::Ok);
}

TEST_CASE("BidRepository: place with zero amount returns -1", "[db][bid][boundary]")
{
    REQUIRE_TEST_DB_CONNECTED();
    Fixture f;

    // place проверяет validate внутри, возвращает -1 при невалидной ставке
    REQUIRE(models::BidRepository::place(f.lotId, f.bidderId, 0.0) == -1);
}

TEST_CASE("BidRepository: multiple bids increment current price", "[db][bid]")
{
    REQUIRE_TEST_DB_CONNECTED();
    Fixture f;

    REQUIRE(models::BidRepository::place(f.lotId, f.bidderId, 1100.0) > 0);
    REQUIRE(models::BidRepository::currentPrice(f.lotId) == 1100.0);

    REQUIRE(models::BidRepository::place(f.lotId, f.bidderId, 1200.0) > 0);
    REQUIRE(models::BidRepository::currentPrice(f.lotId) == 1200.0);

    REQUIRE(models::BidRepository::place(f.lotId, f.bidderId, 1300.0) > 0);
    REQUIRE(models::BidRepository::currentPrice(f.lotId) == 1300.0);
}
