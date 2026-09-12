#include "Database.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>

namespace auctionhub::db {

bool Database::connect()
{
    const QString host     = qEnvironmentVariable("AUCTIONHUB_DB_HOST", "localhost");
    const int     port     = qEnvironmentVariableIntValue("AUCTIONHUB_DB_PORT") > 0
                                 ? qEnvironmentVariableIntValue("AUCTIONHUB_DB_PORT")
                                 : 5432;
    const QString dbName   = qEnvironmentVariable("AUCTIONHUB_DB_NAME", "auctionhub");
    const QString user     = qEnvironmentVariable("AUCTIONHUB_DB_USER", "auctionhub");
    const QString password = qEnvironmentVariable("AUCTIONHUB_DB_PASSWORD");

    if (password.isEmpty()) {
        qCritical() << "AUCTIONHUB_DB_PASSWORD is not set";
        return false;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL", kConnectionName);
    db.setHostName(host);
    db.setPort(port);
    db.setDatabaseName(dbName);
    db.setUserName(user);
    db.setPassword(password);

    if (!db.open()) {
        qCritical() << "Failed to open database:" << db.lastError().text();
        return false;
    }

    qInfo() << "Connected to PostgreSQL at" << host << ":" << port << "/" << dbName;
    return true;
}

void Database::close()
{
    if (QSqlDatabase::contains(kConnectionName)) {
        QSqlDatabase::database(kConnectionName).close();
        QSqlDatabase::removeDatabase(kConnectionName);
    }
}

bool Database::isHealthy()
{
    if (!QSqlDatabase::contains(kConnectionName)) {
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(kConnectionName);
    if (!db.isOpen()) {
        return false;
    }

    QSqlQuery query(db);
    if (!query.exec("SELECT 1")) {
        return false;
    }
    return query.next();
}

QSqlDatabase Database::handle()
{
    return QSqlDatabase::database(kConnectionName);
}

} // namespace auctionhub::db
