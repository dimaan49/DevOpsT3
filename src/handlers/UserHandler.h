#pragma once

#include <QHttpServer>

namespace auctionhub::handlers {

class UserHandler {
public:
    static void registerRoutes(QHttpServer &server);
};

} // namespace auctionhub::handlers
