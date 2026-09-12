include .env
export


BUILD_DIR := build
CMAKE     := cmake
GENERATOR := Ninja
TARGET    := auctionhub_server

.PHONY: setup build run test clean

setup:
	$(CMAKE) -B $(BUILD_DIR) -G $(GENERATOR) -DCMAKE_BUILD_TYPE=Debug

build: setup
	$(CMAKE) --build $(BUILD_DIR)

run: build
	./$(BUILD_DIR)/$(TARGET)

test:
	@echo "Tests are not configured yet."

clean:
	rm -rf $(BUILD_DIR)

migrate:
	psql -h $${AUCTIONHUB_DB_HOST:-localhost} \
	     -U $${AUCTIONHUB_DB_USER:-auctionhub} \
	     -d $${AUCTIONHUB_DB_NAME:-auctionhub} \
	     -f src/db/schema.sql
