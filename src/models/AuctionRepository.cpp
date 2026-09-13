#include "AuctionRepository.h"

#include "../db/Database.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QDebug>

namespace auctionhub::models {

namespace {

Auction rowToAuction(const QSqlQuery &q)
{
    Auction a;
    a.id          = q.value("id").toLongLong();
    a.sellerId    = q.value("seller_id").toLongLong();
    a.title       = q.value("title").toString();
    a.description = q.value("description").toString();
    a.step        = q.value("step").toDouble();
    a.startPrice  = q.value("start_price").toDouble();
    a.status      = q.value("status").toString();
    a.createdAt   = q.value("created_at").toDateTime();
    return a;
}

} // namespace

qint64 AuctionRepository::create(qint64 sellerId,
                                 const QString &title,
                                 const QString &description,
                                 double step,
                                 double startPrice)
{
    if (step <= 0.0 || startPrice <= 0.0) {
        return -1;
    }

    QSqlQuery q(db::Database::handle());
    q.prepare(R"(
        INSERT INTO auctions (seller_id, title, description, step, start_price, status)
        VALUES (:seller_id, :title, :description, :step, :start_price, 'draft')
        RETURNING id
    )");
    q.bindValue(":seller_id",   sellerId);
    q.bindValue(":title",       title);
    q.bindValue(":description", description);
    q.bindValue(":step",        step);
    q.bindValue(":start_price", startPrice);

    if (!q.exec()) {
        qCritical() << "AuctionRepository::create failed:" << q.lastError().text();
        return -1;
    }
    if (!q.next()) {
        return -1;
    }
    return q.value(0).toLongLong();
}

std::optional<Auction> AuctionRepository::findById(qint64 id)
{
    QSqlQuery q(db::Database::handle());
    q.prepare("SELECT * FROM auctions WHERE id = :id LIMIT 1");
    q.bindValue(":id", id);

    if (!q.exec()) {
        qCritical() << "AuctionRepository::findById failed:" << q.lastError().text();
        return std::nullopt;
    }
    if (!q.next()) {
        return std::nullopt;
    }
    return rowToAuction(q);
}

std::vector<Auction> AuctionRepository::findAll()
{
    std::vector<Auction> result;
    QSqlQuery q(db::Database::handle());

    if (!q.exec("SELECT * FROM auctions ORDER BY created_at DESC")) {
        qCritical() << "AuctionRepository::findAll failed:" << q.lastError().text();
        return result;
    }

    while (q.next()) {
        result.push_back(rowToAuction(q));
    }
    return result;
}

std::vector<Auction> AuctionRepository::findBySeller(qint64 sellerId)
{
    std::vector<Auction> result;
    QSqlQuery q(db::Database::handle());
    q.prepare("SELECT * FROM auctions WHERE seller_id = :sid ORDER BY created_at DESC");
    q.bindValue(":sid", sellerId);

    if (!q.exec()) {
        qCritical() << "AuctionRepository::findBySeller failed:" << q.lastError().text();
        return result;
    }
    while (q.next()) {
        result.push_back(rowToAuction(q));
    }
    return result;
}

bool AuctionRepository::updateStatus(qint64 id, const QString &status)
{
    QSqlQuery q(db::Database::handle());
    q.prepare("UPDATE auctions SET status = :status WHERE id = :id");
    q.bindValue(":status", status);
    q.bindValue(":id",     id);

    if (!q.exec()) {
        qCritical() << "AuctionRepository::updateStatus failed:" << q.lastError().text();
        return false;
    }
    return q.numRowsAffected() > 0;
}

int AuctionRepository::cancel(qint64 id, qint64 sellerId)
{
    const auto auction = findById(id);
    if (!auction.has_value()) {
        return -2;
    }

    if (auction->sellerId != sellerId) {
        return -3;
    }

    if (auction->status == "finished" || auction->status == "cancelled") {
        return -4;
    }

    // Проверяем, есть ли ставки по лотам этого аукциона.
    QSqlQuery q(db::Database::handle());
    q.prepare(R"(
        SELECT 1
        FROM bids b
        JOIN lots l ON l.id = b.lot_id
        WHERE l.auction_id = :auction_id
        LIMIT 1
    )");
    q.bindValue(":auction_id", id);

    if (!q.exec()) {
        qCritical() << "AuctionRepository::cancel: bids check failed:"
                    << q.lastError().text();
        return -1;
    }
    if (q.next()) {
        return -5;
    }

    if (!updateStatus(id, "cancelled")) {
        return -1;
    }
    return 0;
}

} // namespace auctionhub::models
