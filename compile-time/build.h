#pragma once

#include "..\datetime.h"
#include "..\string.h"

struct build {
	static cstr_t time();
	static cstr_t date();
	static string_t to_string();
};

class run {

public:
	run();
	string_t to_string() const;
public:
	const datetime::date date;
	const datetime::time time;
};