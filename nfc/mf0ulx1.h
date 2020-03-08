#pragma once

#include "common.h"

namespace nfc {

	class mf0ulx1 {
	public:
		mf0ulx1(_in const scard::device &device, _in const scard::info &scard_info);
	private:
		const scard::device &m__device;
		const scard::info &m__scard_info;
	};

}