#pragma once

#include "..\system-types.h"
#include "..\macro-defs.h"
#include "..\string.h"
#include <initializer_list>

namespace nfc {

	struct status {
		typedef unsigned value;
		static constexpr value ok = 0;
	};

	class scard {
	public:
		typedef handle_t handle;
		enum class protocol {
			t0, t1
		};
		struct info {
			handle handle;
			protocol protocol;
		};
		enum class disposition {
			leave, reset, unpower, eject
		};

		class context {
		public:
			typedef handle_t handle;
			enum class scope {
				user, system
			};
		public:
			context(_in scope scope = scope::user);
			explicit context(_in handle handle);
			~context();
#ifdef _DEBUG
			operator handle() const noexcept;
#endif
			handle get_handle() const noexcept;
		protected:
			static handle open(_in scope scope) noexcept;
			static bool close(_in handle handle) noexcept;

		private:
			const handle _handle;
		};

		class device {
		public:
			enum class share_mode {
				shared, exclusive, direct
			};			
		public:
			device(_in const context &context);
			string::vector enum_all();
			static bool enum_all(_out string::vector &names, _in const context &context = context(nullptr)) noexcept;
			info connect(_in cstr_t name, _in share_mode share_mode, _in std::initializer_list<protocol> protocols = {protocol::t0, protocol::t1});
			/*static?*/ bool disconnect(_in handle handle, _in disposition disposition = disposition::leave);
		protected:
			const context& get_context() const noexcept;
		private:
			const context _context;
		};
	};


}
