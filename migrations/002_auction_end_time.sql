-- Добавляем поле end_time в auctions.

ALTER TABLE auctions
    ADD COLUMN IF NOT EXISTS end_time TIMESTAMPTZ;

CREATE INDEX IF NOT EXISTS idx_auctions_end_time
    ON auctions (end_time);
