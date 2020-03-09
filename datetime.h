#pragma once

#include "string.h"
#include "system-types.h"
#include "macro-defs.h"
#include <ctime>

namespace datetime {

	struct date {
		uint16_t year;		// 0..
		uint8_t month,		// 0..11
			day;			// 0..30

		date(_in uint16_t year = 0, _in uint8_t month = 0, _in uint8_t day = 0);
		string_t to_string() const;
	};
	struct time {
		uint8_t hour, minute, second;

		time(_in uint8_t hour = 0, _in uint8_t minute = 0, _in uint8_t second = 0);
		string_t to_string() const;
	};

	std::time_t now();
	bool to_struct(_out std::tm &tm, _in std::time_t time = now());

}	// namespace datetime