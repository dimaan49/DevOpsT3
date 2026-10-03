#pragma once

#include "../src/db/Database.h"

#include <QCoreApplication>
#include <QSqlQuery>

#include <catch2/catch_test_macros.hpp>

namespace auctionhub::tests {

// Создаёт QCoreApplication один раз для всех тестов.
inline void ensureQtApp()
{
    if (!QCoreApplication::instance()) {
        static int argc = 1;
        static char arg0[] = "tests";
        static char *argv[] = { arg0, nullptr };
        new QCoreApplication(argc, argv);
    }
}

// Настраивает переменные окружения на тестовую БД.
// Вызывается один раз перед всеми тестами с БД.
inline void configureTestDb()
{
    qputenv("AUCTIONHUB_DB_HOST",     "localhost");
    qputenv("AUCTIONHUB_DB_PORT",     "5432");
    qputenv("AUCTIONHUB_DB_NAME",     "auctionhub_test");
    qputenv("AUCTIONHUB_DB_USER",     "dbuser");
    // Пароль берётся из окружения AUCTIONHUB_DB_PASSWORD
}

// Подключается к тестовой БД. Возвращает false, если не удалось.
inline bool connectTestDb()
{
    ensureQtApp();
    configureTestDb();
    return db::Database::connect();
}

// Удаляет все данные из таблиц. Не трогает схему.
inline void truncateAll()
{
    QSqlQuery q(db::Database::handle());
    q.exec("TRUNCATE bids, lots, auctions, users, revoked_tokens RESTART IDENTITY CASCADE");
}

// Общая инициализация для теста с БД.
// Если БД недоступна — тест пропускается через WARN + return.
#define REQUIRE_TEST_DB_CONNECTED()                                     \
    do {                                                                \
        if (!auctionhub::tests::connectTestDb()) {                      \
            WARN("Test database not available. Skipping.");             \
            return;                                                     \
        }                                                               \
        auctionhub::tests::truncateAll();                               \
    } while (0)
}
