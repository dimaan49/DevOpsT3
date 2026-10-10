#pragma once

#include <QHttpServer>

namespace auctionhub::handlers {

class ReviewHandler {
public:
    static void registerRoutes(QHttpServer& server);
};

}  // namespace auctionhub::handlers
