#pragma once
#include "macro-defs.h"
#include <utility>

namespace stdex {

	template <typename type_t> bool is_range(
		_in const type_t &value, _in const std::pair<type_t, type_t> &range
	) {
		return (value >= range.first) && (value < range.second);
	}
	template <typename type_t> bool is_range__inclusive(
		_in const type_t &value, _in const std::pair<type_t, type_t> &range
	) {
		return (value >= range.first) && (value <= range.second);
	}


}
