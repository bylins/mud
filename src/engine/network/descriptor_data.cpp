/**
 \author Created by Sventovit
 \date 26.05.2021.
 \brief Реализация DescriptorData
 \details Реализация класса, описывающего состояние соединения клиента с сервером.
*/

#include "descriptor_data.h"

#include "engine/entities/char_player.h"
#include "engine/network/msdp/msdp.h"
#include "engine/network/msdp/msdp_constants.h"

#include <vector>

static const std::vector<const char*> connection_descriptions = 	{
	// 0-9
	"В игре",
	"Disconnecting",
	"Get name",
	"Confirm name",
	"Get password",
	"Get new PW",
	"Confirm new PW",
	"Select sex",
	"Select class",
	"Reading MOTD",
	// 10-19
	"Main Menu",
	"Get descript.",
	"Changing PW 1",
	"Changing PW 2",
	"Changing PW 3",
	"Self-Delete 1",
	"Self-Delete 2",
	"Disconnecting",
	"Object edit",
	"Room edit",
	// 20-29
	"Zone edit",
	"Mobile edit",
	"Trigger edit",
	"Get name2",
	"Get name3",
	"Get name4",
	"Get name5",
	"Get name6",
	"Select religion",
	"Select race",
	// 30-39
	"ERROR",		// удалено, не используется
	"Select keytable",
	"Get email",
	"Roll stats",
	"Recept edit",
	"Select kin",
	"Write note",		// удалено, можно использовать
	"ERROR",		// удалено, можно использовать
	"map olc",
	"ERROR",		// удалено, можно использовать
	// 40-49
	"Board message edit",
	"House edit",
	"Generate new name",
	"Glory OLC",
	"Base stats reroll",
	"Select place of birth",
	"Clan MoD edit",
	"GloryConst OLC",
	"NamedStuff OLC",
	"Select new kin",
	// 50-57
	"Select new race",
	"Interactive console",
	"обмен гривен",
	"меню сброса параметров",
	"sedit",
	"select new religion",
	"Verification",
	"Just connected"
};

const char *GetConDescription(EConState state) {
	auto index = static_cast<size_t>(state);
	if (index < connection_descriptions.size()) {
		return connection_descriptions[index];
	}
	return "Unknown state";
}

DescriptorData::DescriptorData() : bad_pws(0),
								   idle_tics(0),
								   state(EConState::kPlaying),
								   desc_num(0),
								   input_time(0),
								   login_time(0),
								   showstr_head(nullptr),
								   showstr_vector(nullptr),
								   showstr_count(0),
								   showstr_page(0),
								   max_str(0),
								   backstr(nullptr),
								   mail_to(0),
								   has_prompt(0),
								   output(nullptr),
								   history(nullptr),
								   history_pos(0),
								   bufptr(0),
								   bufspace(0),
								   large_outbuf(nullptr),
								   character(nullptr),
								   original(nullptr),
								   snooping(nullptr),
								   snoop_by(nullptr),
								   next(),
								   olc(nullptr),
								   keytable(0),
								   options(0),
								   deflate(nullptr),
								   mccp_version(0),
								   ip(0),
								   registered_email(false),
								   pers_log(nullptr),
								   cur_vnum(0),
								   old_vnum(0),
								   snoop_with_map(false),
								   m_msdp_support(false),
								   m_msdp_last_max_hit(0),
								   m_msdp_last_max_move(0) {
	host[0] = 0;
	inbuf[0] = 0;
	last_input[0] = 0;
	small_outbuf[0] = 0;
}

void DescriptorData::msdp_support(bool on) {
	log("INFO: MSDP support enabled for client %s.\n", host);
	m_msdp_support = on;
}

void DescriptorData::msdp_report(const std::string &name) {
	if (m_msdp_support && msdp_need_report(name)) {
		msdp::report(this, name);
	}
}

// Should be called periodically to update changing msdp variables.
// this is mostly to overcome complication of hunting every possible place affect are added/removed to/from char.
void DescriptorData::msdp_report_changed_vars() {
	if (!m_msdp_support || !character) {
		return;
	}

	if (m_msdp_last_max_hit != character->get_real_max_hit()) {
		msdp_report(msdp::constants::MAX_HIT);
		m_msdp_last_max_hit = character->get_real_max_hit();
	}

	if (m_msdp_last_max_move != character->get_real_max_move()) {
		msdp_report(msdp::constants::MAX_MOVE);
		m_msdp_last_max_move = character->get_real_max_move();
	}
}

void DescriptorData::string_to_client_encoding(const char *in_str, char *out_str) const {
	if (!in_str) {
		*out_str = '\0';
		return;
	}
	switch (keytable) {
		case kCodePageAlt:
			for (; *in_str; *out_str = KtoA(*in_str), in_str++, out_str++);
			break;
		case kCodePageWin:
			for (; *in_str; in_str++, out_str++) {
				*out_str = KtoW(*in_str);

				// 0xFF is cp1251 'я' and Telnet IAC, so escape it with another IAC
				if (*out_str == '\xFF') {
					*++out_str = '\xFF';
				}
			}
			break;

		case kCodePageWinzOld:
		case kCodePageWinzZ:
			// zMUD before 6.39 or after for backward compatibility  - replace я with z
			for (; *in_str; *out_str = KtoW2(*in_str), in_str++, out_str++);
			break;

		case kCodePageWinz:
			// zMUD after 6.39 and CMUD support 'я' but with some issues
			for (; *in_str; in_str++, out_str++) {
				*out_str = KtoW(*in_str);

				// 0xFF is cp1251 'я' and Telnet IAC, so escape it with antother IAC
				// also there is a bug in zMUD, meaning we need to add an extra byte
				if (*out_str == '\xFF') {
					*++out_str = '\xFF';
					// make it obvious to other clients something is wrong
					*++out_str = '?';
				}
			}
			break;

		case kCodePageUTF8:
			// Anton Gorev (2016-04-25): we have to be careful. String in UTF-8 encoding may
			// contain character with code 0xff which telnet interprets as IAC.
			// II:  FE and FF were never defined for any purpose in UTF-8, we are safe
			koi_to_utf8(const_cast<char *>(in_str), out_str);
			break;

		default:
			for (; *in_str; *out_str = *in_str, in_str++, out_str++);
			break;
	}

	if (keytable != kCodePageUTF8) {
		*out_str = '\0';
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :