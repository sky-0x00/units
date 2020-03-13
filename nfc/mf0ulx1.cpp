#include <cassert>
#include <windows.h>

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
	_in byte_t offset, _out data &response
) const {
	return transmit(command({ 0x30, offset }, command::crc_type::crc_a).data(), response);
}

bool scard_mfu::command__reqa(
	_out data &response
) const {
	return transmit(command({ 0x26 }).data(), response);
}

bool scard_mfu::command__firmware_version(
	_out data &response
) const {
	return transmit(command({ 0xff, 0x00, 0x48, 0x00, 0x00 }).data(), response);
}
bool scard_mfu::command__get_status(
	_out data &response
) const {
	return transmit(command({ 0xff, 0x00, 0x00, 0x00, 0x02, 0xd4, 0x04 }).data(), response);
}

bool scard_mfu::command__antenna_switch(
	_in bool is_on
) const {
	const byte_t value = is_on ? 0x01 : 0x00;
	data response;
	return transmit(command({ 0xff, 0x00, 0x00, 0x00, 0x04, 0xd4, 0x32, 0x01, value }).data(), response);
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

//-------------------------------------------------------------------------------------------------------------------------------------------------------
///*static*/ bool scard_mfu::decode(
//	_in const data_enc &data_enc, _out data_dec &data_dec
//) {
//	if (80 != data_enc.size()) {
//		Winapi::SetLastError(SCARD_E_INVALID_PARAMETER);
//		return false;
//	}
//	return false;
//}

scard_mfu::data_dec::uid_t scard_mfu::data_dec::uid(
) const noexcept {
	return {uid_0, uid_1, uid_2, uid_3, uid_4, uid_5, uid_6};
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------