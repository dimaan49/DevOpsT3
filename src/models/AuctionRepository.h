#pragma once

#include "Auction.h"

#include <optional>
#include <vector>

namespace auctionhub::models {

class AuctionRepository {
public:
    // Создаёт аукцион в статусе "draft". Возвращает id или -1 при ошибке.
    static qint64 create(qint64 sellerId,
                         const QString &title,
                         const QString &description,
                         double step,
                         double startPrice);

    // Найти по id. nullopt, если не найден.
    static std::optional<Auction> findById(qint64 id);

    // Список всех аукционов (последние сверху).
    static std::vector<Auction> findAll();

    // Список аукционов конкретного продавца.
    static std::vector<Auction> findBySeller(qint64 sellerId);

    // Изменить статус. Возвращает true при успехе.
    static bool updateStatus(qint64 id, const QString &status);
};

} // namespace auctionhub::models