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
	typedef handle_t handle;
	enum class protocol {
		t0, t1
	};
	
	class command {
	public:
		
		class crc {
		public:
			typedef uint16_t value;
			enum /*class*/ type {
				a = 1, b = 2
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
			const type m__type;
		};

		typedef unsigned size;
		enum class crc_type {
			none = 0,
			crc_a = crc::a,
			crc_b = crc::b
		};

	public:
		command(_in size size = 0);
		command(_in std::initializer_list<byte_t> bytes, _in crc_type crc_type = crc_type::none);
		//void append_data(_in byte_t byte);
		//void append_data(_in std::initializer_list<byte_t> bytes);
		//void append_crc(_in crc::value crc_value);
		//void append_crc(_in crc::type crc_type);
	public:
		const byte_t& operator[](_in size index) const;
		byte_t& operator[](_in size index);		
	protected:
		void set_bytes(_in std::initializer_list<byte_t> bytes);
		void set_crc(_in crc_type crc_type);
		crc::value get_crc(_in crc::type crc_type) const;
	protected:
		size data_size() const;
		const byte_t* data_bytes() const;
		byte_t* data_bytes();
	public:
		nfc::data& data() noexcept;
		const nfc::data& data() const noexcept;
	private:
		nfc::data m__data;
	};

	class scard;

	class device {
	public:
		class context {
		public:
			enum /*class*/ scope {
				user, system
			};
		public:
			context(_in scope scope = scope::user);
			explicit context(_in handle handle);
			~context();
			handle get_handle() const noexcept;
		protected:
			static handle open(_in scope scope) noexcept;
			static bool close(_in handle handle) noexcept;
		private:
			const handle m__handle;
		};

		enum class share_mode {
			shared, exclusive, direct
		};
		enum class disposition {
			leave, reset, unpower, eject
		};
	public:
		device(_in const context &context);
		string::vector enum_all();
		static bool enum_all(_out string::vector &names, _in const context &context = context(nullptr)) noexcept;
		scard connect(_in cstr_t name, _in share_mode share_mode, _in std::initializer_list<protocol> protocols = { protocol::t0, protocol::t1 });
		/*static?*/ bool disconnect(_in handle scard_handle, _in disposition disposition = disposition::leave);
		static bool transmit(_in const scard &scard, _in const data &data_in, _out data &data_out);		
	protected:
		const context& get_context() const noexcept;
	private:
		const context m__context;
	};

	class scard {
	public:
		scard(_in handle handle, _in protocol protocol);
		handle get_handle() const noexcept;
		protocol get_protocol() const noexcept;
	protected:
		bool transmit(_in const data &data_in, _out data &data_out) const;
	private:
		const handle m__handle;
		const protocol m__protocol;
	};
}
