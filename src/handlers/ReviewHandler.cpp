#include "ReviewHandler.h"

#include "../server/ApiResponse.h"
#include "../server/auth.h"
#include "../models/AuctionRepository.h"
#include "../models/ReviewRepository.h"

#include <QHttpServerRequest>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonParseError>

namespace auctionhub::handlers {

namespace {

QJsonObject parseJsonBody(const QByteArray &body, QString &errorMessage)
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &err);
    if (err.error != QJsonParseError::NoError) {
        errorMessage = QString("Invalid JSON: %1").arg(err.errorString());
        return {};
    }
    if (!doc.isObject()) {
        errorMessage = "Request body must be a JSON object";
        return {};
    }
    return doc.object();
}

QJsonObject reviewToJson(const models::Review &r)
{
    QJsonObject obj;
    obj["id"]         = r.id;
    obj["bidder_id"]  = r.bidderId;
    obj["seller_id"]  = r.sellerId;
    obj["auction_id"] = r.auctionId;
    obj["rating"]     = r.rating;
    obj["comment"]    = r.comment;
    obj["created_at"] = r.createdAt.toString(Qt::ISODate);
    obj["updated_at"] = r.updatedAt.isValid()
                            ? r.updatedAt.toString(Qt::ISODate)
                            : QJsonValue();
    return obj;
}

QHttpServerResponse validationToResponse(models::ReviewValidationResult result)
{
    switch (result) {
    case models::ReviewValidationResult::Ok:
        return QHttpServerResponse(QHttpServerResponse::StatusCode::Ok);
    case models::ReviewValidationResult::AuctionNotFound:
        return api::notFound("Auction not found");
    case models::ReviewValidationResult::AuctionNotFinished:
        return api::conflict("Auction is not finished");
    case models::ReviewValidationResult::BidderDidNotBid:
        return api::forbidden("You did not place any bids in this auction");
    case models::ReviewValidationResult::AlreadyReviewed:
        return api::conflict("You have already reviewed this auction");
    }
    return api::serverError("Unknown validation result");
}

} // namespace

void ReviewHandler::registerRoutes(QHttpServer &server)
{
    // POST /api/reviews
    server.route("/api/reviews", QHttpServerRequest::Method::Post,
        [](const QHttpServerRequest &req) -> QHttpServerResponse {

            const auto user = server::authenticate(req);
            if (!user.has_value()) {
                return api::unauthorized("Invalid or expired token");
            }
            if (user->role != "bidder") {
                return api::forbidden("Only bidders can leave reviews");
            }

            QString parseError;
            const QJsonObject body = parseJsonBody(req.body(), parseError);
            if (!parseError.isEmpty()) {
                return api::badRequest(parseError);
            }

            const qint64 auctionId = body.value("auction_id").toVariant().toLongLong();
            const int    rating    = body.value("rating").toInt(-1);
            const QString comment  = body.value("comment").toString();

            if (auctionId <= 0) {
                return api::badRequest("Field auction_id is required");
            }
            if (rating < 0 || rating > 10) {
                return api::badRequest("Field rating must be 0..10");
            }

            const auto validation = models::ReviewRepository::validateNewReview(
                user->id, auctionId);
            if (validation != models::ReviewValidationResult::Ok) {
                return validationToResponse(validation);
            }

            const auto auction = models::AuctionRepository::findById(auctionId);
            if (!auction.has_value()) {
                return api::notFound("Auction not found");
            }

            const qint64 id = models::ReviewRepository::create(
                user->id, auction->sellerId, auctionId, rating, comment);
            if (id < 0) {
                return api::serverError("Failed to create review");
            }

            const auto review = models::ReviewRepository::findById(id);
            if (!review.has_value()) {
                return api::serverError("Review created but not found");
            }

            return api::created(reviewToJson(*review));
        });

    // GET /api/reviews/{id}
    server.route("/api/reviews/<arg>", QHttpServerRequest::Method::Get,
        [](qint64 id) -> QHttpServerResponse {

            const auto review = models::ReviewRepository::findById(id);
            if (!review.has_value()) {
                return api::notFound(QString("Review %1 not found").arg(id));
            }
            return api::ok(reviewToJson(*review));
        });

    // PATCH /api/reviews/{id}
    server.route("/api/reviews/<arg>", QHttpServerRequest::Method::Patch,
        [](qint64 id, const QHttpServerRequest &req) -> QHttpServerResponse {

            const auto user = server::authenticate(req);
            if (!user.has_value()) {
                return api::unauthorized("Invalid or expired token");
            }

            const auto review = models::ReviewRepository::findById(id);
            if (!review.has_value()) {
                return api::notFound(QString("Review %1 not found").arg(id));
            }
            if (review->bidderId != user->id) {
                return api::forbidden("Only the review author can modify it");
            }

            QString parseError;
            const QJsonObject body = parseJsonBody(req.body(), parseError);
            if (!parseError.isEmpty()) {
                return api::badRequest(parseError);
            }

            int rating = review->rating;
            QString comment = review->comment;

            if (body.contains("rating")) {
                rating = body.value("rating").toInt(-1);
                if (rating < 0 || rating > 10) {
                    return api::badRequest("Field rating must be 0..10");
                }
            }
            if (body.contains("comment")) {
                comment = body.value("comment").toString();
            }

            if (!models::ReviewRepository::update(id, user->id, rating, comment)) {
                return api::serverError("Failed to update review");
            }

            const auto updated = models::ReviewRepository::findById(id);
            return api::ok(reviewToJson(*updated));
        });

    // DELETE /api/reviews/{id}
    server.route("/api/reviews/<arg>", QHttpServerRequest::Method::Delete,
        [](qint64 id, const QHttpServerRequest &req) -> QHttpServerResponse {

            const auto user = server::authenticate(req);
            if (!user.has_value()) {
                return api::unauthorized("Invalid or expired token");
            }

            const auto review = models::ReviewRepository::findById(id);
            if (!review.has_value()) {
                return api::notFound(QString("Review %1 not found").arg(id));
            }

            const bool isAuthor    = review->bidderId == user->id;
            const bool isModerator = user->role == "moderator";

            if (!isAuthor && !isModerator) {
                return api::forbidden(
                    "Only the author or a moderator can delete this review");
            }

            if (!models::ReviewRepository::remove(id, user->id, user->role)) {
                return api::serverError("Failed to delete review");
            }

            QJsonObject response;
            response["status"] = "ok";
            return api::ok(response);
        });

    // GET /api/sellers/{id}/rating
    server.route("/api/sellers/<arg>/rating", QHttpServerRequest::Method::Get,
        [](qint64 sellerId) -> QHttpServerResponse {

            const auto rating = models::ReviewRepository::ratingForSeller(sellerId);

            QJsonObject obj;
            obj["seller_id"]      = sellerId;
            obj["average_rating"] = rating.average;
            obj["review_count"]   = rating.count;
            return api::ok(obj);
        });

    // GET /api/sellers/{id}/reviews
    server.route("/api/sellers/<arg>/reviews", QHttpServerRequest::Method::Get,
        [](qint64 sellerId) -> QHttpServerResponse {

            const auto reviews = models::ReviewRepository::findBySeller(sellerId);

            QJsonArray arr;
            for (const auto &r : reviews) {
                arr.append(reviewToJson(r));
            }

            QJsonObject result;
            result["seller_id"] = sellerId;
            result["items"]     = arr;
            result["count"]     = static_cast<int>(reviews.size());
            return api::ok(result);
        });
}

} // namespace auctionhub::handlers
