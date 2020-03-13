#include <winscard.h>
#include <map>
#include <cassert>

#include "common.h"
#pragma comment (lib, "winscard.lib")

using namespace nfc;

//-- device::context ------------------------------------------------------------------------------------------------------------------------------------------------
/*static*/ handle device::context::open(
	_in scope scope
) noexcept {
	const std::map<enum scope, DWORD> map {
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
/*static*/ bool device::context::close(
	_in handle handle
) noexcept {
	static_assert(sizeof(handle) == sizeof(SCARDCONTEXT), "sizeof");
	auto ctx = reinterpret_cast<SCARDCONTEXT>(handle);

	return status::ok == Winapi::SCardReleaseContext(ctx);
}

device::context::context(
	_in scope scope /*= scope::user*/
) : 
	m__handle(open(scope))
{
	assert(m__handle);
}
/*explicit*/ device::context::context(
	_in handle handle
) :
	m__handle(handle)
{}
device::context::~context(
) {
	if (m__handle)
		close(m__handle);
}

handle device::context::get_handle(
) const noexcept {
	return m__handle;
}

//-- device ------------------------------------------------------------------------------------------------------------------------------------------------
const device::context& device::get_context(
) const noexcept {
	return m__context;
}

device::device(
	_in const context &context
) :
	m__context(context)
{}

/*static*/ bool device::enum_all(
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
string::vector device::enum_all(
) {
	string::vector names;
	if (enum_all(names, m__context))
		return std::move(names);
	throw Winapi::GetLastError();
}

scard device::connect(
	_in cstr_t name, _in share_mode share_mode, _in std::initializer_list<protocol> protocols /*= {protocol::t0, protocol::t1}*/
) {
	const struct {
		std::map<device::share_mode, DWORD> share_mode;
		std::map<protocol, DWORD> protocol;
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

	SCARDHANDLE handle = NULL;		static_assert(sizeof(handle) == sizeof(nfc::handle), "sizeof");	
	auto ctx = reinterpret_cast<SCARDCONTEXT>(m__context.get_handle());	
	
	DWORD protocol = 0;

	const auto status = Winapi::SCardConnectW(ctx, name, info.share_mode, info.protocols, &handle, &protocol);
	if (status::ok != status) {
		Winapi::SetLastError(status);
		return {nullptr, static_cast<nfc::protocol>(SCARD_PROTOCOL_UNDEFINED)};
	}
	
	//trace(L"protocol: %u", protocol);
	const std::map<DWORD, nfc::protocol> map_out {
		{SCARD_PROTOCOL_UNDEFINED,	static_cast<nfc::protocol>(0)	},
		{SCARD_PROTOCOL_T0,			protocol::t0					},
		{SCARD_PROTOCOL_T1,			protocol::t1					}
	};
	
	return {reinterpret_cast<nfc::handle>(handle), map_out.at(protocol)};
}

/*static*/ bool device::transmit(
	_in const scard &scard, _in const data &data_in, _out data &data_out
) {
	DWORD size_out = 64;
	data_out.resize(size_out);

	const auto handle = reinterpret_cast<SCARDHANDLE>(scard.get_handle());
	const std::map<protocol, LPCSCARD_IO_REQUEST> pci {
		{protocol::t0, SCARD_PCI_T0},
		{protocol::t1, SCARD_PCI_T1},
	};
	
	const auto status = Winapi::SCardTransmit(
		handle, pci.at(scard.get_protocol()), data_in.data(), static_cast<DWORD>(data_in.size()), nullptr, data_out.data(), &size_out
	);
	if (status::ok != status) {
		Winapi::SetLastError(status);
		return false;
	}
	data_out.resize(size_out);
	return true;
}

/*static?*/ bool device::disconnect(
	_in handle handle, _in disposition disposition /*= disposition::leave*/
) {
	const std::map<device::disposition, DWORD> map {
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

//-- nfc::command -------------------------------------------------------------------------------------------------------------------------------------------------------
command::command(
	_in size size /*= 0*/
) :
	m__data(size)
{}
command::command(
	_in std::initializer_list<byte_t> bytes, _in crc_type crc_type /*= crc_type::none*/
) {
	auto size = bytes.size();
	if (crc_type::none != crc_type)
		size += sizeof(crc::value);
	m__data.reserve(size);
	set_bytes(bytes);
	if (crc_type::none != crc_type)
		set_crc(static_cast<crc::type>(crc_type));
}

command::size command::data_size(
) const {
	return static_cast<size>(m__data.size());
}

const byte_t* command::data_bytes(
) const {
	return m__data.data();
}
byte_t* command::data_bytes(
) {
	return m__data.data();
}

command::crc::value command::get_crc(
	_in crc::type crc_type
) const {
	return crc::get(crc_type, m__data);
}

/*protected*/ void command::set_bytes(
	_in std::initializer_list<byte_t> bytes
) {
	for (const auto byte : bytes)
		m__data.push_back(byte);
}
void command::set_crc(
	_in crc::type crc_type
) {
	const auto crc = get_crc(crc_type);
	auto p_crc = reinterpret_cast<const byte_t *>(&crc);
	m__data.push_back(p_crc[1]);
	m__data.push_back(p_crc[0]);
}


//void command::append_data(
//	_in byte_t byte
//) {
//	m__data.push_back(byte);
//}
//void command::append_data(
//	_in std::initializer_list<byte_t> bytes
//) {
//	m__data.reserve(m__data.size() + bytes.size());
//	for (const auto &byte : bytes)
//		m__data.push_back(byte);
//}
//
//void command::append_crc(
//	_in crc::value crc_value
//) {
//	auto bytes = reinterpret_cast<const byte_t *>(&crc_value);
//	append_data(bytes[1]);
//	append_data(bytes[0]);
//}
//void command::append_crc(
//	_in crc::type crc_type
//) {
//	append_crc(get_crc(crc_type));
//}


const byte_t& command::operator[](
	_in size index
) const {
	return m__data.at(index);
}
byte_t& command::operator[](
	_in size index
) {
	return m__data.at(index);
}


data& command::data(
) noexcept {
	return m__data;
}
const data& command::data(
) const noexcept {
	return m__data;
}



//-- command::crc -----------------------------------------------------------------------------------------------------------------------------------------------------
/*static*/ void command::crc::update(
	_in _out value &value, _in byte_t byte
) {
	byte ^= static_cast<byte_t>(value & 0x00FF);
	byte ^= static_cast<byte_t>(byte << 4);
	value = (value >> 8) ^ (static_cast<crc::value>(byte) << 8) ^ (static_cast<crc::value>(byte) << 3) ^ (static_cast<crc::value>(byte) >> 4);
}

/*static*/ command::crc::value command::crc::get_a(
	_in const nfc::data &bytes
) {
	value crc = 0x6363;			// ISO/IEC 14443-3 -> ITU-V.41
	for (const auto &byte : bytes)
		update(crc, byte);
	return crc;
}
/*static*/ command::crc::value command::crc::get_b(
	_in const nfc::data &bytes
) {
	value crc = 0xFFFF;			// ISO/IEC 14443-3 -> ISO/IEC 13239 (formerly ISO/IEC 3309)
	for (const auto &byte : bytes)
		update(crc, byte);
	return ~crc;
}
/*static*/ command::crc::value command::crc::get(
	_in type type, _in const nfc::data &bytes
) {
	const std::map<crc::type, value(*)(_in const nfc::data &)> func {
		{type::a, get_a},
		{type::b, get_b},
	};
	return func.at(type)(bytes);
}

command::crc::crc(
	_in type type
) :
	m__type(type)
{}

command::crc::value command::crc::get(
	_in const nfc::data &bytes
) {
	return get(m__type, bytes);
}

//-- scard -----------------------------------------------------------------------------------------------------------------------------------------------------
scard::scard(
	_in handle handle, _in protocol protocol
) :
	m__handle(handle), m__protocol(protocol)
{
	assert(m__handle);
}

handle scard::get_handle(
) const noexcept {
	return m__handle;
}
protocol scard::get_protocol(
) const noexcept {
	return m__protocol;
}

bool scard::transmit(
	_in const data &data_in, _out data &data_out
) const {
	return device::transmit(*this, data_in, data_out);
}

//-- ????? -----------------------------------------------------------------------------------------------------------------------------------------------------