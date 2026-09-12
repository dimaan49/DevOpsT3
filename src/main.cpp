#include <QCoreApplication>
#include <QFile>
#include <QHostAddress>
#include <QHttpServer>
#include <QHttpServerResponse>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTcpServer>
#include <QTextStream>
#include <QDebug>
#include "db/Database.h"
#include "handlers/UserHandler.h"
#include "handlers/AuctionHandler.h"
#include "handlers/BidHandler.h"

static void loadDotEnv(const QString &path)
{



    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }
        const int eq = line.indexOf('=');
        if (eq <= 0) {
            continue;
        }
        const QString key = line.left(eq).trimmed();
        const QString value = line.mid(eq + 1).trimmed();
        if (!qEnvironmentVariableIsSet(key.toUtf8().constData())) {
            qputenv(key.toUtf8().constData(), value.toUtf8());
        }
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("AuctionHub");

    loadDotEnv(".env");

    if (!auctionhub::db::Database::connect()) {
        qCritical() << "Database connection failed. Exiting.";
        return 1;
    }

    const QString host = qEnvironmentVariable("AUCTIONHUB_HOST", "0.0.0.0");
    const int port = qEnvironmentVariableIntValue("AUCTIONHUB_PORT") > 0
                         ? qEnvironmentVariableIntValue("AUCTIONHUB_PORT")
                         : 8080;

    QTcpServer tcpServer;
    if (!tcpServer.listen(QHostAddress(host), static_cast<quint16>(port))) {
        qCritical() << "Failed to listen on" << host << ":" << port;
        qCritical() << "Error:" << tcpServer.errorString();
        return 1;
    }

    QHttpServer httpServer;
	auctionhub::handlers::UserHandler::registerRoutes(httpServer);
    auctionhub::handlers::AuctionHandler::registerRoutes(httpServer);
    auctionhub::handlers::BidHandler::registerRoutes(httpServer);


    httpServer.route("/health", []() {
        QJsonObject obj;
        obj["service"] = "auctionhub";

        const bool dbOk = auctionhub::db::Database::isHealthy();
        obj["status"] = dbOk ? "ok" : "degraded";
        obj["database"] = dbOk ? "ok" : "down";

        const auto code = dbOk
            ? QHttpServerResponse::StatusCode::Ok
            : QHttpServerResponse::StatusCode::ServiceUnavailable;

        return QHttpServerResponse(obj, code);
    });
	httpServer.route("/", QHttpServerRequest::Method::Get,
		[](const QHttpServerRequest &) {
			QFile f("web/index.html");
			if (!f.open(QIODevice::ReadOnly)) {
				return QHttpServerResponse(
					"text/plain",
					QByteArray("web/index.html not found"),
					QHttpServerResponse::StatusCode::NotFound);
			}
			return QHttpServerResponse(
				"text/html",
				f.readAll(),
				QHttpServerResponse::StatusCode::Ok);
		});

	httpServer.route("/style.css", QHttpServerRequest::Method::Get,
		[](const QHttpServerRequest &) {
			QFile f("web/style.css");
			if (!f.open(QIODevice::ReadOnly)) {
				return QHttpServerResponse(
					"text/plain", QByteArray("not found"),
					QHttpServerResponse::StatusCode::NotFound);
			}
			return QHttpServerResponse(
				"text/css", f.readAll(), QHttpServerResponse::StatusCode::Ok);
		});

	httpServer.route("/app.js", QHttpServerRequest::Method::Get,
		[](const QHttpServerRequest &) {
			QFile f("web/app.js");
			if (!f.open(QIODevice::ReadOnly)) {
				return QHttpServerResponse(
					"text/plain", QByteArray("not found"),
					QHttpServerResponse::StatusCode::NotFound);
			}
			return QHttpServerResponse(
				"application/javascript", f.readAll(),
				QHttpServerResponse::StatusCode::Ok);
		});
		httpServer.bind(&tcpServer);

    qInfo() << "AuctionHub server listening on" << host << ":" << port;
    qInfo() << "Healthcheck: http://localhost:" << port << "/health";

    return app.exec();
}
