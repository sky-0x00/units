#pragma once

#include <sal.h>

typedef wchar_t char_t;
typedef __nullterminated char_t *str_t;
typedef __nullterminated const char_t *cstr_t;
typedef __nullnullterminated char_t *mstr_t;
typedef __nullnullterminated const char_t *cmstr_t;

typedef char char_at;
typedef __nullterminated char_at *str_at;
typedef __nullterminated const char_at *cstr_at;
typedef __nullnullterminated char_at *mstr_at;
typedef __nullnullterminated const char_at *cmstr_at;

typedef unsigned argc_t, result_t;
typedef cstr_t argv_t;

typedef void void_t;
typedef void_t *pvoid_t, *handle_t;
typedef const void_t *cpvoid_t;

enum class bool_t : bool {
	no = false,
	yes = true
};

typedef unsigned char byte_t;