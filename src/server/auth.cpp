#include "auth.h"

#include "jwt.h"
#include "../models/RevokedTokenRepository.h"
#include "../models/UserRepository.h"

namespace auctionhub::server {

QString extractToken(const QHttpServerRequest &req)
{
    const QByteArray header = req.value("Authorization");
    if (header.isEmpty()) {
        return {};
    }
    const QString value = QString::fromUtf8(header);
    const QString prefix = "Bearer ";
    if (!value.startsWith(prefix)) {
        return {};
    }
    return value.mid(prefix.length()).trimmed();
}

std::optional<models::User> authenticate(const QHttpServerRequest &req)
{
    const QString token = extractToken(req);
    if (token.isEmpty()) {
        return std::nullopt;
    }

    if (models::RevokedTokenRepository::isRevoked(token)) {
        return std::nullopt;
    }

    const auto payload = jwtDecode(token);
    if (!payload.has_value()) {
        return std::nullopt;
    }

    return models::UserRepository::findById(payload->userId);
}

} // namespace auctionhub::server
