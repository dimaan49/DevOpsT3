#include <catch2/catch_test_macros.hpp>

#include "../src/server/jwt.h"

#include <QCoreApplication>
#include <QDateTime>

namespace {

// Инициализация Qt нужна для QDateTime, QJsonDocument и т.д.
struct QtFixture {
	QtFixture() {
		if (!QCoreApplication::instance()) {
			static int argc = 1;
			static char arg0[] = "tests";
			static char *argv[] = { arg0, nullptr };
			new QCoreApplication(argc, argv);
		}
	}
};

} // namespace

TEST_CASE("JWT: encode then decode returns same user id", "[jwt]")
{
	QtFixture fixture;

	qputenv("AUCTIONHUB_JWT_SECRET", "test_secret_64_chars_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");

	const qint64 userId = 42;
	const qint64 ttl = 3600;

	const QString token = auctionhub::server::jwtEncode(userId, ttl);
	REQUIRE(!token.isEmpty());

	const auto payload = auctionhub::server::jwtDecode(token);
	REQUIRE(payload.has_value());
	REQUIRE(payload->userId == userId);
}

TEST_CASE("JWT: decode with wrong secret fails", "[jwt]")
{
	QtFixture fixture;

	qputenv("AUCTIONHUB_JWT_SECRET", "secret_one_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
	const QString token = auctionhub::server::jwtEncode(1, 3600);

	qputenv("AUCTIONHUB_JWT_SECRET", "secret_two_bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
	const auto payload = auctionhub::server::jwtDecode(token);

	REQUIRE_FALSE(payload.has_value());
}

TEST_CASE("JWT: expired token fails", "[jwt]")
{
	QtFixture fixture;

	qputenv("AUCTIONHUB_JWT_SECRET", "test_secret_64_chars_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");

	// TTL = -1 секунда → токен уже истёк
	const QString token = auctionhub::server::jwtEncode(1, -1);
	const auto payload = auctionhub::server::jwtDecode(token);

	REQUIRE_FALSE(payload.has_value());
}

TEST_CASE("JWT: garbage token fails", "[jwt]")
{
	QtFixture fixture;

	qputenv("AUCTIONHUB_JWT_SECRET", "test_secret_64_chars_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");

	REQUIRE_FALSE(auctionhub::server::jwtDecode("not.a.jwt").has_value());
	REQUIRE_FALSE(auctionhub::server::jwtDecode("").has_value());
	REQUIRE_FALSE(auctionhub::server::jwtDecode("only_one_part").has_value());
}

TEST_CASE("JWT: empty secret returns empty token", "[jwt]")
{
	QtFixture fixture;

	qunsetenv("AUCTIONHUB_JWT_SECRET");

	const QString token = auctionhub::server::jwtEncode(1, 3600);
	REQUIRE(token.isEmpty());
}
