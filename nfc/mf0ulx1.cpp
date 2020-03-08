#include <cassert>

#include "mf0ulx1.h"

using namespace nfc;

scard_mfu::scard_mfu(
	_in handle handle, _in protocol protocol
) :
	scard(handle, protocol)
{}
scard_mfu::scard_mfu(
	_in const scard &scard
) :
	scard_mfu(scard.get_handle(), scard.get_protocol())
{}

bool scard_mfu::command__read(
	_in byte_t offset, _out data &data
) const {
	command out(16);
	if (!transmit(command({0x30, offset}, command::crc_type::crc_a).data(), out.data()))
		return false;
	data = std::move(out.data());
	return true;
}

#ifdef MFU_EV1
	bool scard_mfu::command__fast_read(
		_in const range &range, _out data &data
	) const {
		assert(range.end >= range.begin);
		if (range.end == range.begin) {
			data.clear();
			return true;
		}
		command out((range.end - range.begin) << 2);		// 4*n
		if (!transmit(command({0x3A, range.begin, static_cast<byte_t>(range.end - 1)}, command::crc_type::crc_a).data(), out.data()))
			return false;
		data = std::move(out.data());
		return true;
	}

	bool scard_mfu::command__get_version(
		_out data &data
	) const {
		command out(10);
		if (!transmit(command({0x60}, command::crc_type::crc_a).data(), out.data()))
			return false;
		data = std::move(out.data());
		return true;
	}
#endif	// #ifdef MFU_EV1