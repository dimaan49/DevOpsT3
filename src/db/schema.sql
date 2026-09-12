
CREATE TABLE IF NOT EXISTS users (
    id              BIGSERIAL PRIMARY KEY,
    email           TEXT NOT NULL UNIQUE,
    password_hash   TEXT NOT NULL,
    role            TEXT NOT NULL CHECK (role IN ('seller', 'bidder', 'moderator')),
    age_confirmed   BOOLEAN NOT NULL DEFAULT FALSE,
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS auctions (
    id              BIGSERIAL PRIMARY KEY,
    seller_id       BIGINT NOT NULL REFERENCES users(id) ON DELETE RESTRICT,
    title           TEXT NOT NULL,
    description     TEXT,
    step            NUMERIC(14, 2) NOT NULL CHECK (step > 0),
    start_price     NUMERIC(14, 2) NOT NULL CHECK (start_price > 0),
    status          TEXT NOT NULL DEFAULT 'draft'
                    CHECK (status IN ('draft', 'active', 'finished', 'cancelled')),
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS lots (
    id              BIGSERIAL PRIMARY KEY,
    auction_id      BIGINT NOT NULL REFERENCES auctions(id) ON DELETE CASCADE,
    title           TEXT NOT NULL,
    description     TEXT,
    start_price     NUMERIC(14, 2) NOT NULL CHECK (start_price > 0),
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS bids (
    id              BIGSERIAL PRIMARY KEY,
    lot_id          BIGINT NOT NULL REFERENCES lots(id) ON DELETE CASCADE,
    bidder_id       BIGINT NOT NULL REFERENCES users(id) ON DELETE RESTRICT,
    amount          NUMERIC(14, 2) NOT NULL CHECK (amount > 0),
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_bids_lot_created
    ON bids (lot_id, created_at DESC);

CREATE INDEX IF NOT EXISTS idx_auctions_seller
    ON auctions (seller_id);

CREATE INDEX IF NOT EXISTS idx_lots_auction
    ON lots (auction_id);
