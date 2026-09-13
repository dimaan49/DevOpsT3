#pragma once

#include "../models/User.h"

#include <QHttpServerRequest>
#include <optional>

namespace auctionhub::server {

// Извлекает токен из заголовка Authorization: Bearer <token>.
QString extractToken(const QHttpServerRequest &req);

// Проверяет токен и возвращает пользователя.
// nullopt, если токен невалиден, истёк, отозван, или пользователь не найден.
std::optional<models::User> authenticate(const QHttpServerRequest &req);

} // namespace auctionhub::server
