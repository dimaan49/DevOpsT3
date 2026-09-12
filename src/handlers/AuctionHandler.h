#pragma once

#include <QHttpServer>

namespace auctionhub::handlers {

class AuctionHandler {
public:
    static void registerRoutes(QHttpServer &server);
};

} // namespace auctionhub::handlers