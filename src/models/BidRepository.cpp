#include "BidRepository.h"

#include "AuctionRepository.h"
#include "LotRepository.h"
#include "../db/Database.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QDebug>
#include <cmath>

namespace auctionhub::models {

namespace {

Bid rowToBid(const QSqlQuery &q)
{
    Bid b;
    b.id        = q.value("id").toLongLong();
    b.lotId     = q.value("lot_id").toLongLong();
    b.bidderId  = q.value("bidder_id").toLongLong();
    b.amount    = q.value("amount").toDouble();
    b.createdAt = q.value("created_at").toDateTime();
    return b;
}

// Проверка кратности с учётом плавающей точки.
// Сравниваем остаток с малым эпсилоном, потому что double не точен.
bool isMultipleOf(double value, double step)
{
    const double remainder = std::fmod(value, step);
    const double eps = 1e-6;
    return remainder < eps || std::fabs(remainder - step) < eps;
}

} // namespace

double BidRepository::currentPrice(qint64 lotId)
{
    QSqlQuery q(db::Database::handle());
    q.prepare("SELECT MAX(amount) FROM bids WHERE lot_id = :lid");
    q.bindValue(":lid", lotId);

    if (!q.exec() || !q.next()) {
        qCritical() << "BidRepository::currentPrice failed:" << q.lastError().text();
        return 0.0;
    }

    if (q.value(0).isNull()) {
        // Ставок нет — текущая цена = начальная цена лота.
        const auto lot = LotRepository::findById(lotId);
        if (!lot.has_value()) {
            return 0.0;
        }
        return lot->startPrice;
    }
    return q.value(0).toDouble();
}

BidValidationResult BidRepository::validate(qint64 lotId, double amount)
{
    const auto lotOpt = LotRepository::findById(lotId);
    if (!lotOpt.has_value()) {
        return BidValidationResult::LotNotFound;
    }
    const Lot &lot = *lotOpt;

    const auto auctionOpt = AuctionRepository::findById(lot.auctionId);
    if (!auctionOpt.has_value()) {
        return BidValidationResult::AuctionNotActive;
    }
    if (auctionOpt->status != "active") {
        return BidValidationResult::AuctionNotActive;
    }

    const double current = currentPrice(lotId);
    const double step    = auctionOpt->step;

    // Ставка должна быть строго выше текущей.
    if (amount <= current) {
        return BidValidationResult::TooLow;
    }

    // Разница между новой ставкой и текущей должна быть кратна шагу.
    const double diff = amount - current;
    if (!isMultipleOf(diff, step)) {
        return BidValidationResult::NotMultipleOfStep;
    }

    return BidValidationResult::Ok;
}

qint64 BidRepository::place(qint64 lotId, qint64 bidderId, double amount)
{
    const auto validation = validate(lotId, amount);
    if (validation != BidValidationResult::Ok) {
        return -1;
    }

    QSqlQuery q(db::Database::handle());
    q.prepare(R"(
        INSERT INTO bids (lot_id, bidder_id, amount)
        VALUES (:lot_id, :bidder_id, :amount)
        RETURNING id
    )");
    q.bindValue(":lot_id",    lotId);
    q.bindValue(":bidder_id", bidderId);
    q.bindValue(":amount",    amount);

    if (!q.exec()) {
        qCritical() << "BidRepository::place failed:" << q.lastError().text();
        return -1;
    }
    if (!q.next()) {
        return -1;
    }
    return q.value(0).toLongLong();
}

std::vector<Bid> BidRepository::findByLot(qint64 lotId)
{
    std::vector<Bid> result;
    QSqlQuery q(db::Database::handle());
    q.prepare("SELECT * FROM bids WHERE lot_id = :lid ORDER BY created_at DESC");
    q.bindValue(":lid", lotId);

    if (!q.exec()) {
        qCritical() << "BidRepository::findByLot failed:" << q.lastError().text();
        return result;
    }
    while (q.next()) {
        result.push_back(rowToBid(q));
    }
    return result;
}

} // namespace auctionhub::models