#include <catch2/catch_test_macros.hpp>

#include "DbTestFixture.h"
#include "models/UserRepository.h"
#include "models/AuctionRepository.h"
#include "models/LotRepository.h"
#include "models/BidRepository.h"
#include "models/ReviewRepository.h"

using namespace auctionhub;
using models::ReviewValidationResult;

namespace {

struct Fixture {
    qint64 sellerId;
    qint64 bidderId;
    qint64 auctionId;
    qint64 lotId;

    Fixture() {
        const QString hash = models::UserRepository::hashPassword("secret");
        sellerId = models::UserRepository::create("seller@rev.test", hash, "seller", true);
        bidderId = models::UserRepository::create("bidder@rev.test", hash, "bidder", true);

        auctionId = models::AuctionRepository::create(
            sellerId, "A", "", 100.0, 1000.0);
        models::AuctionRepository::updateStatus(auctionId, "active");

        lotId = models::LotRepository::create(auctionId, "L", "", 1000.0);
        models::BidRepository::place(lotId, bidderId, 1100.0);

        models::AuctionRepository::updateStatus(auctionId, "finished");
    }
};

} // namespace

TEST_CASE("ReviewRepository: valid review after finished auction with bid", "[db][review]")
{
    REQUIRE_TEST_DB_CONNECTED();
    Fixture f;

    REQUIRE(models::ReviewRepository::validateNewReview(f.bidderId, f.auctionId)
            == ReviewValidationResult::Ok);

    const qint64 id = models::ReviewRepository::create(
        f.bidderId, f.sellerId, f.auctionId, 8, "Good seller");
    REQUIRE(id > 0);

    const auto review = models::ReviewRepository::findById(id);
    REQUIRE(review.has_value());
    REQUIRE(review->rating == 8);
    REQUIRE(review->comment == "Good seller");
    REQUIRE(review->bidderId == f.bidderId);
    REQUIRE(review->sellerId == f.sellerId);
    REQUIRE(review->auctionId == f.auctionId);
}

TEST_CASE("ReviewRepository: bidder who did not bid cannot review", "[db][review]")
{
    REQUIRE_TEST_DB_CONNECTED();

    const QString hash = models::UserRepository::hashPassword("secret");
    const qint64 sellerId = models::UserRepository::create("s2@rev.test", hash, "seller", true);
    const qint64 bidderId = models::UserRepository::create("b2@rev.test", hash, "bidder", true);

    const qint64 auctionId = models::AuctionRepository::create(
        sellerId, "A", "", 100.0, 1000.0);
    models::AuctionRepository::updateStatus(auctionId, "finished");

    REQUIRE(models::ReviewRepository::validateNewReview(bidderId, auctionId)
            == ReviewValidationResult::BidderDidNotBid);
}

TEST_CASE("ReviewRepository: cannot review not finished auction", "[db][review]")
{
    REQUIRE_TEST_DB_CONNECTED();

    const QString hash = models::UserRepository::hashPassword("secret");
    const qint64 sellerId = models::UserRepository::create("s3@rev.test", hash, "seller", true);
    const qint64 bidderId = models::UserRepository::create("b3@rev.test", hash, "bidder", true);

    const qint64 auctionId = models::AuctionRepository::create(
        sellerId, "A", "", 100.0, 1000.0);
    // статус draft

    REQUIRE(models::ReviewRepository::validateNewReview(bidderId, auctionId)
            == ReviewValidationResult::AuctionNotFinished);
}

TEST_CASE("ReviewRepository: cannot review missing auction", "[db][review]")
{
    REQUIRE_TEST_DB_CONNECTED();

    REQUIRE(models::ReviewRepository::validateNewReview(1, 99999)
            == ReviewValidationResult::AuctionNotFound);
}

TEST_CASE("ReviewRepository: cannot review same auction twice", "[db][review]")
{
    REQUIRE_TEST_DB_CONNECTED();
    Fixture f;

    models::ReviewRepository::create(f.bidderId, f.sellerId, f.auctionId, 8, "");

    REQUIRE(models::ReviewRepository::validateNewReview(f.bidderId, f.auctionId)
            == ReviewValidationResult::AlreadyReviewed);
}

TEST_CASE("ReviewRepository: rating for seller with reviews", "[db][review]")
{
    REQUIRE_TEST_DB_CONNECTED();
    Fixture f;

    models::ReviewRepository::create(f.bidderId, f.sellerId, f.auctionId, 8, "");

    const auto rating = models::ReviewRepository::ratingForSeller(f.sellerId);
    REQUIRE(rating.count == 1);
    REQUIRE(rating.average == 8.0);
}

TEST_CASE("ReviewRepository: rating for seller with no reviews", "[db][review]")
{
    REQUIRE_TEST_DB_CONNECTED();
    Fixture f;

    const auto rating = models::ReviewRepository::ratingForSeller(f.sellerId);
    REQUIRE(rating.count == 0);
    REQUIRE(rating.average == 0.0);
}

TEST_CASE("ReviewRepository: update by author succeeds", "[db][review]")
{
    REQUIRE_TEST_DB_CONNECTED();
    Fixture f;

    const qint64 id = models::ReviewRepository::create(
        f.bidderId, f.sellerId, f.auctionId, 5, "old");

    REQUIRE(models::ReviewRepository::update(id, f.bidderId, 9, "new"));

    const auto review = models::ReviewRepository::findById(id);
    REQUIRE(review->rating == 9);
    REQUIRE(review->comment == "new");
}

TEST_CASE("ReviewRepository: update by non-author fails", "[db][review]")
{
    REQUIRE_TEST_DB_CONNECTED();
    Fixture f;

    const qint64 id = models::ReviewRepository::create(
        f.bidderId, f.sellerId, f.auctionId, 5, "old");

    REQUIRE_FALSE(models::ReviewRepository::update(id, f.sellerId, 10, "hacked"));

    const auto review = models::ReviewRepository::findById(id);
    REQUIRE(review->rating == 5);
    REQUIRE(review->comment == "old");
}

TEST_CASE("ReviewRepository: delete by author succeeds", "[db][review]")
{
    REQUIRE_TEST_DB_CONNECTED();
    Fixture f;

    const qint64 id = models::ReviewRepository::create(
        f.bidderId, f.sellerId, f.auctionId, 8, "");

    REQUIRE(models::ReviewRepository::remove(id, f.bidderId, "bidder"));
    REQUIRE_FALSE(models::ReviewRepository::findById(id).has_value());
}

TEST_CASE("ReviewRepository: delete by moderator succeeds", "[db][review]")
{
    REQUIRE_TEST_DB_CONNECTED();
    Fixture f;

    const qint64 id = models::ReviewRepository::create(
        f.bidderId, f.sellerId, f.auctionId, 8, "");

    REQUIRE(models::ReviewRepository::remove(id, 0, "moderator"));
    REQUIRE_FALSE(models::ReviewRepository::findById(id).has_value());
}

TEST_CASE("ReviewRepository: delete by seller fails", "[db][review]")
{
    REQUIRE_TEST_DB_CONNECTED();
    Fixture f;

    const qint64 id = models::ReviewRepository::create(
        f.bidderId, f.sellerId, f.auctionId, 8, "");

    REQUIRE_FALSE(models::ReviewRepository::remove(id, f.sellerId, "seller"));
    REQUIRE(models::ReviewRepository::findById(id).has_value());
}

TEST_CASE("ReviewRepository: findBySeller returns reviews sorted by date", "[db][review]")
{
    REQUIRE_TEST_DB_CONNECTED();

    const QString hash = models::UserRepository::hashPassword("secret");
    const qint64 sellerId = models::UserRepository::create("s5@rev.test", hash, "seller", true);

    // Три отзыва от трёх разных bidder'ов
    for (int i = 0; i < 3; ++i) {
        const QString email = QString("b%1@rev.test").arg(i);
        const qint64 bidderId = models::UserRepository::create(email, hash, "bidder", true);

        const qint64 auctionId = models::AuctionRepository::create(
            sellerId, "A", "", 100.0, 1000.0);
        models::AuctionRepository::updateStatus(auctionId, "active");

        const qint64 lotId = models::LotRepository::create(auctionId, "L", "", 1000.0);
        models::BidRepository::place(lotId, bidderId, 1100.0);
        models::AuctionRepository::updateStatus(auctionId, "finished");

        models::ReviewRepository::create(bidderId, sellerId, auctionId, 5 + i, "");
    }

    const auto reviews = models::ReviewRepository::findBySeller(sellerId);
    REQUIRE(reviews.size() == 3);
}

TEST_CASE("ReviewRepository: rating averages multiple reviews", "[db][review]")
{
    REQUIRE_TEST_DB_CONNECTED();

    const QString hash = models::UserRepository::hashPassword("secret");
    const qint64 sellerId = models::UserRepository::create("s6@rev.test", hash, "seller", true);

    // Оценки 8, 9, 10 → среднее 9.0
    for (int i = 0; i < 3; ++i) {
        const QString email = QString("b%1@s6.test").arg(i);
        const qint64 bidderId = models::UserRepository::create(email, hash, "bidder", true);

        const qint64 auctionId = models::AuctionRepository::create(
            sellerId, "A", "", 100.0, 1000.0);
        models::AuctionRepository::updateStatus(auctionId, "active");

        const qint64 lotId = models::LotRepository::create(auctionId, "L", "", 1000.0);
        models::BidRepository::place(lotId, bidderId, 1100.0);
        models::AuctionRepository::updateStatus(auctionId, "finished");

        models::ReviewRepository::create(bidderId, sellerId, auctionId, 8 + i, "");
    }

    const auto rating = models::ReviewRepository::ratingForSeller(sellerId);
    REQUIRE(rating.count == 3);
    REQUIRE(rating.average == 9.0);
}
