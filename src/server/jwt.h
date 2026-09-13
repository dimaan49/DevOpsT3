#pragma once

#include <QString>
#include <QJsonObject>

#include <optional>

namespace auctionhub::server {

struct JwtPayload {
    qint64 userId = 0;
    qint64 expiresAt = 0; // Unix timestamp (секунды)
};

// Создаёт JWT с указанным payload, подписанный HMAC-SHA256.
// Возвращает пустую строку, если секрет не задан.
QString jwtEncode(qint64 userId, qint64 ttlSeconds);

// Декодирует и проверяет JWT. Возвращает payload, если токен валиден.
// nullopt, если подпись неверна, токен истёк или формат нарушен.
std::optional<JwtPayload> jwtDecode(const QString &token);

} // namespace auctionhub::server
