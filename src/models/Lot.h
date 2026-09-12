#pragma once

#include <QDateTime>
#include <QString>

namespace auctionhub::models {

struct Lot {
    qint64      id = 0;
    qint64      auctionId = 0;
    QString     title;
    QString     description;
    double      startPrice = 0.0;
    QDateTime   createdAt;
};

} // namespace auctionhub::models