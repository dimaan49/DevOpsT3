#pragma once

#include <QDateTime>
#include <QString>

namespace auctionhub::models {

struct Auction {
    qint64      id = 0;
    qint64      sellerId = 0;
    QString     title;
    QString     description;
    double      step = 0.0;          // шаг аукциона, > 0
    double      startPrice = 0.0;    // начальная цена, > 0
    QString     status;              // "draft" | "active" | "finished" | "cancelled"
    QDateTime   createdAt;
};

} // namespace auctionhub::models
