#include "UserHandler.h"

#include "../server/ApiResponse.h"
#include "../models/UserRepository.h"

#include <QCryptographicHash>
#include <QHttpServerRequest>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QUrlQuery>

namespace auctionhub::handlers {

namespace {

// Хэш пароля через SHA-256. Для прототипа достаточно.
// TODO: заменить на bcrypt/argon2.
QString hashPassword(const QString &password)
{
    const QByteArray hash = QCryptographicHash::hash(
        password.toUtf8(), QCryptographicHash::Sha256);
    return QString::fromLatin1(hash.toHex());
}

// Проверка простого формата email: есть @ и точка после неё.
bool isEmailValid(const QString &email)
{
    const int at = email.indexOf('@');
    if (at <= 0) return false;
    const int dot = email.indexOf('.', at);
    return dot > at + 1 && dot < email.length() - 1;
}

// Парсит тело запроса как JSON. При ошибке возвращает пустой объект
// и заполняет errorMessage.
QJsonObject parseJsonBody(const QByteArray &body, QString &errorMessage)
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &err);
    if (err.error != QJsonParseError::NoError) {
        errorMessage = QString("Invalid JSON: %1").arg(err.errorString());
        return {};
    }
    if (!doc.isObject()) {
        errorMessage = "Request body must be a JSON object";
        return {};
    }
    return doc.object();
}

} // namespace

void UserHandler::registerRoutes(QHttpServer &server)
{
    // POST /api/users — регистрация
    server.route("/api/users", QHttpServerRequest::Method::Post,
        [](const QHttpServerRequest &req) -> QHttpServerResponse {

            QString parseError;
            const QJsonObject body = parseJsonBody(req.body(), parseError);
            if (!parseError.isEmpty()) {
                return api::badRequest(parseError);
            }

            const QString email    = body.value("email").toString().trimmed();
            const QString password = body.value("password").toString();
            const QString role     = body.value("role").toString().trimmed();
            const bool    ageOk    = body.value("age_confirmed").toBool(false);

            if (email.isEmpty() || password.isEmpty() || role.isEmpty()) {
                return api::badRequest("Fields email, password, role are required");
            }

            if (!isEmailValid(email)) {
                return api::badRequest("Invalid email format");
            }

            if (password.length() < 6) {
                return api::badRequest("Password must be at least 6 characters");
            }

            if (role != "seller" && role != "bidder" && role != "moderator") {
                return api::badRequest("Role must be one of: seller, bidder, moderator");
            }

            if (!ageOk) {
                return api::badRequest("You must confirm you are 18 or older");
            }

            if (models::UserRepository::emailExists(email)) {
                return api::conflict("Email already registered");
            }

            const QString hash = hashPassword(password);
            const qint64 id = models::UserRepository::create(email, hash, role, ageOk);

            if (id == -2) {
                return api::conflict("Email already registered");
            }
            if (id < 0) {
                return api::serverError("Failed to create user");
            }

            QJsonObject response;
            response["id"]    = id;
            response["email"] = email;
            response["role"]  = role;
            return api::created(response);
        });

    // GET /api/users/{id} — получить пользователя
    server.route("/api/users/<arg>", QHttpServerRequest::Method::Get,
        [](qint64 id) -> QHttpServerResponse {

            const auto user = models::UserRepository::findById(id);
            if (!user.has_value()) {
                return api::notFound(QString("User %1 not found").arg(id));
            }

            QJsonObject obj;
            obj["id"]            = user->id;
            obj["email"]         = user->email;
            obj["role"]          = user->role;
            obj["age_confirmed"] = user->ageConfirmed;
            obj["created_at"]    = user->createdAt.toString(Qt::ISODate);
            // password_hash намеренно не возвращаем.
            return api::ok(obj);
        });
}

} // namespace auctionhub::handlers
