/**
 \author Created by Sventovit
 \date 26.05.2021.
 \brief Реализация DescriptorData
 \details Реализация класса, описывающего состояние соединения клиента с сервером.
*/

#include "descriptor_data.h"

#include "chars/char_player.h"
#include "msdp/msdp.h"
#include "msdp/msdp_constants.h"

DESCRIPTOR_DATA::DESCRIPTOR_DATA() : bad_pws(0),
									 idle_tics(0),
									 connected(0),
									 desc_num(0),
									 input_time(0),
									 login_time(0),
									 showstr_head(0),
									 showstr_vector(0),
									 showstr_count(0),
									 showstr_page(0),
									 max_str(0),
									 backstr(0),
									 mail_to(0),
									 has_prompt(0),
									 output(0),
									 history(0),
									 history_pos(0),
									 bufptr(0),
									 bufspace(0),
									 large_outbuf(0),
									 character(0),
									 original(0),
									 snooping(0),
									 snoop_by(0),
									 next(0),
									 olc(0),
									 keytable(0),
									 options(0),
									 deflate(nullptr),
									 mccp_version(0),
									 ip(0),
									 registered_email(0),
									 pers_log(0),
									 cur_vnum(0),
									 old_vnum(0),
									 snoop_with_map(0),
									 m_msdp_support(false),
									 m_msdp_last_max_hit(0),
									 m_msdp_last_max_move(0) {
	host[0] = 0;
	inbuf[0] = 0;
	last_input[0] = 0;
	small_outbuf[0] = 0;
}

void DESCRIPTOR_DATA::msdp_support(bool on) {
	log("INFO: MSDP support enabled for client %s.\n", host);
	m_msdp_support = on;
}

void DESCRIPTOR_DATA::msdp_report(const std::string &name) {
	if (m_msdp_support && msdp_need_report(name)) {
		msdp::report(this, name);
	}
}

// Should be called periodically to update changing msdp variables.
// this is mostly to overcome complication of hunting every possible place affect are added/removed to/from char.
void DESCRIPTOR_DATA::msdp_report_changed_vars() {
	if (!m_msdp_support || !character) {
		return;
	}

	if (m_msdp_last_max_hit != GET_REAL_MAX_HIT(character)) {
		msdp_report(msdp::constants::MAX_HIT);
		m_msdp_last_max_hit = GET_REAL_MAX_HIT(character);
	}

	if (m_msdp_last_max_move != GET_REAL_MAX_MOVE(character)) {
		msdp_report(msdp::constants::MAX_MOVE);
		m_msdp_last_max_move = GET_REAL_MAX_MOVE(character);
	}
}

void DESCRIPTOR_DATA::string_to_client_encoding(const char *input, char *output) const {
	switch (keytable) {
		case kCodePageAlt: for (; *input; *output = KtoA(*input), input++, output++);
			break;

		case kCodePageWin:
			for (; *input; input++, output++) {
				*output = KtoW(*input);

				// 0xFF is cp1251 'я' and Telnet IAC, so escape it with another IAC
				if (*output == '\xFF') {
					*++output = '\xFF';
				}
			}
			break;

		case kCodePageWinzOld:
		case kCodePageWinzZ:
			// zMUD before 6.39 or after for backward compatibility  - replace я with z
			for (; *input; *output = KtoW2(*input), input++, output++);
			break;

		case kCodePageWinz:
			// zMUD after 6.39 and CMUD support 'я' but with some issues
			for (; *input; input++, output++) {
				*output = KtoW(*input);

				// 0xFF is cp1251 'я' and Telnet IAC, so escape it with antother IAC
				// also there is a bug in zMUD, meaning we need to add an extra byte
				if (*output == '\xFF') {
					*++output = '\xFF';
					// make it obvious for other clients something is wrong
					*++output = '?';
				}
			}
			break;

		case kCodePageUTF8:
			// Anton Gorev (2016-04-25): we have to be careful. String in UTF-8 encoding may
			// contain character with code 0xff which telnet interprets as IAC.
			// II:  FE and FF were never defined for any purpose in UTF-8, we are safe
			koi_to_utf8(const_cast<char *>(input), output);
			break;

		default: for (; *input; *output = *input, input++, output++);
			break;
	}

	if (keytable != kCodePageUTF8) {
		*output = '\0';
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :