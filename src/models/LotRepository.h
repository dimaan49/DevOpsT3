#pragma once

#include "Lot.h"

#include <optional>
#include <vector>

namespace auctionhub::models {

class LotRepository {
public:
    static qint64 create(qint64 auctionId,
                         const QString &title,
                         const QString &description,
                         double startPrice);

    static std::optional<Lot> findById(qint64 id);

    static std::vector<Lot> findByAuction(qint64 auctionId);
};

} // namespace auctionhub::models