#pragma once

#include <QDateTime>
#include <QString>

namespace auctionhub::models {

struct Review {
    qint64 id = 0;
    qint64 bidderId = 0;
    qint64 sellerId = 0;
    qint64 auctionId = 0;
    int rating = 0;
    QString comment;
    QDateTime createdAt;
    QDateTime updatedAt;
};

}  // namespace auctionhub::models
