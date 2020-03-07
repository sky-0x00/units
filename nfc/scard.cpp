#include "scard.h"

#include <winscard.h>
#include <map>
#include <cassert>

#pragma comment (lib, "winscard.lib")

using namespace nfc;

//-- scard::context ------------------------------------------------------------------------------------------------------------------------------------------------
/*static*/ scard::context::handle scard::context::open(
	_in scope scope
) noexcept {
	const std::map<enum class scope, DWORD> map {
		{ scope::user,		SCARD_SCOPE_USER	},
		{ scope::system,	SCARD_SCOPE_SYSTEM	}
	};

	static_assert(sizeof(handle) == sizeof(SCARDCONTEXT), "sizeof");
	SCARDCONTEXT ctx;

	const auto status = Winapi::SCardEstablishContext(map.at(scope), nullptr, nullptr, &ctx);
	if (status::ok != status)
		Winapi::SetLastError(status);

	return reinterpret_cast<handle>(ctx);
}
/*static*/ bool scard::context::close(
	_in handle handle
) noexcept {
	static_assert(sizeof(handle) == sizeof(SCARDCONTEXT), "sizeof");
	auto ctx = reinterpret_cast<SCARDCONTEXT>(handle);

	return status::ok == Winapi::SCardReleaseContext(ctx);
}

scard::context::context(
	_in scope scope /*= scope::user*/
) : 
	_handle(open(scope))
{
	assert(_handle);
}
/*explicit*/ scard::context::context(
	_in handle handle
) :
	_handle(handle)
{}
scard::context::~context(
) {
	if (_handle)
		close(_handle);
}

scard::context::handle scard::context::get_handle(
) const noexcept {
	return _handle;
}

#ifdef _DEBUG
scard::context::operator handle(
) const noexcept {
	return get_handle();
}
#endif

//-- scard::device ------------------------------------------------------------------------------------------------------------------------------------------------
const scard::context& scard::device::get_context(
) const noexcept {
	return _context;
}

scard::device::device(
	_in const context &context
) :
	_context(context)
{}

/*static*/ bool scard::device::enum_all(
	_out string::vector &devices, _in const context &context /*= context(nullptr)*/
) noexcept {
	auto ctx = reinterpret_cast<SCARDCONTEXT>(context.get_handle());
	mstr_t names = nullptr;
	DWORD names_size = SCARD_AUTOALLOCATE;

	const auto status = Winapi::SCardListReadersW(ctx, nullptr, reinterpret_cast<mstr_t>(&names), &names_size);
	switch (status) {
		case SCARD_S_SUCCESS:
			for (cstr_t name = names; L'\0' != *name; name += 1 + wcslen(name))
				devices.emplace_back(name);
			Winapi::SCardFreeMemory(ctx, names);
		case SCARD_E_NO_READERS_AVAILABLE:
			return true;
		default:
			Winapi::SetLastError(status);
			return false;
	}
}
string::vector scard::device::enum_all(
) {
	string::vector names;
	if (enum_all(names, _context))
		return std::move(names);
	throw Winapi::GetLastError();
}

scard::info scard::device::connect(
	_in cstr_t name, _in share_mode share_mode, _in std::initializer_list<protocol> protocols /*= {protocol::t0, protocol::t1}*/
) {
	const struct {
		std::map<device::share_mode, DWORD> share_mode;
		std::map<scard::protocol, DWORD> protocol;
	} map_in {
		{
			{ share_mode::shared,		SCARD_SHARE_SHARED		},
			{ share_mode::exclusive,	SCARD_SHARE_EXCLUSIVE	},
			{ share_mode::direct,		SCARD_SHARE_DIRECT		}
		}, {
			{ protocol::t0, SCARD_PROTOCOL_T0 },
			{ protocol::t1, SCARD_PROTOCOL_T1 }
		}
	};

	struct {
		DWORD share_mode, protocols;
	} info {
		map_in.share_mode.at(share_mode), 0
	};
	for (const auto &protocol : protocols)
		info.protocols |= map_in.protocol.at(protocol);

	if (0 == info.protocols)
		assert(share_mode::direct == share_mode);

	SCARDHANDLE handle = NULL;
	DWORD protocol = 0;
	auto ctx = reinterpret_cast<SCARDCONTEXT>(_context.get_handle());

	const auto status = Winapi::SCardConnectW(ctx, name, info.share_mode, info.protocols, &handle, &protocol);
	if (status::ok != status) {
		Winapi::SetLastError(status);
		return {nullptr, static_cast<scard::protocol>(SCARD_PROTOCOL_UNDEFINED)};
	}
	
	//trace(L"protocol: %u", protocol);
	const std::map<DWORD, scard::protocol> map_out {
		{SCARD_PROTOCOL_UNDEFINED,	static_cast<scard::protocol>(0)	},
		{SCARD_PROTOCOL_T0,			protocol::t0					},
		{SCARD_PROTOCOL_T1,			protocol::t1					}
	};
	static_assert(sizeof(handle) == sizeof(scard::handle), "sizeof");
	return {reinterpret_cast<scard::handle>(handle), map_out.at(protocol)};
}

bool scard::device::transmit(
	_in const scard::info &scard_info, _in const data &data_in, _out data &data_out
) const {
	assert(2 <= data_out.size());

	const auto handle = reinterpret_cast<SCARDHANDLE>(scard_info.handle);
	const std::map<scard::protocol, LPCSCARD_IO_REQUEST> pci {
		{scard::protocol::t0, SCARD_PCI_T0},
		{scard::protocol::t1, SCARD_PCI_T1},
	};
	
	DWORD size_out = 0;
	const auto status = Winapi::SCardTransmit(
		handle, pci.at(scard_info.protocol), data_in.data(), static_cast<DWORD>(data_in.size()), nullptr, data_out.data(), &size_out
	);
	if (status::ok != status) {
		Winapi::SetLastError(status);
		return false;
	}
	data_out.resize(size_out);
	return true;
}

/*static?*/ bool scard::device::disconnect(
	_in handle handle, _in disposition disposition /*= disposition::leave*/
) {
	const std::map<scard::disposition, DWORD> map {
		{ disposition::leave,	SCARD_LEAVE_CARD	},
		{ disposition::reset,	SCARD_RESET_CARD	},
		{ disposition::unpower,	SCARD_UNPOWER_CARD	},
		{ disposition::eject,	SCARD_EJECT_CARD	}
	};
	static_assert(sizeof(handle) == sizeof(SCARDHANDLE), "sizeof");
	const auto status = Winapi::SCardDisconnect(reinterpret_cast<SCARDHANDLE>(handle), map.at(disposition));
	if (status::ok == status)
		return true;
	Winapi::SetLastError(status);
	return false;
}

//-- scard::command -------------------------------------------------------------------------------------------------------------------------------------------------------
scard::command::command(
	_in size size /*= 0*/
) :
	_data(size)
{}
scard::command::command(
	_in std::initializer_list<byte_t> bytes
) {
	_set(false, bytes);
}

void scard::command::_set(
	_in bool is__clear_first, _in std::initializer_list<byte_t> bytes
) {
	if (is__clear_first)
		_data.clear();
	_data.reserve(bytes.size());
	for (const auto byte : bytes)
		_data.push_back(byte);
}
void scard::command::set(
	_in std::initializer_list<byte_t> bytes
) {
	_set(true, bytes);
}

void scard::command::clear(
) {
	_data.clear();
}

scard::command& scard::command::operator <<(
	_in byte_t byte
) {
	_data.push_back(byte);
	return *this;
}
scard::command& scard::command::operator <<(
	_in std::initializer_list<byte_t> bytes
) {
	_data.reserve(_data.size() + bytes.size());
	for (const auto byte : bytes)
		_data.push_back(byte);
	return *this;
}

const byte_t& scard::command::operator[](
	_in size index
) const {
	return _data.at(index);
}
byte_t& scard::command::operator[](
	_in size index
) {
	return _data.at(index);
}

scard::command::crc::value scard::command::get_crc(
	_in crc::type crc_type
) const {
	return crc::get(crc_type, _data);
}
scard::command& scard::command::operator <<(
	_in crc::value crc_value
) {
	auto bytes = reinterpret_cast<const byte_t *>(&crc_value);
	return operator << ({bytes[1], bytes[0]});
}

nfc::data& scard::command::data(
) noexcept {
	return _data;
}
const nfc::data& scard::command::data(
) const noexcept {
	return _data;
}

//-- scard::command::crc -----------------------------------------------------------------------------------------------------------------------------------------------------
/*static*/ void scard::command::crc::update(
	_in _out value &value, _in byte_t byte
) {
	byte ^= static_cast<byte_t>(value & 0x00FF);
	byte ^= static_cast<byte_t>(byte << 4);
	value = (value >> 8) ^ (static_cast<crc::value>(byte) << 8) ^ (static_cast<crc::value>(byte) << 3) ^ (static_cast<crc::value>(byte) >> 4);
}

/*static*/ scard::command::crc::value scard::command::crc::get_a(
	_in const nfc::data &bytes
) {
	value crc = 0x6363;			// ISO/IEC 14443-3 -> ITU-V.41
	for (const auto &byte : bytes)
		update(crc, byte);
	return crc;
}
/*static*/ scard::command::crc::value scard::command::crc::get_b(
	_in const nfc::data &bytes
) {
	value crc = 0xFFFF;			// ISO/IEC 14443-3 -> ISO/IEC 13239 (formerly ISO/IEC 3309)
	for (const auto &byte : bytes)
		update(crc, byte);
	return ~crc;
}
/*static*/ scard::command::crc::value scard::command::crc::get(
	_in type type, _in const nfc::data &bytes
) {
	const std::map<crc::type, value(*)(_in const nfc::data &)> func {
		{type::a, get_a},
		{type::b, get_b},
	};
	return func.at(type)(bytes);
}

scard::command::crc::crc(
	_in type type
) :
	_type(type)
{}

scard::command::crc::value scard::command::crc::get(
	_in const nfc::data &bytes
) {
	return get(_type, bytes);
}

//-- scard -----------------------------------------------------------------------------------------------------------------------------------------------------
