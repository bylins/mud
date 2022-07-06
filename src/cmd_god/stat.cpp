#include "stat.h"

#include "ban.h"
#include "entities/char_data.h"
#include "entities/char_player.h"
#include "entities/player_races.h"
#include "utils/utils_char_obj.inl"
#include "description.h"
#include "game_fight/fight_hit.h"
#include "game_fight/pk.h"
#include "handler.h"
#include "olc/olc.h"
#include "game_mechanics/glory.h"
#include "game_mechanics/glory_const.h"
#include "graph.h"
#include "house.h"
#include "liquid.h"
#include "obj_prototypes.h"
#include "color.h"
#include "statistics/mob_stat.h"
#include "modify.h"
#include "structs/global_objects.h"
#include "depot.h"

void do_statip(CharData *ch, CharData *k) {
	log("Start logon list stat");

	// Отображаем список ip-адресов с которых персонаж входил
	if (!LOGON_LIST(k).empty()) {
		// update: логон-лист может быть капитально большим, поэтому пишем это в свой дин.буфер, а не в buf2
		// заодно будет постраничный вывод ип, чтобы имма не посылало на йух с **OVERFLOW**
		std::ostringstream out("Персонаж заходил с IP-адресов:\r\n");
		for (const auto &logon : LOGON_LIST(k)) {
			sprintf(buf1,
					"%16s %5ld %20s%s\r\n",
					logon.ip,
					logon.count,
					rustime(localtime(&logon.lasttime)),
					logon.is_first ? " (создание)" : "");

			out << buf1;
		}
		page_string(ch->desc, out.str());
	}

	log("End logon list stat");
}

void do_stat_character(CharData *ch, CharData *k, const int virt = 0) {
	int i, i2, found = 0;
	ObjData *j;
	struct FollowerType *fol;
	char tmpbuf[128];
	buf[0] = 0;
	int god_level = PRF_FLAGGED(ch, EPrf::kCoderinfo) ? kLvlImplementator : GetRealLevel(ch);
	int k_room = -1;
	if (!virt && (god_level == kLvlImplementator || (god_level == kLvlGreatGod && !k->IsNpc()))) {
		k_room = GET_ROOM_VNUM(IN_ROOM(k));
	}
	// пишем пол  (мужчина)
	sprinttype(to_underlying(GET_SEX(k)), genders, tmpbuf);
	// пишем расу (Человек)
	if (k->IsNpc()) {
		sprinttype(GET_RACE(k) - ENpcRace::kBasic, npc_race_types, smallBuf);
		sprintf(buf, "%s %s ", tmpbuf, smallBuf);
	}
	sprintf(buf2,
			"%s '%s' IDNum: [%ld] В комнате [%d] Текущий Id:[%ld]",
			(!k->IsNpc() ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")),
			GET_NAME(k),
			GET_IDNUM(k),
			k_room,
			GET_ID(k));
	SendMsgToChar(strcat(buf, buf2), ch);
	SendMsgToChar(ch, " ЛАГ: [%d]\r\n", k->get_wait());
	if (IS_MOB(k)) {
		sprintf(buf,
				"Синонимы: &S%s&s, VNum: [%5d], RNum: [%5d]\r\n",
				k->get_pc_name().c_str(),
				GET_MOB_VNUM(k),
				GET_MOB_RNUM(k));
		SendMsgToChar(buf, ch);
	}

	sprintf(buf,
			"Падежи: %s/%s/%s/%s/%s/%s ",
			GET_PAD(k, 0),
			GET_PAD(k, 1),
			GET_PAD(k, 2),
			GET_PAD(k, 3),
			GET_PAD(k, 4),
			GET_PAD(k, 5));
	SendMsgToChar(buf, ch);

	if (!k->IsNpc()) {

		if (!NAME_GOD(k)) {
			sprintf(buf, "Имя никем не одобрено!\r\n");
			SendMsgToChar(buf, ch);
		} else if (NAME_GOD(k) < 1000) {
			sprintf(buf, "Имя запрещено! - %s\r\n", get_name_by_id(NAME_ID_GOD(k)));
			SendMsgToChar(buf, ch);
		} else {
			sprintf(buf, "Имя одобрено! - %s\r\n", get_name_by_id(NAME_ID_GOD(k)));
			SendMsgToChar(buf, ch);
		}

		sprintf(buf, "Вероисповедание: %s\r\n", religion_name[(int) GET_RELIGION(k)][(int) GET_SEX(k)]);
		SendMsgToChar(buf, ch);

		std::string file_name = GET_NAME(k);
		CreateFileName(file_name);
		sprintf(buf, "E-mail: &S%s&s Unique: %d File: %s\r\n", GET_EMAIL(k), GET_UNIQUE(k), file_name.c_str());
		SendMsgToChar(buf, ch);

		std::string text = RegisterSystem::show_comment(GET_EMAIL(k));
		if (!text.empty())
			SendMsgToChar(ch, "Registered by email from %s\r\n", text.c_str());

		if (k->player_specials->saved.telegram_id != 0)
			SendMsgToChar(ch, "Подключен Телеграм, chat_id: %lu\r\n", k->player_specials->saved.telegram_id);

		if (PLR_FLAGGED(k, EPlrFlag::kFrozen) && FREEZE_DURATION(k)) {
			sprintf(buf, "Заморожен : %ld час [%s].\r\n",
					static_cast<long>((FREEZE_DURATION(k) - time(nullptr)) / 3600),
					FREEZE_REASON(k) ? FREEZE_REASON(k) : "-");
			SendMsgToChar(buf, ch);
		}
		if (PLR_FLAGGED(k, EPlrFlag::kHelled) && HELL_DURATION(k)) {
			sprintf(buf, "Находится в темнице : %ld час [%s].\r\n",
					static_cast<long>((HELL_DURATION(k) - time(nullptr)) / 3600),
					HELL_REASON(k) ? HELL_REASON(k) : "-");
			SendMsgToChar(buf, ch);
		}
		if (PLR_FLAGGED(k, EPlrFlag::kNameDenied) && NAME_DURATION(k)) {
			sprintf(buf, "Находится в комнате имени : %ld час.\r\n",
					static_cast<long>((NAME_DURATION(k) - time(nullptr)) / 3600));
			SendMsgToChar(buf, ch);
		}
		if (PLR_FLAGGED(k, EPlrFlag::kMuted) && MUTE_DURATION(k)) {
			sprintf(buf, "Будет молчать : %ld час [%s].\r\n",
					static_cast<long>((MUTE_DURATION(k) - time(nullptr)) / 3600),
					MUTE_REASON(k) ? MUTE_REASON(k) : "-");
			SendMsgToChar(buf, ch);
		}
		if (PLR_FLAGGED(k, EPlrFlag::kDumbed) && DUMB_DURATION(k)) {
			sprintf(buf, "Будет нем : %ld мин [%s].\r\n",
					static_cast<long>((DUMB_DURATION(k) - time(nullptr)) / 60),
					DUMB_REASON(k) ? DUMB_REASON(k) : "-");
			SendMsgToChar(buf, ch);
		}
		if (!PLR_FLAGGED(k, EPlrFlag::kRegistred) && UNREG_DURATION(k)) {
			sprintf(buf, "Не будет зарегистрирован : %ld час [%s].\r\n",
					static_cast<long>((UNREG_DURATION(k) - time(nullptr)) / 3600),
					UNREG_REASON(k) ? UNREG_REASON(k) : "-");
			SendMsgToChar(buf, ch);
		}

		if (GET_GOD_FLAG(k, EGf::kGodsLike) && GCURSE_DURATION(k)) {
			sprintf(buf, "Под защитой Богов : %ld час.\r\n",
					static_cast<long>((GCURSE_DURATION(k) - time(nullptr)) / 3600));
			SendMsgToChar(buf, ch);
		}
		if (GET_GOD_FLAG(k, EGf::kGodscurse) && GCURSE_DURATION(k)) {
			sprintf(buf, "Проклят Богами : %ld час.\r\n",
					static_cast<long>((GCURSE_DURATION(k) - time(nullptr)) / 3600));
			SendMsgToChar(buf, ch);
		}
	}

	sprintf(buf, "Титул: %s\r\n", (k->player_data.title != "" ? k->player_data.title.c_str() : "<Нет>"));
	SendMsgToChar(buf, ch);
	if (k->IsNpc())
		sprintf(buf, "L-Des: %s", (k->player_data.long_descr != "" ? k->player_data.long_descr.c_str() : "<Нет>\r\n"));
	else
		sprintf(buf,
				"L-Des: %s",
				(k->player_data.description != "" ? k->player_data.description.c_str() : "<Нет>\r\n"));
	SendMsgToChar(buf, ch);

	if (!k->IsNpc()) {
		strcpy(smallBuf, MUD::Class(k->GetClass()).GetCName());
		sprintf(buf, "Племя: %s, Род: %s, Профессия: %s",
				PlayerRace::GetKinNameByNum(GET_KIN(k), GET_SEX(k)).c_str(),
				k->get_race_name().c_str(),
				smallBuf);
		SendMsgToChar(buf, ch);
	} else {
		std::string str;
		if (k->get_role_bits().any()) {
			print_bitset(k->get_role_bits(), npc_role_types, ",", str);
		} else {
			str += "нет";
		}
		SendMsgToChar(ch, "Роли NPC: %s%s%s", CCCYN(ch, C_NRM), str.c_str(), CCNRM(ch, C_NRM));
	}

	char tmp_buf[256];
	if (k->get_zone_group() > 1) {
		snprintf(tmp_buf, sizeof(tmp_buf), " : групповой %ldx%d",
				 GET_EXP(k) / k->get_zone_group(), k->get_zone_group());
	} else {
		tmp_buf[0] = '\0';
	}

	sprintf(buf, ", Уровень: [%s%2d%s], Опыт: [%s%10ld%s]%s, Наклонности: [%4d]\r\n",
			CCYEL(ch, C_NRM), GetRealLevel(k), CCNRM(ch, C_NRM), CCYEL(ch, C_NRM),
			GET_EXP(k), CCNRM(ch, C_NRM), tmp_buf, GET_ALIGNMENT(k));

	SendMsgToChar(buf, ch);

	if (!k->IsNpc()) {
		if (CLAN(k)) {
			SendMsgToChar(ch, "Статус дружины: %s\r\n", GET_CLAN_STATUS(k));
		}

		//added by WorM когда статишь файл собсно показывалось текущее время а не время последнего входа
		time_t ltime = get_lastlogon_by_unique(GET_UNIQUE(k));
		char t1[11];
		char t2[11];
		strftime(t1, sizeof(t1), "%d-%m-%Y", localtime(&(k->player_data.time.birth)));
		strftime(t2, sizeof(t2), "%d-%m-%Y", localtime(&ltime));
		t1[10] = t2[10] = '\0';

		sprintf(buf,
				"Создан: [%s] Последний вход: [%s] Играл: [%dh %dm] Возраст: [%d]\r\n",
				t1, t2, k->player_data.time.played / 3600, ((k->player_data.time.played % 3600) / 60), age(k)->year);
		SendMsgToChar(buf, ch);

		k->add_today_torc(0);
		sprintf(buf, "Рента: [%d], Денег: [%9ld], В банке: [%9ld] (Всего: %ld), Гривны: %d/%d/%d %d, Ногат: %d",
				GET_LOADROOM(k), k->get_gold(), k->get_bank(), k->get_total_gold(),
				k->get_ext_money(ExtMoney::kTorcGold),
				k->get_ext_money(ExtMoney::kTorcSilver),
				k->get_ext_money(ExtMoney::kTorcBronze),
				k->get_hryvn(), k->get_nogata());

		//. Display OLC zone for immorts .
		if (GetRealLevel(ch) >= kLvlImmortal) {
			sprintf(buf1, ", %sOLC[%d]%s", CCGRN(ch, C_NRM), GET_OLC_ZONE(k), CCNRM(ch, C_NRM));
			strcat(buf, buf1);
		}
		strcat(buf, "\r\n");
		SendMsgToChar(buf, ch);
	} else {
		sprintf(buf,
				"Сейчас в мире : %d. ",
				GET_MOB_RNUM(k) >= 0 ? mob_index[GET_MOB_RNUM(k)].total_online - (virt ? 1 : 0) : -1);
		SendMsgToChar(buf, ch);
		std::string stats;
		mob_stat::GetLastMobKill(k, stats);
		sprintf(buf, "Последний раз убит: %s", stats.c_str());
		SendMsgToChar(buf, ch);
	}
	sprintf(buf,
			"Сила: [%s%d/%d%s]  Инт : [%s%d/%d%s]  Мудр : [%s%d/%d%s] \r\n"
			"Ловк: [%s%d/%d%s]  Тело:[%s%d/%d%s]  Обаян:[%s%d/%d%s] Размер: [%s%d/%d%s]\r\n",
			CCCYN(ch, C_NRM), k->GetInbornStr(), GET_REAL_STR(k), CCNRM(ch,
																		C_NRM),
			CCCYN(ch, C_NRM), k->GetInbornInt(), GET_REAL_INT(k), CCNRM(ch,
																		C_NRM),
			CCCYN(ch, C_NRM), k->GetInbornWis(), GET_REAL_WIS(k), CCNRM(ch,
																		C_NRM),
			CCCYN(ch, C_NRM), k->GetInbornDex(), GET_REAL_DEX(k), CCNRM(ch,
																		C_NRM),
			CCCYN(ch, C_NRM), k->GetInbornCon(), GET_REAL_CON(k), CCNRM(ch,
																		C_NRM),
			CCCYN(ch, C_NRM), k->GetInbornCha(), GET_REAL_CHA(k), CCNRM(ch,
																		C_NRM),
			CCCYN(ch, C_NRM), GET_SIZE(k), GET_REAL_SIZE(k), CCNRM(ch, C_NRM));
	SendMsgToChar(buf, ch);

	sprintf(buf, "Жизни :[%s%d/%d+%d%s]  Энергии :[%s%d/%d+%d%s]",
			CCGRN(ch, C_NRM), GET_HIT(k), GET_REAL_MAX_HIT(k), hit_gain(k),
			CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), GET_MOVE(k), GET_REAL_MAX_MOVE(k), move_gain(k), CCNRM(ch, C_NRM));
	SendMsgToChar(buf, ch);
	if (IS_MANA_CASTER(k)) {
		sprintf(buf, " Мана :[%s%d/%d+%d%s]\r\n",
				CCGRN(ch, C_NRM), k->mem_queue.stored, GET_MAX_MANA(k), CalcManaGain(k), CCNRM(ch, C_NRM));
	} else {
		sprintf(buf, "\r\n");
	}
	SendMsgToChar(buf, ch);

	sprintf(buf,
			"Glory: [%d], ConstGlory: [%d], AC: [%d/%d(%d)], Броня: [%d], Попадания: [%2d/%2d/%d], Повреждения: [%2d/%2d/%d]\r\n",
			Glory::get_glory(GET_UNIQUE(k)),
			GloryConst::get_glory(GET_UNIQUE(k)),
			GET_AC(k),
			GET_REAL_AC(k),
			compute_armor_class(k),
			GET_ARMOUR(k),
			GET_HR(k),
			GET_REAL_HR(k),
			GET_REAL_HR(k) + str_bonus(GET_REAL_STR(k), STR_TO_HIT),
			GET_DR(k),
			GetRealDamroll(k),
			GetRealDamroll(k) + str_bonus(GET_REAL_STR(k), STR_TO_DAM));
	SendMsgToChar(buf, ch);
	sprintf(buf,
			"Защитн.аффекты: [Will:%d/Crit.:%d/Stab.:%d/Reflex:%d], Поглощ: [%d], Воля: [%d], Здор.: [%d], Стойк.: [%d], Реакц.: [%d]\r\n",
			GET_SAVE(k, ESaving::kWill), GET_SAVE(k, ESaving::kCritical),
			GET_SAVE(k, ESaving::kStability), GET_SAVE(k, ESaving::kReflex),
			GET_ABSORBE(k),
			GET_REAL_SAVING_WILL(k), GET_REAL_SAVING_CRITICAL(k),
			GET_REAL_SAVING_STABILITY(k), GET_REAL_SAVING_REFLEX(k));
	SendMsgToChar(buf, ch);
	sprintf(buf,
			"Резисты: [Огонь:%d/Воздух:%d/Вода:%d/Земля:%d/Жизнь:%d/Разум:%d/Иммунитет:%d/Тьма:%d]\r\n",
			GET_RESIST(k, 0), GET_RESIST(k, 1), GET_RESIST(k, 2), GET_RESIST(k, 3),
			GET_RESIST(k, 4), GET_RESIST(k, 5), GET_RESIST(k, 6), GET_RESIST(k, 7));
	SendMsgToChar(buf, ch);
	sprintf(buf,
			"Защита от маг. аффектов : [%d], Защита от маг. урона : [%d], Защита от физ. урона : [%d]\r\n",
			GET_AR(k),
			GET_MR(k),
			GET_PR(k));
	SendMsgToChar(buf, ch);

	sprintf(buf,
			"Запом: [%d], УспехКолд: [%d], ВоссЖиз: [%d], ВоссСил: [%d], Поглощ: [%d], Удача: [%d], Иниц: [%d]\r\n",
			GET_MANAREG(k),
			GET_CAST_SUCCESS(k),
			GET_HITREG(k),
			GET_MOVEREG(k),
			GET_ABSORBE(k),
			k->calc_morale(),
			GET_INITIATIVE(k));
	SendMsgToChar(buf, ch);

	sprinttype(static_cast<int>(GET_POS(k)), position_types, smallBuf);
	sprintf(buf, "Положение: %s, Сражается: %s, Экипирован в металл: %s",
			smallBuf, (k->GetEnemy() ? GET_NAME(k->GetEnemy()) : "Нет"), (IsEquipInMetall(k) ? "Да" : "Нет"));

	if (k->IsNpc()) {
		strcat(buf, ", Тип атаки: ");
		strcat(buf, attack_hit_text[k->mob_specials.attack_type].singular);
	}
	if (k->desc) {
		sprinttype(STATE(k->desc), connected_types, buf2);
		strcat(buf, ", Соединение: ");
		strcat(buf, buf2);
	}
	SendMsgToChar(strcat(buf, "\r\n"), ch);

	strcpy(buf, "Позиция по умолчанию: ");
	sprinttype(static_cast<int>(k->mob_specials.default_pos), position_types, buf2);
	strcat(buf, buf2);

	sprintf(buf2, ", Таймер отсоединения (тиков) [%d]\r\n", k->char_specials.timer);
	strcat(buf, buf2);
	SendMsgToChar(buf, ch);

	if (k->IsNpc()) {
		k->char_specials.saved.act.sprintbits(action_bits, smallBuf, ",", 4);
		sprintf(buf, "MOB флаги: %s%s%s\r\n", CCCYN(ch, C_NRM), smallBuf, CCNRM(ch, C_NRM));
		SendMsgToChar(buf, ch);
		k->mob_specials.npc_flags.sprintbits(function_bits, smallBuf, ",", 4);
		sprintf(buf, "NPC флаги: %s%s%s\r\n", CCCYN(ch, C_NRM), smallBuf, CCNRM(ch, C_NRM));
		SendMsgToChar(buf, ch);
		SendMsgToChar(ch,
					  "Количество атак: %s%d%s. ",
					  CCCYN(ch, C_NRM),
					  k->mob_specials.extra_attack + 1,
					  CCNRM(ch, C_NRM));
		SendMsgToChar(ch,
					  "Вероятность использования умений: %s%d%%%s. ",
					  CCCYN(ch, C_NRM),
					  k->mob_specials.like_work,
					  CCNRM(ch, C_NRM));
		SendMsgToChar(ch,
					  "Убить до начала замакса: %s%d%s\r\n",
					  CCCYN(ch, C_NRM),
					  k->mob_specials.MaxFactor,
					  CCNRM(ch, C_NRM));
		SendMsgToChar(ch, "Умения:&c");
		for (const auto &skill : MUD::Skills()) {
			if (skill.IsValid() && k->GetSkill(skill.GetId())) {
				SendMsgToChar(ch, " %s:[%3d]", skill.GetName(), k->GetSkill(skill.GetId()));
			}
		}
		SendMsgToChar(ch, CCNRM(ch, C_NRM));
		SendMsgToChar(ch, "\r\n");

		// информация о маршруте моба
		if (k->mob_specials.dest_count > 0) {
			// подготавливаем путевые точки
			std::stringstream str_dest_list;
			for (auto i = 0; i < k->mob_specials.dest_count; i++) {
				if (i) {
					str_dest_list << " - ";
				}
				str_dest_list << std::to_string(k->mob_specials.dest[i]);
			}

			// пытаемся просчитать маршрут на несколько клеток вперед
			std::vector<RoomVnum> predictive_path_vnum_list;
			static const int max_path_size = 25;
			RoomVnum current_room = world[k->in_room]->room_vn;
			while (current_room != GET_DEST(k) && predictive_path_vnum_list.size() < max_path_size && current_room > kNowhere) {
				const auto direction = find_first_step(real_room(current_room), real_room(GET_DEST(k)), k);
				if (direction >= 0) {
					const auto exit_room_rnum = world[real_room(current_room)]->dir_option[direction]->to_room();
					current_room = world[exit_room_rnum]->room_vn;
					predictive_path_vnum_list.push_back(current_room);
				} else {
					break;
				}
			}
			// конвертируем путь из внумов в строку
			std::stringstream str_predictive_path;
			for (const auto &room : predictive_path_vnum_list) {
				if (!str_predictive_path.str().empty()) {
					str_predictive_path << " - ";
				}
				str_predictive_path << std::to_string(room);
			}

			SendMsgToChar(ch,
						  "Заданные путевые точки: %s%s%s\r\n",
						  CCCYN(ch, C_NRM),
						  str_dest_list.str().c_str(),
						  CCNRM(ch, C_NRM));
			if (!virt) {
				SendMsgToChar(ch,
							  "Предполагаемый маршрут: %s%s%s\r\n",
							  CCCYN(ch, C_NRM),
							  str_predictive_path.str().c_str(),
							  CCNRM(ch, C_NRM));
			}
		}
	} else {
		k->char_specials.saved.act.sprintbits(player_bits, smallBuf, ",", 4);
		sprintf(buf, "PLR: %s%s%s\r\n", CCCYN(ch, C_NRM), smallBuf, CCNRM(ch, C_NRM));
		SendMsgToChar(buf, ch);

		k->player_specials->saved.pref.sprintbits(preference_bits, smallBuf, ",", 4);
		sprintf(buf, "PRF: %s%s%s\r\n", CCGRN(ch, C_NRM), smallBuf, CCNRM(ch, C_NRM));
		SendMsgToChar(buf, ch);

		if (IS_IMPL(ch)) {
			sprintbitwd(k->player_specials->saved.GodsLike, godslike_bits, smallBuf, ",");
			sprintf(buf, "GFL: %s%s%s\r\n", CCCYN(ch, C_NRM), smallBuf, CCNRM(ch, C_NRM));
			SendMsgToChar(buf, ch);
		}
	}

	if (IS_MOB(k)) {
		sprintf(buf, "Mob СпецПроц: %s, NPC сила удара: %dd%d\r\n",
				(mob_index[GET_MOB_RNUM(k)].func ? "Есть" : "Нет"),
				k->mob_specials.damnodice, k->mob_specials.damsizedice);
		SendMsgToChar(buf, ch);
	}
	sprintf(buf, "Несет - вес %d, предметов %d; ", IS_CARRYING_W(k), IS_CARRYING_N(k));

	for (i = 0, j = k->carrying; j; j = j->get_next_content(), i++) { ; }
	sprintf(buf + strlen(buf), "(в инвентаре) : %d, ", i);

	for (i = 0, i2 = 0; i < EEquipPos::kNumEquipPos; i++) {
		if (GET_EQ(k, i)) {
			i2++;
		}
	}
	sprintf(buf2, "(надето): %d\r\n", i2);
	strcat(buf, buf2);
	SendMsgToChar(buf, ch);

	if (!k->IsNpc()) {
		sprintf(buf, "Голод: %d, Жажда: %d, Опьянение: %d\r\n",
				GET_COND(k, FULL), GET_COND(k, THIRST), GET_COND(k, DRUNK));
		SendMsgToChar(buf, ch);
	}

	if (god_level >= kLvlGreatGod) {
		sprintf(buf, "Ведущий: %s, Ведомые:", (k->has_master() ? GET_NAME(k->get_master()) : "<нет>"));

		for (fol = k->followers; fol; fol = fol->next) {
			sprintf(buf2, "%s %s", found++ ? "," : "", PERS(fol->follower, ch, 0));
			strcat(buf, buf2);
			if (strlen(buf) >= 62) {
				if (fol->next)
					SendMsgToChar(strcat(buf, ",\r\n"), ch);
				else
					SendMsgToChar(strcat(buf, "\r\n"), ch);
				*buf = found = 0;
			}
		}

		if (*buf)
			SendMsgToChar(strcat(buf, "\r\n"), ch);
	}
	// Showing the bitvector
	k->char_specials.saved.affected_by.sprintbits(affected_bits, smallBuf, ",", 4);
	sprintf(buf, "Аффекты: %s%s%s\r\n", CCYEL(ch, C_NRM), smallBuf, CCNRM(ch, C_NRM));
	SendMsgToChar(buf, ch);
	sprintf(buf, "&GПеревоплощений: %d\r\n&n", GetRealRemort(k));
	SendMsgToChar(buf, ch);
	// Routine to show what spells a char is affected by
	if (!k->affected.empty()) {
		for (const auto &aff : k->affected) {
			*buf2 = '\0';
			sprintf(buf, "Заклинания: (%3d%s|%s) %s%-21s%s ", aff->duration + 1,
					(aff->battleflag & kAfPulsedec) || (aff->battleflag & kAfSameTime) ? "плс" : "мин",
					(aff->battleflag & kAfBattledec) || (aff->battleflag & kAfSameTime) ? "рнд" : "мин",
					CCCYN(ch, C_NRM), MUD::Spell(aff->type).GetCName(), CCNRM(ch, C_NRM));
			if (aff->modifier) {
				sprintf(buf2, "%+d to %s", aff->modifier, apply_types[(int) aff->location]);
				strcat(buf, buf2);
			}
			if (aff->bitvector) {
				if (*buf2) {
					strcat(buf, ", sets ");
				} else {
					strcat(buf, "sets ");
				}
				sprintbit(aff->bitvector, affected_bits, buf2);
				strcat(buf, buf2);
			}
			SendMsgToChar(strcat(buf, "\r\n"), ch);
		}
	}

	// check mobiles for a script
	if (k->IsNpc() && god_level >= kLvlBuilder) {
		do_sstat_character(ch, k);
		if (MEMORY(k)) {
			struct MemoryRecord *memchar;
			SendMsgToChar("Помнит:\r\n", ch);
			for (memchar = MEMORY(k); memchar; memchar = memchar->next) {
				sprintf(buf, "%10ld - %10ld\r\n",
						static_cast<long>(memchar->id),
						static_cast<long>(memchar->time - time(nullptr)));
				SendMsgToChar(buf, ch);
			}
		}
	} else        // this is a PC, display their global variables
	{
		if (SCRIPT(k)->global_vars) {
			struct TriggerVar *tv;
			char name[kMaxInputLength];
			void find_uid_name(char *uid, char *name);
			SendMsgToChar("Глобальные переменные:\r\n", ch);
			// currently, variable context for players is always 0, so it is
			// not displayed here. in the future, this might change
			for (tv = k->script->global_vars; tv; tv = tv->next) {
				if (*(tv->value) == UID_CHAR) {
					find_uid_name(tv->value, name);
					sprintf(buf, "    %10s:  [CharUID]: %s\r\n", tv->name, name);
				} else if (*(tv->value) == UID_OBJ) {
					find_uid_name(tv->value, name);
					sprintf(buf, "    %10s:  [ObjUID]: %s\r\n", tv->name, name);
				} else if (*(tv->value) == UID_ROOM) {
					find_uid_name(tv->value, name);
					sprintf(buf, "    %10s:  [RoomUID]: %s\r\n", tv->name, name);
				} else
					sprintf(buf, "    %10s:  %s\r\n", tv->name, tv->value);
				SendMsgToChar(buf, ch);
			}
		}

		std::string quested(k->quested_print());
		if (!quested.empty())
			SendMsgToChar(ch, "Выполнил квесты:\r\n%s\r\n", quested.c_str());

		if (NORENTABLE(k)) {
			sprintf(buf, "Не может уйти на постой %ld\r\n",
					static_cast<long int>(NORENTABLE(k) - time(nullptr)));
			SendMsgToChar(buf, ch);
		}
		if (AGRO(k)) {
			sprintf(buf, "Агрессор %ld\r\n", static_cast<long int>(AGRO(k) - time(nullptr)));
			SendMsgToChar(buf, ch);
		}
		pk_list_sprintf(k, buf);
		SendMsgToChar(buf, ch);
		// Отображаем карму.
		if (KARMA(k)) {
			sprintf(buf, "Карма:\r\n%s", KARMA(k));
			SendMsgToChar(buf, ch);
		}

	}
}

void do_stat_object(CharData *ch, ObjData *j, const int virt = 0) {
	int i, found;
	ObjVnum rnum, vnum;
	ObjData *j2;
	long int li;
	bool is_grgod = (IS_GRGOD(ch) || PRF_FLAGGED(ch, EPrf::kCoderinfo)) ? true : false;

	vnum = GET_OBJ_VNUM(j);
	rnum = GET_OBJ_RNUM(j);

	sprintf(buf, "Название: '%s%s%s',\r\nСинонимы: '&c%s&n',",
			CCYEL(ch, C_NRM),
			(!j->get_short_description().empty() ? j->get_short_description().c_str() : "<None>"),
			CCNRM(ch, C_NRM),
			j->get_aliases().c_str());
	SendMsgToChar(buf, ch);
	if (j->get_custom_label() && j->get_custom_label()->label_text) {
		sprintf(buf, " нацарапано: '&c%s&n',", j->get_custom_label()->label_text);
		SendMsgToChar(buf, ch);
	}
	sprintf(buf, "\r\n");
	SendMsgToChar(buf, ch);
	sprinttype(GET_OBJ_TYPE(j), item_types, buf1);
	if (rnum >= 0) {
		strcpy(buf2, (obj_proto.func(j->get_rnum()) ? "Есть" : "Нет"));
	} else {
		strcpy(buf2, "None");
	}

	SendMsgToChar(ch, "VNum: [%s%5d%s], RNum: [%5d], UID: [%d], Id: [%ld]\r\n",
				  CCGRN(ch, C_NRM), vnum, CCNRM(ch, C_NRM), GET_OBJ_RNUM(j), GET_OBJ_UID(j), j->get_id());

	SendMsgToChar(ch, "Расчет критерия: %f, мортов: (%f) \r\n", j->show_koef_obj(), j->show_mort_req());
	SendMsgToChar(ch, "Тип: %s, СпецПроцедура: %s", buf1, buf2);

	if (GET_OBJ_OWNER(j)) {
		SendMsgToChar(ch, ", Владелец : %s", get_name_by_unique(GET_OBJ_OWNER(j)));
	}
//	if (GET_OBJ_ZONE(j))
	SendMsgToChar(ch, ", Принадлежит зоне VNUM : %d", GET_OBJ_VNUM_ZONE_FROM(j));
	if (GET_OBJ_MAKER(j)) {
		const char *to_name = get_name_by_unique(GET_OBJ_MAKER(j));
		if (to_name)
			SendMsgToChar(ch, ", Создатель : %s", to_name);
		else
			SendMsgToChar(ch, ", Создатель : не найден");
	}
	if (GET_OBJ_PARENT(j)) {
		SendMsgToChar(ch, ", Родитель(VNum) : [%d]", GET_OBJ_PARENT(j));
	}
	if (GET_OBJ_CRAFTIMER(j) > 0) {
		SendMsgToChar(ch, ", &Yскрафчена с таймером : [%d]&n", GET_OBJ_CRAFTIMER(j));
	}
	if (j->get_is_rename()) // изменены падежи
	{
		SendMsgToChar(ch, ", &Gпадежи отличны от прототипа&n");
	}
	sprintf(buf, "\r\nL-Des: %s\r\n%s",
			!j->get_description().empty() ? j->get_description().c_str() : "Нет",
			CCNRM(ch, C_NRM));
	SendMsgToChar(buf, ch);

	if (j->get_ex_description()) {
		sprintf(buf, "Экстра описание:%s", CCCYN(ch, C_NRM));
		for (auto desc = j->get_ex_description(); desc; desc = desc->next) {
			strcat(buf, " ");
			strcat(buf, desc->keyword);
		}
		strcat(buf, CCNRM(ch, C_NRM));
		SendMsgToChar(strcat(buf, "\r\n"), ch);
	}
	SendMsgToChar("Может быть надет : ", ch);
	sprintbit(j->get_wear_flags(), wear_bits, buf);
	strcat(buf, "\r\n");
	SendMsgToChar(buf, ch);
	sprinttype(j->get_material(), material_name, buf2);
	snprintf(buf, kMaxStringLength, "Материал : %s, макс.прочность : %d, тек.прочность : %d\r\n",
			 buf2, j->get_maximum_durability(), j->get_current_durability());
	SendMsgToChar(buf, ch);

	SendMsgToChar("Неудобства : ", ch);
	j->get_no_flags().sprintbits(no_bits, buf, ",", 4);
	strcat(buf, "\r\n");
	SendMsgToChar(buf, ch);

	SendMsgToChar("Запреты : ", ch);
	j->get_anti_flags().sprintbits(anti_bits, buf, ",", 4);
	strcat(buf, "\r\n");
	SendMsgToChar(buf, ch);

	SendMsgToChar("Устанавливает аффекты : ", ch);
	j->get_affect_flags().sprintbits(weapon_affects, buf, ",", 4);
	strcat(buf, "\r\n");
	SendMsgToChar(buf, ch);

	SendMsgToChar("Дополнительные флаги  : ", ch);
	GET_OBJ_EXTRA(j).sprintbits(extra_bits, buf, ",", 4);
	strcat(buf, "\r\n");
	SendMsgToChar(buf, ch);

	sprintf(buf, "Вес: %d, Цена: %d, Рента(eq): %d, Рента(inv): %d, ",
			GET_OBJ_WEIGHT(j), GET_OBJ_COST(j), GET_OBJ_RENTEQ(j), GET_OBJ_RENT(j));
	SendMsgToChar(buf, ch);
	if (check_unlimited_timer(j))
		sprintf(buf, "Таймер: нерушимо, ");
	else
		sprintf(buf, "Таймер: %d, ", j->get_timer());
	SendMsgToChar(buf, ch);
	sprintf(buf, "Таймер на земле: %d\r\n", GET_OBJ_DESTROY(j));
	SendMsgToChar(buf, ch);

	auto room = get_room_where_obj(j);
	strcpy(buf, "Находится в комнате : ");
	if (room == kNowhere || !is_grgod) {
		strcat(buf, "нигде");
	} else {
		sprintf(buf2, "%d", room);
		strcat(buf, buf2);
	}

	strcat(buf, ", В контейнере: ");
	if (j->get_in_obj() && is_grgod) {
		sprintf(buf2, "[%d] %s", GET_OBJ_VNUM(j->get_in_obj()), j->get_in_obj()->get_short_description().c_str());
		strcat(buf, buf2);
	} else {
		const auto param = Depot::look_obj_depot(j);
		if ( param != nullptr)
			strcat(buf, param);
		else
			strcat(buf, "Нет");
	}

	strcat(buf, ", В инвентаре: ");
	if (j->get_carried_by() && is_grgod) {
		strcat(buf, GET_NAME(j->get_carried_by()));
	} else if (j->get_in_obj() && j->get_in_obj()->get_carried_by() && is_grgod) {
		strcat(buf, GET_NAME(j->get_in_obj()->get_carried_by()));
	} else {
		strcat(buf, "Нет");
	}

	strcat(buf, ", Надет: ");
	if (j->get_worn_by() && is_grgod) {
		strcat(buf, GET_NAME(j->get_worn_by()));
	} else if (j->get_in_obj() && j->get_in_obj()->get_worn_by() && is_grgod) {
		strcat(buf, GET_NAME(j->get_in_obj()->get_worn_by()));
	} else {
		strcat(buf, "Нет");
	}

	strcat(buf, "\r\n");
	SendMsgToChar(buf, ch);

	switch (GET_OBJ_TYPE(j)) {
		case EObjType::kBook:

			switch (GET_OBJ_VAL(j, 0)) {
				case EBook::kSpell: {
					auto spell_id = static_cast<ESpell>(GET_OBJ_VAL(j, 1));
					if (spell_id >= ESpell::kFirst && spell_id <= ESpell::kLast) {
						sprintf(buf, "содержит заклинание        : \"%s\"", MUD::Spell(spell_id).GetCName());
					} else
						sprintf(buf, "неверный номер заклинания");
					break;
				}
				case EBook::kSkill: {
					auto skill_id = static_cast<ESkill>(GET_OBJ_VAL(j, 1));
					if (MUD::Skill(skill_id).IsValid()) {
						sprintf(buf, "содержит секрет умения     : \"%s\"", MUD::Skill(skill_id).GetName());
					} else
						sprintf(buf, "неверный номер умения");
					break;
				}
				case EBook::kSkillUpgrade: {
					auto skill_id = static_cast<ESkill>(GET_OBJ_VAL(j, 1));
					if (MUD::Skills().IsValid(skill_id)) {
						if (GET_OBJ_VAL(j, 3) > 0) {
							sprintf(buf, "повышает умение \"%s\" (максимум %d)",
									MUD::Skill(skill_id).GetName(), GET_OBJ_VAL(j, 3));
						} else {
							sprintf(buf, "повышает умение \"%s\" (не больше максимума текущего перевоплощения)",
									MUD::Skill(skill_id).GetName());
						}
					} else {
						sprintf(buf, "неверный номер повышаемоего умения");
					}
				}
					break;
				case EBook::kFeat: {
					const auto feat_id = static_cast<EFeat>(GET_OBJ_VAL(j, 1));
					if (MUD::Feat(feat_id).IsValid()) {
						sprintf(buf, "содержит секрет способности : \"%s\"", MUD::Feat(feat_id).GetCName());
					} else {
						sprintf(buf, "неверный номер способности");
					}
				}
					break;
				case EBook::kReceipt: {
					const auto recipe = im_get_recipe(GET_OBJ_VAL(j, 1));
					if (recipe >= 0) {
						const auto recipelevel = std::max(GET_OBJ_VAL(j, 2), imrecipes[recipe].level);
						const auto recipemort = imrecipes[recipe].remort;
						if ((recipelevel >= 0) && (recipemort >= 0)) {
							sprintf(buf,
									"содержит рецепт отвара     : \"%s\", уровень изучения: %d, количество ремортов: %d",
									imrecipes[recipe].name,
									recipelevel,
									recipemort);
						} else {
							sprintf(buf, "Некорректная запись рецепта (нет уровня или реморта)");
						}
					} else
						sprintf(buf, "Некорректная запись рецепта");
				}
					break;
				default:sprintf(buf, "НЕВЕРНО УКАЗАН ТИП КНИГИ");
					break;
			}
			break;
		case EObjType::kLightSource:
			if (GET_OBJ_VAL(j, 2) < 0) {
				strcpy(buf, "Вечный свет!");
			} else {
				sprintf(buf, "Осталось светить: [%d]", GET_OBJ_VAL(j, 2));
			}
			break;

		case EObjType::kScroll:
		case EObjType::kPotion: {
			std::ostringstream out;
			out << "Заклинания: (Уровень %d" << GET_OBJ_VAL(j, 0) << ") ";
			for (auto val = 1; val < 4; ++val) {
				auto spell_id = static_cast<ESpell>(GET_OBJ_VAL(j, val));
				if (MUD::Spell(spell_id).IsValid()) {
					out << MUD::Spell(spell_id).GetName();
					if (val < 3) {
						out << ", ";
					} else {
						out << ".";
					}
				}
			}
			sprintf(buf, "%s", out.str().c_str());
			break;
		}
		case EObjType::kWand:
		case EObjType::kStaff:
			sprintf(buf, "Заклинание: %s уровень %d, %d (из %d) зарядов осталось",
					MUD::Spell(static_cast<ESpell>(GET_OBJ_VAL(j, 3))).GetCName(),
					GET_OBJ_VAL(j, 0),
					GET_OBJ_VAL(j, 2),
					GET_OBJ_VAL(j, 1));
			break;

		case EObjType::kWeapon:
			sprintf(buf, "Повреждения: %dd%d, Тип повреждения: %d",
					GET_OBJ_VAL(j, 1),
					GET_OBJ_VAL(j, 2),
					GET_OBJ_VAL(j, 3));
			break;

		case EObjType::kArmor:
		case EObjType::kLightArmor:
		case EObjType::kMediumArmor:
		case EObjType::kHeavyArmor:sprintf(buf, "AC: [%d]  Броня: [%d]", GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1));
			break;

		case EObjType::kTrap:sprintf(buf, "Spell: %d, - Hitpoints: %d", GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1));
			break;

		case EObjType::kContainer:sprintbit(GET_OBJ_VAL(j, 1), container_bits, smallBuf);
			//sprintf(buf, "Объем: %d, Тип ключа: %s, Номер ключа: %d, Труп: %s",
			//	GET_OBJ_VAL(j, 0), buf2, GET_OBJ_VAL(j, 2), YESNO(GET_OBJ_VAL(j, 3)));
			if (IS_CORPSE(j)) {
				sprintf(buf, "Объем: %d, Тип ключа: %s, VNUM моба: %d, Труп: да",
						GET_OBJ_VAL(j, 0), smallBuf, GET_OBJ_VAL(j, 2));
			} else {
				sprintf(buf, "Объем: %d, Тип ключа: %s, Номер ключа: %d, Сложность замка: %d",
						GET_OBJ_VAL(j, 0), smallBuf, GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
			}
			break;

		case EObjType::kLiquidContainer:
		case EObjType::kFountain:sprinttype(GET_OBJ_VAL(j, 2), drinks, smallBuf);
			{
				std::string spells = drinkcon::print_spells(ch, j);
				utils::Trim(spells);
				sprintf(buf, "Обьем: %d, Содержит: %d, Таймер (если 1 отравлено): %d, Жидкость: %s\r\n%s",
						GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 3), smallBuf, spells.c_str());
			}
			break;

		case EObjType::kNote:sprintf(buf, "Tongue: %d", GET_OBJ_VAL(j, 0));
			break;

		case EObjType::kKey:strcpy(buf, "");
			break;

		case EObjType::kFood:
			sprintf(buf,
					"Насыщает(час): %d, Таймер (если 1 отравлено): %d",
					GET_OBJ_VAL(j, 0),
					GET_OBJ_VAL(j, 3));
			break;

		case EObjType::kMoney:
			sprintf(buf, "Сумма: %d\r\nВалюта: %s", GET_OBJ_VAL(j, 0),
					GET_OBJ_VAL(j, 1) == currency::GOLD ? "куны" :
					GET_OBJ_VAL(j, 1) == currency::ICE ? "искристые снежинки" :
					"что-то другое"
			);
			break;

		case EObjType::kIngredient:sprintbit(GET_OBJ_SKILL(j), ingradient_bits, smallBuf);
			sprintf(buf, "ingr bits %s", smallBuf);

			if (IS_SET(GET_OBJ_SKILL(j), kItemCheckUses)) {
				sprintf(buf + strlen(buf), "\r\nможно применить %d раз", GET_OBJ_VAL(j, 2));
			}

			if (IS_SET(GET_OBJ_SKILL(j), kItemCheckLag)) {
				sprintf(buf + strlen(buf), "\r\nможно применить 1 раз в %d сек", (i = GET_OBJ_VAL(j, 0) & 0xFF));
				if (GET_OBJ_VAL(j, 3) == 0 || GET_OBJ_VAL(j, 3) + i < time(nullptr))
					sprintf(buf + strlen(buf), "(можно применять).");
				else {
					li = GET_OBJ_VAL(j, 3) + i - time(nullptr);
					sprintf(buf + strlen(buf), "(осталось %ld сек).", li);
				}
			}

			if (IS_SET(GET_OBJ_SKILL(j), kItemCheckLevel)) {
				sprintf(buf + strlen(buf), "\r\nможно применить с %d уровня.", (GET_OBJ_VAL(j, 0) >> 8) & 0x1F);
			}

			if ((i = real_object(GET_OBJ_VAL(j, 1))) >= 0) {
				sprintf(buf + strlen(buf), "\r\nпрототип %s%s%s.",
						CCICYN(ch, C_NRM), obj_proto[i]->get_PName(0).c_str(), CCNRM(ch, C_NRM));
			}
			break;
		case EObjType::kMagicContaner:
		case EObjType::kMagicArrow:
			sprintf(buf, "Заклинание: [%s]. Объем [%d]. Осталось стрел[%d].",
					MUD::Spell(static_cast<ESpell>(GET_OBJ_VAL(j, 0))).GetCName(),
					GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2));
			break;

		default:
			sprintf(buf, "Values 0-3: [%d] [%d] [%d] [%d]",
					GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
			break;
	}
	SendMsgToChar(strcat(buf, "\r\n"), ch);

	// * I deleted the "equipment status" code from here because it seemed
	// * more or less useless and just takes up valuable screen space.

	if (j->get_contains()) {
		sprintf(buf, "\r\nСодержит:%s", CCGRN(ch, C_NRM));
		for (found = 0, j2 = j->get_contains(); j2; j2 = j2->get_next_content()) {
			sprintf(buf2, "%s %s", found++ ? "," : "", j2->get_short_description().c_str());
			strcat(buf, buf2);
			if (strlen(buf) >= 62) {
				if (j2->get_next_content()) {
					SendMsgToChar(strcat(buf, ",\r\n"), ch);
				} else {
					SendMsgToChar(strcat(buf, "\r\n"), ch);
				}
				*buf = found = 0;
			}
		}

		if (*buf) {
			SendMsgToChar(strcat(buf, "\r\n"), ch);
		}
		SendMsgToChar(CCNRM(ch, C_NRM), ch);
	}
	found = 0;
	SendMsgToChar("Аффекты:", ch);
	for (i = 0; i < kMaxObjAffect; i++) {
		if (j->get_affected(i).modifier) {
			sprinttype(j->get_affected(i).location, apply_types, smallBuf);
			sprintf(buf, "%s %+d to %s", found++ ? "," : "", j->get_affected(i).modifier, smallBuf);
			SendMsgToChar(buf, ch);
		}
	}
	if (!found) {
		SendMsgToChar(" Нет", ch);
	}

	if (j->has_skills()) {
		CObjectPrototype::skills_t skills;
		j->get_skills(skills);

		SendMsgToChar("\r\nУмения :", ch);
		for (const auto &it : skills) {
			if (it.second == 0) {
				continue;
			}
			sprintf(buf, " %+d%% to %s", it.second, MUD::Skill(it.first).GetName());
			SendMsgToChar(buf, ch);
		}
	}
	SendMsgToChar("\r\n", ch);

	if (j->get_ilevel() > 0) {
		SendMsgToChar(ch, "Уровень (ilvl): %f\r\n", j->get_ilevel());
	}

	if (j->get_minimum_remorts() != 0) {
		SendMsgToChar(ch, "Проставлено поле перевоплощений: %d\r\n", j->get_minimum_remorts());
	} else if (j->get_auto_mort_req() > 0) {
		SendMsgToChar(ch, "Вычислено поле минимальных перевоплощений: %d\r\n", j->get_auto_mort_req());
	}

	if (is_grgod) {
		sprintf(buf,
				"Сейчас в мире : %d. На постое : %d. Макс в мире: %d\r\n",
				rnum >= 0 ? obj_proto.CountInWorld(rnum) - (virt ? 1 : 0) : -1,
				rnum >= 0 ? obj_proto.stored(rnum) : -1,
				GET_OBJ_MIW(j));
		SendMsgToChar(buf, ch);
		// check the object for a script
		do_sstat_object(ch, j);
	}
}

void do_stat_room(CharData *ch, const int rnum = 0) {
	RoomData *rm = world[ch->in_room];
	int i, found;
	ObjData *j;
	CharData *k;
	char tmpBuf[255];

	if (rnum != 0) {
		rm = world[rnum];
	}

	sprintf(buf, "Комната : %s%s%s\r\n", CCCYN(ch, C_NRM), rm->name, CCNRM(ch, C_NRM));
	SendMsgToChar(buf, ch);

	sprinttype(rm->sector_type, sector_types, smallBuf);
	sprintf(buf,
			"Зона: [%3d], VNum: [%s%5d%s], RNum: [%5d], Тип  сектора: %s\r\n",
			zone_table[rm->zone_rn].vnum, CCGRN(ch, C_NRM), rm->room_vn, CCNRM(ch, C_NRM), ch->in_room, smallBuf);
	SendMsgToChar(buf, ch);

	rm->flags_sprint(smallBuf, ",");
	sprintf(buf, "СпецПроцедура: %s, Флаги: %s\r\n", (rm->func == nullptr) ? "None" : "Exists", smallBuf);
	SendMsgToChar(buf, ch);

	SendMsgToChar("Описание:\r\n", ch);
	SendMsgToChar(RoomDescription::show_desc(rm->description_num), ch);

	if (rm->ex_description) {
		sprintf(buf, "Доп. описание:%s", CCCYN(ch, C_NRM));
		for (auto desc = rm->ex_description; desc; desc = desc->next) {
			strcat(buf, " ");
			strcat(buf, desc->keyword);
		}
		strcat(buf, CCNRM(ch, C_NRM));
		SendMsgToChar(strcat(buf, "\r\n"), ch);
	}
	sprintf(buf, "Живые существа:%s", CCYEL(ch, C_NRM));
	found = 0;
	size_t counter = 0;
	for (auto k_i = rm->people.begin(); k_i != rm->people.end(); ++k_i) {
		const auto k = *k_i;
		++counter;

		if (!CAN_SEE(ch, k)) {
			continue;
		}
		sprintf(buf2, "%s %s(%s)", found++ ? "," : "", GET_NAME(k),
				(!k->IsNpc() ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")));
		strcat(buf, buf2);
		if (strlen(buf) >= 62) {
			if (counter != rm->people.size()) {
				SendMsgToChar(strcat(buf, ",\r\n"), ch);
			} else {
				SendMsgToChar(strcat(buf, "\r\n"), ch);
			}
			*buf = found = 0;
		}
	}

	if (*buf) {
		SendMsgToChar(strcat(buf, "\r\n"), ch);
	}
	SendMsgToChar(CCNRM(ch, C_NRM), ch);

	if (rm->contents) {
		sprintf(buf, "Предметы:%s", CCGRN(ch, C_NRM));
		for (found = 0, j = rm->contents; j; j = j->get_next_content()) {
			if (!CAN_SEE_OBJ(ch, j))
				continue;
			sprintf(buf2, "%s %s", found++ ? "," : "", j->get_short_description().c_str());
			strcat(buf, buf2);
			if (strlen(buf) >= 62) {
				if (j->get_next_content()) {
					SendMsgToChar(strcat(buf, ",\r\n"), ch);
				} else {
					SendMsgToChar(strcat(buf, "\r\n"), ch);
				}
				*buf = found = 0;
			}
		}

		if (*buf) {
			SendMsgToChar(strcat(buf, "\r\n"), ch);
		}
		SendMsgToChar(CCNRM(ch, C_NRM), ch);
	}
	for (i = 0; i < EDirection::kMaxDirNum; i++) {
		if (rm->dir_option[i]) {
			if (rm->dir_option[i]->to_room() == kNowhere)
				sprintf(smallBuf, " %sNONE%s", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
			else
				sprintf(smallBuf, "%s%5d%s", CCCYN(ch, C_NRM),
						GET_ROOM_VNUM(rm->dir_option[i]->to_room()), CCNRM(ch, C_NRM));
			sprintbit(rm->dir_option[i]->exit_info, exit_bits, tmpBuf);
			sprintf(buf,
					"Выход %s%-5s%s:  Ведет в : [%s], Ключ: [%5d], Название: %s (%s), Тип: %s\r\n",
					CCCYN(ch, C_NRM), dirs[i], CCNRM(ch, C_NRM), smallBuf,
					rm->dir_option[i]->key,
					rm->dir_option[i]->keyword ? rm->dir_option[i]->keyword : "Нет(дверь)",
					rm->dir_option[i]->vkeyword ? rm->dir_option[i]->vkeyword : "Нет(дверь)", tmpBuf);
			SendMsgToChar(buf, ch);
			if (!rm->dir_option[i]->general_description.empty()) {
				sprintf(buf, "  %s\r\n", rm->dir_option[i]->general_description.c_str());
			} else {
				strcpy(buf, "  Нет описания выхода.\r\n");
			}
			SendMsgToChar(buf, ch);
		}
	}

	if (!rm->affected.empty()) {
		sprintf(buf1, " Аффекты на комнате:\r\n");
		for (const auto &aff : rm->affected) {
			sprintf(buf1 + strlen(buf1), "       Заклинание \"%s\" (%d) - %s.\r\n",
					MUD::Spell(aff->type).GetCName(),
					aff->duration,
					((k = find_char(aff->caster_id))
					 ? GET_NAME(k)
					 : "неизвестно"));
		}
		SendMsgToChar(buf1, ch);
	}
	// check the room for a script
	do_sstat_room(rm, ch);
}

void do_stat(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *victim;
	ObjData *object;
	int tmp;

	half_chop(argument, buf1, buf2);

	if (!*buf1) {
		SendMsgToChar("Состояние КОГО или ЧЕГО?\r\n", ch);
		return;
	}

	int level = PRF_FLAGGED(ch, EPrf::kCoderinfo) ? kLvlImplementator : GetRealLevel(ch);

	if (utils::IsAbbr(buf1, "room") && level >= kLvlBuilder) {
		int vnum, rnum = kNowhere;
		if (*buf2 && (vnum = atoi(buf2))) {
			if ((rnum = real_room(vnum)) != kNowhere)
				do_stat_room(ch, rnum);
			else
				SendMsgToChar("Состояние какой комнаты?\r\n", ch);
		}
		if (!*buf2)
			do_stat_room(ch);
	} else if (utils::IsAbbr(buf1, "mob") && level >= kLvlBuilder) {
		if (!*buf2)
			SendMsgToChar("Состояние какого создания?\r\n", ch);
		else {
			if ((victim = get_char_vis(ch, buf2, EFind::kCharInWorld)) != nullptr)
				do_stat_character(ch, victim, 0);
			else
				SendMsgToChar("Нет такого создания в этом МАДе.\r\n", ch);
		}
	} else if (utils::IsAbbr(buf1, "player")) {
		if (!*buf2) {
			SendMsgToChar("Состояние какого игрока?\r\n", ch);
		} else {
			if ((victim = get_player_vis(ch, buf2, EFind::kCharInWorld)) != nullptr)
				do_stat_character(ch, victim);
			else
				SendMsgToChar("Этого персонажа сейчас нет в игре.\r\n", ch);
		}
	} else if (utils::IsAbbr(buf1, "ip")) {
		if (!*buf2) {
			SendMsgToChar("Состояние ip какого игрока?\r\n", ch);
		} else {
			if ((victim = get_player_vis(ch, buf2, EFind::kCharInWorld)) != nullptr) {
				do_statip(ch, victim);
				return;
			} else {
				SendMsgToChar("Этого персонажа сейчас нет в игре, смотрим пфайл.\r\n", ch);
			}

			Player t_vict;
			if (load_char(buf2, &t_vict) > -1) {
				//Clan::SetClanData(&t_vict); не понял зачем проставлять клановый статус тут?
				do_statip(ch, &t_vict);
			} else {
				SendMsgToChar("Такого игрока нет ВООБЩЕ.\r\n", ch);
			}
		}
	} else if (utils::IsAbbr(buf1, "file")) {
		if (!*buf2) {
			SendMsgToChar("Состояние какого игрока(из файла)?\r\n", ch);
		} else {
			Player t_vict;
			if (load_char(buf2, &t_vict) > -1) {
				if (GetRealLevel(&t_vict) > level) {
					SendMsgToChar("Извините, вам это еще рано.\r\n", ch);
				} else {
					Clan::SetClanData(&t_vict);
					do_stat_character(ch, &t_vict);
				}
			} else {
				SendMsgToChar("Такого игрока нет ВООБЩЕ.\r\n", ch);
			}
		}
	} else if (utils::IsAbbr(buf1, "object") && level >= kLvlBuilder) {
		if (!*buf2)
			SendMsgToChar("Состояние какого предмета?\r\n", ch);
		else {
			if ((object = get_obj_vis(ch, buf2)) != nullptr)
				do_stat_object(ch, object);
			else
				SendMsgToChar("Нет такого предмета в игре.\r\n", ch);
		}
	} else {
		if (level >= kLvlBuilder) {
			if ((object = get_object_in_equip_vis(ch, buf1, ch->equipment, &tmp)) != nullptr)
				do_stat_object(ch, object);
			else if ((object = get_obj_in_list_vis(ch, buf1, ch->carrying)) != nullptr)
				do_stat_object(ch, object);
			else if ((victim = get_char_vis(ch, buf1, EFind::kCharInRoom)) != nullptr)
				do_stat_character(ch, victim);
			else if ((object = get_obj_in_list_vis(ch, buf1, world[ch->in_room]->contents)) != nullptr)
				do_stat_object(ch, object);
			else if ((victim = get_char_vis(ch, buf1, EFind::kCharInWorld)) != nullptr)
				do_stat_character(ch, victim);
			else if ((object = get_obj_vis(ch, buf1)) != nullptr)
				do_stat_object(ch, object);
			else
				SendMsgToChar("Ничего похожего с этим именем нет.\r\n", ch);
		} else {
			if ((victim = get_player_vis(ch, buf1, EFind::kCharInRoom)) != nullptr)
				do_stat_character(ch, victim);
			else if ((victim = get_player_vis(ch, buf1, EFind::kCharInWorld)) != nullptr)
				do_stat_character(ch, victim);
			else
				SendMsgToChar("Никого похожего с этим именем нет.\r\n", ch);
		}
	}
}
