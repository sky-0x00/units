#pragma once

#include "..\system-types.h"
#include "..\macro-defs.h"
#include "..\string.h"
#include <initializer_list>
#include <vector>

namespace nfc {

	struct status {
		typedef unsigned value;
		static constexpr value ok = 0;
	};
	typedef std::vector<byte_t> data;
	
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
		
		class command {
		public:
			typedef unsigned size;
			class crc {
			public:
				typedef uint16_t value;
				enum class type {
					a, b
				};
				static value get_a(_in const data &bytes);
				static value get_b(_in const data &bytes);
				static value get(_in type type, _in const data &bytes);
			public:
				crc(_in type type);
				value get(_in const data &bytes);
			protected:
				static void update(_in _out value &value, _in byte_t byte);
			private:
				const type _type;
			};
		public:
			command(_in size size = 0);
			command(_in std::initializer_list<byte_t> bytes);
			void set(_in std::initializer_list<byte_t> bytes);
			void clear();
			command& operator <<(_in byte_t byte);
			command& operator <<(_in std::initializer_list<byte_t> bytes);
			const byte_t& operator[](_in size index) const;
			byte_t& operator[](_in size index);
		public:
			nfc::data& data() noexcept;
			const nfc::data& data() const noexcept;
		public:
			crc::value get_crc(_in crc::type crc_type) const;
			command& operator <<(_in crc::value crc_value);
		protected:
			void _set(_in bool is__clear_first, _in std::initializer_list<byte_t> bytes);
		private:
			nfc::data _data;
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
			bool transmit(_in const scard::info &scard_info, _in const data &data_in, _out data &data_out) const;
			/*static?*/ bool disconnect(_in handle handle, _in disposition disposition = disposition::leave);
		protected:
			const context& get_context() const noexcept;
		private:
			const context _context;
		};
	};


}
