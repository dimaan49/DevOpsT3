#pragma once

#include "Review.h"

#include <optional>
#include <vector>

namespace auctionhub::models {

enum class ReviewValidationResult {
    Ok,
    AuctionNotFound,
    AuctionNotFinished,
    BidderDidNotBid,
    AlreadyReviewed
};

class ReviewRepository {
public:
    struct SellerRating {
        double average = 0.0;
        int count = 0;
    };

    static ReviewValidationResult validateNewReview(qint64 bidderId, qint64 auctionId);

    static qint64 create(qint64 bidderId, qint64 sellerId, qint64 auctionId, int rating,
                         const QString& comment);

    static std::optional<Review> findById(qint64 id);

    static std::vector<Review> findBySeller(qint64 sellerId);

    static SellerRating ratingForSeller(qint64 sellerId);

    static bool update(qint64 reviewId, qint64 userId, int newRating, const QString& newComment);

    static bool remove(qint64 reviewId, qint64 userId, const QString& userRole);
};

}  // namespace auctionhub::models
