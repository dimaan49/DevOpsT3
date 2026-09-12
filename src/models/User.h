#pragma once

#include <QDateTime>
#include <QString>
#include <optional>

namespace auctionhub::models {

struct User {
    qint64      id = 0;
    QString     email;
    QString     passwordHash;
    QString     role;            // "seller" | "bidder" | "moderator"
    bool        ageConfirmed = false;
    QDateTime   createdAt;
};

} // namespace auctionhub::models
