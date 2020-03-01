#include "build.h"
#include "..\macro-defs.h"
#include "..\string.h"
#include <cassert>

/*static*/ cstr_t build::time() {
	static string_t value {WIDE(__TIME__)};
	return value.c_str();
}

/*static*/ cstr_t build::date(
) {
	constexpr const auto size = _countof(__DATE__);
	typedef char_t date_t[size];

	auto get_value = [size](_in const date_t &date) {
		string_t value {date + 7};
		value.reserve(size - 1);

		auto get_month = [](_in const date_t &date) -> string::size_type {
			constexpr const auto size = 4U;
			typedef char_t month_name__t[size];
			const month_name__t month_names[] = {L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun", L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"};
			for (const auto &month_name : month_names) {
				auto is_equal = [size](_in const date_t &lhs, _in const month_name__t &rhs) {
					for (std::remove_const<decltype(size)>::type i = 0U; i+1 < size; ++i)
						if (lhs[i] != rhs[i])
							return false;
					return true;
				};
				if (is_equal(date, month_name)) {
					static_assert(4 == size, "4 == size");
					return 1 + ((month_name - month_names[0]) >> 2);
				}
			}
			return 0;
		};
		const auto month = get_month(date);
		assert(0 != month);

		value.push_back(L'-');
		value.push_back(month < 10 ? L'0' : L'1');
		value.push_back(month % 10 + L'0');

		value.push_back(L'-');
		value.push_back(date[4]);
		value.push_back(date[5]);

		return value;
	};
	static string_t value {get_value(WIDE(__DATE__)).c_str()};
	return value.c_str();
}