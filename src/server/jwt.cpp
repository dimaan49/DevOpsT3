#include "jwt.h"

#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QByteArray>

namespace auctionhub::server {

namespace {

QByteArray base64UrlEncode(const QByteArray &data)
{
    QByteArray b64 = data.toBase64(QByteArray::Base64UrlEncoding |
                                    QByteArray::OmitTrailingEquals);
    return b64;
}

QByteArray base64UrlDecode(const QByteArray &data)
{
    return QByteArray::fromBase64(data, QByteArray::Base64UrlEncoding);
}

QByteArray secret()
{
    return qEnvironmentVariable("AUCTIONHUB_JWT_SECRET").toUtf8();
}

QByteArray sign(const QByteArray &data)
{
    QMessageAuthenticationCode mac(QCryptographicHash::Sha256, secret());
    mac.addData(data);
    return mac.result();
}

} // namespace

QString jwtEncode(qint64 userId, qint64 ttlSeconds)
{
    if (secret().isEmpty()) {
        return {};
    }

    QJsonObject header;
    header["alg"] = "HS256";
    header["typ"] = "JWT";

    const qint64 now = QDateTime::currentSecsSinceEpoch();

    QJsonObject payload;
    payload["sub"] = QString::number(userId);
    payload["iat"] = now;
    payload["exp"] = now + ttlSeconds;

    const QByteArray headerB64 = base64UrlEncode(
        QJsonDocument(header).toJson(QJsonDocument::Compact));
    const QByteArray payloadB64 = base64UrlEncode(
        QJsonDocument(payload).toJson(QJsonDocument::Compact));

    const QByteArray signingInput = headerB64 + "." + payloadB64;
    const QByteArray signature = base64UrlEncode(sign(signingInput));

    return QString::fromLatin1(signingInput + "." + signature);
}

std::optional<JwtPayload> jwtDecode(const QString &token)
{
    if (secret().isEmpty()) {
        return std::nullopt;
    }

    const QByteArray raw = token.toLatin1();
    const QList<QByteArray> parts = raw.split('.');
    if (parts.size() != 3) {
        return std::nullopt;
    }

    const QByteArray signingInput = parts[0] + "." + parts[1];
    const QByteArray expectedSig = base64UrlEncode(sign(signingInput));
    if (expectedSig != parts[2]) {
        return std::nullopt;
    }

    const QByteArray payloadJson = base64UrlDecode(parts[1]);
    const QJsonDocument doc = QJsonDocument::fromJson(payloadJson);
    if (!doc.isObject()) {
        return std::nullopt;
    }
    const QJsonObject payload = doc.object();

    const qint64 exp = payload.value("exp").toVariant().toLongLong();
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    if (exp <= now) {
        return std::nullopt;
    }

    JwtPayload result;
    result.userId = payload.value("sub").toString().toLongLong();
    result.expiresAt = exp;
    if (result.userId <= 0) {
        return std::nullopt;
    }
    return result;
}

} // namespace auctionhub::server
