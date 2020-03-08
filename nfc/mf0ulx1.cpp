#include "mf0ulx1.h"

using namespace nfc;

mf0ulx1::mf0ulx1(
	_in const scard::device &device, _in const scard::info &scard_info
) :
	m__device(device), m__scard_info(scard_info)
{}