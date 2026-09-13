#pragma once

#include <QString>

namespace auctionhub::models {

class RevokedTokenRepository {
public:
    // Добавляет токен в blacklist. Возвращает true при успехе.
    static bool revoke(const QString &token);

    // Проверяет, отозван ли токен.
    static bool isRevoked(const QString &token);
};

} // namespace auctionhub::models
