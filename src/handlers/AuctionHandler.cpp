#include "AuctionHandler.h"

#include "../server/ApiResponse.h"
#include "../models/AuctionRepository.h"
#include "../models/LotRepository.h"
#include "../models/UserRepository.h"

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

QJsonObject auctionToJson(const models::Auction &a)
{
    QJsonObject obj;
    obj["id"]           = a.id;
    obj["seller_id"]    = a.sellerId;
    obj["title"]        = a.title;
    obj["description"]  = a.description;
    obj["step"]         = a.step;
    obj["start_price"]  = a.startPrice;
    obj["status"]       = a.status;
    obj["created_at"]   = a.createdAt.toString(Qt::ISODate);
    return obj;
}

QJsonObject lotToJson(const models::Lot &l)
{
    QJsonObject obj;
    obj["id"]           = l.id;
    obj["auction_id"]   = l.auctionId;
    obj["title"]        = l.title;
    obj["description"]  = l.description;
    obj["start_price"]  = l.startPrice;
    obj["created_at"]   = l.createdAt.toString(Qt::ISODate);
    return obj;
}

} // namespace

void AuctionHandler::registerRoutes(QHttpServer &server)
{
    // POST /api/auctions — создать аукцион
    server.route("/api/auctions", QHttpServerRequest::Method::Post,
                 [](const QHttpServerRequest &req) -> QHttpServerResponse {

                     // Продавец идентифицируется заголовком X-User-Id. упрощенка
                     const QByteArray userIdHeader = req.value("X-User-Id");
                     if (userIdHeader.isEmpty()) {
                         return api::unauthorized("X-User-Id header is required");
                     }
                     bool ok = false;
                     const qint64 sellerId = userIdHeader.toLongLong(&ok);
                     if (!ok || sellerId <= 0) {
                         return api::unauthorized("X-User-Id header is invalid");
                     }

                     const auto seller = models::UserRepository::findById(sellerId);
                     if (!seller.has_value()) {
                         return api::unauthorized("Seller not found");
                     }
                     if (seller->role != "seller") {
                         return api::forbidden("Only sellers can create auctions");
                     }

                     QString parseError;
                     const QJsonObject body = parseJsonBody(req.body(), parseError);
                     if (!parseError.isEmpty()) {
                         return api::badRequest(parseError);
                     }

                     const QString title       = body.value("title").toString().trimmed();
                     const QString description = body.value("description").toString();
                     const double  step        = body.value("step").toDouble(0.0);
                     const double  startPrice  = body.value("start_price").toDouble(0.0);

                     if (title.isEmpty()) {
                         return api::badRequest("Field title is required");
                     }
                     if (step <= 0.0) {
                         return api::badRequest("Field step must be greater than 0");
                     }
                     if (startPrice <= 0.0) {
                         return api::badRequest("Field start_price must be greater than 0");
                     }

                     const qint64 id = models::AuctionRepository::create(
                         sellerId, title, description, step, startPrice);

                     if (id < 0) {
                         return api::serverError("Failed to create auction");
                     }

                     const auto created = models::AuctionRepository::findById(id);
                     if (!created.has_value()) {
                         return api::serverError("Auction created but not found");
                     }

                     return api::created(auctionToJson(*created));
                 });

    // GET /api/auctions — список всех аукционов
    server.route("/api/auctions", QHttpServerRequest::Method::Get,
                 [](const QHttpServerRequest &) -> QHttpServerResponse {

                     const auto auctions = models::AuctionRepository::findAll();
                     QJsonArray arr;
                     for (const auto &a : auctions) {
                         arr.append(auctionToJson(a));
                     }
                     QJsonObject result;
                     result["items"] = arr;
                     result["count"] = static_cast<int>(auctions.size());
                     return api::ok(result);
                 });

    // GET /api/auctions/{id} — детали аукциона с лотами
    server.route("/api/auctions/<arg>", QHttpServerRequest::Method::Get,
                 [](qint64 id) -> QHttpServerResponse {

                     const auto auction = models::AuctionRepository::findById(id);
                     if (!auction.has_value()) {
                         return api::notFound(QString("Auction %1 not found").arg(id));
                     }

                     QJsonObject obj = auctionToJson(*auction);

                     QJsonArray lotsArr;
                     for (const auto &lot : models::LotRepository::findByAuction(id)) {
                         lotsArr.append(lotToJson(lot));
                     }
                     obj["lots"] = lotsArr;

                     return api::ok(obj);
                 });

    // POST /api/auctions/{id}/publish — перевести в статус active
    server.route("/api/auctions/<arg>/publish",
                 QHttpServerRequest::Method::Post,
                 [](qint64 id, const QHttpServerRequest &req) -> QHttpServerResponse {

                     const QByteArray userIdHeader = req.value("X-User-Id");
                     if (userIdHeader.isEmpty()) {
                         return api::unauthorized("X-User-Id header is required");
                     }
                     bool ok = false;
                     const qint64 userId = userIdHeader.toLongLong(&ok);
                     if (!ok || userId <= 0) {
                         return api::unauthorized("X-User-Id header is invalid");
                     }

                     const auto auction = models::AuctionRepository::findById(id);
                     if (!auction.has_value()) {
                         return api::notFound(QString("Auction %1 not found").arg(id));
                     }
                     if (auction->sellerId != userId) {
                         return api::forbidden("Only the seller can publish this auction");
                     }
                     if (auction->status != "draft") {
                         return api::conflict("Auction is not in draft status");
                     }

                     if (!models::AuctionRepository::updateStatus(id, "active")) {
                         return api::serverError("Failed to publish auction");
                     }

                     const auto updated = models::AuctionRepository::findById(id);
                     return api::ok(auctionToJson(*updated));
                 });

    // POST /api/auctions/{id}/lots — добавить лот
    server.route("/api/auctions/<arg>/lots",
                 QHttpServerRequest::Method::Post,
                 [](qint64 auctionId, const QHttpServerRequest &req) -> QHttpServerResponse {

                     const QByteArray userIdHeader = req.value("X-User-Id");
                     if (userIdHeader.isEmpty()) {
                         return api::unauthorized("X-User-Id header is required");
                     }
                     bool ok = false;
                     const qint64 userId = userIdHeader.toLongLong(&ok);
                     if (!ok || userId <= 0) {
                         return api::unauthorized("X-User-Id header is invalid");
                     }

                     const auto auction = models::AuctionRepository::findById(auctionId);
                     if (!auction.has_value()) {
                         return api::notFound(QString("Auction %1 not found").arg(auctionId));
                     }
                     if (auction->sellerId != userId) {
                         return api::forbidden("Only the seller can add lots to this auction");
                     }

                     QString parseError;
                     const QJsonObject body = parseJsonBody(req.body(), parseError);
                     if (!parseError.isEmpty()) {
                         return api::badRequest(parseError);
                     }

                     const QString title       = body.value("title").toString().trimmed();
                     const QString description = body.value("description").toString();
                     const double  startPrice  = body.value("start_price").toDouble(0.0);

                     if (title.isEmpty()) {
                         return api::badRequest("Field title is required");
                     }
                     if (startPrice <= 0.0) {
                         return api::badRequest("Field start_price must be greater than 0");
                     }

                     const qint64 lotId = models::LotRepository::create(
                         auctionId, title, description, startPrice);
                     if (lotId < 0) {
                         return api::serverError("Failed to create lot");
                     }

                     const auto lot = models::LotRepository::findById(lotId);
                     if (!lot.has_value()) {
                         return api::serverError("Lot created but not found");
                     }
                     return api::created(lotToJson(*lot));
                 });


server.route("/api/auctions/<arg>/cancel",
             QHttpServerRequest::Method::Post,
    [](qint64 id, const QHttpServerRequest &req) -> QHttpServerResponse {

        const QByteArray userIdHeader = req.value("X-User-Id");
        if (userIdHeader.isEmpty()) {
            return api::unauthorized("X-User-Id header is required");
        }
        bool ok = false;
        const qint64 userId = userIdHeader.toLongLong(&ok);
        if (!ok || userId <= 0) {
            return api::unauthorized("X-User-Id header is invalid");
        }

        const int result = models::AuctionRepository::cancel(id, userId);
        switch (result) {
        case 0: {
            const auto updated = models::AuctionRepository::findById(id);
            if (!updated.has_value()) {
                return api::serverError("Auction cancelled but not found");
            }
            return api::ok(auctionToJson(*updated));
        }
        case -2:
            return api::notFound(QString("Auction %1 not found").arg(id));
        case -3:
            return api::forbidden("Only the seller can cancel this auction");
        case -4:
            return api::conflict("Auction cannot be cancelled in its current status");
        case -5:
            return api::conflict("Cannot cancel auction: it already has bids");
        default:
            return api::serverError("Failed to cancel auction");
        }
    });
}
} // namespace auctionhub::handlers

