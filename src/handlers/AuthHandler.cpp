#include "AuthHandler.h"

#include "../server/ApiResponse.h"
#include "../server/auth.h"
#include "../server/jwt.h"
#include "../models/RevokedTokenRepository.h"
#include "../models/UserRepository.h"

#include <QHttpServerRequest>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonParseError>

namespace auctionhub::handlers {

namespace {

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

QJsonObject userToJson(const models::User &u)
{
    QJsonObject obj;
    obj["id"]            = u.id;
    obj["email"]         = u.email;
    obj["role"]          = u.role;
    obj["age_confirmed"] = u.ageConfirmed;
    obj["created_at"]    = u.createdAt.toString(Qt::ISODate);
    return obj;
}

constexpr qint64 kTokenTtlSeconds = 30LL * 24 * 60 * 60; // 30 дней

} // namespace

void AuthHandler::registerRoutes(QHttpServer &server)
{
    // POST /api/auth/login
    server.route("/api/auth/login", QHttpServerRequest::Method::Post,
        [](const QHttpServerRequest &req) -> QHttpServerResponse {

            QString parseError;
            const QJsonObject body = parseJsonBody(req.body(), parseError);
            if (!parseError.isEmpty()) {
                return api::badRequest(parseError);
            }

            const QString email    = body.value("email").toString().trimmed();
            const QString password = body.value("password").toString();

            if (email.isEmpty() || password.isEmpty()) {
                return api::badRequest("Fields email and password are required");
            }

            const auto user = models::UserRepository::verifyPassword(email, password);
            if (!user.has_value()) {
                return api::unauthorized("Invalid email or password");
            }

            const QString token = server::jwtEncode(user->id, kTokenTtlSeconds);
            if (token.isEmpty()) {
                return api::serverError("Failed to create token");
            }

            QJsonObject response;
            response["token"] = token;
            response["user"]  = userToJson(*user);
            return api::ok(response);
        });

    // POST /api/auth/logout
    server.route("/api/auth/logout", QHttpServerRequest::Method::Post,
        [](const QHttpServerRequest &req) -> QHttpServerResponse {

            const QString token = server::extractToken(req);
            if (token.isEmpty()) {
                return api::unauthorized("Authorization header is required");
            }

            models::RevokedTokenRepository::revoke(token);

            QJsonObject response;
            response["status"] = "ok";
            return api::ok(response);
        });

    // GET /api/auth/me
    server.route("/api/auth/me", QHttpServerRequest::Method::Get,
        [](const QHttpServerRequest &req) -> QHttpServerResponse {

            const auto user = server::authenticate(req);
            if (!user.has_value()) {
                return api::unauthorized("Invalid or expired token");
            }

            return api::ok(userToJson(*user));
        });
}

} // namespace auctionhub::handlers
