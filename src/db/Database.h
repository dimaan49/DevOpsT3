#pragma once

#include <QSqlDatabase>
#include <QString>

namespace auctionhub::db {

class Database {
public:
    // Читает параметры из переменных окружения и открывает соединение.
    // Возвращает true, если подключение успешно.
    static bool connect();

    // Закрывает соединение. Вызывается при завершении приложения.
    static void close();

    // Проверяет, что соединение живо, простым запросом SELECT 1.
    static bool isHealthy();

    // Доступ к QSqlDatabase по имени соединения по умолчанию.
    static QSqlDatabase handle();

private:
    static constexpr const char *kConnectionName = "auctionhub";
};

} // namespace auctionhub::db
