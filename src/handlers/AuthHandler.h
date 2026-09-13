#pragma once

#include <QHttpServer>

namespace auctionhub::handlers {

class AuthHandler {
public:
    static void registerRoutes(QHttpServer &server);
};

} // namespace auctionhub::handlers
