#include "ReviewRepository.h"

#include "../db/Database.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace auctionhub::models {

namespace {

Review rowToReview(const QSqlQuery &q)
{
    Review r;
    r.id        = q.value("id").toLongLong();
    r.bidderId  = q.value("bidder_id").toLongLong();
    r.sellerId  = q.value("seller_id").toLongLong();
    r.auctionId = q.value("auction_id").toLongLong();
    r.rating    = q.value("rating").toInt();
    r.comment   = q.value("comment").toString();
    r.createdAt = q.value("created_at").toDateTime();
    r.updatedAt = q.value("updated_at").toDateTime();
    return r;
}

QString escapeSql(const QString &value)
{
    QString escaped = value;
    escaped.replace("'", "''");
    return escaped;
}

} // namespace

ReviewValidationResult ReviewRepository::validateNewReview(
    qint64 bidderId, qint64 auctionId)
{
    // 1. Аукцион существует и завершён?
    {
        QSqlQuery q(db::Database::handle());
        const QString sql = QString(
            "SELECT status FROM auctions WHERE id = %1 LIMIT 1").arg(auctionId);

        if (!q.exec(sql)) {
            qCritical() << "validateNewReview: auction check failed:"
                        << q.lastError().text();
            return ReviewValidationResult::AuctionNotFound;
        }
        if (!q.next()) {
            return ReviewValidationResult::AuctionNotFound;
        }
        if (q.value("status").toString() != "finished") {
            return ReviewValidationResult::AuctionNotFinished;
        }
    }

    // 2. Участник делал ставку по лотам этого аукциона?
    {
        QSqlQuery q(db::Database::handle());
        const QString sql = QString(
            "SELECT 1 FROM bids b "
            "JOIN lots l ON l.id = b.lot_id "
            "WHERE l.auction_id = %1 AND b.bidder_id = %2 LIMIT 1")
            .arg(auctionId).arg(bidderId);

        if (!q.exec(sql)) {
            qCritical() << "validateNewReview: bid check failed:"
                        << q.lastError().text();
            return ReviewValidationResult::BidderDidNotBid;
        }
        if (!q.next()) {
            return ReviewValidationResult::BidderDidNotBid;
        }
    }

    // 3. Отзыв уже есть?
    {
        QSqlQuery q(db::Database::handle());
        const QString sql = QString(
            "SELECT 1 FROM reviews "
            "WHERE bidder_id = %1 AND auction_id = %2 LIMIT 1")
            .arg(bidderId).arg(auctionId);

        if (!q.exec(sql)) {
            qCritical() << "validateNewReview: review check failed:"
                        << q.lastError().text();
            return ReviewValidationResult::AlreadyReviewed;
        }
        if (q.next()) {
            return ReviewValidationResult::AlreadyReviewed;
        }
    }

    return ReviewValidationResult::Ok;
}

qint64 ReviewRepository::create(qint64 bidderId, qint64 sellerId,
                                qint64 auctionId, int rating,
                                const QString &comment)
{
    QSqlQuery q(db::Database::handle());

    const QString sql = QString(
        "INSERT INTO reviews (bidder_id, seller_id, auction_id, rating, comment) "
        "VALUES (%1, %2, %3, %4, '%5') RETURNING id")
        .arg(bidderId)
        .arg(sellerId)
        .arg(auctionId)
        .arg(rating)
        .arg(escapeSql(comment));

    if (!q.exec(sql) || !q.next()) {
        qCritical() << "ReviewRepository::create failed:" << q.lastError().text();
        return -1;
    }
    return q.value(0).toLongLong();
}

std::optional<Review> ReviewRepository::findById(qint64 id)
{
    QSqlQuery q(db::Database::handle());
    const QString sql = QString(
        "SELECT * FROM reviews WHERE id = %1 LIMIT 1").arg(id);

    if (!q.exec(sql)) {
        qCritical() << "ReviewRepository::findById failed:" << q.lastError().text();
        return std::nullopt;
    }
    if (!q.next()) {
        return std::nullopt;
    }
    return rowToReview(q);
}

std::vector<Review> ReviewRepository::findBySeller(qint64 sellerId)
{
    std::vector<Review> result;
    QSqlQuery q(db::Database::handle());
    const QString sql = QString(
        "SELECT * FROM reviews WHERE seller_id = %1 ORDER BY created_at DESC")
        .arg(sellerId);

    if (!q.exec(sql)) {
        qCritical() << "ReviewRepository::findBySeller failed:"
                    << q.lastError().text();
        return result;
    }
    while (q.next()) {
        result.push_back(rowToReview(q));
    }
    return result;
}

ReviewRepository::SellerRating ReviewRepository::ratingForSeller(qint64 sellerId)
{
    SellerRating result;

    QSqlQuery q(db::Database::handle());
    const QString sql = QString(
        "SELECT AVG(rating)::float, COUNT(*) FROM reviews WHERE seller_id = %1")
        .arg(sellerId);

    if (!q.exec(sql) || !q.next()) {
        qCritical() << "ReviewRepository::ratingForSeller failed:"
                    << q.lastError().text();
        return result;
    }

    if (q.value(0).isNull()) {
        return result;
    }

    result.average = q.value(0).toDouble();
    result.count   = q.value(1).toInt();
    return result;
}

bool ReviewRepository::update(qint64 reviewId, qint64 userId,
                              int newRating, const QString &newComment)
{
    QSqlQuery q(db::Database::handle());
    const QString sql = QString(
        "UPDATE reviews "
        "SET rating = %1, comment = '%2', updated_at = NOW() "
        "WHERE id = %3 AND bidder_id = %4")
        .arg(newRating)
        .arg(escapeSql(newComment))
        .arg(reviewId)
        .arg(userId);

    if (!q.exec(sql)) {
        qCritical() << "ReviewRepository::update failed:" << q.lastError().text();
        return false;
    }
    return q.numRowsAffected() > 0;
}

bool ReviewRepository::remove(qint64 reviewId, qint64 userId,
                              const QString &userRole)
{
    QSqlQuery q(db::Database::handle());

    QString sql;
    if (userRole == "moderator") {
        sql = QString("DELETE FROM reviews WHERE id = %1").arg(reviewId);
    } else {
        sql = QString("DELETE FROM reviews WHERE id = %1 AND bidder_id = %2")
                  .arg(reviewId).arg(userId);
    }

    if (!q.exec(sql)) {
        qCritical() << "ReviewRepository::remove failed:" << q.lastError().text();
        return false;
    }
    return q.numRowsAffected() > 0;
}

} // namespace auctionhub::models
