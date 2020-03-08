#pragma once

#include "common.h"

#define MFU_EV1

namespace nfc {

	class scard_mfu: public scard {
	public:
		//typedef byte_t page[4];
		//typedef page block[4];
		struct range {
			byte_t begin, end;
		};
	public:
		scard_mfu(_in handle handle, _in protocol protocol);
		scard_mfu(_in const scard &scard);
	public:
		bool command__read(_in byte_t offset, _out data &data) const;				// read memory-block of 4 pages starting at offset address
#ifdef MFU_EV1
		bool command__fast_read(_in const range &range, _out data &data) const;		// fast-read memory-block of range pages [range.begin; range.end)
		bool command__get_version(_out data &data) const;
#endif
	private:		
	};

}