# AuctionHub

Программа для проведения интернет-аукционов.

## Назначение

Клиент-серверное приложение для проведения открытых английских аукционов.
Поддерживает учёт аукционов, лотов, продавцов, покупателей и фактов продажи.

## Стек

- C++17
- Qt 6 (Core, Network, Sql, HttpServer)
- PostgreSQL
- CMake + Ninja
- Catch2 — для модульных тестов
- easylogging++ — для логирования
- clang-format, clang-tidy

## Структура проекта

```
.
├── CMakeLists.txt
├── Makefile
├── .env.example
├── docs/                 — документация
├── src/
│   ├── main.cpp          — точка входа
│   ├── server/           — HTTP-сервер
│   ├── db/               — работа с PostgreSQL
│   ├── models/           — сущности предметной области
│   └── handlers/         — обработчики эндпоинтов
├── web/                  — HTML/CSS/JS
└── tests/                — автотесты
```

## Документация

- [HTTP API](docs/api.md) — описание эндпоинтов
- [Схема данных](docs/schema.md) — таблицы и связи
- [Правила внесения изменений](docs/contributing.md) — обязательные проверки, запреты, порядок приёмки

## Требования

- Arch Linux / CachyOS (или другая система с Qt6)
- Пакеты: `base-devel`, `cmake`, `ninja`, `gcc`, `qt6-base`, `qt6-httpserver`, `postgresql-libs`

## Локальная проверка

```bash
make setup     # конфигурация CMake
make build     # сборка
make run       # запуск сервера
make clean     # очистка сборки
```

## Конфигурация

Скопируйте `.env.example` в `.env` и заполните значения:

```bash
cp .env.example .env
```

Сам `.env` в репозиторий не попадает.

## Healthcheck

После запуска:

```bash
curl http://localhost:8080/health
```

Ожидаемый ответ:

```json
{"database":"ok","service":"auctionhub","status":"ok"}
```
