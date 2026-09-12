#pragma once

#include <QHttpServer>

namespace auctionhub::handlers {

class BidHandler {
public:
    static void registerRoutes(QHttpServer &server);
};

} // namespace auctionhub::handlers