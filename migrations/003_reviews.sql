-- Таблица отзывов о продавцах.
-- Участник может оставить отзыв после завершения аукциона,
-- в котором он делал ставку. Один отзыв на пару (bidder, auction).

CREATE TABLE IF NOT EXISTS reviews (
    id          BIGSERIAL PRIMARY KEY,
    bidder_id   BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    seller_id   BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    auction_id  BIGINT NOT NULL REFERENCES auctions(id) ON DELETE CASCADE,
    rating      INTEGER NOT NULL CHECK (rating >= 0 AND rating <= 10),
    comment     TEXT,
    created_at  TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    UNIQUE (bidder_id, auction_id)
);

CREATE INDEX IF NOT EXISTS idx_reviews_seller
    ON reviews (seller_id);

CREATE INDEX IF NOT EXISTS idx_reviews_auction
    ON reviews (auction_id);
