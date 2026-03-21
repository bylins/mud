//
// Created by Sventovit on 08.09.2024.
//

#include "engine/ui/cmd/do_who.h"

#include "engine/entities/char_data.h"
#include "engine/db/global_objects.h"
#include "engine/ui/color.h"
#include "engine/ui/modify.h"
#include "gameplay/classes/pc_classes.h"
#include "engine/db/player_index.h"

namespace {

const char *IMM_WHO_FORMAT =
	"Формат: кто [минуров[-максуров]] [-n имя] [-c профлист] [-s] [-r] [-z] [-h] [-b|-и]\r\n";

const char *MORT_WHO_FORMAT = "Формат: кто [имя] [-?]\r\n";

} // namespace

void DoWho(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char name_search[kMaxInputLength];
	name_search[0] = '\0';

	// Флаги для опций
	int low = 0, high = kLvlImplementator;
	int num_can_see = 0;
	int imms_num = 0, morts_num = 0, demigods_num = 0;
	bool localwho = false, short_list = false;
	bool who_room = false, showname = false;
	ECharClass showclass{ECharClass::kUndefined};

	skip_spaces(&argument);
	strcpy(buf, argument);

	// Проверка аргументов команды "кто"
	while (*buf) {
		half_chop(buf, arg, buf1);
		if (!str_cmp(arg, "боги") && strlen(arg) == 4) {
			low = kLvlImmortal;
			high = kLvlImplementator;
			strcpy(buf, buf1);
		} else if (a_isdigit(*arg)) {
			if (IS_GOD(ch) || ch->IsFlagged(EPrf::kCoderinfo))
				sscanf(arg, "%d-%d", &low, &high);
			strcpy(buf, buf1);
		} else if (*arg == '-') {
			const char mode = *(arg + 1);    // just in case; we destroy arg in the switch
			switch (mode) {
				case 'b':
				case 'и':
					if (IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, EGf::kDemigod) || ch->IsFlagged(EPrf::kCoderinfo))
						showname = true;
					strcpy(buf, buf1);
					break;
				case 'z':
					if (IS_GOD(ch) || ch->IsFlagged(EPrf::kCoderinfo))
						localwho = true;
					strcpy(buf, buf1);
					break;
				case 's':
					if (IS_IMMORTAL(ch) || ch->IsFlagged(EPrf::kCoderinfo))
						short_list = true;
					strcpy(buf, buf1);
					break;
				case 'l': half_chop(buf1, arg, buf);
					if (IS_GOD(ch) || ch->IsFlagged(EPrf::kCoderinfo))
						sscanf(arg, "%d-%d", &low, &high);
					break;
				case 'n': half_chop(buf1, name_search, buf);
					break;
				case 'r':
					if (IS_GOD(ch) || ch->IsFlagged(EPrf::kCoderinfo))
						who_room = true;
					strcpy(buf, buf1);
					break;
				case 'c': half_chop(buf1, arg, buf);
					if (IS_GOD(ch) || ch->IsFlagged(EPrf::kCoderinfo)) {
/*						const size_t len = strlen(arg);
						for (size_t i = 0; i < len; i++) {
							showclass |= FindCharClassMask(arg[i]);
						}*/
						showclass = FindAvailableCharClassId(arg);
					}
					break;
				case 'h':
				case '?':
				default:
					if (IS_IMMORTAL(ch) || ch->IsFlagged(EPrf::kCoderinfo))
						SendMsgToChar(IMM_WHO_FORMAT, ch);
					else
						SendMsgToChar(MORT_WHO_FORMAT, ch);
					return;
			}    // end of switch
		} else    // endif
		{
			strcpy(name_search, arg);
			strcpy(buf, buf1);

		}
	}            // end while (parser)

	if (PerformWhoSpamcontrol(ch, strlen(name_search) ? kWhoListname : kWhoListall))
		return;

	// Строки содержащие имена
	sprintf(buf, "%sБОГИ%s\r\n", kColorBoldCyn, kColorNrm);
	std::string imms(buf);

	sprintf(buf, "%sПривилегированные%s\r\n", kColorCyn, kColorNrm);
	std::string demigods(buf);

	sprintf(buf, "%sИгроки%s\r\n", kColorCyn, kColorNrm);
	std::string morts(buf);

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

		if (*name_search && !(isname(name_search, GET_NAME(tch)))) {
			continue;
		}

		if (!CAN_SEE_CHAR(ch, tch) || GetRealLevel(tch) < low || GetRealLevel(tch) > high) {
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
		if (showname && !(!NAME_GOD(tch) && GetRealLevel(tch) <= kNameLevel)) {
			continue;
		}
		if (tch->IsFlagged(EPlrFlag::kNameDenied) && NAME_DURATION(tch)
			&& !IS_IMMORTAL(ch) && !ch->IsFlagged(EPrf::kCoderinfo)
			&& ch != tch.get()) {
			continue;
		}

		*buf = '\0';
		num_can_see++;
		if (short_list) {
			char tmp[kMaxInputLength];
			snprintf(tmp, sizeof(tmp), "%s%s%s", GetPkNameColor(tch), GET_NAME(tch), kColorNrm);
			if (IS_IMPL(ch) || ch->IsFlagged(EPrf::kCoderinfo)) {
				sprintf(buf, "%s[%2d %s] %-30s%s",
						IS_GOD(tch) ? kColorWht : "",
						GetRealLevel(tch), MUD::Class(tch->GetClass()).GetCName(),
						tmp, IS_GOD(tch) ? kColorNrm : "");
			} else {
				sprintf(buf, "%s%-30s%s",
						IS_IMMORTAL(tch) ? kColorWht : "",
						tmp, IS_IMMORTAL(tch) ? kColorNrm : "");
			}
		} else {
			if (IS_IMPL(ch)
				|| ch->IsFlagged(EPrf::kCoderinfo)) {
				sprintf(buf, "%s[%2d %2d %s(%5d)] %s%s%s%s",
						IS_IMMORTAL(tch) ? kColorWht : "",
						GetRealLevel(tch),
						GetRealRemort(tch),
						MUD::Class(tch->GetClass()).GetAbbr().c_str(),
						tch->get_pfilepos(),
						GetPkNameColor(tch),
						IS_IMMORTAL(tch) ? kColorWht : "", tch->race_or_title().c_str(), kColorNrm);
			} else {
				sprintf(buf, "%s %s%s%s",
						GetPkNameColor(tch),
						IS_IMMORTAL(tch) ? kColorWht : "", tch->race_or_title().c_str(), kColorNrm);
			}

			if (GET_INVIS_LEV(tch))
				sprintf(buf + strlen(buf), " (i%d)", GET_INVIS_LEV(tch));
			else if (AFF_FLAGGED(tch, EAffect::kInvisible))
				sprintf(buf + strlen(buf), " (невидим%s)", GET_CH_SUF_6(tch));
			if (AFF_FLAGGED(tch, EAffect::kHide))
				strcat(buf, " (прячется)");
			if (AFF_FLAGGED(tch, EAffect::kDisguise))
				strcat(buf, " (маскируется)");

			if (tch->IsFlagged(EPlrFlag::kMailing))
				strcat(buf, " (отправляет письмо)");
			else if (tch->IsFlagged(EPlrFlag::kWriting))
				strcat(buf, " (пишет)");

			if (tch->IsFlagged(EPrf::kNoHoller))
				sprintf(buf + strlen(buf), " (глух%s)", GET_CH_SUF_1(tch));
			if (tch->IsFlagged(EPrf::kNoTell))
				sprintf(buf + strlen(buf), " (занят%s)", GET_CH_SUF_6(tch));
			if (tch->IsFlagged(EPlrFlag::kMuted))
				sprintf(buf + strlen(buf), " (молчит)");
			if (tch->IsFlagged(EPlrFlag::kDumbed))
				sprintf(buf + strlen(buf), " (нем%s)", GET_CH_SUF_6(tch));
			if (tch->IsFlagged(EPlrFlag::kKiller) == EPlrFlag::kKiller)
				sprintf(buf + strlen(buf), "&R (ДУШЕГУБ)&n");
			if ((IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, EGf::kDemigod)) && !NAME_GOD(tch)
				&& GetRealLevel(tch) <= kNameLevel) {
				sprintf(buf + strlen(buf), " &W!НЕ ОДОБРЕНО!&n");
				if (showname) {
					sprintf(buf + strlen(buf),
							"\r\nПадежи: %s/%s/%s/%s/%s/%s Email: &S%s&s Пол: %s",
							GET_PAD(tch, 0), GET_PAD(tch, 1), GET_PAD(tch, 2),
							GET_PAD(tch, 3), GET_PAD(tch, 4), GET_PAD(tch, 5),
							GET_GOD_FLAG(ch, EGf::kDemigod) ? "скрыто" : GET_EMAIL(tch),
							genders[static_cast<int>(tch->get_sex())]);
				}
			}
			if ((GetRealLevel(ch) == kLvlImplementator) && (NORENTABLE(tch)))
				sprintf(buf + strlen(buf), " &R(В КРОВИ)&n");
			else if ((IS_IMMORTAL(ch) || ch->IsFlagged(EPrf::kCoderinfo)) && NAME_BAD(tch)) {
				sprintf(buf + strlen(buf), " &Wзапрет %s!&n", GetNameById(NAME_ID_GOD(tch)).c_str());
			}
			if (IS_GOD(ch) && (GET_GOD_FLAG(tch, EGf::kAllowTesterMode)))
				sprintf(buf + strlen(buf), " &G(ТЕСТЕР!)&n");
			if (IS_GOD(ch) && (GET_GOD_FLAG(tch, EGf::kSkillTester)))
				sprintf(buf + strlen(buf), " &G(СКИЛЛТЕСТЕР!)&n");
			if (IS_GOD(ch) && (tch->IsFlagged(EPlrFlag::kAutobot)))
				sprintf(buf + strlen(buf), " &G(БОТ!)&n");
			if (IS_IMMORTAL(tch))
				strcat(buf, kColorNrm);
		}        // endif shortlist

		if (IS_IMMORTAL(tch)) {
			imms_num++;
			imms += buf;
			if (!short_list || !(imms_num % 4)) {
				imms += "\r\n";
			}
		} else if (GET_GOD_FLAG(tch, EGf::kDemigod)
			&& (IS_IMMORTAL(ch) || ch->IsFlagged(EPrf::kCoderinfo) || GET_GOD_FLAG(tch, EGf::kDemigod))) {
			demigods_num++;
			demigods += buf;
			if (!short_list || !(demigods_num % 4)) {
				demigods += "\r\n";
			}
		} else {
			morts_num++;
			morts += buf;
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
		sprintf(buf, " бессмертных %d", imms_num);
		out += buf;
	}
	if (demigods_num) {
		sprintf(buf, " привилегированных %d", demigods_num);
		out += buf;
	}
	if (all && morts_num) {
		sprintf(buf, " смертных %d (видимых %d)", all, morts_num);
		out += buf;
	} else if (morts_num) {
		sprintf(buf, " смертных %d", morts_num);
		out += buf;
	}

	out += ".\r\n";
	page_string(ch->desc, out);
}

// спам-контроль для команды кто и списка по дружинам
// работает аналогично восстановлению и расходованию маны у волхвов
// константы пока определены через #define в interpreter.h
// возвращает истину, если спамконтроль сработал и игроку придется подождать
bool PerformWhoSpamcontrol(CharData *ch, unsigned short int mode) {
	if (IS_IMMORTAL(ch)) {
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
