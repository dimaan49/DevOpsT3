#include "UserRepository.h"

#include "../db/Database.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QDebug>

namespace auctionhub::models {

namespace {

User rowToUser(const QSqlQuery &q)
{
    User u;
    u.id           = q.value("id").toLongLong();
    u.email        = q.value("email").toString();
    u.passwordHash = q.value("password_hash").toString();
    u.role         = q.value("role").toString();
    u.ageConfirmed = q.value("age_confirmed").toBool();
    u.createdAt    = q.value("created_at").toDateTime();
    return u;
}

}

qint64 UserRepository::create(const QString &email,
                              const QString &passwordHash,
                              const QString &role,
                              bool ageConfirmed)
{
    if (emailExists(email)) {
        return -2;
    }

    QSqlQuery q(db::Database::handle());
    q.prepare(R"(
        INSERT INTO users (email, password_hash, role, age_confirmed)
        VALUES (:email, :password_hash, :role, :age_confirmed)
        RETURNING id
    )");
    q.bindValue(":email",         email);
    q.bindValue(":password_hash", passwordHash);
    q.bindValue(":role",          role);
    q.bindValue(":age_confirmed", ageConfirmed);

    if (!q.exec()) {
        qCritical() << "UserRepository::create failed:" << q.lastError().text();
        return -1;
    }

    if (!q.next()) {
        qCritical() << "UserRepository::create: no id returned";
        return -1;
    }
    return q.value(0).toLongLong();
}

std::optional<User> UserRepository::findByEmail(const QString &email)
{
    QSqlQuery q(db::Database::handle());
    q.prepare("SELECT * FROM users WHERE email = :email LIMIT 1");
    q.bindValue(":email", email);

    if (!q.exec()) {
        qCritical() << "UserRepository::findByEmail failed:" << q.lastError().text();
        return std::nullopt;
    }
    if (!q.next()) {
        return std::nullopt;
    }
    return rowToUser(q);
}

std::optional<User> UserRepository::findById(qint64 id)
{
    QSqlQuery q(db::Database::handle());
    q.prepare("SELECT * FROM users WHERE id = :id LIMIT 1");
    q.bindValue(":id", id);

    if (!q.exec()) {
        qCritical() << "UserRepository::findById failed:" << q.lastError().text();
        return std::nullopt;
    }
    if (!q.next()) {
        return std::nullopt;
    }
    return rowToUser(q);
}

bool UserRepository::emailExists(const QString &email)
{
    QSqlQuery q(db::Database::handle());
    q.prepare("SELECT 1 FROM users WHERE email = :email LIMIT 1");
    q.bindValue(":email", email);

    if (!q.exec()) {
        qCritical() << "UserRepository::emailExists failed:" << q.lastError().text();
        return false;
    }
    return q.next();
}

} // namespace auctionhub::models
