#pragma once

#include <QDateTime>

namespace auctionhub::models {

struct Bid {
    qint64      id = 0;
    qint64      lotId = 0;
    qint64      bidderId = 0;
    double      amount = 0.0;
    QDateTime   createdAt;
};

} // namespace auctionhub::models