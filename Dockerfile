FROM debian:trixie-slim@sha256:a29215f6a35e51e22adffa17f89e9d2ef06214e64a2bad10d765c46aea49f11f AS build

ENV DEBIAN_FRONTEND=noninteractive
WORKDIR /src

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates cmake g++ git ninja-build \
    qt6-base-dev qt6-httpserver-dev qt6-websockets-dev libqt6sql6-psql libpq-dev \
    && rm -rf /var/lib/apt/lists/*

COPY . .
RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --target auctionhub_server --parallel 2

FROM debian:trixie-slim@sha256:a29215f6a35e51e22adffa17f89e9d2ef06214e64a2bad10d765c46aea49f11f AS runtime

ENV DEBIAN_FRONTEND=noninteractive \
    AUCTIONHUB_HOST=0.0.0.0 \
    AUCTIONHUB_PORT=8080

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates curl postgresql-client bash \
    libqt6core6t64 libqt6network6 libqt6sql6 \
    libqt6httpserver6 libqt6websockets6 libqt6sql6-psql libpq5 \
    && rm -rf /var/lib/apt/lists/* \
    && groupadd --system --gid 10001 auctionhub \
    && useradd --system --uid 10001 --gid auctionhub \
       --home-dir /app --no-create-home --shell /usr/sbin/nologin auctionhub \
    && mkdir -p /app/web /app/migrations /app/scripts \
    && chown -R root:root /app

WORKDIR /app

COPY --from=build /src/build/auctionhub_server /usr/local/bin/auctionhub_server
COPY web/ /app/web/
COPY migrations/ /app/migrations/
COPY scripts/migrate.sh /app/scripts/migrate.sh
COPY docker/app-entrypoint.sh /app/entrypoint.sh

RUN chmod 0555 /app/entrypoint.sh /app/scripts/migrate.sh \
    && chmod 0555 /usr/local/bin/auctionhub_server

USER 10001:10001
EXPOSE 8080
ENTRYPOINT ["/app/entrypoint.sh"]
