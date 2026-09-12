#pragma once

#include <QHttpServerResponse>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

namespace auctionhub::api {

// Успешный ответ с JSON-объектом.
inline QHttpServerResponse ok(const QJsonObject &obj)
{
    return QHttpServerResponse(obj, QHttpServerResponse::StatusCode::Ok);
}

// Ответ 201 Created.
inline QHttpServerResponse created(const QJsonObject &obj)
{
    return QHttpServerResponse(obj, QHttpServerResponse::StatusCode::Created);
}

// Единый формат ошибки: {"error": "<code>", "message": "<text>"}
inline QHttpServerResponse error(QHttpServerResponse::StatusCode status,
                                 const QString &code,
                                 const QString &message)
{
    QJsonObject obj;
    obj["error"]   = code;
    obj["message"] = message;
    return QHttpServerResponse(obj, status);
}

// Частые варианты ошибок.
inline QHttpServerResponse badRequest(const QString &message)
{
    return error(QHttpServerResponse::StatusCode::BadRequest, "bad_request", message);
}

inline QHttpServerResponse unauthorized(const QString &message)
{
    return error(QHttpServerResponse::StatusCode::Unauthorized, "unauthorized", message);
}

inline QHttpServerResponse forbidden(const QString &message)
{
    return error(QHttpServerResponse::StatusCode::Forbidden, "forbidden", message);
}

inline QHttpServerResponse notFound(const QString &message)
{
    return error(QHttpServerResponse::StatusCode::NotFound, "not_found", message);
}

inline QHttpServerResponse conflict(const QString &message)
{
    return error(QHttpServerResponse::StatusCode::Conflict, "conflict", message);
}

inline QHttpServerResponse serverError(const QString &message)
{
    return error(QHttpServerResponse::StatusCode::InternalServerError,
                 "internal_error", message);
}

} // namespace auctionhub::api
