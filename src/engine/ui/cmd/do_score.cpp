/*
 \authors Created by Sventovit
 \date 26.02.2022.
 \brief Команды "счет"
 \details Команды "счет" - основная статистика персонажа игрока. Включает краткий счет, счет все и вариант для
 слабовидящих.
 */

#include "engine/ui/color.h"
#include "gameplay/communication/mail.h"
#include "gameplay/communication/parcel.h"
#include "engine/entities/char_data.h"
#include "engine/entities/char_player.h"
#include "gameplay/mechanics/player_races.h"
#include "gameplay/fight/fight_hit.h"
#include "gameplay/mechanics/bonus.h"
#include "gameplay/mechanics/glory.h"
#include "gameplay/mechanics/glory_const.h"
#include "gameplay/mechanics/liquid.h"
#include "engine/db/global_objects.h"
#include "gameplay/magic/magic.h"
#include "gameplay/mechanics/weather.h"
#include "gameplay/classes/pc_classes.h"
#include "gameplay/core/game_limits.h"
#include "gameplay/core/base_stats.h"
#include "gameplay/mechanics/groups.h"

#include <cmath>
#include <third_party_libs/fmt/include/fmt/format.h>

void PrintScoreBase(CharData *ch);
void PrintScoreList(CharData *ch);
void PrintScoreAll(CharData *ch);
void PrintRentableInfo(CharData *ch, std::ostringstream &out);
const char *GetPositionStr(CharData *ch);
const char *GetShortPositionStr(CharData *ch);
int CalcHitroll(CharData *ch);

/* extern */
int CalcAntiSavings(CharData *ch);
int calc_initiative(CharData *ch, bool mode);
void GetClassWeaponMod(ECharClass class_id, ESkill skill, int *damroll, int *hitroll);

const char *ac_text[] =
	{
		"&WВы защищены как БОГ",    //  -30
		"&WВы защищены как БОГ",    //  -29
		"&WВы защищены как БОГ",    //  -28
		"&gВы защищены почти как БОГ",    //  -27
		"&gВы защищены почти как БОГ",    //  -26
		"&gВы защищены почти как БОГ",    //  -25
		"&gНаилучшая защита",    //  -24
		"&gНаилучшая защита",    //  -23
		"&gНаилучшая защита",    //  -22
		"&gВеликолепная защита",    //  -21
		"&gВеликолепная защита",    //  -20
		"&gВеликолепная защита",    //  -19
		"&gОтличная защита",    //  -18
		"&gОтличная защита",    //  -17
		"&gОтличная защита",    //  -16
		"&GОчень хорошая защита",    //  -15
		"&GОчень хорошая защита",    //  -14
		"&GОчень хорошая защита",    //  -13
		"&GВесьма хорошая защита",    //  -12
		"&GВесьма хорошая защита",    //  -11
		"&GВесьма хорошая защита",    //  -10
		"&GХорошая защита",    //   -9
		"&GХорошая защита",    //   -8
		"&GХорошая защита",    //   -7
		"&GНеплохая защита",    //   -6
		"&GНеплохая защита",    //   -5
		"&GНеплохая защита",    //   -4
		"&YЗащита чуть выше среднего",    //   -3
		"&YЗащита чуть выше среднего",    //   -2
		"&YЗащита чуть выше среднего",    //   -1
		"&YСредняя защита",    //    0
		"&YЗащита чуть ниже среднего",
		"&YСлабая защита",
		"&RСлабая защита",
		"&RОчень слабая защита",
		"&RВы немного защищены",    // 5
		"&RВы совсем немного защищены",
		"&rВы чуть-чуть защищены",
		"&rВы легко уязвимы",
		"&rВы почти полностью уязвимы",
		"&rВы полностью уязвимы",    // 10
	};

void DoScore(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	skip_spaces(&argument);

	if (ch->IsNpc())
		return;

	if (utils::IsAbbr(argument, "все") || utils::IsAbbr(argument, "all")) {
		PrintScoreAll(ch);
		return;
	} else if (utils::IsAbbr(argument, "список") || utils::IsAbbr(argument, "list")) {
		PrintScoreList(ch);
		return;
	} else {
		PrintScoreBase(ch);
	}
}

void PrintBonusStateInfo(CharData *ch, std::ostringstream &out);

// \todo Переписать на вывод в поток с использованием общих со "счет все" функций
void PrintScoreList(CharData *ch) {
	sprintf(buf, "%s", PlayerRace::GetKinNameByNum(GET_KIN(ch), ch->get_sex()).c_str());
	buf[0] = LOWER(buf[0]);
	sprintf(buf1, "%s", religion_name[GET_RELIGION(ch)][static_cast<int>(ch->get_sex())]);
	buf1[0] = LOWER(buf1[0]);
	SendMsgToChar(ch, "Вы %s, %s, %s, %s, уровень %d, перевоплощений %d.\r\n", ch->get_name().c_str(),
				  buf,
				  MUD::Class(ch->GetClass()).GetCName(),
				  buf1,
				  GetRealLevel(ch),
				  GetRealRemort(ch));
	SendMsgToChar(ch, "Ваш возраст: %d, размер: %d(%d), рост: %d(%d), вес %d(%d).\r\n",
				  CalcCharAge(ch)->year,
				  GET_SIZE(ch), GET_REAL_SIZE(ch),
				  GET_HEIGHT(ch), GET_REAL_HEIGHT(ch),
				  GET_WEIGHT(ch), GET_REAL_WEIGHT(ch));
	SendMsgToChar(ch, "Вы можете выдержать %d(%d) %s повреждений, и пройти %d(%d) %s по ровной местности.\r\n",
				  ch->get_hit(), ch->get_real_max_hit(), GetDeclensionInNumber(ch->get_hit(), EWhat::kOneU),
				  ch->get_move(), ch->get_real_max_move(), GetDeclensionInNumber(ch->get_move(), EWhat::kMoveU));
	if (IS_MANA_CASTER(ch)) {
		SendMsgToChar(ch, "Ваша магическая энергия %d(%d) и вы восстанавливаете %d в сек.\r\n",
					  ch->mem_queue.stored, GET_MAX_MANA(ch), CalcManaGain(ch));
	}
	SendMsgToChar(ch, "Ваша сила: %d(%d), ловкость: %d(%d), телосложение: %d(%d), ум: %d(%d), мудрость: %d(%d), обаяние: %d(%d).\r\n",
				  ch->get_str(), GetRealStr(ch),
				  ch->get_dex(), GetRealDex(ch),
				  ch->get_con(), GetRealCon(ch),
				  ch->get_int(), GetRealInt(ch),
				  ch->get_wis(), GetRealWis(ch),
				  ch->get_cha(), GetRealCha(ch));

	HitData hit_params;
	hit_params.weapon = fight::kMainHand;
	hit_params.skill_num = ESkill::kAny; 
	hit_params.Init(ch, ch);
	bool need_dice = false;
	int max_dam = hit_params.CalcDmg(ch, need_dice); // без кубиков

	SendMsgToChar(ch, "Попадание: %d, повреждение: %d, запоминание: %d, успех колдовства: %d, удача: %d, инициатива: %d.\r\n",
				  CalcHitroll(ch),
				  max_dam,
				  int(GET_MANAREG(ch) * ch->get_cond_penalty(P_CAST)),
				  CalcAntiSavings(ch),
				  ch->calc_morale(),
				  calc_initiative(ch, false));
	SendMsgToChar(ch, "Бонусы в процентах: маг.урон: %d, физ. урон: %d, опыт: %d\r\n",
				  ch->add_abils.percent_spellpower_add,
				  ch->add_abils.percent_physdam_add,
				  ch->add_abils.percent_exp_add);
	SendMsgToChar(ch, "Сопротивление: огню: %d, воздуху: %d, воде: %d, земле: %d, тьме: %d, живучесть: %d, разум: %d, иммунитет: %d.\r\n",
				  MIN(GET_RESIST(ch, EResist::kFire), 75),
				  MIN(GET_RESIST(ch, EResist::kAir), 75),
				  MIN(GET_RESIST(ch, EResist::kWater), 75),
				  MIN(GET_RESIST(ch, EResist::kEarth), 75),
				  MIN(GET_RESIST(ch, EResist::kDark), 75),
				  MIN(GET_RESIST(ch, EResist::kVitality), 75),
				  MIN(GET_RESIST(ch, EResist::kMind), 75),
				  MIN(GET_RESIST(ch, EResist::kImmunity), 75));
	SendMsgToChar(ch, "Спас броски: воля: %d, здоровье: %d, стойкость: %d, реакция: %d, маг.резист: %d, физ.резист %d, отчар.резист: %d.\r\n",
			-CalcSaving(ch, ch, ESaving::kWill, false),
			-CalcSaving(ch, ch, ESaving::kCritical, false),
			-CalcSaving(ch, ch, ESaving::kStability, false),
			-CalcSaving(ch, ch, ESaving::kReflex, false),
			GET_MR(ch),
			GET_PR(ch),
			GET_AR(ch));
	SendMsgToChar(ch, "Восстановление: жизни: +%d%% (+%d), сил: +%d%% (+%d).\r\n",
				  ch->get_hitreg(),
				  hit_gain(ch),
				  ch->get_movereg(),
				  move_gain(ch));
	int ac = CalcBaseAc(ch) / 10;
	if (ac < 5) {
		const int mod = (1 - ch->get_cond_penalty(P_AC)) * 40;
		ac = ac + mod > 5 ? 5 : ac + mod;
	}
	SendMsgToChar(ch, "Броня: %d, защита: %d, поглощение %d.\r\n",
				  GET_ARMOUR(ch),
				  ac,
				  GET_ABSORBE(ch));
	SendMsgToChar(ch, "Вы имеете кун: на руках: %ld, на счету %ld. Гривны: %d, опыт: %ld, ДСУ: %ld.\r\n",
				  ch->get_gold(),
				  ch->get_bank(),
				  ch->get_hryvn(),
				  ch->get_exp(),
				  IS_IMMORTAL(ch) ? 1 : GetExpUntilNextLvl(ch, GetRealLevel(ch) + 1) - ch->get_exp());
	if (!ch->IsOnHorse())
		SendMsgToChar(ch, "Ваша позиция: %s", GetPositionStr(ch));
	else
		SendMsgToChar(ch, "Ваша позиция: Вы верхом на %s.\r\n", GET_PAD(ch->get_horse(), 5));
	if (ch->IsFlagged(EPrf::KSummonable))
		SendMsgToChar(ch, "Вы можете быть призваны.\r\n");
	else
		SendMsgToChar(ch, "Вы защищены от призыва.\r\n");
	SendMsgToChar(ch, "Голоден: %s, жажда: %s.\r\n", (GET_COND(ch, FULL) > kNormCondition)? "да" : "нет", GET_COND_M(ch, THIRST)? "да" : "нет");
	//Напоминаем о метке, если она есть.
	RoomData *label_room = room_spells::FindAffectedRoomByCasterID(ch->get_uid(), ESpell::kRuneLabel);
	if (label_room) {
		const int timer_room_label = room_spells::GetUniqueAffectDuration(ch->get_uid(), ESpell::kRuneLabel);
		if (timer_room_label > 0) {
			*buf2 = '\0';
			(timer_room_label + 1) / kSecsPerMudHour ? sprintf(buf2, "%d %s.", (timer_room_label + 1) / kSecsPerMudHour + 1,
															   GetDeclensionInNumber(
																   (timer_room_label + 1) / kSecsPerMudHour + 1,
																   EWhat::kHour)) : sprintf(buf2, "менее часа.");
			SendMsgToChar(ch, "Вы поставили рунную метку в комнате: '%s', она продержится еще %s\r\n", label_room->name, buf2);
			*buf2 = '\0';
		}
	}
	if (!NAME_GOD(ch) && GetRealLevel(ch) <= kNameLevel) {
		SendMsgToChar(ch, "ВНИМАНИЕ! ваше имя не одобрил никто из богов!\r\n");
		SendMsgToChar(ch, "Cкоро вы прекратите получать опыт, обратитесь к богам для одобрения имени.\r\n");
	} else if (NAME_BAD(ch)) {
		SendMsgToChar(ch, "ВНИМАНИЕ! ваше имя запрещено богами. Очень скоро вы прекратите получать опыт.\r\n");
	}
	SendMsgToChar(ch, "Вы можете вступить в группу с максимальной разницей в %2d %-75s\r\n",
				  grouping[ch->GetClass()][static_cast<int>(GetRealRemort(ch))],
				  (std::string(
					  GetDeclensionInNumber(grouping[ch->GetClass()][static_cast<int>(GetRealRemort(
						  ch))], EWhat::kLvl)
						  + std::string(" без потерь для опыта.")).substr(0, 76).c_str()));

	SendMsgToChar(ch, "Вы можете принять в группу максимум %d соратников.\r\n", group::max_group_size(ch));
	std::ostringstream out;
	out.str("");
	PrintRentableInfo(ch, out);
	PrintBonusStateInfo(ch, out);
	SendMsgToChar(out.str(), ch);
}

/*
 *  Функции для "счет все".
 */

const std::string &InfoStrPrefix(CharData *ch) {
	static const std::string cyan_star = fmt::format("{}*{}", kColorBoldCyn, kColorNrm);
	static const std::string space_str;
	if (ch->IsFlagged(EPrf::kBlindMode)) {
		return space_str;
	} else {
		return cyan_star;
	}
}

void PrintHorseInfo(CharData *ch, std::ostringstream &out) {
	if (ch->has_horse(false)) {
		if (ch->IsOnHorse()) {
			out << InfoStrPrefix(ch) << "Вы верхом на " << GET_PAD(ch->get_horse(), 5) << "." << "\r\n";
		} else {
			out << InfoStrPrefix(ch) << "У вас есть " << ch->get_horse()->get_name() << "." << "\r\n";
		}
	}
}

void PrintRuneLabelInfo(CharData *ch, std::ostringstream &out) {
	RoomData *label_room = room_spells::FindAffectedRoomByCasterID(ch->get_uid(), ESpell::kRuneLabel);
	if (label_room) {
		int timer_room_label = room_spells::GetUniqueAffectDuration(ch->get_uid(), ESpell::kRuneLabel);
		out << InfoStrPrefix(ch) << kColorBoldGrn << "Вы поставили рунную метку в комнате \'"
			<< label_room->name << "\' ";
		if (timer_room_label > 0) {
			out << "[продержится еще ";
			timer_room_label = (timer_room_label + 1) / kSecsPerMudHour + 1;
			if (timer_room_label > 0) {
				out << timer_room_label << " " << GetDeclensionInNumber(timer_room_label, EWhat::kHour) << "]." << "\r\n";
			} else {
				out << "менее часа]." << "\r\n";
			}
		}
		out << "\r\n";
	}
}

void PrintGloryInfo(CharData *ch, std::ostringstream &out) {
	auto glory = GloryConst::get_glory(ch->get_uid());
	if (glory > 0) {
		out << InfoStrPrefix(ch) << "Вы заслужили "
			<< glory << " " << GetDeclensionInNumber(glory, EWhat::kPoint) << " постоянной славы." << "\r\n";
	}
}

void PrintNameStatusInfo(CharData *ch, std::ostringstream &out) {
	if (!NAME_GOD(ch) && GetRealLevel(ch) <= kNameLevel) {
		out << InfoStrPrefix(ch) << kColorBoldRed << "ВНИМАНИЕ! " << kColorNrm
			<< "ваше имя не одобрил никто из богов!" << "\r\n";
		out << InfoStrPrefix(ch) << kColorBoldRed << "ВНИМАНИЕ! " << kColorNrm
			<< "Cкоро вы прекратите получать опыт, обратитесь к богам для одобрения имени." << "\r\n";
	} else if (NAME_BAD(ch)) {
		out << InfoStrPrefix(ch) << kColorBoldRed << "ВНИМАНИЕ! " << kColorNrm
			<< "Ваше имя запрещено богами. Вы не можете получать опыт." << "\r\n";
	}
}

void PrintSummonableInfo(CharData *ch, std::ostringstream &out) {
	if (ch->IsFlagged(EPrf::KSummonable)) {
		out << InfoStrPrefix(ch) << kColorBoldYel << "Вы можете быть призваны." << kColorNrm << "\r\n";
	} else {
		out << InfoStrPrefix(ch) << "Вы защищены от призыва." << "\r\n";
	}
}

void PrintBonusStateInfo(CharData *ch, std::ostringstream &out) {
	if (Bonus::is_bonus_active()) {
		out << InfoStrPrefix(ch) << Bonus::active_bonus_as_string() << " "
			<< Bonus::time_to_bonus_end_as_string() << "\r\n";
	}
}

void PrintExpTaxInfo(CharData *ch, std::ostringstream &out) {
	if (GET_GOD_FLAG(ch, EGf::kRemort) && CLAN(ch)) {
		out << InfoStrPrefix(ch) << "Вы самоотверженно отдаете весь получаемый опыт своей дружине." << "\r\n";
	}
}

void PrintBlindModeInfo(CharData *ch, std::ostringstream &out) {
	if (ch->IsFlagged(EPrf::kBlindMode)) {
		out << InfoStrPrefix(ch) << "Режим слепого игрока включен." << "\r\n";
	}
}

void PrintGroupMembershipInfo(CharData *ch, std::ostringstream &out) {
	if (GetRealLevel(ch) < kLvlImmortal) {
		out << InfoStrPrefix(ch) << "Вы можете вступить в группу с максимальной разницей "
			<< grouping[ch->GetClass()][static_cast<int>(GetRealRemort(ch))] << " "
			<< GetDeclensionInNumber(grouping[ch->GetClass()][static_cast<int>(GetRealRemort(ch))],
									 EWhat::kLvl)
			<< " без потерь для опыта." << "\r\n";

		out << InfoStrPrefix(ch) << "Вы можете принять в группу максимум "
			<< group::max_group_size(ch) << " соратников." << "\r\n";
	}
}

void PrintRentableInfo(CharData *ch, std::ostringstream &out) {
	if (NORENTABLE(ch)) {
		const time_t rent_time = NORENTABLE(ch) - time(nullptr);
		const auto minutes = rent_time > 60 ? rent_time / 60 : 0;
		out << InfoStrPrefix(ch) << kColorBoldRed << "В связи с боевыми действиями вы не можете уйти на постой еще ";
		if (minutes) {
			out << minutes << " " << GetDeclensionInNumber(minutes, EWhat::kMinU) << "." << kColorNrm << "\r\n";
		} else {
			out << rent_time << " " << GetDeclensionInNumber(rent_time, EWhat::kSec) << "." << kColorNrm << "\r\n";
		}
	} else if ((ch->in_room != kNowhere) && ROOM_FLAGGED(ch->in_room, ERoomFlag::kPeaceful) && !ch->IsFlagged(EPlrFlag::kKiller)) {
		out << InfoStrPrefix(ch) << kColorBoldGrn << "Тут вы чувствуете себя в безопасности." << kColorNrm << "\r\n";
	}
}
// \todo Сделать авторазмещение в комнате-кузнице горна и убрать эту функцию.
void PrinForgeInfo(CharData *ch, std::ostringstream &out) {
	if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kForge)
		&& (ch->GetSkill(ESkill::kJewelry)
		|| ch->GetSkill(ESkill::kRepair)
		|| ch->GetSkill(ESkill::kReforging))) {
		out << InfoStrPrefix(ch) << kColorBoldYel << "Это место отлично подходит для занятий кузнечным делом."
			<< kColorNrm << "\r\n";
	}
}

void PrintPostInfo(CharData *ch, std::ostringstream &out) {
	if (mail::has_mail(ch->get_uid())) {
		out << InfoStrPrefix(ch) << kColorBoldGrn << "Вас ожидает новое письмо, зайдите на почту." << kColorNrm << "\r\n";
	}
	if (Parcel::has_parcel(ch)) {
		out << InfoStrPrefix(ch) << kColorBoldGrn << "Вас ожидает посылка, зайдите на почту." << kColorNrm << "\r\n";
	}
}

void PrintProtectInfo(CharData *ch, std::ostringstream &out) {
	if (ch->get_protecting() && !ch->get_protecting()->purged()) {
		out << InfoStrPrefix(ch) << "Вы прикрываете " << GET_PAD(ch->get_protecting(), 3)
			<< " от нападения.\r\n";
	}
	if (!ch->who_protecting.empty()) {
		out << InfoStrPrefix(ch) << "Вас прикрывает: ";
		for (auto it : ch->who_protecting) {
			out << it->get_name() << " ";
		}
		out << "от нападения.\r\n";
	}
}

//	*** Функции для распечатки информации о наказаниях ***

struct ScorePunishmentInfo {
	ScorePunishmentInfo() = delete;
	explicit ScorePunishmentInfo(CharData *ch)
		: ch{ch} {};

	void SetFInfo(EPlrFlag punish_flag, const punishments::Punish *punish_info) {
		flag = punish_flag;
		punish = punish_info;
	};

	CharData *ch{};
	const punishments::Punish *punish{};
	std::string msg;
  	EPlrFlag flag{};
};

void PrintSinglePunishmentInfo(const ScorePunishmentInfo &info, std::ostringstream &out) {
	const auto current_time = time(nullptr);
	const auto hrs = (info.punish->duration - current_time)/3600;
	const auto mins = ((info.punish->duration - current_time)%3600 + 59)/60;

	out << InfoStrPrefix(info.ch) << kColorBoldRed << info.msg
		<< hrs << " " << GetDeclensionInNumber(hrs, EWhat::kHour) << " "
		<< mins << " " << GetDeclensionInNumber(mins, EWhat::kMinU);

	if (info.punish->reason != nullptr) {
		if (info.punish->reason[0] != '\0' && str_cmp(info.punish->reason, "(null)") != 0) {
			out << " [" << info.punish->reason << "]";
		}
	}
	out << "." << kColorNrm << "\r\n";
}

// \todo Место этого - в структурах, которые работают с наказаниями персонажа. Куда их и следует перенести.
bool IsPunished(const ScorePunishmentInfo &info) {
	return (info.ch->IsFlagged(info.flag)
		&& info.punish->duration != 0
		&& info.punish->duration > time(nullptr));
}

/*
 *  Аналогично, большая часть некрасивого кода этой функции написана потому, что нет внятного интерфейса для
 *  единообразной работы с системой наказаний персонажа. Городить тут ее эрзац, копируя куда-то все по третьему
 *  разу не вижу смысла.
 */
void PrintPunishmentsInfo(CharData *ch, std::ostringstream &out) {

	ScorePunishmentInfo punish_info{ch};

	punish_info.SetFInfo(EPlrFlag::kHelled, &ch->player_specials->phell);
	if (IsPunished(punish_info)) {
		punish_info.msg = "Вам предстоит провести в темнице еще ";
		PrintSinglePunishmentInfo(punish_info, out);
	}

	punish_info.SetFInfo(EPlrFlag::kFrozen, &ch->player_specials->pfreeze);
	if (IsPunished(punish_info)) {
		punish_info.msg = "Вы будете заморожены еще ";
		PrintSinglePunishmentInfo(punish_info, out);
	}

	punish_info.SetFInfo(EPlrFlag::kMuted, &ch->player_specials->pmute);
	if (IsPunished(punish_info)) {
		punish_info.msg = "Вы не сможете кричать еще ";
		PrintSinglePunishmentInfo(punish_info, out);
	}

	punish_info.SetFInfo(EPlrFlag::kDumbed, &ch->player_specials->pdumb);
	if (IsPunished(punish_info)) {
		punish_info.msg = "Вы будете молчать еще ";
		PrintSinglePunishmentInfo(punish_info, out);
	}

	/*
 	* Это приходится делать вручную, потому что флаги проклятия/любимца богов реализованы не через систему
 	* наказаний, а отдельными флагами. По-хорошему, надо их привести к общему знаменателю (тем более, что счесть
 	* избранность богами наказанием - в некотором смысле символично).
 	*/
	if (GET_GOD_FLAG(ch, EGf::kGodscurse) && GCURSE_DURATION(ch)) {
		punish_info.punish = &ch->player_specials->pgcurse;
		punish_info.msg = "Вы прокляты Богами на ";
		PrintSinglePunishmentInfo(punish_info, out);
	}

	/*
	 * Схоже с предыдущим пунктом: флаг "зарегистрирован" имеет противоположный остальным флагам наказаний смысл:
	 * персонаж НЕ наказан. Было бы разумнее ставить новым персонажам по умолчанию флаг unreg и снимать его при
	 * регистрации, но увы.
	 */
	if (!ch->IsFlagged(EPlrFlag::kRegistred) && UNREG_DURATION(ch) != 0 && UNREG_DURATION(ch) > time(nullptr)) {
		punish_info.punish = &ch->player_specials->punreg;
		punish_info.msg = "Вы не сможете входить с одного IP еще ";
		PrintSinglePunishmentInfo(punish_info, out);
	}
}

void PrintMorphInfo(CharData *ch, std::ostringstream &out) {
	if (ch->is_morphed()) {
		out << InfoStrPrefix(ch) << kColorBoldYel << "Вы находитесь в звериной форме - "
			<< ch->get_morph_desc() << "." << kColorNrm << "\r\n";
	}
}

/**
 * Вывести в таблицу основную информацию персонажа, начиная с указанного столбца.
 * @param ch  - персонаж, для которого выводится статистика.
 * @param table - таблица, в которую поступают сведения.
 * @param col - номер столбца.
 * @return - количество заполненных функцией столбцов.
 */
int PrintBaseInfoToTable(CharData *ch, table_wrapper::Table &table, std::size_t col) {
	std::size_t row{0};
	table[row][col] = std::string("Племя: ") + PlayerRace::GetRaceNameByNum(GET_KIN(ch), GET_RACE(ch), ch->get_sex());
	table[++row][col] = std::string("Вера: ") + religion_name[GET_RELIGION(ch)][static_cast<int>(ch->get_sex())];
	table[++row][col] = std::string("Уровень: ") + std::to_string(GetRealLevel(ch));
	table[++row][col] = std::string("Перевоплощений: ") + std::to_string(GetRealRemort(ch));
	table[++row][col] = std::string("Возраст: ") + std::to_string(CalcCharAge(ch)->year);
	if (ch->GetLevel() < kLvlImmortal) {
		table[++row][col] = std::string("Опыт: ") + PrintNumberByDigits(ch->get_exp());
		table[++row][col] = std::string("ДСУ: ") + PrintNumberByDigits(
			GetExpUntilNextLvl(ch, GetRealLevel(ch) + 1) - ch->get_exp());
	}
	table[++row][col] = std::string("Кун: ") + PrintNumberByDigits(ch->get_gold());
	table[++row][col] = std::string("На счету: ") + PrintNumberByDigits(ch->get_bank());
	table[++row][col] = GetShortPositionStr(ch);
	table[++row][col] = std::string("Голоден: ") + (GET_COND(ch, FULL) > kNormCondition ? "Угу :(" : "Нет");
	table[++row][col] = std::string("Жажда: ") + (GET_COND_M(ch, THIRST) ? "Наливай!" : "Нет");
	if (GET_COND(ch, DRUNK) >= kDrunked) {
		table[++row][col] = (IsAffectedBySpell(ch, ESpell::kAbstinent) ? "Похмелье." : "Вы пьяны.");
	}
	if (PlayerSystem::weight_dex_penalty(ch)) {
		table[++row][col] = "Вы перегружены!";
	}

	return 1; // заполнено столбцов
}

/**
 * Вывести в таблицу базовые параметры персонажа, начиная с указанного столбца.
 * @param ch  - персонаж, для которого выводится статистика.
 * @param table - таблица, в которую поступают сведения.
 * @param col - номер столбца.
 * @return - количество заполненных функцией столбцов.
 */
int PrintBaseStatsToTable(CharData *ch, table_wrapper::Table &table, std::size_t col) {
	std::size_t row{0};
	table[row][col] = "Сила";			table[row][col + 1] = std::to_string(ch->get_str()) + " (" + std::to_string(GetRealStr(ch)) + ")";
	table[++row][col] = "Ловкость";		table[row][col + 1] = std::to_string(ch->get_dex()) + " (" + std::to_string(GetRealDex(ch)) + ")";
	table[++row][col] = "Телосложение";	table[row][col + 1] = std::to_string(ch->get_con()) + " (" + std::to_string(GetRealCon(ch)) + ")";
	table[++row][col] = "Мудрость";		table[row][col + 1] = std::to_string(ch->get_wis()) + " (" + std::to_string(GetRealWis(ch)) + ")";
	table[++row][col] = "Интеллект";	table[row][col + 1] = std::to_string(ch->get_int()) + " (" + std::to_string(GetRealInt(ch)) + ")";
	table[++row][col] = "Обаяние";		table[row][col + 1] = std::to_string(ch->get_cha()) + " (" + std::to_string(GetRealCha(ch)) + ")";
	table[++row][col] = "Рост";			table[row][col + 1] = std::to_string(GET_HEIGHT(ch)) + " (" + std::to_string(GET_REAL_HEIGHT(ch)) + ")";
	table[++row][col] = "Вес";			table[row][col + 1] = std::to_string(GET_WEIGHT(ch)) + " (" + std::to_string(GET_REAL_WEIGHT(ch)) + ")";
	table[++row][col] = "Размер";		table[row][col + 1] = std::to_string(GET_SIZE(ch)) + " (" + std::to_string(GET_REAL_SIZE(ch)) + ")";
	table[++row][col] = "Жизнь";		table[row][col + 1] = std::to_string(ch->get_hit()) + "(" + std::to_string(ch->get_real_max_hit()) + ")";
	table[++row][col] = "Восст. жизни";	table[row][col + 1] = "+" + std::to_string(ch->get_hitreg()) + "% (" + std::to_string(hit_gain(ch)) + ")";
	table[++row][col] = "Выносливость";	table[row][col + 1] = std::to_string(ch->get_move()) + "(" + std::to_string(ch->get_real_max_move()) + ")";
	table[++row][col] = "Восст. сил";	table[row][col + 1] = "+" + std::to_string(ch->get_movereg()) + "% (" + std::to_string(move_gain(ch)) + ")";
	if (IS_MANA_CASTER(ch)) {
		table[++row][col] = "Мана"; 		table[row][col + 1] = std::to_string(ch->mem_queue.stored) + "(" + std::to_string(GET_MAX_MANA(ch)) + ")";
		table[++row][col] = "Восст. маны";	table[row][col + 1] = "+" + std::to_string(CalcManaGain(ch)) + " сек.";
	}

	return 2; // заполнено столбцов
}

/**
 * Вывести в таблицу вторичные параметры персонажа, начиная с указанного столбца.
 * @param ch  - персонаж, для которого выводится статистика.
 * @param table - таблица, в которую поступают сведения.
 * @param col - номер столбца.
 * @return - количество заполненных функцией столбцов.
 */
int PrintSecondaryStatsToTable(CharData *ch, table_wrapper::Table &table, std::size_t col) {
	HitData hit_params;
	hit_params.weapon = fight::kMainHand;
	hit_params.skill_num = ESkill::kAny; 
	hit_params.Init(ch, ch);
	auto max_dam = hit_params.CalcDmg(ch, false);

	std::size_t row{0};
	table[row][col] = "Попадание";		table[row][col + 1] = std::to_string(CalcHitroll(ch));
	table[++row][col] = "Повреждение";	table[row][col + 1] = std::to_string(max_dam);
	table[++row][col] = "Колдовство";	table[row][col + 1] = std::to_string(CalcAntiSavings(ch));
	table[++row][col] = "Запоминание";	table[row][col + 1] = std::to_string(std::lround(GET_MANAREG(ch)*ch->get_cond_penalty(P_CAST)));
	table[++row][col] = "Удача";		table[row][col + 1] = std::to_string(ch->calc_morale());
	table[++row][col] = "Сила заклинаний %";	table[row][col + 1] = std::to_string(ch->add_abils.percent_spellpower_add);
	table[++row][col] = "Физ. урон %";	table[row][col + 1] = std::to_string(ch->add_abils.percent_physdam_add);
	table[++row][col] = "Инициатива";	table[row][col + 1] = std::to_string(calc_initiative(ch, false));
	table[++row][col] = "Спас-броски:";	table[row][col + 1] = " ";
	table[++row][col] = "Воля";			table[row][col + 1] = std::to_string(-CalcSaving(ch, ch, ESaving::kWill, false));
	table[++row][col] = "Здоровье";		table[row][col + 1] = std::to_string(-CalcSaving(ch, ch, ESaving::kCritical, false));
	table[++row][col] = "Стойкость";	table[row][col + 1] = std::to_string(-CalcSaving(ch, ch, ESaving::kStability, false));
	table[++row][col] = "Реакция";		table[row][col + 1] = std::to_string(-CalcSaving(ch, ch, ESaving::kReflex, false));

	return 2; // заполнено столбцов
}

/**
 * Вывести в таблицу защитные параметры персонажа, начиная с указанного столбца таблицы.
 * @param ch  - персонаж, для которого выводится статистика.
 * @param table - таблица, в которую поступают сведения.
 * @param col - номер столбца.
 * @return - количество заполненных функцией столбцов.
 */
int PrintProtectiveStatsToTable(CharData *ch, table_wrapper::Table &table, std::size_t col) {
	std::size_t row{0};
	int ac = CalcBaseAc(ch) / 10;
	if (ac < 5) {
		const int mod = (1 - ch->get_cond_penalty(P_AC)) * 40;
		ac = ac + mod > 5 ? 5 : ac + mod;
	}

	table[row][col] = "Броня";				table[row][col + 1] = std::to_string(GET_ARMOUR(ch));
	table[++row][col] = "Защита";			table[row][col + 1] = std::to_string(ac);
	table[++row][col] = "Поглощение";		table[row][col + 1] = std::to_string(GET_ABSORBE(ch));
	table[++row][col] = "Сопротивления: ";	table[row][col + 1] = " ";
	table[++row][col] = "Урону";			table[row][col + 1] = std::to_string(GET_PR(ch));
	table[++row][col] = "Заклинаниям";		table[row][col + 1] = std::to_string(GET_MR(ch));
	table[++row][col] = "Магии огня";		table[row][col + 1] = std::to_string(std::min(GET_RESIST(ch, EResist::kFire), kMaxPcResist));
	table[++row][col] = "Магии воды";		table[row][col + 1] = std::to_string(std::min(GET_RESIST(ch, EResist::kWater), kMaxPcResist));
	table[++row][col] = "Магии земли";		table[row][col + 1] = std::to_string(std::min(GET_RESIST(ch, EResist::kEarth), kMaxPcResist));
	table[++row][col] = "Магии воздуха";	table[row][col + 1] = std::to_string(std::min(GET_RESIST(ch, EResist::kAir), kMaxPcResist));
	table[++row][col] = "Магии тьмы";		table[row][col + 1] = std::to_string(std::min(GET_RESIST(ch, EResist::kDark), kMaxPcResist));
	table[++row][col] = "Магии разума";		table[row][col + 1] = std::to_string(std::min(GET_RESIST(ch, EResist::kMind), kMaxPcResist));
	table[++row][col] = "Тяжелым ранам";	table[row][col + 1] = std::to_string(std::min(GET_RESIST(ch, EResist::kVitality), kMaxPcResist));
	table[++row][col] = "Ядам и болезням";	table[row][col + 1] = std::to_string(std::min(GET_RESIST(ch, EResist::kImmunity), kMaxPcResist));

	return 2; // заполнено столбцов
}

void PrintSelfHitrollInfo(CharData *ch, std::ostringstream &out) {
	HitData hit;
	hit.weapon = fight::AttackType::kMainHand;
	hit.skill_num = ESkill::kAny; 
	hit.Init(ch, ch);
	hit.CalcBaseHitroll(ch);
	hit.CalcStaticHitroll(ch);
	hit.CalcCircumstantialAc(ch);

	HitData hit2;
	hit2.weapon = fight::AttackType::kOffHand;
	hit2.skill_num = ESkill::kAny; 
	hit2.Init(ch, ch);
	hit2.CalcBaseHitroll(ch);
	hit2.CalcStaticHitroll(ch);

	out << InfoStrPrefix(ch) << kColorBoldCyn
		<< "RIGHT_WEAPON: hitroll=" << -hit.calc_thaco
		<< ", LEFT_WEAPON: hitroll=" << hit2.calc_thaco
		<< ", AC=" << hit.victim_ac << "." << kColorNrm << "\r\n";
}

void PrintTesterModeInfo(CharData *ch, std::ostringstream &out) {
	if (ch->IsFlagged(EPrf::kTester)) {
		out << InfoStrPrefix(ch) << kColorBoldCyn << "Включен режим тестирования." << kColorNrm << "\r\n";
		PrintSelfHitrollInfo(ch, out);
	}
}
/**
 * Вывести в поток дополнительные строки о состоянии персонажа игрока.
 * Каждая строка начинается с префикса (в режиме незрячего игрока - с пробела).
 * @param ch - Персонаж, для которого выводится информация.
 * @param out - Поток для вывода.
 */
void PrintAdditionalInfo(CharData *ch, std::ostringstream &out) {
	/* Плюс-минус игровая информация */
	PrintHorseInfo(ch, out);
	PrintMorphInfo(ch, out);
	PrintRuneLabelInfo(ch, out);
	PrintProtectInfo(ch, out);
	PrintBonusStateInfo(ch, out);
	PrinForgeInfo(ch, out);
	/* Игромеханическая информация */
	PrintGroupMembershipInfo(ch, out);
	PrintGloryInfo(ch, out);
	PrintExpTaxInfo(ch, out);
	PrintPostInfo(ch, out);
	/* Всякого рода наказания и гандикапы */
	PrintRentableInfo(ch, out);
	PrintNameStatusInfo(ch, out);
	PrintPunishmentsInfo(ch, out);
	PrintSummonableInfo(ch, out);
	PrintBlindModeInfo(ch, out);
	/* Добавочная информация для тестеров */
	PrintTesterModeInfo(ch, out);
}

void PrintScoreAll(CharData *ch) {
	// Пишем заголовок таблицы (увы, библиоетка таблиц их не поддерживает)
	std::ostringstream out;
	out << "  Вы " << ch->get_name() << ", " << MUD::Class(ch->GetClass()).GetName() << ". Ваши показатели:" << "\r\n";

	// Заполняем основную таблицу и выводим в поток
	table_wrapper::Table table;
	std::size_t current_column{0};
	current_column += PrintBaseInfoToTable(ch, table, current_column);
	current_column += PrintBaseStatsToTable(ch, table, current_column);
	current_column += PrintSecondaryStatsToTable(ch, table, current_column);
	PrintProtectiveStatsToTable(ch, table, current_column);
	table_wrapper::DecorateCuteTable(ch, table);
	table_wrapper::PrintTableToStream(out, table);

	PrintAdditionalInfo(ch, out);

	SendMsgToChar(out.str(), ch);
}

// \todo Переписать на вывод в поток с использованием общих со "счет все" функций
void PrintScoreBase(CharData *ch) {
	std::ostringstream out;

	out << "Вы " << ch->GetTitleAndName() << " ("
		<< PlayerRace::GetKinNameByNum(GET_KIN(ch), ch->get_sex()) << ", "
		<< PlayerRace::GetRaceNameByNum(GET_KIN(ch), GET_RACE(ch), ch->get_sex()) << ", "
		<< religion_name[GET_RELIGION(ch)][static_cast<int>(ch->get_sex())] << ", "
		<< MUD::Class(ch->GetClass()).GetCName() << " "
		<< GetRealLevel(ch) << " уровня)." << "\r\n";

	PrintNameStatusInfo(ch, out);
	const auto &age = CalcCharAge(ch);
	out << "Сейчас вам " << age->year << " " << GetDeclensionInNumber(age->year, EWhat::kYear) << ".";
	if (CalcCharAge(ch)->month == 0 && CalcCharAge(ch)->day == 0) {
		out << kColorBoldRed << " У вас сегодня День Варенья!" << kColorNrm;
	}
	out << "\r\n";

	SendMsgToChar(out.str(), ch);
	// Продолжить с этого места

	sprintf(buf,
			"Вы можете выдержать %d(%d) %s повреждения, и пройти %d(%d) %s по ровной местности.\r\n",
			ch->get_hit(), ch->get_real_max_hit(), GetDeclensionInNumber(ch->get_hit(),
																	 EWhat::kOneU),
			ch->get_move(), ch->get_real_max_move(), GetDeclensionInNumber(ch->get_move(), EWhat::kMoveU));

	if (IS_MANA_CASTER(ch)) {
		sprintf(buf + strlen(buf),
				"Ваша магическая энергия %d(%d) и вы восстанавливаете %d в сек.\r\n",
				ch->mem_queue.stored, GET_MAX_MANA(ch), CalcManaGain(ch));
	}

	sprintf(buf + strlen(buf),
			"%sВаши характеристики :\r\n"
			"  Сила : %2d(%2d)"
			"  Подв : %2d(%2d)"
			"  Тело : %2d(%2d)"
			"  Мудр : %2d(%2d)"
			"  Ум   : %2d(%2d)"
			"  Обаян: %2d(%2d)\r\n"
			"  Размер %3d(%3d)"
			"  Рост   %3d(%3d)"
			"  Вес    %3d(%3d)%s\r\n",
			kColorBoldCyn, ch->get_str(), GetRealStr(ch),
			ch->get_dex(), GetRealDex(ch),
			ch->get_con(), GetRealCon(ch),
			ch->get_wis(), GetRealWis(ch),
			ch->get_int(), GetRealInt(ch),
			ch->get_cha(), GetRealCha(ch),
			GET_SIZE(ch), GET_REAL_SIZE(ch),
			GET_HEIGHT(ch), GET_REAL_HEIGHT(ch), GET_WEIGHT(ch), GET_REAL_WEIGHT(ch), kColorNrm);

	if (IS_IMMORTAL(ch)) {
		sprintf(buf + strlen(buf),
				"%sВаши боевые качества :\r\n"
				"  AC   : %4d(%4d)"
				"  DR   : %4d(%4d)%s\r\n",
				kColorBoldGrn, GET_AC(ch), CalcBaseAc(ch),
				GET_DR(ch), GetRealDamroll(ch), kColorNrm);
	} else {
		int ac = CalcBaseAc(ch) / 10;

		if (ac < 5) {
			const int mod = (1 - ch->get_cond_penalty(P_AC)) * 40;
			ac = ac + mod > 5 ? 5 : ac + mod;
		}

		int ac_t = std::clamp(ac + 30, 0, 40);
		sprintf(buf + strlen(buf), "&GВаши боевые качества :\r\n"
								   "  Защита  (AC)     : %4d - %s&G\r\n"
								   "  Броня/Поглощение : %4d/%d&n\r\n",
				ac, ac_text[ac_t], GET_ARMOUR(ch), GET_ABSORBE(ch));
	}
	sprintf(buf + strlen(buf), "Ваш опыт - %ld %s, бонус %d %c. ", ch->get_exp(),
			GetDeclensionInNumber(ch->get_exp(), EWhat::kPoint), ch->add_abils.percent_exp_add, '%');
	if (GetRealLevel(ch) < kLvlImmortal) {
		if (ch->IsFlagged(EPrf::kBlindMode)) {
			sprintf(buf + strlen(buf), "\r\n");
		}
		sprintf(buf + strlen(buf),
				"Вам осталось набрать %ld %s до следующего уровня.\r\n",
				GetExpUntilNextLvl(ch, GetRealLevel(ch) + 1) - ch->get_exp(),
				GetDeclensionInNumber(GetExpUntilNextLvl(ch, GetRealLevel(ch) + 1) - ch->get_exp(), EWhat::kPoint));
	} else
		sprintf(buf + strlen(buf), "\r\n");

	sprintf(buf + strlen(buf),
			"У вас на руках %ld %s и %d %s",
			ch->get_gold(),
			GetDeclensionInNumber(ch->get_gold(), EWhat::kMoneyA),
			ch->get_hryvn(),
			GetDeclensionInNumber(ch->get_hryvn(), EWhat::kTorc));
	if (ch->get_bank() > 0)
		sprintf(buf + strlen(buf), " (и еще %ld %s припрятано в лежне).\r\n",
				ch->get_bank(), GetDeclensionInNumber(ch->get_bank(), EWhat::kMoneyA));
	else
		strcat(buf, ".\r\n");

	if (GetRealLevel(ch) < kLvlImmortal) {
		sprintf(buf + strlen(buf),
				"Вы можете вступить в группу с максимальной разницей в %d %s без потерь для опыта.\r\n",
				grouping[ch->GetClass()][static_cast<int>(GetRealRemort(ch))],
				GetDeclensionInNumber(grouping[ch->GetClass()][static_cast<int>(GetRealRemort(ch))],
									  EWhat::kLvl));
	}

	//Напоминаем о метке, если она есть.
	RoomData *label_room = room_spells::FindAffectedRoomByCasterID(ch->get_uid(), ESpell::kRuneLabel);
	if (label_room) {
		sprintf(buf + strlen(buf),
				"&G&qВы поставили рунную метку в комнате '%s'.&Q&n\r\n",
				std::string(label_room->name).c_str());
	}

	int glory = Glory::get_glory(ch->get_uid());
	if (glory) {
	//	sprintf(buf + strlen(buf), "Вы заслужили %d %s славы.\r\n", glory, GetDeclensionInNumber(glory, EWhat::kPoint));
	}
	glory = GloryConst::get_glory(ch->get_uid());
	if (glory) {
		sprintf(buf + strlen(buf), "Вы заслужили %d %s постоянной славы.\r\n",
				glory, GetDeclensionInNumber(glory, EWhat::kPoint));
	}

	TimeInfoData playing_time = *CalcRealTimePassed((time(nullptr) - ch->player_data.time.logon) + ch->player_data.time.played, 0);
	sprintf(buf + strlen(buf), "Вы играете %d %s %d %s реального времени.\r\n",
			playing_time.day, GetDeclensionInNumber(playing_time.day, EWhat::kDay),
			playing_time.hours, GetDeclensionInNumber(playing_time.hours, EWhat::kHour));
	SendMsgToChar(buf, ch);

	if (!ch->IsOnHorse())
		SendMsgToChar(ch, "%s", GetPositionStr(ch));

	strcpy(buf, kColorBoldGrn);
	const auto value_drunked = GET_COND(ch, DRUNK);
	if (value_drunked >= kDrunked) {
		if (IsAffectedBySpell(ch, ESpell::kAbstinent))
			strcat(buf, "Привет с большого бодуна!\r\n");
		else {
			if (value_drunked >= kMortallyDrunked)
				strcat(buf, "Вы так пьяны, что ваши ноги не хотят слушаться вас...\r\n");
			else if (value_drunked >= 10)
				strcat(buf, "Вы так пьяны, что вам хочется петь песни.\r\n");
			else if (value_drunked >= 5)
				strcat(buf, "Вы пьяны.\r\n");
			else
				strcat(buf, "Вы немного пьяны.\r\n");
		}

	}
	if (GET_COND_M(ch, FULL))
		strcat(buf, "Вы голодны.\r\n");
	if (GET_COND_M(ch, THIRST))
		strcat(buf, "Вас мучает жажда.\r\n");
	/*
	   strcat(buf, KICYN);
	   strcat(buf,"Аффекты :\r\n");
	   (ch)->char_specials.saved.affected_by.sprintbits(affected_bits, buf2, "\r\n");
	   strcat(buf,buf2);
	 */
	if (ch->IsFlagged(EPrf::KSummonable))
		strcat(buf, "Вы можете быть призваны.\r\n");

	if (ch->has_horse(false)) {
		if (ch->IsOnHorse())
			sprintf(buf + strlen(buf), "Вы верхом на %s.\r\n", GET_PAD(ch->get_horse(), 5));
		else
			sprintf(buf + strlen(buf), "У вас есть %s.\r\n", GET_NAME(ch->get_horse()));
	}
	strcat(buf, kColorNrm);
	SendMsgToChar(buf, ch);
	if (NORENTABLE(ch)) {
		sprintf(buf,
				"%sВ связи с боевыми действиями вы не можете уйти на постой.%s\r\n",
				kColorBoldRed, kColorNrm);
		SendMsgToChar(buf, ch);
	} else if ((ch->in_room != kNowhere) && ROOM_FLAGGED(ch->in_room, ERoomFlag::kPeaceful) && !ch->IsFlagged(EPlrFlag::kKiller)) {
		sprintf(buf, "%sТут вы чувствуете себя в безопасности.%s\r\n", kColorBoldGrn, kColorNrm);
		SendMsgToChar(buf, ch);
	}

	if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kForge)
		&& (ch->GetSkill(ESkill::kJewelry) || ch->GetSkill(ESkill::kRepair) || ch->GetSkill(ESkill::kReforging))) {
		sprintf(buf,
				"%sЭто место отлично подходит для занятий кузнечным делом.%s\r\n",
				kColorBoldGrn,
				kColorNrm);
		SendMsgToChar(buf, ch);
	}

	if (mail::has_mail(ch->get_uid())) {
		sprintf(buf, "%sВас ожидает новое письмо, зайдите на почту!%s\r\n", kColorBoldGrn, kColorNrm);
		SendMsgToChar(buf, ch);
	}

	if (Parcel::has_parcel(ch)) {
		sprintf(buf, "%sВас ожидает посылка, зайдите на почту!%s\r\n", kColorBoldGrn, kColorNrm);
		SendMsgToChar(buf, ch);
	}

	if (ch->IsFlagged(EPlrFlag::kHelled) && HELL_DURATION(ch) && HELL_DURATION(ch) > time(nullptr)) {
		const int hrs = (HELL_DURATION(ch) - time(nullptr)) / 3600;
		const int mins = ((HELL_DURATION(ch) - time(nullptr)) % 3600 + 59) / 60;
		sprintf(buf,
				"Вам предстоит провести в темнице еще %d %s %d %s [%s].\r\n",
				hrs, GetDeclensionInNumber(hrs, EWhat::kHour), mins, GetDeclensionInNumber(mins,
																						   EWhat::kMinU),
				HELL_REASON(ch) ? HELL_REASON(ch) : "-");
		SendMsgToChar(buf, ch);
	}
	if (ch->IsFlagged(EPlrFlag::kMuted) && MUTE_DURATION(ch) != 0 && MUTE_DURATION(ch) > time(nullptr)) {
		const int hrs = (MUTE_DURATION(ch) - time(nullptr)) / 3600;
		const int mins = ((MUTE_DURATION(ch) - time(nullptr)) % 3600 + 59) / 60;
		sprintf(buf, "Вы не сможете кричать еще %d %s %d %s [%s].\r\n",
				hrs, GetDeclensionInNumber(hrs, EWhat::kHour),
				mins, GetDeclensionInNumber(mins, EWhat::kMinU), MUTE_REASON(ch) ? MUTE_REASON(ch) : "-");
		SendMsgToChar(buf, ch);
	}
	if (ch->IsFlagged(EPlrFlag::kDumbed) && DUMB_DURATION(ch) != 0 && DUMB_DURATION(ch) > time(nullptr)) {
		const int hrs = (DUMB_DURATION(ch) - time(nullptr)) / 3600;
		const int mins = ((DUMB_DURATION(ch) - time(nullptr)) % 3600 + 59) / 60;
		sprintf(buf, "Вы будете молчать еще %d %s %d %s [%s].\r\n",
				hrs, GetDeclensionInNumber(hrs, EWhat::kHour),
				mins, GetDeclensionInNumber(mins, EWhat::kMinU), DUMB_REASON(ch) ? DUMB_REASON(ch) : "-");
		SendMsgToChar(buf, ch);
	}
	if (ch->IsFlagged(EPlrFlag::kFrozen) && FREEZE_DURATION(ch) != 0 && FREEZE_DURATION(ch) > time(nullptr)) {
		const int hrs = (FREEZE_DURATION(ch) - time(nullptr)) / 3600;
		const int mins = ((FREEZE_DURATION(ch) - time(nullptr)) % 3600 + 59) / 60;
		sprintf(buf, "Вы будете заморожены еще %d %s %d %s [%s].\r\n",
				hrs, GetDeclensionInNumber(hrs, EWhat::kHour),
				mins, GetDeclensionInNumber(mins, EWhat::kMinU), FREEZE_REASON(ch) ? FREEZE_REASON(ch) : "-");
		SendMsgToChar(buf, ch);
	}

	if (!ch->IsFlagged(EPlrFlag::kRegistred) && UNREG_DURATION(ch) != 0 && UNREG_DURATION(ch) > time(nullptr)) {
		const int hrs = (UNREG_DURATION(ch) - time(nullptr)) / 3600;
		const int mins = ((UNREG_DURATION(ch) - time(nullptr)) % 3600 + 59) / 60;
		sprintf(buf, "Вы не сможете заходить с одного IP еще %d %s %d %s [%s].\r\n",
				hrs, GetDeclensionInNumber(hrs, EWhat::kHour),
				mins, GetDeclensionInNumber(mins, EWhat::kMinU), UNREG_REASON(ch) ? UNREG_REASON(ch) : "-");
		SendMsgToChar(buf, ch);
	}

	if (GET_GOD_FLAG(ch, EGf::kGodscurse) && GCURSE_DURATION(ch)) {
		const int hrs = (GCURSE_DURATION(ch) - time(nullptr)) / 3600;
		const int mins = ((GCURSE_DURATION(ch) - time(nullptr)) % 3600 + 59) / 60;
		sprintf(buf, "Вы прокляты Богами на %d %s %d %s.\r\n",
				hrs, GetDeclensionInNumber(hrs, EWhat::kHour), mins, GetDeclensionInNumber(mins, EWhat::kMinU));
		SendMsgToChar(buf, ch);
	}

	if (ch->is_morphed()) {
		sprintf(buf, "Вы находитесь в звериной форме - %s.\r\n", ch->get_morph_desc().c_str());
		SendMsgToChar(buf, ch);
	}
	if (CanUseFeat(ch, EFeat::kSoulsCollector)) {
		const int souls = ch->get_souls();
		if (souls == 0) {
			sprintf(buf, "Вы не имеете чужих душ.\r\n");
			SendMsgToChar(buf, ch);
		} else {
			if (souls == 1) {
				sprintf(buf, "Вы имеете всего одну душу в запасе.\r\n");
				SendMsgToChar(buf, ch);
			}
			if (souls > 1 && souls < 5) {
				sprintf(buf, "Вы имеете %d души в запасе.\r\n", souls);
				SendMsgToChar(buf, ch);
			}
			if (souls >= 5) {
				sprintf(buf, "Вы имеете %d чужих душ в запасе.\r\n", souls);
				SendMsgToChar(buf, ch);
			}
		}
	}
	if (ch->get_ice_currency() > 0) {
		if (ch->get_ice_currency() == 1) {
			sprintf(buf, "У вас в наличии есть одна жалкая искристая снежинка.\r\n");
			SendMsgToChar(buf, ch);
		} else if (ch->get_ice_currency() < 5) {
			sprintf(buf, "У вас в наличии есть жалкие %d искристые снежинки.\r\n", ch->get_ice_currency());
			SendMsgToChar(buf, ch);
		} else {
			sprintf(buf, "У вас в наличии есть %d искристых снежинок.\r\n", ch->get_ice_currency());
			SendMsgToChar(buf, ch);
		}
	}
	if (ch->get_nogata() > 0 && ROOM_FLAGGED(ch->in_room, ERoomFlag::kDominationArena)) {
		int value = ch->get_nogata();
		if (ch->get_nogata() == 1) {
			sprintf(buf, "У вас в наличии есть одна жалкая ногата.\r\n");
		}
		else {
			sprintf(buf, "У вас в наличии есть %d %s.\r\n", value, GetDeclensionInNumber(value, EWhat::kNogataU));
		}
		SendMsgToChar(buf, ch);
	}
}

int CalcHitroll(CharData *ch) {
	ESkill skill = ESkill::kTwohands;
	int hr = 0;
	int max_dam = 0;
	ObjData *weapon = GET_EQ(ch, kBoths);
	if (weapon) {
		if (weapon->get_type() == EObjType::kWeapon) {
			skill = static_cast<ESkill>(weapon->get_spec_param());
			if (ch->GetSkill(skill) == 0) {
				hr -= (50 - std::min(50, GetRealInt(ch))) / 3;
			} else {
				GetClassWeaponMod(ch->GetClass(), skill, &max_dam, &hr);
			}
		}
	} else {
		weapon = GET_EQ(ch, kHold);
		if (weapon) {
			if (weapon->get_type() == EObjType::kWeapon) {
				skill = static_cast<ESkill>(weapon->get_spec_param());
				if (ch->GetSkill(skill) == 0) {
					hr -= (50 - std::min(50, GetRealInt(ch))) / 3;
				} else {
					GetClassWeaponMod(ch->GetClass(), skill, &max_dam, &hr);
				}
			}
		}
		weapon = GET_EQ(ch, kWield);
		if (weapon) {
			if (weapon->get_type() == EObjType::kWeapon) {
				skill = static_cast<ESkill>(weapon->get_spec_param());
				if (ch->GetSkill(skill) == 0) {
					hr -= (50 - std::min(50, GetRealInt(ch))) / 3;
				} else {
					GetClassWeaponMod(ch->GetClass(), skill, &max_dam, &hr);
				}
			}
		}
	}
	if (weapon) {
		int tmphr = 0;
		HitData::CheckWeapFeats(ch, static_cast<ESkill>(weapon->get_spec_param()), tmphr, max_dam);
		hr -= tmphr;
	} else {
		HitData::CheckWeapFeats(ch, ESkill::kPunch, hr, max_dam);
	}
	if (CanUseFeat(ch, EFeat::kWeaponFinesse)) {
		hr += str_bonus(GetRealDex(ch), STR_TO_HIT);
	} else {
		hr += str_bonus(GetRealStr(ch), STR_TO_HIT);
	}
	hr += GET_REAL_HR(ch) - GetThac0(ch->GetClass(), GetRealLevel(ch));
	if (ch->IsFlagged(EPrf::kPerformPowerAttack)) {
		hr -= 2;
	}
	if (ch->IsFlagged(EPrf::kPerformGreatPowerAttack)) {
		hr -= 4;
	}
	if (ch->IsFlagged(EPrf::kPerformAimingAttack)) {
		hr += 2;
	}
	if (ch->IsFlagged(EPrf::kPerformGreatAimingAttack)) {
		hr += 4;
	}
	hr -= (ch->IsOnHorse() ? (10 - ch->GetSkill(ESkill::kRiding) / 20) : 0);
	hr *= ch->get_cond_penalty(P_HITROLL);
	return hr;
}

const char *GetPositionStr(CharData *ch) {
	switch (ch->GetPosition()) {
		case EPosition::kDead:
			return "Вы МЕРТВЫ!\r\n";
		case EPosition::kPerish:
			return "Вы смертельно ранены и нуждаетесь в помощи!\r\n";
		case EPosition::kIncap:
			return "Вы без сознания и медленно умираете...\r\n";
		case EPosition::kStun:
			return "Вы в обмороке!\r\n";
		case EPosition::kSleep:
			return "Вы спите.\r\n";
		case EPosition::kRest:
			return "Вы отдыхаете.\r\n";
		case EPosition::kSit:
			return "Вы сидите.\r\n";
		case EPosition::kFight:
			if (ch->GetEnemy()) {
				sprintf(buf1, "Вы сражаетесь с %s.\r\n", GET_PAD(ch->GetEnemy(), 4));
				return buf1;
			}
			else
				return "Вы машете кулаками по воздуху.\r\n";
		case EPosition::kStand:
			return "Вы стоите.\r\n";
		default:
			break;
	}
	return "Вы незнамо что делаете!!!\r\n";
}

const char *GetShortPositionStr(CharData *ch) {
	if (!ch->IsOnHorse()) {
		switch (ch->GetPosition()) {
			case EPosition::kDead: return "Вы МЕРТВЫ!";
			case EPosition::kPerish: return "Вы умираете!";
			case EPosition::kIncap: return "Вы без сознания.";
			case EPosition::kStun: return "Вы в обмороке!";
			case EPosition::kSleep: return "Вы спите.";
			case EPosition::kRest: return "Вы отдыхаете.";
			case EPosition::kSit: return "Вы сидите.";
			case EPosition::kFight:
				if (ch->GetEnemy()) {
					return "Вы сражаетесь!";
				} else {
					return "Вы машете кулаками.";
				}
			case EPosition::kStand: return "Вы стоите.";
			default: return "You are void...";
		}
	} else {
		return "Вы сидите верхом.";
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
