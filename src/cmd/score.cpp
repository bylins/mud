/*
 \authors Created by Sventovit
 \date 26.02.2022.
 \brief Команды "счет"
 \details Команды "счет" - основная статистика персонажа игрока. Включает краткий счет, счет все и вариант для
 слабовидящих.
 */

#include "act_other.h"
#include "color.h"
#include "communication/mail.h"
#include "communication/parcel.h"
#include "entities/char_data.h"
#include "entities/char_player.h"
#include "entities/player_races.h"
#include "fightsystem/fight_hit.h"
#include "game_classes/classes.h"
#include "game_mechanics/bonus.h"
#include "game_mechanics/glory.h"
#include "game_mechanics/glory_const.h"
#include "structs/global_objects.h"
//#include "utils/table_wrapper.h"

void PrintScoreBase(CharData *ch);
void PrintScoreList(CharData *ch);
void PrintScoreAll(CharData *ch);
const char *GetPositionStr(CharData *ch);
const char *GetShortPositionStr(CharData *ch);
int CalcHitroll(CharData *ch);

/* extern */
int CalcAntiSavings(CharData *ch);
int calc_initiative(CharData *ch, bool mode);
TimeInfoData *real_time_passed(time_t t2, time_t t1);
void apply_weapon_bonus(int ch_class, ESkill skill, int *damroll, int *hitroll);

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

	if (IS_NPC(ch))
		return;

	if (utils::IsAbbrev(argument, "все") || utils::IsAbbrev(argument, "all")) {
		PrintScoreAll(ch);
		return;
	} else if (utils::IsAbbrev(argument, "список") || utils::IsAbbrev(argument, "list")) {
		PrintScoreList(ch);
		return;
	} else {
		PrintScoreBase(ch);
	}
}

// \todo Переписать на вывод в поток с использованием общих со "счет все" функций
void PrintScoreList(CharData *ch) {
	sprintf(buf, "%s", PlayerRace::GetKinNameByNum(GET_KIN(ch), GET_SEX(ch)).c_str());
	buf[0] = LOWER(buf[0]);
	sprintf(buf1, "%s", religion_name[GET_RELIGION(ch)][static_cast<int>(GET_SEX(ch))]);
	buf1[0] = LOWER(buf1[0]);
	send_to_char(ch, "Вы %s, %s, %s, %s, уровень %d, перевоплощений %d.\r\n", ch->get_name().c_str(),
				 buf,
				 MUD::Classes()[ch->get_class()].GetCName(),
				 buf1,
				 GetRealLevel(ch),
				 GET_REAL_REMORT(ch));
	send_to_char(ch, "Ваш возраст: %d, размер: %d(%d), рост: %d(%d), вес %d(%d).\r\n",
				 GET_AGE(ch),
				 GET_SIZE(ch), GET_REAL_SIZE(ch),
				 GET_HEIGHT(ch), GET_REAL_HEIGHT(ch),
				 GET_WEIGHT(ch), GET_REAL_WEIGHT(ch));
	send_to_char(ch, "Вы можете выдержать %d(%d) %s повреждений, и пройти %d(%d) %s по ровной местности.\r\n",
				 GET_HIT(ch), GET_REAL_MAX_HIT(ch), desc_count(GET_HIT(ch),  WHAT_ONEu),
				 GET_MOVE(ch), GET_REAL_MAX_MOVE(ch), desc_count(GET_MOVE(ch), WHAT_MOVEu));
	if (IS_MANA_CASTER(ch)) {
		send_to_char(ch, "Ваша магическая энергия %d(%d) и вы восстанавливаете %d в сек.\r\n",
					 GET_MANA_STORED(ch), GET_MAX_MANA(ch), mana_gain(ch));
	}
	send_to_char(ch, "Ваша сила: %d(%d), ловкость: %d(%d), телосложение: %d(%d), ум: %d(%d), мудрость: %d(%d), обаяние: %d(%d).\r\n",
				 ch->get_str(), GET_REAL_STR(ch),
				 ch->get_dex(), GET_REAL_DEX(ch),
				 ch->get_con(), GET_REAL_CON(ch),
				 ch->get_int(), GET_REAL_INT(ch),
				 ch->get_wis(), GET_REAL_WIS(ch),
				 ch->get_cha(), GET_REAL_CHA(ch));

	HitData hit_params;
	hit_params.weapon = fight::kMainHand;
	hit_params.init(ch, ch);
	bool need_dice = false;
	int max_dam = hit_params.calc_damage(ch, need_dice); // без кубиков

	send_to_char(ch, "Попадание: %d, повреждение: %d, запоминание: %d, успех колдовства: %d, удача: %d, маг.урон: %d, физ. урон: %d.\r\n",
				CalcHitroll(ch),
				max_dam,
				int (GET_MANAREG(ch) * ch->get_cond_penalty(P_CAST)),
				CalcAntiSavings(ch),
				ch->calc_morale(),
				ch->add_abils.percent_magdam_add + ch->obj_bonus().calc_mage_dmg(100),
				ch->add_abils.percent_physdam_add + ch->obj_bonus().calc_phys_dmg(100));
	send_to_char(ch, "Сопротивление: огню: %d, воздуху: %d, воде: %d, земле: %d, тьме: %d, живучесть: %d, разум: %d, иммунитет: %d.\r\n",
				 MIN(GET_RESIST(ch, FIRE_RESISTANCE), 75),
				 MIN(GET_RESIST(ch, AIR_RESISTANCE), 75),
				 MIN(GET_RESIST(ch, WATER_RESISTANCE), 75),
				 MIN(GET_RESIST(ch, EARTH_RESISTANCE), 75),
				 MIN(GET_RESIST(ch, DARK_RESISTANCE), 75),
				 MIN(GET_RESIST(ch, VITALITY_RESISTANCE), 75),
				 MIN(GET_RESIST(ch, MIND_RESISTANCE), 75),
				 MIN(GET_RESIST(ch, IMMUNITY_RESISTANCE), 75));
	send_to_char(ch, "Спас броски: воля: %d, здоровье: %d, стойкость: %d, реакция: %d, маг.резист: %d, физ.резист %d, отчар.резист: %d.\r\n",
				 GET_REAL_SAVING_WILL(ch),
				 GET_REAL_SAVING_CRITICAL(ch),
				 GET_REAL_SAVING_STABILITY(ch),
				 GET_REAL_SAVING_REFLEX(ch),
				 GET_MR(ch),
				 GET_PR(ch),
				 GET_AR(ch));
	send_to_char(ch, "Восстановление: жизни: +%d%% (+%d), сил: +%d%% (+%d).\r\n",
				 GET_HITREG(ch),
				 hit_gain(ch),
				 GET_MOVEREG(ch),
				 move_gain(ch));
	int ac = compute_armor_class(ch) / 10;
	if (ac < 5) {
		const int mod = (1 - ch->get_cond_penalty(P_AC)) * 40;
		ac = ac + mod > 5 ? 5 : ac + mod;
	}
	send_to_char(ch, "Броня: %d, защита: %d, поглощение %d.\r\n",
				 GET_ARMOUR(ch),
				 ac,
				 GET_ABSORBE(ch));
	send_to_char(ch, "Вы имеете кун: на руках: %ld, на счету %ld. Гривны: %d, опыт: %ld, ДСУ: %ld.\r\n",
				 ch->get_gold(),
				 ch->get_bank(),
				 ch->get_hryvn(),
				 GET_EXP(ch),
				 IS_IMMORTAL(ch) ? 1: level_exp(ch, GetRealLevel(ch) + 1) - GET_EXP(ch));
	if (!ch->ahorse())
		send_to_char(ch, "Ваша позиция: %s", GetPositionStr(ch));
	else
		send_to_char(ch, "Ваша позиция: Вы верхом на %s.\r\n", GET_PAD(ch->get_horse(), 5));
	if (PRF_FLAGGED(ch, PRF_SUMMONABLE))
		send_to_char(ch, "Вы можете быть призваны.\r\n");
	else
		send_to_char(ch, "Вы защищены от призыва.\r\n");
	send_to_char(ch, "Голоден: %s, жажда: %s.\r\n", (GET_COND(ch, FULL) > NORM_COND_VALUE)? "да" : "нет", GET_COND_M(ch, THIRST)? "да" : "нет");
	//Напоминаем о метке, если она есть.
	RoomData *label_room = room_spells::FindAffectedRoom(GET_ID(ch), kSpellRuneLabel);
	if (label_room) {
		const int timer_room_label = room_spells::GetUniqueAffectDuration(GET_ID(ch), kSpellRuneLabel);
		if (timer_room_label > 0) {
			*buf2 = '\0';
			(timer_room_label + 1) / SECS_PER_MUD_HOUR ? sprintf(buf2, "%d %s.", (timer_room_label + 1) / SECS_PER_MUD_HOUR + 1,
																 desc_count((timer_room_label + 1) / SECS_PER_MUD_HOUR + 1, WHAT_HOUR)) : sprintf(buf2, "менее часа.");
			send_to_char(ch, "Вы поставили рунную метку в комнате: '%s', она продержится еще %s\r\n", label_room->name, buf2);
			*buf2 = '\0';
		}
	}
	if (!NAME_GOD(ch) && GetRealLevel(ch) <= NAME_LEVEL) {
		send_to_char(ch, "ВНИМАНИЕ! ваше имя не одобрил никто из богов!\r\n");
		send_to_char(ch, "Cкоро вы прекратите получать опыт, обратитесь к богам для одобрения имени.\r\n");
	} else if (NAME_BAD(ch)) {
		send_to_char(ch, "ВНИМАНИЕ! ваше имя запрещено богами. Очень скоро вы прекратите получать опыт.\r\n");
	}
	send_to_char(ch, "Вы можете вступить в группу с максимальной разницей в %2d %-75s\r\n",
				 grouping[static_cast<int>(GET_CLASS(ch))][static_cast<int>(GET_REAL_REMORT(ch))],
				 (std::string(desc_count(grouping[static_cast<int>(GET_CLASS(ch))][static_cast<int>(GET_REAL_REMORT(ch))],  WHAT_LEVEL)
							 + std::string(" без потерь для опыта.")).substr(0, 76).c_str()));

	send_to_char(ch,"Вы можете принять в группу максимум %d соратников.\r\n", max_group_size(ch));
}

/*
 *  Функции для "счет все".
 */

const std::string &InfoStrPrefix(CharData *ch) {
	static const std::string cyan_star{KICYN " * " KNRM};
	static const std::string space_str{" "};
	if (PRF_FLAGGED(ch, PRF_BLIND)) {
		return space_str;
	} else {
		return cyan_star;
	}
}

void PrintHorseInfo(CharData *ch, std::ostringstream &out) {
	if (ch->has_horse(false)) {
		if (ch->ahorse()) {
			out << InfoStrPrefix(ch) << "Вы верхом на " << GET_PAD(ch->get_horse(), 5) << "." << std::endl;
		} else {
			out << InfoStrPrefix(ch) << "У вас есть " << ch->get_horse()->get_name() << "." << std::endl;
		}
	}
}

void PrintRuneLabelInfo(CharData *ch, std::ostringstream &out) {
	RoomData *label_room = room_spells::FindAffectedRoom(GET_ID(ch), kSpellRuneLabel);
	if (label_room) {
		int timer_room_label = room_spells::GetUniqueAffectDuration(GET_ID(ch), kSpellRuneLabel);
		out << InfoStrPrefix(ch) << KIGRN << "Вы поставили рунную метку в комнате \'"
			<< label_room->name << "\' ";
		if (timer_room_label > 0) {
			out << "[продержится еще ";
			timer_room_label = (timer_room_label + 1) / SECS_PER_MUD_HOUR + 1;
			if (timer_room_label > 0) {
				out << timer_room_label << " " << desc_count(timer_room_label, WHAT_HOUR) << "]." << std::endl;
			} else {
				out << "менее часа]." << std::endl;
			}
		}
		out << std::endl;
	}
}

void PrintGloryInfo(CharData *ch, std::ostringstream &out) {
	auto glory = GloryConst::get_glory(GET_UNIQUE(ch));
	if (glory > 0) {
		out << InfoStrPrefix(ch) << "Вы заслужили "
			<< glory << " " << desc_count(glory, WHAT_POINT) << " постоянной славы." << std::endl;
	}
}

void PrintNameStatusInfo(CharData *ch, std::ostringstream &out) {
	if (!NAME_GOD(ch) && GetRealLevel(ch) <= NAME_LEVEL) {
		out << InfoStrPrefix(ch) << KIRED << "ВНИМАНИЕ! " << KNRM
			<< "ваше имя не одобрил никто из богов!" << std::endl;
		out << InfoStrPrefix(ch) << KIRED << "ВНИМАНИЕ! " << KNRM
			<< "Cкоро вы прекратите получать опыт, обратитесь к богам для одобрения имени." << std::endl;
	} else if (NAME_BAD(ch)) {
		out << InfoStrPrefix(ch) << KIRED << "ВНИМАНИЕ! " << KNRM
			<< "Ваше имя запрещено богами. Вы не можете получать опыт." << std::endl;
	}
}

void PrintSummonableInfo(CharData *ch, std::ostringstream &out) {
	if (PRF_FLAGGED(ch, PRF_SUMMONABLE)) {
		out << InfoStrPrefix(ch) << KIYEL << "Вы можете быть призваны." << KNRM << std::endl;
	} else {
		out << InfoStrPrefix(ch) << "Вы защищены от призыва." << std::endl;
	}
}

void PrintBonusStateInfo(CharData *ch, std::ostringstream &out) {
	if (Bonus::is_bonus_active()) {
		out << InfoStrPrefix(ch) << Bonus::active_bonus_as_string() << " "
			<< Bonus::time_to_bonus_end_as_string() << std::endl;
	}
}

void PrintExpTaxInfo(CharData *ch, std::ostringstream &out) {
	if (GET_GOD_FLAG(ch, GF_REMORT) && CLAN(ch)) {
		out << InfoStrPrefix(ch) << "Вы самоотверженно отдаете весь получаемый опыт своей дружине." << std::endl;
	}
}

void PrintBlindModeInfo(CharData *ch, std::ostringstream &out) {
	if (PRF_FLAGGED(ch, PRF_BLIND)) {
		out << InfoStrPrefix(ch) << "Режим слепого игрока включен." << std::endl;
	}
}

void PrintGroupMembershipInfo(CharData *ch, std::ostringstream &out) {
	if (GetRealLevel(ch) < kLvlImmortal) {
		out << InfoStrPrefix(ch) << "Вы можете вступить в группу с максимальной разницей "
			<< grouping[static_cast<int>(GET_CLASS(ch))][static_cast<int>(GET_REAL_REMORT(ch))] << " "
			<< desc_count(grouping[static_cast<int>(GET_CLASS(ch))][static_cast<int>(GET_REAL_REMORT(ch))], WHAT_LEVEL)
			<< " без потерь для опыта." << std::endl;

		out << InfoStrPrefix(ch) << "Вы можете принять в группу максимум "
			<< max_group_size(ch) << " соратников." << std::endl;
	}
}

void PrintRentableInfo(CharData *ch, std::ostringstream &out) {
	if (NORENTABLE(ch)) {
		const time_t rent_time = NORENTABLE(ch) - time(nullptr);
		const auto minutes = rent_time > 60 ? rent_time / 60 : 0;
		out << InfoStrPrefix(ch) << KIRED << "В связи с боевыми действиями вы не можете уйти на постой еще ";
		if (minutes) {
			out << minutes << " " << desc_count(minutes, WHAT_MINu) << "." << KNRM << std::endl;
		} else {
			out << rent_time << " " << desc_count(rent_time, WHAT_SEC) << "." << KNRM << std::endl;
		}
	} else if ((ch->in_room != kNowhere) && ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL) && !PLR_FLAGGED(ch, PLR_KILLER)) {
		out << InfoStrPrefix(ch) << KIGRN << "Тут вы чувствуете себя в безопасности." << KNRM << std::endl;
	}
}
// \todo Сделать авторазмещение в комнате-кузнице горна и убрать эту функцию.
void PrinForgeInfo(CharData *ch, std::ostringstream &out) {
	if (ROOM_FLAGGED(ch->in_room, ROOM_SMITH)
		&& (ch->get_skill(ESkill::kJewelry)
		|| ch->get_skill(ESkill::kRepair)
		|| ch->get_skill(ESkill::kReforging))) {
		out << InfoStrPrefix(ch) << KIYEL << "Это место отлично подходит для занятий кузнечным делом."
			<< KNRM << std::endl;
	}
}

void PrintPostInfo(CharData *ch, std::ostringstream &out) {
	if (mail::has_mail(ch->get_uid())) {
		out << InfoStrPrefix(ch) << KIGRN << "Вас ожидает новое письмо, зайдите на почту." << KNRM << std::endl;
	}
	if (Parcel::has_parcel(ch)) {
		out << InfoStrPrefix(ch) << KIGRN << "Вас ожидает посылка, зайдите на почту." << KNRM << std::endl;
	}
}

void PrintProtectInfo(CharData *ch, std::ostringstream &out) {
	if (ch->get_protecting()) {
		out << InfoStrPrefix(ch) << "Вы прикрываете " << GET_PAD(ch->get_protecting(), 3)
			<< " от нападения." << std::endl;
	}
}

//	*** Функции для распечатки информации о наказаниях ***

struct ScorePunishmentInfo {
	ScorePunishmentInfo() = delete;
	explicit ScorePunishmentInfo(CharData *ch)
		: ch{ch} {};

	void SetFInfo(Bitvector punish_flag, const Punish *punish_info) {
		flag = punish_flag;
		punish = punish_info;
	};

	CharData *ch{};
	const Punish *punish{};
	std::string msg;
	Bitvector flag{};
};

void PrintSinglePunishmentInfo(const ScorePunishmentInfo &info, std::ostringstream &out) {
	const auto current_time = time(nullptr);
	const auto hrs = (info.punish->duration - current_time)/3600;
	const auto mins = ((info.punish->duration - current_time)%3600 + 59)/60;

	out << InfoStrPrefix(info.ch) << KIRED << info.msg
		<< hrs <<  " " << desc_count(hrs, WHAT_HOUR) <<  " "
		<< mins <<  " " << desc_count(mins, WHAT_MINu);

	if (info.punish->reason != nullptr) {
		if (info.punish->reason[0] != '\0' && str_cmp(info.punish->reason, "(null)") != 0) {
			out << " [" << info.punish->reason << "]";
		}
	}
	out << "." << KNRM  << std::endl;
}

// \todo Место этого - в структурах, которые работают с наказаниями персонажа. Куда их и следует перенести.
bool IsPunished(const ScorePunishmentInfo &info) {
	return (PLR_FLAGGED(info.ch, info.flag)
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

	punish_info.SetFInfo(PLR_HELLED, &ch->player_specials->phell);
	if (IsPunished(punish_info)) {
		punish_info.msg = "Вам предстоит провести в темнице еще ";
		PrintSinglePunishmentInfo(punish_info, out);
	}

	punish_info.SetFInfo(PLR_FROZEN, &ch->player_specials->pfreeze);
	if (IsPunished(punish_info)) {
		punish_info.msg = "Вы будете заморожены еще ";
		PrintSinglePunishmentInfo(punish_info, out);
	}

	punish_info.SetFInfo(PLR_MUTE, &ch->player_specials->pmute);
	if (IsPunished(punish_info)) {
		punish_info.msg = "Вы не сможете кричать еще ";
		PrintSinglePunishmentInfo(punish_info, out);
	}

	punish_info.SetFInfo(PLR_DUMB, &ch->player_specials->pdumb);
	if (IsPunished(punish_info)) {
		punish_info.msg = "Вы будете молчать еще ";
		PrintSinglePunishmentInfo(punish_info, out);
	}

	/*
 	* Это приходится делать вручную, потому что флаги проклятия/любимца богов реализованы не через систему
 	* наказаний, а отдельными флагами. По-хорошему, надо их привести к общему знаменателю (тем более, что счесть
 	* избранность богами наказанием - в некотором смысле символично).
 	*/
	if (GET_GOD_FLAG(ch, GF_GODSCURSE) && GCURSE_DURATION(ch)) {
		punish_info.punish = &ch->player_specials->pgcurse;
		punish_info.msg = "Вы прокляты Богами на ";
		PrintSinglePunishmentInfo(punish_info, out);
	}

	/*
	 * Схоже с предыдущим пунктом: флаг "зарегистрирован" имеет противоположный остальным флагам наказаний смысл:
	 * персонаж НЕ наказан. Было бы разумнее ставить новым персонажам по умолчанию флаг unreg и снимать его при
	 * регистрации, но увы.
	 */
	if (!PLR_FLAGGED(ch, PLR_REGISTERED) && UNREG_DURATION(ch) != 0 && UNREG_DURATION(ch) > time(nullptr)) {
		punish_info.punish = &ch->player_specials->punreg;
		punish_info.msg = "Вы не сможете входить с одного IP еще ";
		PrintSinglePunishmentInfo(punish_info, out);
	}
}

void PrintMorphInfo(CharData *ch, std::ostringstream &out) {
	if (ch->is_morphed()) {
		out << InfoStrPrefix(ch) << KIYEL << "Вы находитесь в звериной форме - "
		<< ch->get_morph_desc() << "." << KNRM << std::endl;
	}
}

/**
 * Вывести в таблицу основную информацию персонажа, начиная с указанного столбца.
 * @param ch  - персонаж, для которого выводится статистика.
 * @param table - таблица, в которую поступают сведения.
 * @param col - номер столбца.
 * @return - количество заполненных функцией столбцов.
 */
int PrintBaseInfoToTable(CharData *ch, fort::char_table &table, std::size_t col) {
	std::size_t row{0};
	table[row][col] = std::string("Племя: ") + PlayerRace::GetRaceNameByNum(GET_KIN(ch), GET_RACE(ch), GET_SEX(ch));
	table[++row][col] = std::string("Вера: ") + religion_name[GET_RELIGION(ch)][static_cast<int>(GET_SEX(ch))];
	table[++row][col] = std::string("Уровень: ") + std::to_string(GetRealLevel(ch));
	table[++row][col] = std::string("Перевоплощений: ") + std::to_string(GET_REAL_REMORT(ch));
	table[++row][col] = std::string("Возраст: ") + std::to_string(GET_AGE(ch));
	if (ch->get_level() < kLvlImmortal) {
		table[++row][col] = std::string("Опыт: ") + PrintNumberByDigits(GET_EXP(ch));
		table[++row][col] = std::string("ДСУ: ") + PrintNumberByDigits(level_exp(ch, GetRealLevel(ch) + 1) - GET_EXP(ch));
	}
	table[++row][col] = std::string("Кун: ") + PrintNumberByDigits(ch->get_gold());
	table[++row][col] = std::string("На счету: ") + PrintNumberByDigits(ch->get_bank());
	table[++row][col] = GetShortPositionStr(ch);
	table[++row][col] = std::string("Голоден: ") + (GET_COND(ch, FULL) > NORM_COND_VALUE ? "Угу :(" : "Нет");
	table[++row][col] = std::string("Жажда: ") + (GET_COND_M(ch, THIRST) ? "Наливай!" : "Нет");
	if (GET_COND(ch, DRUNK) >= CHAR_DRUNKED) {
		table[++row][col] = (affected_by_spell(ch, kSpellAbstinent) ? "Похмелье." : "Вы пьяны.");
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
int PrintBaseStatsToTable(CharData *ch, fort::char_table &table, std::size_t col) {
	std::size_t row{0};
	table[row][col] = "Сила";			table[row][col + 1] = std::to_string(ch->get_str()) + " (" + std::to_string(GET_REAL_STR(ch)) + ")";
	table[++row][col] = "Ловкость";		table[row][col + 1] = std::to_string(ch->get_dex()) + " (" + std::to_string(GET_REAL_DEX(ch)) + ")";
	table[++row][col] = "Телосложение";	table[row][col + 1] = std::to_string(ch->get_con()) + " (" + std::to_string(GET_REAL_CON(ch)) + ")";
	table[++row][col] = "Мудрость";		table[row][col + 1] = std::to_string(ch->get_wis()) + " (" + std::to_string(GET_REAL_WIS(ch)) + ")";
	table[++row][col] = "Интеллект";	table[row][col + 1] = std::to_string(ch->get_int()) + " (" + std::to_string(GET_REAL_INT(ch)) + ")";
	table[++row][col] = "Обаяние";		table[row][col + 1] = std::to_string(ch->get_cha()) + " (" + std::to_string(GET_REAL_CHA(ch)) + ")";
	table[++row][col] = "Рост";			table[row][col + 1] = std::to_string(GET_HEIGHT(ch)) + " (" + std::to_string(GET_REAL_HEIGHT(ch)) + ")";
	table[++row][col] = "Вес";			table[row][col + 1] = std::to_string(GET_WEIGHT(ch)) + " (" + std::to_string(GET_REAL_WEIGHT(ch)) + ")";
	table[++row][col] = "Размер";		table[row][col + 1] = std::to_string(GET_SIZE(ch)) + " (" + std::to_string(GET_REAL_SIZE(ch)) + ")";
	table[++row][col] = "Жизнь";		table[row][col + 1] = std::to_string(GET_HIT(ch)) + "(" + std::to_string(GET_REAL_MAX_HIT(ch)) + ")";
	table[++row][col] = "Восст. жизни";	table[row][col + 1] = "+" + std::to_string(GET_HITREG(ch)) + "% (" + std::to_string(hit_gain(ch)) + ")";
	table[++row][col] = "Выносливость";	table[row][col + 1] = std::to_string(GET_MOVE(ch)) + "(" + std::to_string(GET_REAL_MAX_MOVE(ch)) + ")";
	table[++row][col] = "Восст. сил";	table[row][col + 1] = "+" + std::to_string(GET_MOVEREG(ch)) + "% (" + std::to_string(move_gain(ch)) + ")";
	if (IS_MANA_CASTER(ch)) {
		table[++row][col] = "Мана"; 		table[row][col + 1] = std::to_string(GET_MANA_STORED(ch)) + "(" + std::to_string(GET_MAX_MANA(ch)) + ")";
		table[++row][col] = "Восст. маны";	table[row][col + 1] = "+" + std::to_string(mana_gain(ch)) + " сек.";
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
int PrintSecondaryStatsToTable(CharData *ch, fort::char_table &table, std::size_t col) {
	HitData hit_params;
	hit_params.weapon = fight::kMainHand;
	hit_params.init(ch, ch);
	auto max_dam = hit_params.calc_damage(ch, false);

	std::size_t row{0};
	table[row][col] = "Атака";			table[row][col + 1] = std::to_string(CalcHitroll(ch));
	table[++row][col] = "Урон";			table[row][col + 1] = std::to_string(max_dam);
	table[++row][col] = "Колдовство";	table[row][col + 1] = std::to_string(CalcAntiSavings(ch));
	table[++row][col] = "Запоминание";	table[row][col + 1] = std::to_string(std::lround(GET_MANAREG(ch)*ch->get_cond_penalty(P_CAST)));
	table[++row][col] = "Удача";		table[row][col + 1] = std::to_string(ch->calc_morale());
	table[++row][col] = "Маг. урон %";		table[row][col + 1] = std::to_string(ch->add_abils.percent_magdam_add + ch->obj_bonus().calc_mage_dmg(100));
	table[++row][col] = "Физ. урон %";	table[row][col + 1] = std::to_string(ch->add_abils.percent_physdam_add + ch->obj_bonus().calc_phys_dmg(100));
	table[++row][col] = "Инициатива";	table[row][col + 1] = std::to_string(calc_initiative(ch, false));
	table[++row][col] = "Спас-броски:";	table[row][col + 1] = " ";
	table[++row][col] = "Воля";			table[row][col + 1] = std::to_string(GET_REAL_SAVING_WILL(ch));
	table[++row][col] = "Здоровье";		table[row][col + 1] = std::to_string(GET_REAL_SAVING_CRITICAL(ch));
	table[++row][col] = "Стойкость";	table[row][col + 1] = std::to_string(GET_REAL_SAVING_STABILITY(ch));
	table[++row][col] = "Реакция";		table[row][col + 1] = std::to_string(GET_REAL_SAVING_REFLEX(ch));

	return 2; // заполнено столбцов
}

/**
 * Вывести в таблицу защитные параметры персонажа, начиная с указанного столбца таблицы.
 * @param ch  - персонаж, для которого выводится статистика.
 * @param table - таблица, в которую поступают сведения.
 * @param col - номер столбца.
 * @return - количество заполненных функцией столбцов.
 */
int PrintProtectiveStatsToTable(CharData *ch, fort::char_table &table, std::size_t col) {
	std::size_t row{0};
	int ac = compute_armor_class(ch) / 10;
	if (ac < 5) {
		const int mod = (1 - ch->get_cond_penalty(P_AC)) * 40;
		ac = ac + mod > 5 ? 5 : ac + mod;
	}

	table[row][col] = "Броня";				table[row][col + 1] = std::to_string(GET_ARMOUR(ch));
	table[++row][col] = "Защита";			table[row][col + 1] = std::to_string(ac);
	table[++row][col] = "Поглощение";		table[row][col + 1] = std::to_string(GET_ABSORBE(ch));
	table[++row][col] = "Сопротивления: ";	table[row][col + 1] = " ";
	table[++row][col] = "Урону";			table[row][col + 1] = std::to_string(GET_MR(ch));
	table[++row][col] = "Заклинаниям";		table[row][col + 1] = std::to_string(GET_PR(ch));
	table[++row][col] = "Магии огня";		table[row][col + 1] = std::to_string(std::min(GET_RESIST(ch, FIRE_RESISTANCE), kMaxPlayerResist));
	table[++row][col] = "Магии воды";		table[row][col + 1] = std::to_string(std::min(GET_RESIST(ch, WATER_RESISTANCE), kMaxPlayerResist));
	table[++row][col] = "Магии земли";		table[row][col + 1] = std::to_string(std::min(GET_RESIST(ch, EARTH_RESISTANCE), kMaxPlayerResist));
	table[++row][col] = "Магии воздуха";	table[row][col + 1] = std::to_string(std::min(GET_RESIST(ch, AIR_RESISTANCE), kMaxPlayerResist));
	table[++row][col] = "Магии тьмы";		table[row][col + 1] = std::to_string(std::min(GET_RESIST(ch, DARK_RESISTANCE), kMaxPlayerResist));
	table[++row][col] = "Магии разума";		table[row][col + 1] = std::to_string(std::min(GET_RESIST(ch, MIND_RESISTANCE), kMaxPlayerResist));
	table[++row][col] = "Тяжелым ранам";	table[row][col + 1] = std::to_string(std::min(GET_RESIST(ch, VITALITY_RESISTANCE), kMaxPlayerResist));
	table[++row][col] = "Ядам и болезням";	table[row][col + 1] = std::to_string(std::min(GET_RESIST(ch, IMMUNITY_RESISTANCE), kMaxPlayerResist));

	return 2; // заполнено столбцов
}

void PrintSelfHitrollInfo(CharData *ch, std::ostringstream &out) {
	HitData hit;
	hit.weapon = fight::AttackType::kMainHand;
	hit.init(ch, ch);
	hit.calc_base_hr(ch);
	hit.calc_stat_hr(ch);
	hit.calc_ac(ch);

	HitData hit2;
	hit2.weapon = fight::AttackType::kOffHand;
	hit2.init(ch, ch);
	hit2.calc_base_hr(ch);
	hit2.calc_stat_hr(ch);

	out << InfoStrPrefix(ch) << KICYN
		<< "RIGHT_WEAPON: hitroll=" << -hit.calc_thaco
		<< ", LEFT_WEAPON: hitroll=" << hit2.calc_thaco
		<< ", AC=" << hit.victim_ac << "." << KNRM << std::endl;
}

void PrintTesterModeInfo(CharData *ch, std::ostringstream &out) {
	if (PRF_FLAGGED(ch, PRF_TESTER)) {
		out << InfoStrPrefix(ch) << KICYN << "Включен режим тестирования." << KNRM << std::endl;
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
	out << "  Вы " << ch->get_name() << ", " << MUD::Classes()[ch->get_class()].GetName() << ". Ваши показатели:" << std::endl;

	// Заполняем основную таблицу и выводим в поток
	fort::char_table table;
	std::size_t current_column{0};
	current_column += PrintBaseInfoToTable(ch, table, current_column);
	current_column += PrintBaseStatsToTable(ch, table, current_column);
	current_column += PrintSecondaryStatsToTable(ch, table, current_column);
	PrintProtectiveStatsToTable(ch, table, current_column);
	table_wrapper::DecorateCuteTable(ch, table);
	table_wrapper::PrintTableToStream(out, table);

	PrintAdditionalInfo(ch, out);

	send_to_char(out.str(), ch);
}

// \todo Переписать на вывод в поток с использованием общих со "счет все" функций
void PrintScoreBase(CharData *ch) {
	std::ostringstream out;

	out << "Вы " << ch->only_title() << " ("
		<< PlayerRace::GetKinNameByNum(GET_KIN(ch), GET_SEX(ch)) << ", "
		<< PlayerRace::GetRaceNameByNum(GET_KIN(ch), GET_RACE(ch), GET_SEX(ch)) << ", "
		<< religion_name[GET_RELIGION(ch)][static_cast<int>(GET_SEX(ch))] << ", "
		<< MUD::Classes()[ch->get_class()].GetCName() <<  " "
		<< GetRealLevel(ch) << " уровня)." << std::endl;

	PrintNameStatusInfo(ch, out);

	out << "Сейчас вам "  <<   GET_REAL_AGE(ch) << " " << desc_count(GET_REAL_AGE(ch), WHAT_YEAR) << ".";
	if (age(ch)->month == 0 && age(ch)->day == 0) {
		out << KIRED << " У вас сегодня День Варенья!" << KNRM;
	}
	out << std::endl;

	send_to_char(out.str(), ch);
	// Продолжить с этого места

	sprintf(buf,
			"Вы можете выдержать %d(%d) %s повреждения, и пройти %d(%d) %s по ровной местности.\r\n",
			GET_HIT(ch), GET_REAL_MAX_HIT(ch), desc_count(GET_HIT(ch),
														  WHAT_ONEu),
			GET_MOVE(ch), GET_REAL_MAX_MOVE(ch), desc_count(GET_MOVE(ch), WHAT_MOVEu));

	if (IS_MANA_CASTER(ch)) {
		sprintf(buf + strlen(buf),
				"Ваша магическая энергия %d(%d) и вы восстанавливаете %d в сек.\r\n",
				GET_MANA_STORED(ch), GET_MAX_MANA(ch), mana_gain(ch));
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
			CCICYN(ch, C_NRM), ch->get_str(), GET_REAL_STR(ch),
			ch->get_dex(), GET_REAL_DEX(ch),
			ch->get_con(), GET_REAL_CON(ch),
			ch->get_wis(), GET_REAL_WIS(ch),
			ch->get_int(), GET_REAL_INT(ch),
			ch->get_cha(), GET_REAL_CHA(ch),
			GET_SIZE(ch), GET_REAL_SIZE(ch),
			GET_HEIGHT(ch), GET_REAL_HEIGHT(ch), GET_WEIGHT(ch), GET_REAL_WEIGHT(ch), CCNRM(ch, C_NRM));

	if (IS_IMMORTAL(ch)) {
		sprintf(buf + strlen(buf),
				"%sВаши боевые качества :\r\n"
				"  AC   : %4d(%4d)"
				"  DR   : %4d(%4d)%s\r\n",
				CCIGRN(ch, C_NRM), GET_AC(ch), compute_armor_class(ch),
				GET_DR(ch), GetRealDamroll(ch), CCNRM(ch, C_NRM));
	} else {
		int ac = compute_armor_class(ch) / 10;

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
	sprintf(buf + strlen(buf), "Ваш опыт - %ld %s. ", GET_EXP(ch), desc_count(GET_EXP(ch), WHAT_POINT));
	if (GetRealLevel(ch) < kLvlImmortal) {
		if (PRF_FLAGGED(ch, PRF_BLIND)) {
			sprintf(buf + strlen(buf), "\r\n");
		}
		sprintf(buf + strlen(buf),
				"Вам осталось набрать %ld %s до следующего уровня.\r\n",
				level_exp(ch, GetRealLevel(ch) + 1) - GET_EXP(ch),
				desc_count(level_exp(ch, GetRealLevel(ch) + 1) - GET_EXP(ch), WHAT_POINT));
	} else
		sprintf(buf + strlen(buf), "\r\n");

	sprintf(buf + strlen(buf),
			"У вас на руках %ld %s и %d %s",
			ch->get_gold(),
			desc_count(ch->get_gold(), WHAT_MONEYa),
			ch->get_hryvn(),
			desc_count(ch->get_hryvn(), WHAT_TORC));
	if (ch->get_bank() > 0)
		sprintf(buf + strlen(buf), " (и еще %ld %s припрятано в лежне).\r\n",
				ch->get_bank(), desc_count(ch->get_bank(), WHAT_MONEYa));
	else
		strcat(buf, ".\r\n");

	if (GetRealLevel(ch) < kLvlImmortal) {
		sprintf(buf + strlen(buf),
				"Вы можете вступить в группу с максимальной разницей в %d %s без потерь для опыта.\r\n",
				grouping[static_cast<int>(GET_CLASS(ch))][static_cast<int>(GET_REAL_REMORT(ch))],
				desc_count(grouping[static_cast<int>(GET_CLASS(ch))][static_cast<int>(GET_REAL_REMORT(ch))], WHAT_LEVEL));
	}

	//Напоминаем о метке, если она есть.
	RoomData *label_room = room_spells::FindAffectedRoom(GET_ID(ch), kSpellRuneLabel);
	if (label_room) {
		sprintf(buf + strlen(buf),
				"&G&qВы поставили рунную метку в комнате '%s'.&Q&n\r\n",
				std::string(label_room->name).c_str());
	}

	int glory = Glory::get_glory(GET_UNIQUE(ch));
	if (glory) {
		sprintf(buf + strlen(buf), "Вы заслужили %d %s славы.\r\n",
				glory, desc_count(glory, WHAT_POINT));
	}
	glory = GloryConst::get_glory(GET_UNIQUE(ch));
	if (glory) {
		sprintf(buf + strlen(buf), "Вы заслужили %d %s постоянной славы.\r\n",
				glory, desc_count(glory, WHAT_POINT));
	}

	TimeInfoData playing_time = *real_time_passed((time(nullptr) - ch->player_data.time.logon) + ch->player_data.time.played, 0);
	sprintf(buf + strlen(buf), "Вы играете %d %s %d %s реального времени.\r\n",
			playing_time.day, desc_count(playing_time.day, WHAT_DAY),
			playing_time.hours, desc_count(playing_time.hours, WHAT_HOUR));
	send_to_char(buf, ch);

	if (!ch->ahorse())
		send_to_char(ch, "%s", GetPositionStr(ch));

	strcpy(buf, CCIGRN(ch, C_NRM));
	const auto value_drunked = GET_COND(ch, DRUNK);
	if (value_drunked >= CHAR_DRUNKED) {
		if (affected_by_spell(ch, kSpellAbstinent))
			strcat(buf, "Привет с большого бодуна!\r\n");
		else {
			if (value_drunked >= CHAR_MORTALLY_DRUNKED)
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
	   strcat(buf, CCICYN(ch, C_NRM));
	   strcat(buf,"Аффекты :\r\n");
	   (ch)->char_specials.saved.affected_by.sprintbits(affected_bits, buf2, "\r\n");
	   strcat(buf,buf2);
	 */
	if (PRF_FLAGGED(ch, PRF_SUMMONABLE))
		strcat(buf, "Вы можете быть призваны.\r\n");

	if (ch->has_horse(false)) {
		if (ch->ahorse())
			sprintf(buf + strlen(buf), "Вы верхом на %s.\r\n", GET_PAD(ch->get_horse(), 5));
		else
			sprintf(buf + strlen(buf), "У вас есть %s.\r\n", GET_NAME(ch->get_horse()));
	}
	strcat(buf, CCNRM(ch, C_NRM));
	send_to_char(buf, ch);
	if (NORENTABLE(ch)) {
		sprintf(buf,
				"%sВ связи с боевыми действиями вы не можете уйти на постой.%s\r\n",
				CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
	} else if ((ch->in_room != kNowhere) && ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL) && !PLR_FLAGGED(ch, PLR_KILLER)) {
		sprintf(buf, "%sТут вы чувствуете себя в безопасности.%s\r\n", CCIGRN(ch, C_NRM), CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
	}

	if (ROOM_FLAGGED(ch->in_room, ROOM_SMITH)
		&& (ch->get_skill(ESkill::kJewelry) || ch->get_skill(ESkill::kRepair) || ch->get_skill(ESkill::kReforging))) {
		sprintf(buf,
				"%sЭто место отлично подходит для занятий кузнечным делом.%s\r\n",
				CCIGRN(ch, C_NRM),
				CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
	}

	if (mail::has_mail(ch->get_uid())) {
		sprintf(buf, "%sВас ожидает новое письмо, зайдите на почту!%s\r\n", CCIGRN(ch, C_NRM), CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
	}

	if (Parcel::has_parcel(ch)) {
		sprintf(buf, "%sВас ожидает посылка, зайдите на почту!%s\r\n", CCIGRN(ch, C_NRM), CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
	}

	if (PLR_FLAGGED(ch, PLR_HELLED) && HELL_DURATION(ch) && HELL_DURATION(ch) > time(nullptr)) {
		const int hrs = (HELL_DURATION(ch) - time(nullptr)) / 3600;
		const int mins = ((HELL_DURATION(ch) - time(nullptr)) % 3600 + 59) / 60;
		sprintf(buf,
				"Вам предстоит провести в темнице еще %d %s %d %s [%s].\r\n",
				hrs, desc_count(hrs, WHAT_HOUR), mins, desc_count(mins,
																  WHAT_MINu),
				HELL_REASON(ch) ? HELL_REASON(ch) : "-");
		send_to_char(buf, ch);
	}
	if (PLR_FLAGGED(ch, PLR_MUTE) && MUTE_DURATION(ch) != 0 && MUTE_DURATION(ch) > time(nullptr)) {
		const int hrs = (MUTE_DURATION(ch) - time(nullptr)) / 3600;
		const int mins = ((MUTE_DURATION(ch) - time(nullptr)) % 3600 + 59) / 60;
		sprintf(buf, "Вы не сможете кричать еще %d %s %d %s [%s].\r\n",
				hrs, desc_count(hrs, WHAT_HOUR),
				mins, desc_count(mins, WHAT_MINu), MUTE_REASON(ch) ? MUTE_REASON(ch) : "-");
		send_to_char(buf, ch);
	}
	if (PLR_FLAGGED(ch, PLR_DUMB) && DUMB_DURATION(ch) != 0 && DUMB_DURATION(ch) > time(nullptr)) {
		const int hrs = (DUMB_DURATION(ch) - time(nullptr)) / 3600;
		const int mins = ((DUMB_DURATION(ch) - time(nullptr)) % 3600 + 59) / 60;
		sprintf(buf, "Вы будете молчать еще %d %s %d %s [%s].\r\n",
				hrs, desc_count(hrs, WHAT_HOUR),
				mins, desc_count(mins, WHAT_MINu), DUMB_REASON(ch) ? DUMB_REASON(ch) : "-");
		send_to_char(buf, ch);
	}
	if (PLR_FLAGGED(ch, PLR_FROZEN) && FREEZE_DURATION(ch) != 0 && FREEZE_DURATION(ch) > time(nullptr)) {
		const int hrs = (FREEZE_DURATION(ch) - time(nullptr)) / 3600;
		const int mins = ((FREEZE_DURATION(ch) - time(nullptr)) % 3600 + 59) / 60;
		sprintf(buf, "Вы будете заморожены еще %d %s %d %s [%s].\r\n",
				hrs, desc_count(hrs, WHAT_HOUR),
				mins, desc_count(mins, WHAT_MINu), FREEZE_REASON(ch) ? FREEZE_REASON(ch) : "-");
		send_to_char(buf, ch);
	}

	if (!PLR_FLAGGED(ch, PLR_REGISTERED) && UNREG_DURATION(ch) != 0 && UNREG_DURATION(ch) > time(nullptr)) {
		const int hrs = (UNREG_DURATION(ch) - time(nullptr)) / 3600;
		const int mins = ((UNREG_DURATION(ch) - time(nullptr)) % 3600 + 59) / 60;
		sprintf(buf, "Вы не сможете заходить с одного IP еще %d %s %d %s [%s].\r\n",
				hrs, desc_count(hrs, WHAT_HOUR),
				mins, desc_count(mins, WHAT_MINu), UNREG_REASON(ch) ? UNREG_REASON(ch) : "-");
		send_to_char(buf, ch);
	}

	if (GET_GOD_FLAG(ch, GF_GODSCURSE) && GCURSE_DURATION(ch)) {
		const int hrs = (GCURSE_DURATION(ch) - time(nullptr)) / 3600;
		const int mins = ((GCURSE_DURATION(ch) - time(nullptr)) % 3600 + 59) / 60;
		sprintf(buf, "Вы прокляты Богами на %d %s %d %s.\r\n",
				hrs, desc_count(hrs, WHAT_HOUR), mins, desc_count(mins, WHAT_MINu));
		send_to_char(buf, ch);
	}

	if (ch->is_morphed()) {
		sprintf(buf, "Вы находитесь в звериной форме - %s.\r\n", ch->get_morph_desc().c_str());
		send_to_char(buf, ch);
	}
	if (can_use_feat(ch, COLLECTORSOULS_FEAT)) {
		const int souls = ch->get_souls();
		if (souls == 0) {
			sprintf(buf, "Вы не имеете чужих душ.\r\n");
			send_to_char(buf, ch);
		} else {
			if (souls == 1) {
				sprintf(buf, "Вы имеете всего одну душу в запасе.\r\n");
				send_to_char(buf, ch);
			}
			if (souls > 1 && souls < 5) {
				sprintf(buf, "Вы имеете %d души в запасе.\r\n", souls);
				send_to_char(buf, ch);
			}
			if (souls >= 5) {
				sprintf(buf, "Вы имеете %d чужих душ в запасе.\r\n", souls);
				send_to_char(buf, ch);
			}
		}
	}
	if (ch->get_ice_currency() > 0) {
		if (ch->get_ice_currency() == 1) {
			sprintf(buf, "У вас в наличии есть одна жалкая искристая снежинка.\r\n");
			send_to_char(buf, ch);
		} else if (ch->get_ice_currency() < 5) {
			sprintf(buf, "У вас в наличии есть жалкие %d искристые снежинки.\r\n", ch->get_ice_currency());
			send_to_char(buf, ch);
		} else {
			sprintf(buf, "У вас в наличии есть %d искристых снежинок.\r\n", ch->get_ice_currency());
			send_to_char(buf, ch);
		}
	}
	if (ch->get_nogata() > 0 && ROOM_FLAGGED(ch->in_room, ROOM_ARENA_DOMINATION)) {
		int value = ch->get_nogata();
		if (ch->get_nogata() == 1) {
			sprintf(buf, "У вас в наличии есть одна жалкая ногата.\r\n");
		}
		else {
			sprintf(buf, "У вас в наличии есть %d %s.\r\n", value, desc_count(value, WHAT_NOGATAu));
		}
		send_to_char(buf, ch);
	}
}

int CalcHitroll(CharData *ch) {
	ESkill skill = ESkill::kTwohands;
	int hr = 0;
	int max_dam = 0;
	ObjData *weapon = GET_EQ(ch, WEAR_BOTHS);
	if (weapon) {
		if (GET_OBJ_TYPE(weapon) == ObjData::ITEM_WEAPON) {
			skill = static_cast<ESkill>(GET_OBJ_SKILL(weapon));
			if (ch->get_skill(skill) == 0) {
				hr -= (50 - std::min(50, GET_REAL_INT(ch))) / 3;
			} else {
				apply_weapon_bonus(GET_CLASS(ch), skill, &max_dam, &hr);
			}
		}
	} else {
		weapon = GET_EQ(ch, WEAR_HOLD);
		if (weapon) {
			if (GET_OBJ_TYPE(weapon) == ObjData::ITEM_WEAPON) {
				skill = static_cast<ESkill>(GET_OBJ_SKILL(weapon));
				if (ch->get_skill(skill) == 0) {
					hr -= (50 - std::min(50, GET_REAL_INT(ch))) / 3;
				} else {
					apply_weapon_bonus(GET_CLASS(ch), skill, &max_dam, &hr);
				}
			}
		}
		weapon = GET_EQ(ch, WEAR_WIELD);
		if (weapon) {
			if (GET_OBJ_TYPE(weapon) == ObjData::ITEM_WEAPON) {
				skill = static_cast<ESkill>(GET_OBJ_SKILL(weapon));
				if (ch->get_skill(skill) == 0) {
					hr -= (50 - std::min(50, GET_REAL_INT(ch))) / 3;
				} else {
					apply_weapon_bonus(GET_CLASS(ch), skill, &max_dam, &hr);
				}
			}
		}
	}
	if (weapon) {
		int tmphr = 0;
		HitData::CheckWeapFeats(ch, static_cast<ESkill>(weapon->get_skill()), tmphr, max_dam);
		hr -= tmphr;
	} else {
		HitData::CheckWeapFeats(ch, ESkill::kPunch, hr, max_dam);
	}
	if (can_use_feat(ch, WEAPON_FINESSE_FEAT)) {
		hr += str_bonus(GET_REAL_DEX(ch), STR_TO_HIT);
	} else {
		hr += str_bonus(GET_REAL_STR(ch), STR_TO_HIT);
	}
	hr += GET_REAL_HR(ch) - thaco(static_cast<int>(GET_CLASS(ch)), GetRealLevel(ch));
	if (PRF_FLAGGED(ch, PRF_POWERATTACK)) {
		hr -= 2;
	}
	if (PRF_FLAGGED(ch, PRF_GREATPOWERATTACK)) {
		hr -= 4;
	}
	if (PRF_FLAGGED(ch, PRF_AIMINGATTACK)) {
		hr += 2;
	}
	if (PRF_FLAGGED(ch, PRF_GREATAIMINGATTACK)) {
		hr += 4;
	}
	hr -= (ch->ahorse() ? (10 - GET_SKILL(ch, ESkill::kRiding) / 20) : 0);
	hr *= ch->get_cond_penalty(P_HITROLL);
	return hr;
}

const char *GetPositionStr(CharData *ch) {
	switch (GET_POS(ch)) {
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
			if (ch->get_fighting()) {
				sprintf(buf1, "Вы сражаетесь с %s.\r\n", GET_PAD(ch->get_fighting(), 4));
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
	if (!ch->ahorse()) {
		switch (GET_POS(ch)) {
			case EPosition::kDead: return "Вы МЕРТВЫ!";
			case EPosition::kPerish: return "Вы умираете!";
			case EPosition::kIncap: return "Вы без сознания.";
			case EPosition::kStun: return "Вы в обмороке!";
			case EPosition::kSleep: return "Вы спите.";
			case EPosition::kRest: return "Вы отдыхаете.";
			case EPosition::kSit: return "Вы сидите.";
			case EPosition::kFight:
				if (ch->get_fighting()) {
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
