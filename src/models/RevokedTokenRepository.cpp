#include "RevokedTokenRepository.h"

#include "../db/Database.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QDebug>

namespace auctionhub::models {

bool RevokedTokenRepository::revoke(const QString &token)
{
    QSqlQuery q(db::Database::handle());
    q.prepare(R"(
        INSERT INTO revoked_tokens (token)
        VALUES (:token)
        ON CONFLICT (token) DO NOTHING
    )");
    q.bindValue(":token", token);

    if (!q.exec()) {
        qCritical() << "RevokedTokenRepository::revoke failed:"
                    << q.lastError().text();
        return false;
    }
    return true;
}

bool RevokedTokenRepository::isRevoked(const QString &token)
{
    QSqlQuery q(db::Database::handle());
    q.prepare("SELECT 1 FROM revoked_tokens WHERE token = :token LIMIT 1");
    q.bindValue(":token", token);

    if (!q.exec()) {
        qCritical() << "RevokedTokenRepository::isRevoked failed:"
                    << q.lastError().text();
        return false;
    }
    return q.next();
}

} // namespace auctionhub::models
