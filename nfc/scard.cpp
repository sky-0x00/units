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

//-- scard -------------------------------------------------------------------------------------------------------------------------------------------------------