#pragma once

#define _in
#define _out
#define _optional

#define set_lasterror(x) x

#ifndef NAMESPACE_WINAPI
	#define Winapi
#endif

#define _WIDE(x)	L##x
#define WIDE(x)		_WIDE(x)
#define _STR(x)		L#x
#define STR(x)		_STR(x)