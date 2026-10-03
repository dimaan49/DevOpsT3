BUILD_DIR := build
CMAKE     := cmake
GENERATOR := Ninja
TARGET    := auctionhub_server

-include .env
export

# --- Обязательные команды локальной проверки ---
# setup     — первоначальная настройка
# run       — локальный запуск
# quality   — форматирование и анализ        (пункт 7)
# migrate   — миграции БД                    (пункт 8)
# backup    — резервная копия БД             (пункт 11)
# restore   — восстановление из копии        (пункт 11)
# verify    — полный набор проверок          (пункт 13)

.PHONY: setup deps env configure build run test clean

# --- Первоначальная настройка ---
setup: deps env build
	@echo
	@echo "=== Setup complete ==="
	@echo "Next: make run"

deps:
	bash scripts/install-deps.sh

env:
	@if [ ! -f .env ]; then \
		cp .env.example .env; \
		echo "Created .env from .env.example"; \
	else \
		echo ".env already exists"; \
	fi

# --- Сборка ---
configure:
	$(CMAKE) -B $(BUILD_DIR) -G $(GENERATOR) -DCMAKE_BUILD_TYPE=Debug

build: configure
	$(CMAKE) --build $(BUILD_DIR)

# --- Запуск ---
run: build
	./$(BUILD_DIR)/$(TARGET)

# --- Очистка ---
clean:
	rm -rf $(BUILD_DIR)

test:
	$(CMAKE) --build $(BUILD_DIR) --target auctionhub_tests
	cd $(BUILD_DIR) && \
	    AUCTIONHUB_DB_PASSWORD="$(AUCTIONHUB_TEST_DB_PASSWORD)" \
	    ctest --output-on-failure

test-unit:
	cd $(BUILD_DIR) && ./tests/auctionhub_tests "[smoke],[jwt],[password]"

test-db:
	cd $(BUILD_DIR) && \
	    AUCTIONHUB_DB_PASSWORD="$(AUCTIONHUB_TEST_DB_PASSWORD)" \
	    ./tests/auctionhub_tests "[db]"
