BUILD_DIR           := build
BUILD_COVERAGE_DIR  := build-coverage
CMAKE               := cmake
GENERATOR           := Ninja
TARGET              := auctionhub_server

CLANG_FORMAT        := clang-format
CLANG_TIDY          := clang-tidy
CXX					:= clang++
GCOVR               := gcovr

SRC_FILES           := $(shell find src -name '*.cpp' -o -name '*.h')

-include .env
export

# --- Обязательные команды локальной проверки ---
# setup     — первоначальная настройка
# run       — локальный запуск
# test      — автотесты
# quality   — форматирование и статический анализ
# migrate   — миграции БД                    (пункт 8)
# backup    — резервная копия БД             (пункт 11)
# restore   — восстановление из копии        (пункт 11)
# verify    — полный набор проверок          (пункт 13)

.PHONY: setup deps env configure build run clean \
        test test-unit test-db \
        format-fix quality \
        coverage clean-coverage \
        verify \
		backup restore \
		test-migrations test-backup-restore

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
	$(CMAKE) -B $(BUILD_DIR) -G $(GENERATOR) \
	    -DCMAKE_BUILD_TYPE=Debug \
	    -DCMAKE_CXX_COMPILER=$(CXX)
build: configure
	$(CMAKE) --build $(BUILD_DIR)

# --- Запуск ---
run: build
	./$(BUILD_DIR)/$(TARGET)

# --- Очистка ---
clean:
	rm -rf $(BUILD_DIR) $(BUILD_COVERAGE_DIR)

# --- Тесты ---
test:
	$(CMAKE) --build $(BUILD_DIR) --target auctionhub_tests
	cd $(BUILD_DIR) && \
	    AUCTIONHUB_DB_PASSWORD="$(AUCTIONHUB_TEST_DB_PASSWORD)" \
	    ctest --output-on-failure \
	          --output-junit test-report.xml
	@echo
	@echo "Report saved: $(BUILD_DIR)/test-report.xml"

test-unit:
	$(CMAKE) --build $(BUILD_DIR) --target auctionhub_tests
	cd $(BUILD_DIR) && ./tests/auctionhub_tests "[smoke],[jwt],[password]"

test-db:
	$(CMAKE) --build $(BUILD_DIR) --target auctionhub_tests
	cd $(BUILD_DIR) && \
	    AUCTIONHUB_DB_PASSWORD="$(AUCTIONHUB_TEST_DB_PASSWORD)" \
	    ./tests/auctionhub_tests "[db]"

# --- Форматирование ---
format-fix:
	$(CLANG_FORMAT) -i $(SRC_FILES)
	@echo "Formatting applied."

# --- Статический анализ ---
quality: build
	@echo "=== clang-format check ==="
	$(CLANG_FORMAT) --dry-run --Werror $(SRC_FILES)
	@echo
	@echo "=== clang-tidy (SAST) ==="
	@set -o pipefail; \
	$(CLANG_TIDY) -p $(BUILD_DIR) $(SRC_FILES) --quiet 2>&1 \
	    | tee $(BUILD_DIR)/clang-tidy-report.txt
	@echo
	@echo "Report saved: $(BUILD_DIR)/clang-tidy-report.txt"
	@echo "=== Quality checks passed ==="
# --- Покрытие ---
coverage:
	rm -rf $(BUILD_COVERAGE_DIR)
	$(CMAKE) -B $(BUILD_COVERAGE_DIR) -G $(GENERATOR) \
	    -DCMAKE_BUILD_TYPE=Debug \
	    -DCMAKE_CXX_COMPILER=$(CXX) \
	    -DCMAKE_CXX_FLAGS="--coverage -O0 -g"
	$(CMAKE) --build $(BUILD_COVERAGE_DIR)
	cd $(BUILD_COVERAGE_DIR) && \
	    AUCTIONHUB_DB_PASSWORD="$(AUCTIONHUB_TEST_DB_PASSWORD)" \
	    ctest --output-on-failure
	@echo
	@echo "=== Coverage summary ==="
	cd $(BUILD_COVERAGE_DIR) && \
	    $(GCOVR) \
	        --root .. \
	        --filter '../src/.*' \
	        --exclude '../src/main.cpp' \
	        --exclude '.*_deps.*' \
	        --gcov-executable "llvm-cov gcov" \
	        --gcov-ignore-errors=source_not_found \
	        --gcov-ignore-errors=no_working_dir_found \
	        --html-details coverage.html \
	        --print-summary | tee coverage-summary.txt

clean-coverage:
	rm -rf $(BUILD_COVERAGE_DIR)

# --- Полная проверка ---
verify: quality test test-migrations test-backup-restoret
	@echo
	@echo "=== All verification checks passed ==="


# --- Миграции ---
migrate:
	bash scripts/migrate.sh auctionhub

migrate-test:
	bash scripts/migrate.sh auctionhub_test

test-migrations:
	bash scripts/test-migrations.sh

backup:
	bash scripts/backup.sh auctionhub

restore:
	@if [ -z "$(FILE)" ]; then \
		echo "Usage: make restore FILE=backups/auctionhub_YYYYMMDD_HHMMSS.sql"; \
		exit 1; \
	fi
	bash scripts/restore.sh "$(FILE)" auctionhub
	
test-backup-restore:
	bash scripts/test-backup-restore.sh
