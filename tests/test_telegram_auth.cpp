#include "../server/telegram_auth.h"
#include "doctest/doctest.h"
#include <algorithm>
#include <map>
#include <set>

extern std::map<std::string, std::string> auth_codes;

TEST_SUITE("telegram_auth") {
	TEST_CASE("generate_auth_code → 6 digits") {
		auto code = generate_auth_code();
		CHECK(code.size() == 6);
		CHECK(std::all_of(code.begin(), code.end(), ::isdigit));
	}

	TEST_CASE("codes look random-ish") {
		std::set<std::string> s;
		for (int i = 0; i < 50; ++i)
			s.insert(generate_auth_code());
		CHECK(s.size() > 1);
	}

	TEST_CASE("verify_auth_code normal flow") {
		auth_codes.clear();
		auth_codes["42"] = "123456";

		CHECK(verify_auth_code("42", "123456"));
		CHECK_FALSE(verify_auth_code("42", "000000"));
		CHECK_FALSE(verify_auth_code("99", "123456"));
	}
}
