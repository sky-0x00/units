#include "datetime.h"

using namespace datetime;

date::date(
	_in uint16_t year /*= 0*/, _in uint8_t month /*= 0*/, _in uint8_t day /*= 0*/
) :
	year(year), month(month), day(day)
{}

time::time(
	_in uint8_t hour /*= 0*/, _in uint8_t minute /*= 0*/, _in uint8_t second /*= 0*/
) :
	hour(hour), minute(minute), second(second)
{}

std::time_t datetime::now(
) {
	return std::time(nullptr);
}
bool datetime::to_struct(
	_out std::tm &tm, _in std::time_t time /*= now()*/
) {
	return 0 == localtime_s(&tm, &time);
}

string_t date::to_string(
) const {
	char_t buffer[] = L"yyyy-mm-dd";
	swprintf_s(buffer, L"%04u-%02u-%02u", year, month+1, day+1);
	return buffer;
}

string_t time::to_string(
) const {
	char_t buffer[] = L"hh:mm:ss";
	swprintf_s(buffer, L"%02u:%02u:%02u", hour, minute, second);
	return buffer;
}