#include "BidHandler.h"

#include "../server/ApiResponse.h"
#include "../models/BidRepository.h"
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

QJsonObject bidToJson(const models::Bid &b)
{
    QJsonObject obj;
    obj["id"]         = b.id;
    obj["lot_id"]     = b.lotId;
    obj["bidder_id"]  = b.bidderId;
    obj["amount"]     = b.amount;
    obj["created_at"] = b.createdAt.toString(Qt::ISODate);
    return obj;
}

// Переводит результат валидации в HTTP-ответ с осмысленным сообщением.
QHttpServerResponse validationToResponse(models::BidValidationResult result)
{
    switch (result) {
    case models::BidValidationResult::Ok:
        return QHttpServerResponse(QHttpServerResponse::StatusCode::Ok);
    case models::BidValidationResult::LotNotFound:
        return api::notFound("Lot not found");
    case models::BidValidationResult::AuctionNotActive:
        return api::conflict("Auction is not active");
    case models::BidValidationResult::TooLow:
        return api::conflict("Bid must be higher than current price");
    case models::BidValidationResult::NotMultipleOfStep:
        return api::conflict("Bid difference must be a multiple of the auction step");
    }
    return api::serverError("Unknown validation result");
}

} // namespace

void BidHandler::registerRoutes(QHttpServer &server)
{
    // POST /api/lots/{id}/bids — подать ставку
    server.route("/api/lots/<arg>/bids",
                 QHttpServerRequest::Method::Post,
                 [](qint64 lotId, const QHttpServerRequest &req) -> QHttpServerResponse {

                     const QByteArray userIdHeader = req.value("X-User-Id");
                     if (userIdHeader.isEmpty()) {
                         return api::unauthorized("X-User-Id header is required");
                     }
                     bool ok = false;
                     const qint64 bidderId = userIdHeader.toLongLong(&ok);
                     if (!ok || bidderId <= 0) {
                         return api::unauthorized("X-User-Id header is invalid");
                     }

                     const auto bidder = models::UserRepository::findById(bidderId);
                     if (!bidder.has_value()) {
                         return api::unauthorized("Bidder not found");
                     }
                     if (bidder->role != "bidder" && bidder->role != "seller") {
                         return api::forbidden("Only bidders can place bids");
                     }

                     QString parseError;
                     const QJsonObject body = parseJsonBody(req.body(), parseError);
                     if (!parseError.isEmpty()) {
                         return api::badRequest(parseError);
                     }

                     if (!body.contains("amount")) {
                         return api::badRequest("Field amount is required");
                     }
                     const double amount = body.value("amount").toDouble(0.0);
                     if (amount <= 0.0) {
                         return api::badRequest("Field amount must be greater than 0");
                     }

                     // Валидация правила предметной области.
                     const auto validation = models::BidRepository::validate(lotId, amount);
                     if (validation != models::BidValidationResult::Ok) {
                         return validationToResponse(validation);
                     }

                     const qint64 bidId = models::BidRepository::place(lotId, bidderId, amount);
                     if (bidId < 0) {
                         return api::serverError("Failed to place bid");
                     }

                     // Возвращаем созданную ставку.
                     const auto bids = models::BidRepository::findByLot(lotId);
                     for (const auto &b : bids) {
                         if (b.id == bidId) {
                             return api::created(bidToJson(b));
                         }
                     }
                     return api::serverError("Bid created but not found");
                 });

    // GET /api/lots/{id}/bids — история ставок по лоту
    server.route("/api/lots/<arg>/bids",
                 QHttpServerRequest::Method::Get,
                 [](qint64 lotId) -> QHttpServerResponse {

                     const auto lot = models::LotRepository::findById(lotId);
                     if (!lot.has_value()) {
                         return api::notFound(QString("Lot %1 not found").arg(lotId));
                     }

                     const auto bids = models::BidRepository::findByLot(lotId);
                     QJsonArray arr;
                     for (const auto &b : bids) {
                         arr.append(bidToJson(b));
                     }

                     QJsonObject result;
                     result["lot_id"]       = lotId;
                     result["current_price"] = models::BidRepository::currentPrice(lotId);
                     result["items"]        = arr;
                     result["count"]        = static_cast<int>(bids.size());
                     return api::ok(result);
                 });
}

} // namespace auctionhub::handlers