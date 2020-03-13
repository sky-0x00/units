#pragma once

#include "common.h"
#include <array>

//#define MFU_EV1

namespace nfc {

	class scard_mfu: public scard {
	public:
		//typedef byte_t page[4];
		//typedef page block[4];
#ifdef MFU_EV1
		struct range {
			byte_t begin, end;
		};
#endif
		typedef byte_t data_enc[80], *p_data_enc;
		typedef const byte_t *pc_data_enc;
		
#pragma pack(push,1)
		struct data_dec {

			//typedef byte_t uid_t[7];
			typedef std::array<byte_t, 7> uid_t;
			typedef std::array<byte_t, 2> bcc_t;
			typedef byte_t internal_t;
			typedef std::array<byte_t, 2> lock_bytes_t;
			typedef std::array<byte_t, 4> otp_t;

			static const byte_t ct = 0x88;

			byte_t uid_0, uid_1, uid_2, bcc_0;							// page_0, bcc_0 = ct ^ uid_0 ^ uid_1 ^ uid_2
			byte_t uid_3, uid_4, uid_5, uid_6;							// page_1
			byte_t bcc_1, internal, lock_bytes_0, lock_bytes_1;			// page_2, bcc_1 = uid_3 ^ uid_4 ^ uid_5 ^ uid_6
			byte_t otp_0, otp_1, otp_2, otp_3;							// page_3
			byte_t user_data[4*(16-4)];									// page_4..15
			uint32_t cfg_0, cfg_1;										// page_16..17
			uint32_t pwd;												// page_18
			uint16_t pack, rfui;										// page_19
			
			uid_t uid() const noexcept;
		};
		typedef data_dec *p_data_dec;
		typedef const data_dec *pc_data_dec;
#pragma pack(pop)

	public:
		scard_mfu(_in handle handle, _in protocol protocol);
		scard_mfu(_in const scard &scard);
		//static bool decode(_in const data_enc &data_enc, _out data_dec &data_dec);
	public:
		bool command__read(_in byte_t offset, _out data &response) const;				// read memory-block of 4 pages starting at offset address
		bool command__reqa(_out data &response) const;									// in req-a, out atq-a (not working)
		bool command__firmware_version(_out data &response) const;						// firmware-version of the reader
		bool command__get_status(_out data &response) const;							// get-status
		bool command__antenna_switch(_in bool is_on) const;								// turn antenna on/off
#ifdef MFU_EV1
		bool command__fast_read(_in const range &range, _out data &data) const;			// fast-read memory-block of range pages [range.begin; range.end)
		bool command__get_version(_out data &data) const;
#endif
	private:		
	};

}