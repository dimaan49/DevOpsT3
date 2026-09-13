# AuctionHub — схема данных

База данных: PostgreSQL. Схема описана в `src/db/schema.sql`.

---

## Обзор

Четыре таблицы, связанные внешними ключами:

```
users ─────< auctions ─────< lots ─────< bids
  ▲                                        │
  └────────────────────────────────────────┘
        (продавец)                    (участник)
```

- `users` — продавцы, участники и модераторы. Одна таблица, роль в поле `role`.
- `auctions` — аукционы, каждый принадлежит продавцу.
- `lots` — лоты, каждый привязан к аукциону.
- `bids` — ставки, каждая привязана к лоту и участнику.

---

## Таблица `users`

Хранит всех пользователей системы.

| Колонка | Тип | Ограничения | Описание |
|---------|-----|-------------|----------|
| `id` | `BIGSERIAL` | `PRIMARY KEY` | Идентификатор |
| `email` | `TEXT` | `NOT NULL`, `UNIQUE` | Email пользователя |
| `password_hash` | `TEXT` | `NOT NULL` | SHA-256 хэш пароля |
| `role` | `TEXT` | `NOT NULL`, `CHECK IN ('seller','bidder','moderator')` | Роль |
| `age_confirmed` | `BOOLEAN` | `NOT NULL DEFAULT FALSE` | Подтверждение 18+ |
| `created_at` | `TIMESTAMPTZ` | `NOT NULL DEFAULT NOW()` | Дата регистрации |

**Ограничения:**

- `UNIQUE(email)` — email не может повторяться.
- `CHECK(role IN ...)` — только три допустимые роли.

---

## Таблица `auctions`

Хранит аукционы.

| Колонка | Тип | Ограничения | Описание |
|---------|-----|-------------|----------|
| `id` | `BIGSERIAL` | `PRIMARY KEY` | Идентификатор |
| `seller_id` | `BIGINT` | `NOT NULL`, `REFERENCES users(id) ON DELETE RESTRICT` | Продавец |
| `title` | `TEXT` | `NOT NULL` | Название аукциона |
| `description` | `TEXT` | | Описание |
| `step` | `NUMERIC(14,2)` | `NOT NULL`, `CHECK (step > 0)` | Шаг аукциона |
| `start_price` | `NUMERIC(14,2)` | `NOT NULL`, `CHECK (start_price > 0)` | Начальная цена |
| `status` | `TEXT` | `NOT NULL DEFAULT 'draft'`, `CHECK IN ('draft','active','finished','cancelled')` | Статус |
| `created_at` | `TIMESTAMPTZ` | `NOT NULL DEFAULT NOW()` | Дата создания |

**Ограничения:**

- `REFERENCES users(id) ON DELETE RESTRICT` — нельзя удалить пользователя, если у него есть аукционы.
- `CHECK(step > 0)` — шаг положительный.
- `CHECK(start_price > 0)` — начальная цена положительная.
- `CHECK(status IN ...)` — только четыре допустимых статуса.

**Индексы:**

- `idx_auctions_seller` по `seller_id` — для быстрого поиска аукционов продавца.

---

## Таблица `lots`

Хранит лоты.

| Колонка | Тип | Ограничения | Описание |
|---------|-----|-------------|----------|
| `id` | `BIGSERIAL` | `PRIMARY KEY` | Идентификатор |
| `auction_id` | `BIGINT` | `NOT NULL`, `REFERENCES auctions(id) ON DELETE CASCADE` | Аукцион |
| `title` | `TEXT` | `NOT NULL` | Название лота |
| `description` | `TEXT` | | Описание |
| `start_price` | `NUMERIC(14,2)` | `NOT NULL`, `CHECK (start_price > 0)` | Начальная цена |
| `created_at` | `TIMESTAMPTZ` | `NOT NULL DEFAULT NOW()` | Дата создания |

**Ограничения:**

- `REFERENCES auctions(id) ON DELETE CASCADE` — при удалении аукциона удаляются его лоты.
- `CHECK(start_price > 0)` — цена положительная.

**Индексы:**

- `idx_lots_auction` по `auction_id` — для быстрого поиска лотов аукциона.

---

## Таблица `bids`

Хранит ставки.

| Колонка | Тип | Ограничения | Описание |
|---------|-----|-------------|----------|
| `id` | `BIGSERIAL` | `PRIMARY KEY` | Идентификатор |
| `lot_id` | `BIGINT` | `NOT NULL`, `REFERENCES lots(id) ON DELETE CASCADE` | Лот |
| `bidder_id` | `BIGINT` | `NOT NULL`, `REFERENCES users(id) ON DELETE RESTRICT` | Участник |
| `amount` | `NUMERIC(14,2)` | `NOT NULL`, `CHECK (amount > 0)` | Сумма ставки |
| `created_at` | `TIMESTAMPTZ` | `NOT NULL DEFAULT NOW()` | Дата ставки |

**Ограничения:**

- `REFERENCES lots(id) ON DELETE CASCADE` — при удалении лота удаляются его ставки.
- `REFERENCES users(id) ON DELETE RESTRICT` — нельзя удалить пользователя, если у него есть ставки.
- `CHECK(amount > 0)` — сумма положительная.

**Индексы:**

- `idx_bids_lot_created` по `(lot_id, created_at DESC)` — для быстрого получения истории ставок по лоту в обратном хронологическом порядке.

---

## Правило предметной области

Правило «ставка кратна шагу и выше текущей» **не** реализовано на уровне БД. Причины:

- проверка зависит от текущего максимума ставок по лоту;
- при вставке ставки нужно сравнить её с максимумом — это не `CHECK`, а подзапрос;
- можно было бы реализовать через триггер, но это усложняет отладку и тестирование;
- правило живёт в одном месте — `BidRepository::validate` в C++.

На уровне БД гарантируется только базовое: `amount > 0`. Всё остальное — на серверной стороне.

---

## Миграции

Схема создаётся выполнением `src/db/schema.sql`:

```bash
make migrate
```

Скрипт идемпотентный — использует `CREATE TABLE IF NOT EXISTS`, поэтому повторный запуск не ломает существующую базу.
