//
// Created by Sventovit on 08.09.2024.
//

#include "engine/ui/cmd/do_who.h"
#include "utils/russian_keys.h"
#include "utils/native_text.h"
#include "utils/utils_string.h"
#include <string>
#include <utility>
#include <fmt/format.h>
#include "administration/privilege.h"
#include "utils/grammar/gender.h"
#include "gameplay/mechanics/sight.h"

#include "engine/entities/char_data.h"
#include "engine/db/global_objects.h"
#include "engine/ui/color.h"
#include "engine/ui/modify.h"
#include "gameplay/classes/pc_classes.h"
#include "engine/db/player_index.h"
#include "gameplay/core/remort.h"

namespace {

const char *IMM_WHO_FORMAT =
	"Формат: кто [минуров[-максуров]] [-n имя] [-c профлист] [-s] [-r] [-z] [-h] [-b|-и]\r\n";

const char *MORT_WHO_FORMAT = "Формат: кто [имя] [-?]\r\n";

// Первое слово в нижнем регистре и остаток -- utils::ExtractFirstArgument плюс то, что
// half_chop делал сам: понижение регистра (сравнения ниже рассчитывают на него) и срез
// ведущих пробелов в остатке.
std::pair<std::string, std::string> ChopWord(const std::string &line) {
	std::string rest;
	std::string word = utils::ExtractFirstArgument(line, rest);
	native_text::to_lower(word);
	utils::TrimLeft(rest);
	return {std::move(word), std::move(rest)};
}

} // namespace

void DoWho(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	std::string name_search;

	// Флаги для опций
	int low = 0, high = kLvlImplementator;
	int num_can_see = 0;
	int imms_num = 0, morts_num = 0, demigods_num = 0;
	bool localwho = false, short_list = false;
	bool who_room = false, showname = false;
	ECharClass showclass{ECharClass::kUndefined};

	skip_spaces(&argument);
	// Разбор идёт по своим строкам, а не по глобальным buf/arg/buf1: те общие на весь процесс,
	// и любой вызов посреди разбора затирал бы разобранное (issue #3807).
	std::string rest = argument ? argument : "";

	// Проверка аргументов команды "кто"
	while (!rest.empty()) {
		auto [token, tail] = ChopWord(rest);
		if (token.empty()) {
			break;
		}
		if (token == "боги") {
			low = kLvlImmortal;
			high = kLvlImplementator;
			rest = tail;
		} else if (a_isdigit(token.front())) {
			if (privilege::IsGod(ch) || ch->IsFlagged(EPrf::kCoderinfo))
				sscanf(token.c_str(), "%d-%d", &low, &high);
			rest = tail;
		} else if (token.front() == '-') {
			const char32_t mode = native_text::first_char_code(token.c_str() + 1);
			switch (mode) {
				case 'b':
				case rus::kI:
					if (privilege::IsImmortal(ch) || GET_GOD_FLAG(ch, EGf::kDemigod) || ch->IsFlagged(EPrf::kCoderinfo))
						showname = true;
					rest = tail;
					break;
				case 'z':
					if (privilege::IsGod(ch) || ch->IsFlagged(EPrf::kCoderinfo))
						localwho = true;
					rest = tail;
					break;
				case 's':
					if (privilege::IsImmortal(ch) || ch->IsFlagged(EPrf::kCoderinfo))
						short_list = true;
					rest = tail;
					break;
				case 'l': {
					auto [value, next] = ChopWord(tail);
					rest = next;
					if (privilege::IsGod(ch) || ch->IsFlagged(EPrf::kCoderinfo))
						sscanf(value.c_str(), "%d-%d", &low, &high);
					break;
				}
				case 'n': {
					auto [value, next] = ChopWord(tail);
					name_search = value;
					rest = next;
					break;
				}
				case 'r':
					if (privilege::IsGod(ch) || ch->IsFlagged(EPrf::kCoderinfo))
						who_room = true;
					rest = tail;
					break;
				case 'c': {
					auto [value, next] = ChopWord(tail);
					rest = next;
					if (privilege::IsGod(ch) || ch->IsFlagged(EPrf::kCoderinfo)) {
						showclass = FindAvailableCharClassId(value.c_str());
					}
					break;
				}
				case 'h':
				case '?':
				default:
					if (privilege::IsImmortal(ch) || ch->IsFlagged(EPrf::kCoderinfo))
						SendMsgToChar(IMM_WHO_FORMAT, ch);
					else
						SendMsgToChar(MORT_WHO_FORMAT, ch);
					return;
			}    // end of switch
		} else    // endif
		{
			name_search = token;
			rest = tail;
		}
	}            // end while (parser)

	if (PerformWhoSpamcontrol(ch, name_search.empty() ? kWhoListall : kWhoListname))
		return;

	// Строки содержащие имена
	std::string imms = fmt::format("{}БОГИ{}\r\n", kColorBoldCyn, kColorNrm);
	std::string demigods = fmt::format("{}Привилегированные{}\r\n", kColorCyn, kColorNrm);
	std::string morts = fmt::format("{}Игроки{}\r\n", kColorCyn, kColorNrm);

	int all = 0;

	for (const auto &tch: character_list) {
		if (tch->IsNpc()) {
			continue;
		}

		if (!HERE(tch)) {
			continue;
		}

		if (!*argument && GetRealLevel(tch) < kLvlImmortal) {
			++all;
		}

		if (!name_search.empty() && !isname(name_search, GET_NAME(tch))) {
			continue;
		}

		if (!sight::CanSeeIgnoringLight(ch, tch) || GetRealLevel(tch) < low || GetRealLevel(tch) > high) {
			continue;
		}
		if (localwho && world[ch->in_room]->zone_rn != world[tch->in_room]->zone_rn) {
			continue;
		}
		if (who_room && (tch->in_room != ch->in_room)) {
			continue;
		}
		if (showclass != ECharClass::kUndefined && showclass != tch->GetClass()) {
			continue;
		}
		if (showname && !(!(tch)->player_specials->saved.NameGod && GetRealLevel(tch) <= kNameLevel)) {
			continue;
		}
		if (tch->IsFlagged(EPlrFlag::kNameDenied) && punishments::Get(tch, punishments::EType::kName).duration
			&& !privilege::IsImmortal(ch) && !ch->IsFlagged(EPrf::kCoderinfo)
			&& ch != tch.get()) {
			continue;
		}

		// Строка игрока собирается в свою std::string: глобальный buf на это не годится --
		// он общий на весь процесс и переполняется молча (issue #3807).
		std::string line;
		num_can_see++;
		if (short_list) {
			// Ширину добираем ВНУТРИ цветовых кодов, а не поверх них: код цвета -- семь невидимых
			// символов, и "{:<30}" по строке вместе с ними давал колонку не в 30 знаков, а в 16.
			const std::string colored_name =
				fmt::format("{}{:<30}{}", GetPkNameColor(tch), GET_NAME(tch), kColorNrm);
			if (privilege::IsImpl(ch) || ch->IsFlagged(EPrf::kCoderinfo)) {
				line = fmt::format("{}[{:2} {}] {}{}",
								   privilege::IsGod(tch.get()) ? kColorWht : "",
								   GetRealLevel(tch), MUD::Class(tch->GetClass()).GetCName(),
								   colored_name,
								   privilege::IsGod(tch.get()) ? kColorNrm : "");
			} else {
				line = fmt::format("{}{}{}",
								   privilege::IsImmortal(tch.get()) ? kColorWht : "",
								   colored_name,
								   privilege::IsImmortal(tch.get()) ? kColorNrm : "");
			}
		} else {
			if (privilege::IsImpl(ch)
				|| ch->IsFlagged(EPrf::kCoderinfo)) {
				line = fmt::format("{}[{:2d} {:2d} {}({:5d})] {}{}{}{}",
								   privilege::IsImmortal(tch.get()) ? kColorWht : "",
								   GetRealLevel(tch),
								   remort::GetRealRemort(tch),
								   MUD::Class(tch->GetClass()).GetAbbr(),
								   tch->get_pfilepos(),
								   GetPkNameColor(tch),
								   privilege::IsImmortal(tch.get()) ? kColorWht : "", tch->race_or_title(), kColorNrm);
			} else {
				line = fmt::format("{} {}{}{}",
								   GetPkNameColor(tch),
								   privilege::IsImmortal(tch.get()) ? kColorWht : "", tch->race_or_title(), kColorNrm);
			}

			if (GET_INVIS_LEV(tch))
				line += fmt::format(" (i{})", GET_INVIS_LEV(tch));
			else if (AFF_FLAGGED(tch, EAffect::kInvisible))
				line += fmt::format(" (невидим{})", grammar::SexEnding((tch)->get_sex(), 6));
			if (AFF_FLAGGED(tch, EAffect::kHide))
				line += " (прячется)";
			if (AFF_FLAGGED(tch, EAffect::kDisguise))
				line += " (маскируется)";

			if (tch->IsFlagged(EPlrFlag::kMailing))
				line += " (отправляет письмо)";
			else if (tch->IsFlagged(EPlrFlag::kWriting))
				line += " (пишет)";

			if (tch->IsFlagged(EPrf::kNoHoller))
				line += fmt::format(" (глух{})", grammar::SexEnding((tch)->get_sex(), 1));
			if (tch->IsFlagged(EPrf::kNoTell))
				line += fmt::format(" (занят{})", grammar::SexEnding((tch)->get_sex(), 6));
			if (tch->IsFlagged(EPlrFlag::kMuted))
				line += " (молчит)";
			if (tch->IsFlagged(EPlrFlag::kDumbed))
				line += fmt::format(" (нем{})", grammar::SexEnding((tch)->get_sex(), 6));
			if (tch->IsFlagged(EPlrFlag::kKiller) == EPlrFlag::kKiller)
				line += "&R (ДУШЕГУБ)&n";
			if ((privilege::IsImmortal(ch) || GET_GOD_FLAG(ch, EGf::kDemigod)) && !(tch)->player_specials->saved.NameGod
				&& GetRealLevel(tch) <= kNameLevel) {
				line += " &W!НЕ ОДОБРЕНО!&n";
				if (showname) {
					line += fmt::format("\r\nПадежи: {}/{}/{}/{}/{}/{} Email: &S{}&s Пол: {}",
										GET_PAD(tch, 0), GET_PAD(tch, 1), GET_PAD(tch, 2),
										GET_PAD(tch, 3), GET_PAD(tch, 4), GET_PAD(tch, 5),
										GET_GOD_FLAG(ch, EGf::kDemigod) ? "скрыто" : GET_EMAIL(tch),
										genders[static_cast<int>(tch->get_sex())]);
				}
			}
			if ((GetRealLevel(ch) == kLvlImplementator) && (NORENTABLE(tch)))
				line += " &R(В КРОВИ)&n";
			else if ((privilege::IsImmortal(ch) || ch->IsFlagged(EPrf::kCoderinfo)) && NAME_BAD(tch)) {
				line += fmt::format(" &Wзапрет {}!&n", GetNameById((tch)->player_specials->saved.NameIDGod));
			}
			if (privilege::IsGod(ch) && (GET_GOD_FLAG(tch, EGf::kAllowTesterMode)))
				line += " &G(ТЕСТЕР!)&n";
			if (privilege::IsGod(ch) && (GET_GOD_FLAG(tch, EGf::kSkillTester)))
				line += " &G(СКИЛЛТЕСТЕР!)&n";
			if (privilege::IsGod(ch) && (tch->IsFlagged(EPlrFlag::kAutobot)))
				line += " &G(БОТ!)&n";
			if (privilege::IsImmortal(tch.get()))
				line += kColorNrm;
		}        // endif shortlist

		if (privilege::IsImmortal(tch.get())) {
			imms_num++;
			imms += line;
			if (!short_list || !(imms_num % 4)) {
				imms += "\r\n";
			}
		} else if (GET_GOD_FLAG(tch, EGf::kDemigod)
			&& (privilege::IsImmortal(ch) || ch->IsFlagged(EPrf::kCoderinfo) || GET_GOD_FLAG(tch, EGf::kDemigod))) {
			demigods_num++;
			demigods += line;
			if (!short_list || !(demigods_num % 4)) {
				demigods += "\r\n";
			}
		} else {
			morts_num++;
			morts += line;
			if (!short_list || !(morts_num % 4))
				morts += "\r\n";
		}
	}            // end of for

	if (morts_num + imms_num + demigods_num == 0) {
		SendMsgToChar("\r\nВы никого не видите.\r\n", ch);
		// !!!
		return;
	}

	std::string out;

	if (imms_num > 0) {
		out += imms;
	}
	if (demigods_num > 0) {
		if (short_list) {
			out += "\r\n";
		}
		out += demigods;
	}
	if (morts_num > 0) {
		if (short_list) {
			out += "\r\n";
		}
		out += morts;
	}

	out += "\r\nВсего:";
	if (imms_num) {
		out += fmt::format(" бессмертных {}", imms_num);
	}
	if (demigods_num) {
		out += fmt::format(" привилегированных {}", demigods_num);
	}
	if (all && morts_num) {
		out += fmt::format(" смертных {} (видимых {})", all, morts_num);
	} else if (morts_num) {
		out += fmt::format(" смертных {}", morts_num);
	}

	out += ".\r\n";
	page_string(ch->desc, out);
}

// спам-контроль для команды кто и списка по дружинам
// работает аналогично восстановлению и расходованию маны у волхвов
// константы пока определены через #define в interpreter.h
// возвращает истину, если спамконтроль сработал и игроку придется подождать
bool PerformWhoSpamcontrol(CharData *ch, unsigned short int mode) {
	if (privilege::IsImmortal(ch)) {
		return false;
	}

	unsigned int cost{0};
	switch (mode) {
		case kWhoListall: cost = kWhoCost;
			break;
		case kWhoListname: cost = kWhoCostName;
			break;
		case kWhoListclan: cost = kWhoCostClan;
			break;
		default: cost = kWhoCost;
			break;
	}

	auto who_cost_mana = ch->get_who_mana();
	auto last = ch->get_who_last();

	// рестим ману, в БД скорость реста маны удваивается
	time_t ctime = time(nullptr);
	who_cost_mana = MIN(kWhoManaMax,
						who_cost_mana + (ctime - last) * kWhoManaRestPerSecond
							+ (ctime - last) * kWhoManaRestPerSecond * (NORENTABLE(ch) ? 1 : 0));
	ch->set_who_mana(who_cost_mana);
	ch->set_who_last(ctime);

	if (who_cost_mana < cost) {
		SendMsgToChar("Запрос обрабатывается, ожидайте...\r\n", ch);
		return true;
	} else {
		who_cost_mana -= cost;
		ch->set_who_mana(who_cost_mana);
	}
	return false;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
