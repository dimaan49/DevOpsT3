#include "LotRepository.h"

#include "../db/Database.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QDebug>

namespace auctionhub::models {

namespace {

Lot rowToLot(const QSqlQuery &q)
{
    Lot l;
    l.id          = q.value("id").toLongLong();
    l.auctionId   = q.value("auction_id").toLongLong();
    l.title       = q.value("title").toString();
    l.description = q.value("description").toString();
    l.startPrice  = q.value("start_price").toDouble();
    l.createdAt   = q.value("created_at").toDateTime();
    return l;
}

} // namespace

qint64 LotRepository::create(qint64 auctionId,
                             const QString &title,
                             const QString &description,
                             double startPrice)
{
    if (startPrice <= 0.0) {
        return -1;
    }

    QSqlQuery q(db::Database::handle());
    q.prepare(R"(
        INSERT INTO lots (auction_id, title, description, start_price)
        VALUES (:auction_id, :title, :description, :start_price)
        RETURNING id
    )");
    q.bindValue(":auction_id",   auctionId);
    q.bindValue(":title",        title);
    q.bindValue(":description",  description);
    q.bindValue(":start_price",  startPrice);

    if (!q.exec()) {
        qCritical() << "LotRepository::create failed:" << q.lastError().text();
        return -1;
    }
    if (!q.next()) {
        return -1;
    }
    return q.value(0).toLongLong();
}

std::optional<Lot> LotRepository::findById(qint64 id)
{
    QSqlQuery q(db::Database::handle());
    q.prepare("SELECT * FROM lots WHERE id = :id LIMIT 1");
    q.bindValue(":id", id);

    if (!q.exec()) {
        qCritical() << "LotRepository::findById failed:" << q.lastError().text();
        return std::nullopt;
    }
    if (!q.next()) {
        return std::nullopt;
    }
    return rowToLot(q);
}

std::vector<Lot> LotRepository::findByAuction(qint64 auctionId)
{
    std::vector<Lot> result;
    QSqlQuery q(db::Database::handle());
    q.prepare("SELECT * FROM lots WHERE auction_id = :aid ORDER BY created_at DESC");
    q.bindValue(":aid", auctionId);

    if (!q.exec()) {
        qCritical() << "LotRepository::findByAuction failed:" << q.lastError().text();
        return result;
    }
    while (q.next()) {
        result.push_back(rowToLot(q));
    }
    return result;
}

} // namespace auctionhub::models