#include "db/Database.h"
#include "handlers/AuctionHandler.h"
#include "handlers/AuthHandler.h"
#include "handlers/BidHandler.h"
#include "handlers/ReviewHandler.h"
#include "handlers/UserHandler.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QHostAddress>
#include <QHttpServer>
#include <QHttpServerResponse>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QSocketNotifier>
#include <QTcpServer>
#include <QTextStream>
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <unistd.h>

static void loadDotEnv(const QString& path) {

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

namespace {
int terminationPipe[2] = {-1, -1};

void handleTerminationSignal(int signalNumber) {
    const unsigned char value = static_cast<unsigned char>(signalNumber);
    if (terminationPipe[1] >= 0) {
        const ssize_t ignored = ::write(terminationPipe[1], &value, sizeof(value));
        (void)ignored;
    }
}
}  // namespace

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    if (::pipe(terminationPipe) != 0) {
        qCritical() << "Cannot create termination pipe";
        return 1;
    }

    for (int fd : terminationPipe) {
        const int flags = ::fcntl(fd, F_GETFL, 0);
        if (flags == -1 || ::fcntl(fd, F_SETFL, flags | static_cast<int>(O_NONBLOCK)) == -1) {
            qCritical() << "Cannot configure termination pipe";
            ::close(terminationPipe[0]);
            ::close(terminationPipe[1]);
            terminationPipe[0] = terminationPipe[1] = -1;
            return 1;
        }
    }

    struct sigaction action{};
    action.sa_handler = handleTerminationSignal;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;

    if (::sigaction(SIGTERM, &action, nullptr) != 0 || ::sigaction(SIGINT, &action, nullptr) != 0) {
        qCritical() << "Cannot install termination signal handlers";
        ::close(terminationPipe[0]);
        ::close(terminationPipe[1]);
        terminationPipe[0] = terminationPipe[1] = -1;
        return 1;
    }

    auto* signalNotifier = new QSocketNotifier(terminationPipe[0], QSocketNotifier::Read, &app);

    QObject::connect(signalNotifier, &QSocketNotifier::activated, &app,
                     [&app](QSocketDescriptor, QSocketNotifier::Type) {
                         unsigned char buffer[32];
                         while (::read(terminationPipe[0], buffer, sizeof(buffer)) > 0) {
                         }
                         qInfo() << "Termination signal received; shutting down gracefully";
                         app.quit();
                     });

    QObject::connect(&app, &QCoreApplication::aboutToQuit, [] {
        qInfo() << "Closing database connection";
        auctionhub::db::Database::close();
        if (terminationPipe[0] >= 0)
            ::close(terminationPipe[0]);
        if (terminationPipe[1] >= 0)
            ::close(terminationPipe[1]);
        terminationPipe[0] = terminationPipe[1] = -1;
    });

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
    auctionhub::handlers::AuthHandler::registerRoutes(httpServer);
    auctionhub::handlers::ReviewHandler::registerRoutes(httpServer);

    httpServer.route("/health", []() {
        QJsonObject obj;
        obj["service"] = "auctionhub";

        const bool dbOk = auctionhub::db::Database::isHealthy();
        obj["status"] = dbOk ? "ok" : "degraded";
        obj["database"] = dbOk ? "ok" : "down";

        const auto code = dbOk ? QHttpServerResponse::StatusCode::Ok
                               : QHttpServerResponse::StatusCode::ServiceUnavailable;

        return QHttpServerResponse(obj, code);
    });

    httpServer.route("/version", QHttpServerRequest::Method::Get, [](const QHttpServerRequest&) {
        QJsonObject obj;
        obj["service"] = "auctionhub";
        obj["version"] = qEnvironmentVariable("AUCTIONHUB_VERSION", "dev");
        return QHttpServerResponse(obj, QHttpServerResponse::StatusCode::Ok);
    });

    httpServer.route("/", QHttpServerRequest::Method::Get, [](const QHttpServerRequest&) {
        QFile f("web/index.html");
        if (!f.open(QIODevice::ReadOnly)) {
            return QHttpServerResponse("text/plain", QByteArray("web/index.html not found"),
                                       QHttpServerResponse::StatusCode::NotFound);
        }
        return QHttpServerResponse("text/html", f.readAll(), QHttpServerResponse::StatusCode::Ok);
    });

    httpServer.route("/style.css", QHttpServerRequest::Method::Get, [](const QHttpServerRequest&) {
        QFile f("web/style.css");
        if (!f.open(QIODevice::ReadOnly)) {
            return QHttpServerResponse("text/plain", QByteArray("not found"),
                                       QHttpServerResponse::StatusCode::NotFound);
        }
        return QHttpServerResponse("text/css", f.readAll(), QHttpServerResponse::StatusCode::Ok);
    });

    httpServer.route("/app.js", QHttpServerRequest::Method::Get, [](const QHttpServerRequest&) {
        QFile f("web/app.js");
        if (!f.open(QIODevice::ReadOnly)) {
            return QHttpServerResponse("text/plain", QByteArray("not found"),
                                       QHttpServerResponse::StatusCode::NotFound);
        }
        return QHttpServerResponse("application/javascript", f.readAll(),
                                   QHttpServerResponse::StatusCode::Ok);
    });

    httpServer.route("/common.js", QHttpServerRequest::Method::Get, [](const QHttpServerRequest&) {
        QFile f("web/common.js");
        if (!f.open(QIODevice::ReadOnly)) {
            return QHttpServerResponse("text/plain", QByteArray("not found"),
                                       QHttpServerResponse::StatusCode::NotFound);
        }
        return QHttpServerResponse("application/javascript", f.readAll(),
                                   QHttpServerResponse::StatusCode::Ok);
    });

    httpServer.route("/auctions.html", QHttpServerRequest::Method::Get,
                     [](const QHttpServerRequest&) {
                         QFile f("web/auctions.html");
                         if (!f.open(QIODevice::ReadOnly)) {
                             return QHttpServerResponse("text/plain", QByteArray("not found"),
                                                        QHttpServerResponse::StatusCode::NotFound);
                         }
                         return QHttpServerResponse("text/html", f.readAll(),
                                                    QHttpServerResponse::StatusCode::Ok);
                     });

    httpServer.route("/auctions.js", QHttpServerRequest::Method::Get,
                     [](const QHttpServerRequest&) {
                         QFile f("web/auctions.js");
                         if (!f.open(QIODevice::ReadOnly)) {
                             return QHttpServerResponse("text/plain", QByteArray("not found"),
                                                        QHttpServerResponse::StatusCode::NotFound);
                         }
                         return QHttpServerResponse("application/javascript", f.readAll(),
                                                    QHttpServerResponse::StatusCode::Ok);
                     });

    httpServer.route("/my-auctions.html", QHttpServerRequest::Method::Get,
                     [](const QHttpServerRequest&) {
                         QFile f("web/my-auctions.html");
                         if (!f.open(QIODevice::ReadOnly)) {
                             return QHttpServerResponse("text/plain", QByteArray("not found"),
                                                        QHttpServerResponse::StatusCode::NotFound);
                         }
                         return QHttpServerResponse("text/html", f.readAll(),
                                                    QHttpServerResponse::StatusCode::Ok);
                     });

    httpServer.route("/my-auctions.js", QHttpServerRequest::Method::Get,
                     [](const QHttpServerRequest&) {
                         QFile f("web/my-auctions.js");
                         if (!f.open(QIODevice::ReadOnly)) {
                             return QHttpServerResponse("text/plain", QByteArray("not found"),
                                                        QHttpServerResponse::StatusCode::NotFound);
                         }
                         return QHttpServerResponse("application/javascript", f.readAll(),
                                                    QHttpServerResponse::StatusCode::Ok);
                     });

    httpServer.bind(&tcpServer);

    qInfo() << "AuctionHub server listening on" << host << ":" << port;
    qInfo() << "Healthcheck: http://localhost:" << port << "/health";

    return app.exec();
}
