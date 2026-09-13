#pragma once

#include "User.h"

#include <optional>

namespace auctionhub::models {

class UserRepository {
public:
    // Создаёт пользователя. Возвращает id или -1 при ошибке.
    // Если email уже занят — возвращает -2 (для обработки конфликта на уровне API).
    static qint64 create(const QString &email,
                         const QString &passwordHash,
                         const QString &role,
                         bool ageConfirmed);

    // Поиск по email. nullopt, если не найден.
    static std::optional<User> findByEmail(const QString &email);

    // Поиск по id. nullopt, если не найден.
    static std::optional<User> findById(qint64 id);

    // Проверка существования email.
    static bool emailExists(const QString &email);
	// Хэширует пароль SHA-256.
	static QString hashPassword(const QString &password);

	// Проверяет пароль. Возвращает пользователя, если пароль верный.
	static std::optional<User> verifyPassword(const QString &email,
                                          const QString &password);
};

} // namespace auctionhub::models
