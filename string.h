#pragma once
#include <string>
#include <list>
#include <vector>

typedef std::string string_at;
typedef std::wstring string_t;

class string {
public:
	typedef unsigned size_type;

	typedef std::list<string_t> list;			// change string_t -> string
	typedef std::vector<string_t> vector;		// change string_t -> string

public:
private:
	string_t _data;
};