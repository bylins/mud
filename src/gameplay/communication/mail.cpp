/* ************************************************************************
*   File: mail.cpp                                      Part of Bylins    *
*  Usage: Internal funcs and player spec-procs of mud-mail system         *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "mail.h"

#include "engine/db/world_objects.h"
#include "engine/core/handler.h"
#include "parcel.h"
#include "engine/entities/char_player.h"
#include "gameplay/mechanics/named_stuff.h"
#include "engine/ui/color.h"
#include "engine/ui/modify.h"
#include "engine/db/player_index.h"

#include <third_party_libs/pugixml/pugixml.h>

extern RoomRnum r_helled_start_room;
extern RoomRnum r_named_start_room;
extern RoomRnum r_unreg_start_room;

void postmaster_send_mail(CharData *ch, CharData *mailman, int cmd, char *arg);
void postmaster_check_mail(CharData *ch, CharData *mailman, int cmd, char *arg);
void postmaster_receive_mail(CharData *ch, CharData *mailman, int cmd, char *arg);
int postmaster(CharData *ch, void *me, int cmd, char *argument);

namespace mail {

bool check_poster_cnt(CharData *ch);

/// для undelivered_list
struct undelivered_node {
	undelivered_node(const char *name, int to_uid) : total_num(1), names(name) {
		uids.insert(to_uid);
	};
	/// кол-во недоставленных писем
	size_t total_num;
	/// строка с именами адресатов
	std::string names;
	/// уиды адресатов для уникальности имен
	std::unordered_set<int> uids;
};
/// уид отправителя, инфа о недоставленных письмах на момент ребута
std::unordered_map<int, undelivered_node> undelivered_list;

void add_undelivered(int from_uid, const char *name, int to_uid) {
	if (from_uid < 0 || !name) return;

	auto i = undelivered_list.find(from_uid);
	if (i != undelivered_list.end()) {
		i->second.total_num += 1;
		// проверка на уникальность и добавление имени в строку
		if (i->second.uids.find(to_uid) == i->second.uids.end()) {
			i->second.names += " ";
			i->second.names += name;
			if (!(i->second.total_num % 8)) {
				i->second.names += "\r\n";
			}
			i->second.uids.insert(to_uid);
		}
	} else {
		undelivered_node tmp(name, to_uid);
		undelivered_list.insert(std::make_pair(from_uid, tmp));
	}
}

void print_undelivered(CharData *ch) {
	auto i = undelivered_list.find(ch->get_uid());
	if (i != undelivered_list.end()) {
		std::ostringstream out;
		out << "Информация по недоставленным (на момент перезагрузки) письмам." << "\r\n"
			<< "Количество писем: " << i->second.total_num << "." << "\r\n"
			<< "Адресаты: " << i->second.names << "." << "\r\n";
		SendMsgToChar(out.str(), ch);
	}
}

} // namespace mail

using namespace mail;

// мин.уровень для отправки почты
const int MIN_MAIL_LEVEL = 2;
// стоимость отправки письма
const int STAMP_PRICE = 50;
// макс. размер сообщения
const int MAX_MAIL_SIZE = 32768;

//****************************************************************
//* Below is the spec_proc for a postmaster using the above      *
//* routines.  Written by Jeremy Elson (jelson@circlemud.org)    *
//****************************************************************
int postmaster(CharData *ch, void *me, int cmd, char *argument) {
	if (!ch->desc || ch->IsNpc())
		return (0);    // so mobs don't get caught here

	if (!(CMD_IS("mail") || CMD_IS("check") || CMD_IS("receive")
		|| CMD_IS("почта") || CMD_IS("получить") || CMD_IS("отправить")
		|| CMD_IS("return") || CMD_IS("вернуть")))
		return (0);

	if (CMD_IS("mail") || CMD_IS("отправить")) {
		postmaster_send_mail(ch, (CharData *) me, cmd, argument);
		return (1);
	} else if (CMD_IS("check") || CMD_IS("почта")) {
		postmaster_check_mail(ch, (CharData *) me, cmd, argument);
		return (1);
	} else if (CMD_IS("receive") || CMD_IS("получить")) {
		one_argument(argument, arg);
		if (utils::IsAbbr(arg, "вещи")) {
			NamedStuff::receive_items(ch, (CharData *) me);
		} else {
			postmaster_receive_mail(ch, (CharData *) me, cmd, argument);
		}
		return (1);
	} else if (CMD_IS("return") || CMD_IS("вернуть")) {
		Parcel::bring_back(ch, (CharData *) me);
		return (1);
	} else
		return (0);
}

void postmaster_check_mail(CharData *ch, CharData *mailman, int/* cmd*/, char * /*arg*/) {
	bool empty = true;
	if (mail::has_mail(ch->get_uid())) {
		empty = false;
		act("$n сказал$g вам : 'Вас ожидает почта.'", false, mailman, 0, ch, kToVict);
	}
	if (Parcel::has_parcel(ch)) {
		empty = false;
		act("$n сказал$g вам : 'Вас ожидает посылка.'", false, mailman, 0, ch, kToVict);
	}

	if (empty) {
		act("$n сказал$g вам : 'Похоже, сегодня вам ничего нет.'",
			false, mailman, 0, ch, kToVict);
	}
	Parcel::print_sending_stuff(ch);
	print_undelivered(ch);
}

void postmaster_receive_mail(CharData *ch, CharData *mailman, int/* cmd*/, char * /*arg*/) {
	if (!Parcel::has_parcel(ch) && !mail::has_mail(ch->get_uid())) {
		act("$n удивленно сказал$g вам : 'Но для вас нет писем!?'",
			false, mailman, 0, ch, kToVict);
		return;
	}
	Parcel::receive(ch, mailman);
	mail::receive(ch, mailman);
}

void postmaster_send_mail(CharData *ch, CharData *mailman, int/* cmd*/, char *arg) {
	int recipient;
	int cost;
	char buf[256];

	IS_IMMORTAL(ch) || ch->IsFlagged(EPrf::kCoderinfo) ? cost = 0 : cost = STAMP_PRICE;

	if (GetRealLevel(ch) < MIN_MAIL_LEVEL) {
		sprintf(buf,
				"$n сказал$g вам, 'Извините, вы должны достигнуть %d уровня, чтобы отправить письмо!'",
				MIN_MAIL_LEVEL);
		act(buf, false, mailman, 0, ch, kToVict);
		return;
	}
	if (!mail::check_poster_cnt(ch)) {
		act("$n сказал$g вам, 'Извините, вы уже отправили максимальное кол-во сообщений!'",
			false, mailman, 0, ch, kToVict);
		return;
	}

	arg = one_argument(arg, buf);

	if (!*buf)        // you'll get no argument from me!
	{
		act("$n сказал$g вам, 'Вы не указали адресата!'", false, mailman, 0, ch, kToVict);
		return;
	}
	if ((recipient = GetUniqueByName(buf)) <= 0) {
		act("$n сказал$g вам, 'Извините, но такой игрок не зарегистрирован в игре!'", false, mailman, 0, ch, kToVict);
		return;
	}

	skip_spaces(&arg);
	if (*arg) {
		if ((ch->in_room == r_helled_start_room) ||
			(ch->in_room == r_named_start_room) ||
			(ch->in_room == r_unreg_start_room)) {
			act("$n сказал$g вам : 'Посылку? Не положено!'", false, mailman, 0, ch, kToVict);
			return;
		}
		long vict_uid = GetUniqueByName(buf);
		if (vict_uid > 0) {
			Parcel::send(ch, mailman, vict_uid, arg);
		} else {
			act("$n сказал$g вам : 'Ошибочка вышла, сообщите Богам!'", false, mailman, 0, ch, kToVict);
		}
		return;
	}

	if (ch->get_gold() < cost) {
		sprintf(buf, "$n сказал$g вам, 'Письмо стоит %d %s.'\r\n"
					 "$n сказал$g вам, '...которых у вас просто-напросто нет.'",
				STAMP_PRICE, GetDeclensionInNumber(STAMP_PRICE, EWhat::kMoneyU));
		act(buf, false, mailman, 0, ch, kToVict);
		return;
	}

	act("$n начал$g писать письмо.", true, ch, 0, 0, kToRoom);
	if (cost == 0) {
		sprintf(buf, "$n сказал$g вам, 'Со своих - почтовый сбор не берем.'\r\n"
					 "$n сказал$g вам, 'Можете писать, (/s saves /h for help)'");
	} else {
		sprintf(buf,
				"$n сказал$g вам, 'Отлично, с вас %d %s почтового сбора.'\r\n"
				"$n сказал$g вам, 'Можете писать, (/s saves /h for help)'",
				STAMP_PRICE, GetDeclensionInNumber(STAMP_PRICE, EWhat::kMoneyA));
	}

	act(buf, false, mailman, 0, ch, kToVict);
	ch->remove_gold(cost);
	ch->SetFlag(EPlrFlag::kMailing);    // string_write() sets writing.

	// Start writing!
	utils::AbstractStringWriter::shared_ptr writer(new utils::StdStringWriter());
	string_write(ch->desc, writer, MAX_MAIL_SIZE, recipient, nullptr);
}

namespace coder {

static const std::string base64_chars =
	"abcdefghijklmnopqrstuvwxyz"
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"0123456789+/";

static inline bool is_base64(unsigned char c) {
	return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string base64_encode(const char *in_str, size_t in_len) {
	const unsigned char *bytes_to_encode =
		reinterpret_cast<const unsigned char *>(in_str);
	std::string ret;
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];

	while (in_len--) {
		char_array_3[i++] = *(bytes_to_encode++);
		if (i == 3) {
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for (i = 0; (i < 4); i++)
				ret += base64_chars[char_array_4[i]];
			i = 0;
		}
	}

	if (i) {
		for (j = i; j < 3; j++)
			char_array_3[j] = '\0';

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for (j = 0; (j < i + 1); j++)
			ret += base64_chars[char_array_4[j]];

		while ((i++ < 3))
			ret += '=';

	}

	return ret;

}

std::string base64_decode(const std::string &encoded_string) {
	size_t in_len = encoded_string.size();
	int i = 0;
	int j = 0;
	int in_ = 0;
	unsigned char char_array_4[4], char_array_3[3];
	std::string ret;

	while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
		char_array_4[i++] = encoded_string[in_];
		in_++;
		if (i == 4) {
			for (i = 0; i < 4; i++) {
				char_array_4[i] = static_cast<unsigned char>(base64_chars.find(char_array_4[i]));
			}

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (i = 0; (i < 3); i++) {
				ret += char_array_3[i];
			}
			i = 0;
		}
	}

	if (i) {
		for (j = i; j < 4; j++)
			char_array_4[j] = 0;

		for (j = 0; j < 4; j++)
			char_array_4[j] = static_cast<unsigned char>(base64_chars.find(char_array_4[j]));

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

		for (j = 0; (j < i - 1); j++) {
			ret += char_array_3[j];
		}
	}

	return ret;
}

std::string to_iso8601(time_t time) {
	char buf_[kMaxInputLength];
	strftime(buf_, sizeof(buf_), "%Y-%m-%dT%H:%M:%S", localtime(&time));
	return buf_;
}

time_t from_iso8601(const std::string &str) {
	std::tm tm_time;
	int parsed = sscanf(str.c_str(), "%d-%d-%dT%d:%d:%d",
						&tm_time.tm_year, &tm_time.tm_mon, &tm_time.tm_mday,
						&tm_time.tm_hour, &tm_time.tm_min, &tm_time.tm_sec);

	time_t time = 0;
	if (parsed == 6) {
		tm_time.tm_mon -= 1;
		tm_time.tm_year -= 1900;
		tm_time.tm_isdst = -1;
		time = std::mktime(&tm_time);
	}
	return time;
}

} // namespace coder

namespace mail {

const char *MAIL_XML_FILE = LIB_ETC"plrmail.xml";
// макс сообщений в доставке от безмортовых чаров до NAME_LEVEL уровня
const int LOW_LVL_MAX_POST = 10;
// тоже самое для всех остальных
const int HIGH_LVL_MAX_POST = 100;

struct mail_node {
	mail_node() : from(-1), date(0) {};

	// уид автора (может быть невалидным)
	int from;
	// дата написания письма
	time_t date;
	// текст письма в перемешанном base64
	std::string text;
	// (дата в iso8601 + автор + получатель) в перемешанном base64
	std::string header;
};

// флаг для избегания пустых автосохранений
bool need_save = false;
// уид получателя -> письмо
std::unordered_multimap<int, mail_node> mail_list;
// уид -> кол-во отправленных сообщений в бд (спам-контроль)
std::unordered_map<int, int> poster_list;
// уид - условный список чаров (с дальнейшей проверкой), которым нужно
// единоразово вывести уведомление про новую почту (спам-контроль)
std::unordered_set<int> notice_list;

void add_notice(int uid) {
	notice_list.insert(uid);
}

void print_notices() {
	for (auto i = notice_list.begin(); i != notice_list.end(); ++i) {
		if (!has_mail(*i)) {
			continue;
		}
		DescriptorData *d = DescriptorByUid(*i);
		if (d) {
			SendMsgToChar(d->character.get(),
						  "%sВам пришло письмо, зайдите на почту и распишитесь!%s\r\n",
						  kColorWht, kColorNrm);
		}
	}
	notice_list.clear();
}

void add_poster(int uid) {
	if (uid < 0) return;

	auto i = poster_list.find(uid);
	if (i != poster_list.end()) {
		i->second += 1;
	} else {
		poster_list[uid] = 1;
	}
}

void sub_poster(int uid) {
	if (uid < 0) return;

	auto i = poster_list.find(uid);
	if (i != poster_list.end()) {
		i->second -= 1;
		if (i->second <= 0) {
			poster_list.erase(i);
		}
	}
}

bool check_poster_cnt(CharData *ch) {
	auto i = poster_list.find(ch->get_uid());
	if (i != poster_list.end()) {
		if (GetRealRemort(ch) <= 0
			&& GetRealLevel(ch) <= kNameLevel
			&& i->second >= LOW_LVL_MAX_POST) {
			return false;
		}
		if (i->second >= HIGH_LVL_MAX_POST) {
			return false;
		}
	}
	return true;
}

void add(int to_uid, int from_uid, const char *message) {
	if (to_uid < 0 || !message || !*message) {
		return;
	}

	mail_node node;
	node.from = from_uid;
	node.date = time(0);
	node.text = coder::base64_encode(message, strlen(message));

	char buf_[kMaxStringLength];
	snprintf(buf_, sizeof(buf_), "%s %d %d",
			 coder::to_iso8601(node.date).c_str(), from_uid, to_uid);
	node.header = coder::base64_encode(buf_, strlen(buf_));

	mail_list.insert(std::make_pair(to_uid, node));
	add_poster(from_uid);
	need_save = true;
}

void add_by_id(int to_id, int from_id, char *message) {
	const int to_uid = GetPlayerUidByName(to_id);
	const int from_uid = from_id >= 0 ? GetPlayerUidByName(from_id) : from_id;

	add(to_uid, from_uid, message);
}

bool has_mail(int uid) {
	return (mail_list.find(uid) != mail_list.end());
}

std::string get_author_name(int uid) {
	std::string out;
	if (uid == -1) {
		out = "Почтовая служба";
	} else if (uid == -2) {
		out = "<персонаж был удален>";
	} else if (uid < 0) {
		out = "Неизвестно";
	} else {
		auto name = GetPlayerNameByUnique(uid);
		if (name.empty()) {
			out = "<персонаж был удален>";
		} else {
			out = name;
			name_convert(out);
		}
	}
	return out;
}

void receive(CharData *ch, CharData *mailman) {
	auto rng = mail_list.equal_range(ch->get_uid());
	for (auto i = rng.first; i != rng.second; ++i) {
		const auto obj = world_objects.create_blank();
		obj->set_aliases("mail paper letter письмо почта бумага");
		obj->set_short_description("письмо");
		obj->set_description("Кто-то забыл здесь свое письмо.");
		obj->set_PName(ECase::kNom, "письмо");
		obj->set_PName(ECase::kGen, "письма");
		obj->set_PName(ECase::kDat, "письма");
		obj->set_PName(ECase::kAcc, "письмо");
		obj->set_PName(ECase::kIns, "письмом");
		obj->set_PName(ECase::kPre, "письме");
		obj->set_sex(EGender::kNeutral);
		obj->set_type(EObjType::kNote);
		obj->set_wear_flags(to_underlying(EWearFlag::kTake) | to_underlying(EWearFlag::kHold));
		obj->set_weight(1);
		obj->set_material(EObjMaterial::kPaper);
		obj->set_cost(0);
		obj->set_rent_off(10);
		obj->set_rent_on(10);
		obj->set_timer(24 * 60);
		obj->set_extra_flag(EObjFlag::kNodonate);
		obj->set_extra_flag(EObjFlag::kNosell);

		char buf_date[kMaxInputLength];
		strftime(buf_date, sizeof(buf_date), "%H:%M %d-%m-%Y", localtime(&i->second.date));

		char buf_[kMaxInputLength];
		snprintf(buf_, sizeof(buf_),
				 " * * * * Княжеская почта * * * *\r\n"
				 "Дата: %s\r\n"
				 "Кому: %s\r\n"
				 "  От: %s\r\n\r\n",
				 buf_date, ch->get_name().c_str(), get_author_name(i->second.from).c_str());

		std::string text = coder::base64_decode(i->second.text);
		utils::Trim(text);
		obj->set_action_description(buf_ + text + "\r\n\r\n");
		PlaceObjToInventory(obj.get(), ch);
		act("$n дал$g вам письмо.", false, mailman, 0, ch, kToVict);
		act("$N дал$G $n2 письмо.", false, ch, 0, mailman, kToRoom);

		sub_poster(i->second.from);
	}
	mail_list.erase(rng.first, rng.second);
	need_save = true;
}

void save() {
	print_notices();

	if (!need_save) return;

	pugi::xml_document doc;
	doc.append_child().set_name("mail");
	pugi::xml_node mail_n = doc.child("mail");

	for (auto i = mail_list.cbegin(), iend = mail_list.cend(); i != iend; ++i) {
		pugi::xml_node msg_n = mail_n.append_child();
		msg_n.set_name("m");
		msg_n.append_attribute("h") = i->second.header.c_str();
		msg_n.append_attribute("t") = i->second.text.c_str();
	}

	doc.save_file(MAIL_XML_FILE);
	need_save = false;
}

void load() {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(MAIL_XML_FILE);
	if (!result) {
		snprintf(buf, kMaxStringLength, "...%s", result.description());
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
		return;
	}
	pugi::xml_node mail_n = doc.child("mail");
	if (!mail_n) {
		snprintf(buf, kMaxStringLength, "...<mail> read fail");
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
		return;
	}

	for (pugi::xml_node msg_n = mail_n.child("m");
		 msg_n; msg_n = msg_n.next_sibling("m")) {
		mail_node message;
		message.header = parse::ReadAattrAsStr(msg_n, "h");
		message.text = parse::ReadAattrAsStr(msg_n, "t");
		// раскодируем строку хедера
		std::string header = coder::base64_decode(message.header);
		int to_uid = -1;
		// вытаскиваем дату, отправителя и получателя
		std::vector<std::string> param_list;
		param_list = utils::Split(header);
		if (param_list.size() == 3) {
			message.date = coder::from_iso8601(param_list[0]);
			try {
				message.from = std::stol(param_list[1], nullptr, 10);
				to_uid = std::stol(param_list[2], nullptr, 10);
			}
			catch (const std::invalid_argument &) {
				log("SYSERROR: ошибка чтения mail header #1 (%s)",
					header.c_str());
				continue;
			}
		} else {
			log("SYSERROR: ошибка чтения mail header #2 (%s)",
				header.c_str());
			continue;
		}
		auto to_name = GetPlayerNameByUnique(to_uid);
		if (to_name.empty()) {
			continue;
		}
		if (message.from == -1 && time(0) - message.date > 60 * 60 * 24 * 365) {
			// технические сообщения старше года
			continue;
		}
		if (message.from > 0 && GetPlayerNameByUnique(message.from).empty()) {
			// убираем левые уиды, чтобы потом с кем-нить другим не совпало
			message.from = -2;
		}
		mail_list.insert(std::make_pair(to_uid, message));
		add_poster(message.from);
		add_undelivered(message.from, to_name.c_str(), to_uid);
	}
	need_save = true;
}

size_t get_msg_count() {
	return mail_list.size();
}

} // namespace mail

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
