#ifndef BIDREPOSITORY_H
#define BIDREPOSITORY_H

#endif // BIDREPOSITORY_H
#pragma once

#include "Bid.h"

#include <optional>
#include <vector>

    namespace auctionhub::models {

    // Результат проверки возможности сделать ставку.
    enum class BidValidationResult {
        Ok,
        LotNotFound,
        AuctionNotActive,
        TooLow,           // меньше текущей максимальной
        NotMultipleOfStep // не кратно шагу
    };

    class BidRepository {
    public:
        // Возвращает текущую максимальную ставку по лоту.
        // Если ставок нет — возвращает начальную цену лота.
        static double currentPrice(qint64 lotId);

        // Проверяет, можно ли сделать ставку с указанной суммой.
        // Возвращает код причины отказа либо Ok.
        static BidValidationResult validate(qint64 lotId, double amount);

        // Вставляет ставку, если валидация прошла.
        // Возвращает id ставки или -1 при ошибке/нарушении правил.
        static qint64 place(qint64 lotId, qint64 bidderId, double amount);

        // Список ставок по лоту (последние сверху).
        static std::vector<Bid> findByLot(qint64 lotId);
    };

} // namespace auctionhub::models