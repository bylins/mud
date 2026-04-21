#include "do_stat.h"

#include "administration/ban.h"
#include "engine/entities/char_player.h"
#include "gameplay/mechanics/player_races.h"
#include "engine/core/utils_char_obj.inl"
#include "engine/db/description.h"
#include "gameplay/fight/fight_hit.h"
#include "engine/olc/olc.h"
#include "gameplay/mechanics/glory.h"
#include "gameplay/mechanics/glory_const.h"
#include "gameplay/ai/graph.h"
#include "gameplay/clans/house.h"
#include "gameplay/mechanics/liquid.h"
#include "engine/db/obj_prototypes.h"
#include "engine/ui/color.h"
#include "gameplay/statistics/mob_stat.h"
#include "engine/ui/modify.h"
#include "engine/db/global_objects.h"
#include "gameplay/mechanics/depot.h"
#include "gameplay/magic/magic.h"
#include "gameplay/mechanics/noob.h"
#include "administration/privilege.h"
#include "gameplay/mechanics/stable_objs.h"
#include "gameplay/economics/ext_money.h"
#include "administration/proxy.h"
#include "gameplay/ai/spec_procs.h"
#include "gameplay/mechanics/weather.h"
#include "gameplay/core/game_limits.h"
#include "gameplay/economics/exchange.h"
#include "gameplay/communication/parcel.h"
#include "gameplay/mechanics/armor.h"
#include "engine/db/player_index.h"

#include <third_party_libs/fmt/include/fmt/format.h>

extern char *diag_weapon_to_char(const CObjectPrototype *obj, int show_wear);

std::string print_special(CharData *mob) {
	std::string out;

	if (mob_index[mob->get_rnum()].func) {
		auto func = mob_index[mob->get_rnum()].func;
		if (func == shop_ext)
			out += "торговец";
		else if (func == receptionist)
			out += "рентер";
		else if (func == postmaster)
			out += "почтальон";
		else if (func == bank)
			out += "банкир";
		else if (func == exchange)
			out += "зазывала";
		else if (func == horse_keeper)
			out += "конюх";
		else if (func == guilds::GuildInfo::DoGuildLearn)
			out += "учитель";
		else if (func == torc)
			out += "меняла";
		else if (func == Noob::outfit)
			out += "нубхелпер";
		else if (func == mercenary)
			out += "ватажник";
		else
			out += "глюк";
	} else {
		out += "нет";
	}
	return out;
}


void do_statip(CharData *ch, CharData *k) {
	log("Start logon list stat");

	// Отображаем список ip-адресов с которых персонаж входил
	if (!LOGON_LIST(k).empty()) {
		// update: логон-лист может быть капитально большим, поэтому пишем это в свой дин.буфер, а не в buf2
		// заодно будет постраничный вывод ип, чтобы имма не посылало на йух с **OVERFLOW**
		std::ostringstream out("Персонаж заходил с IP-адресов:\r\n");
		for (const auto &logon : LOGON_LIST(k)) {
			snprintf(buf1, sizeof(buf1),
					"%16s %5ld %20s%s\r\n",
					logon.ip.c_str(),
					logon.count,
					rustime(localtime(&logon.lasttime)),
					logon.is_first ? " (создание)" : "");

			out << buf1;
		}
		page_string(ch->desc, out.str());
	}
	log("End logon list stat");
}

void DoStatKarma(CharData *ch, CharData *victim) {
	std::stringstream ss;
	if (!KARMA(victim)) {
		SendMsgToChar(ch, "Карма у игрока %s отсутствует.\r\n", GET_NAME(victim));
		return;
	}
	ss << "Карма игрока " << GET_NAME(victim) << ":\r\n" << KARMA(victim);
	SendMsgToChar(ss.str(), ch);
}

void do_stat_character(CharData *ch, CharData *k, const int virt = 0) {
	int i, i2;
	ObjData *j;
	char tmpbuf[128];
	buf[0] = 0;
	int god_level = ch->IsFlagged(EPrf::kCoderinfo) ? kLvlImplementator : GetRealLevel(ch);
	int k_room = -1;
	if (!virt && (god_level == kLvlImplementator || (god_level == kLvlGreatGod && !k->IsNpc()))) {
		k_room = GET_ROOM_VNUM(k->in_room);
	}

	sprinttype(to_underlying(k->get_sex()), genders, tmpbuf);
	if (k->IsNpc()) {
		sprinttype(GET_RACE(k) - ENpcRace::kBasic, npc_race_types, smallBuf);
		snprintf(buf, sizeof(buf), "%s %s ", tmpbuf, smallBuf);
	}
	snprintf(buf2, sizeof(buf2),
			"%s '%s' В комнате [%d] Текущий UID:[%ld]",
			(!k->IsNpc() ? "PC" : "MOB"),
			GET_NAME(k),
			k_room,
			k->get_uid());
	strncat(buf, buf2, sizeof(buf) - strlen(buf) - 1);
	SendMsgToChar(buf, ch);
	SendMsgToChar(ch, " ЛАГ: [%d]\r\n", k->get_wait());
	if (k->IsNpc()) {
		snprintf(buf, sizeof(buf),
				"Синонимы: &S%s&s, VNum: [%5d], RNum: [%5d]\r\n",
				k->GetCharAliases().c_str(),
				GET_MOB_VNUM(k),
				k->get_rnum());
		SendMsgToChar(buf, ch);
	}

	snprintf(buf, sizeof(buf),
			"Падежи: %s/%s/%s/%s/%s/%s ",
			GET_PAD(k, 0),
			GET_PAD(k, 1),
			GET_PAD(k, 2),
			GET_PAD(k, 3),
			GET_PAD(k, 4),
			GET_PAD(k, 5));
	SendMsgToChar(buf, ch);

	if (!k->IsNpc()) {

		if (!(k)->player_specials->saved.NameGod) {
			snprintf(buf, sizeof(buf), "Имя никем не одобрено!\r\n");
			SendMsgToChar(buf, ch);
		} else if ((k)->player_specials->saved.NameGod < 1000) {
			snprintf(buf, sizeof(buf), "Имя запрещено! - %s\r\n", GetNameById((k)->player_specials->saved.NameIDGod).c_str());
			SendMsgToChar(buf, ch);
		} else {
			snprintf(buf, sizeof(buf), "Имя одобрено! - %s\r\n", GetNameById((k)->player_specials->saved.NameIDGod).c_str());
			SendMsgToChar(buf, ch);
		}

		snprintf(buf, sizeof(buf), "Вероисповедание: %s\r\n", religion_name[(int) GET_RELIGION(k)][(int) k->get_sex()]);
		SendMsgToChar(buf, ch);

		std::string file_name = GET_NAME(k);
		CreateFileName(file_name);
		snprintf(buf, sizeof(buf), "E-mail: &S%s&s File: %s\r\n", GET_EMAIL(k), file_name.c_str());
		SendMsgToChar(buf, ch);

		std::string text = RegisterSystem::ShowComment(GET_EMAIL(k));
		if (!text.empty())
			SendMsgToChar(ch, "Registered by email from %s\r\n", text.c_str());

		if (k->player_specials->saved.telegram_id != 0)
			SendMsgToChar(ch, "Подключен Телеграм, chat_id: %lu\r\n", k->player_specials->saved.telegram_id);

		if (k->IsFlagged(EPlrFlag::kFrozen) && FREEZE_DURATION(k)) {
			snprintf(buf, sizeof(buf), "Заморожен : %ld час [%s].\r\n",
					static_cast<long>((FREEZE_DURATION(k) - time(nullptr)) / 3600),
					FREEZE_REASON(k) ? FREEZE_REASON(k) : "-");
			SendMsgToChar(buf, ch);
		}
		if (k->IsFlagged(EPlrFlag::kHelled) && HELL_DURATION(k)) {
			snprintf(buf, sizeof(buf), "Находится в темнице : %ld час [%s].\r\n",
					static_cast<long>((HELL_DURATION(k) - time(nullptr)) / 3600),
					HELL_REASON(k) ? HELL_REASON(k) : "-");
			SendMsgToChar(buf, ch);
		}
		if (k->IsFlagged(EPlrFlag::kNameDenied) && NAME_DURATION(k)) {
			snprintf(buf, sizeof(buf), "Находится в комнате имени : %ld час.\r\n",
					static_cast<long>((NAME_DURATION(k) - time(nullptr)) / 3600));
			SendMsgToChar(buf, ch);
		}
		if (k->IsFlagged(EPlrFlag::kMuted) && MUTE_DURATION(k)) {
			snprintf(buf, sizeof(buf), "Будет молчать : %ld час [%s].\r\n",
					static_cast<long>((MUTE_DURATION(k) - time(nullptr)) / 3600),
					MUTE_REASON(k) ? MUTE_REASON(k) : "-");
			SendMsgToChar(buf, ch);
		}
		if (k->IsFlagged(EPlrFlag::kDumbed) && DUMB_DURATION(k)) {
			snprintf(buf, sizeof(buf), "Будет нем : %ld мин [%s].\r\n",
					static_cast<long>((DUMB_DURATION(k) - time(nullptr)) / 60),
					DUMB_REASON(k) ? DUMB_REASON(k) : "-");
			SendMsgToChar(buf, ch);
		}
		if (!k->IsFlagged(EPlrFlag::kRegistred) && UNREG_DURATION(k)) {
			snprintf(buf, sizeof(buf), "Не будет зарегистрирован : %ld час [%s].\r\n",
					static_cast<long>((UNREG_DURATION(k) - time(nullptr)) / 3600),
					UNREG_REASON(k) ? UNREG_REASON(k) : "-");
			SendMsgToChar(buf, ch);
		}

		if (GET_GOD_FLAG(k, EGf::kGodsLike) && GCURSE_DURATION(k)) {
			snprintf(buf, sizeof(buf), "Под защитой Богов : %ld час.\r\n",
					static_cast<long>((GCURSE_DURATION(k) - time(nullptr)) / 3600));
			SendMsgToChar(buf, ch);
		}
		if (GET_GOD_FLAG(k, EGf::kGodscurse) && GCURSE_DURATION(k)) {
			snprintf(buf, sizeof(buf), "Проклят Богами : %ld час.\r\n",
					static_cast<long>((GCURSE_DURATION(k) - time(nullptr)) / 3600));
			SendMsgToChar(buf, ch);
		}
	}

	const auto &title = k->GetTitleStr();
	snprintf(buf, sizeof(buf), "Титул: %s\r\n", (title.empty() ? "<Нет>" : title.c_str()));
	SendMsgToChar(buf, ch);
	if (k->IsNpc())
		snprintf(buf, sizeof(buf), "L-Des: %s", (!k->player_data.long_descr.empty() ? k->player_data.long_descr.c_str() : "<Нет>\r\n"));
	else
		snprintf(buf, sizeof(buf),
				"L-Des: %s",
				(!k->player_data.description.empty() ? k->player_data.description.c_str() : "<Нет>\r\n"));
	SendMsgToChar(buf, ch);

	if (!k->IsNpc()) {
		snprintf(smallBuf, sizeof(smallBuf), "%s", MUD::Class(k->GetClass()).GetCName());
		snprintf(buf, sizeof(buf), "Племя: %s, Род: %s, Профессия: %s",
				PlayerRace::GetKinNameByNum(GET_KIN(k), k->get_sex()).c_str(),
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
		SendMsgToChar(ch, "Роли NPC: %s%s%s", kColorCyn, str.c_str(), kColorNrm);
	}

	char tmp_buf[256];
	if (k->get_zone_group() > 1) {
		snprintf(tmp_buf, sizeof(tmp_buf), " : групповой %ldx%d",
				 k->get_exp() / k->get_zone_group(), k->get_zone_group());
	} else {
		tmp_buf[0] = '\0';
	}

	snprintf(buf, sizeof(buf), ", Уровень: [%s%2d%s], Опыт: [%s%10ld%s]%s, Наклонности: [%4d]\r\n",
			kColorYel, GetRealLevel(k), kColorNrm, kColorYel,
			k->get_exp(), kColorNrm, tmp_buf, GET_ALIGNMENT(k));

	SendMsgToChar(buf, ch);

	if (!k->IsNpc()) {
		if (CLAN(k)) {
			SendMsgToChar(ch, "Статус дружины: %s\r\n", GET_CLAN_STATUS(k));
		}

		//added by WorM когда статишь файл собсно показывалось текущее время а не время последнего входа
		time_t ltime = GetLastlogonByUnique(k->get_uid());
		char t1[11];
		char t2[11];
		strftime(t1, sizeof(t1), "%d-%m-%Y", localtime(&(k->player_data.time.birth)));
		strftime(t2, sizeof(t2), "%d-%m-%Y", localtime(&ltime));
		t1[10] = t2[10] = '\0';

		snprintf(buf, sizeof(buf),
				"Создан: [%s] Последний вход: [%s] Играл: [%dh %dm] Возраст: [%d]\r\n",
				t1, t2, k->player_data.time.played / 3600, ((k->player_data.time.played % 3600) / 60), CalcCharAge(k)->year);
		SendMsgToChar(buf, ch);

		k->add_today_torc(0);
		snprintf(buf, sizeof(buf), "Рента: [%d], Денег: [%9ld], В банке: [%9ld] (Всего: %ld), Гривны: %d/%d/%d %d, Ногат: %d",
				GET_LOADROOM(k), k->get_gold(), k->get_bank(), k->get_total_gold(),
				k->get_ext_money(ExtMoney::kTorcGold),
				k->get_ext_money(ExtMoney::kTorcSilver),
				k->get_ext_money(ExtMoney::kTorcBronze),
				k->get_hryvn(), k->get_nogata());

		//. Display OLC zone for immorts .
		if (GetRealLevel(ch) >= kLvlImmortal) {
			snprintf(buf1, sizeof(buf1), ", %sOLC[%d]%s", kColorGrn, GET_OLC_ZONE(k), kColorNrm);
			strncat(buf, buf1, sizeof(buf) - strlen(buf) - 1);
		}
		strncat(buf, "\r\n", sizeof(buf) - strlen(buf) - 1);
		SendMsgToChar(buf, ch);
	} else {
		int mob_online = mob_index[k->get_rnum()].total_online - (virt ? 1 : 0);
		snprintf(buf, sizeof(buf), "Сейчас в мире : %d, макс %d. ", mob_online, mob_index[k->get_rnum()].stored);
		SendMsgToChar(buf, ch);
		std::string stats;
		mob_stat::GetLastMobKill(k, stats);
		snprintf(buf, sizeof(buf), "Последний раз убит: &r%s&n", stats.c_str());
		SendMsgToChar(buf, ch);
	}
	snprintf(buf, sizeof(buf),
			"Сила: [%s%d/%d%s]  Инт : [%s%d/%d%s]  Мудр : [%s%d/%d%s] \r\n"
			"Ловк: [%s%d/%d%s]  Тело:[%s%d/%d%s]  Обаян:[%s%d/%d%s] Размер: [%s%d/%d%s]\r\n",
			kColorCyn, k->GetInbornStr(), GetRealStr(k), kColorNrm,
			kColorCyn, k->GetInbornInt(), GetRealInt(k), kColorNrm,
			kColorCyn, k->GetInbornWis(), GetRealWis(k), kColorNrm,
			kColorCyn, k->GetInbornDex(), GetRealDex(k), kColorNrm,
			kColorCyn, k->GetInbornCon(), GetRealCon(k), kColorNrm,
			kColorCyn, k->GetInbornCha(), GetRealCha(k), kColorNrm,
			kColorCyn, GET_SIZE(k), GET_REAL_SIZE(k), kColorNrm);
	SendMsgToChar(buf, ch);

	snprintf(buf, sizeof(buf), "Жизни :[%s%d/%d+%d%s]  Энергии :[%s%d/%d+%d%s]",
			kColorGrn, k->get_hit(), k->get_real_max_hit(), hit_gain(k),
			kColorNrm, kColorGrn, k->get_move(), k->get_real_max_move(), move_gain(k), kColorNrm);
	SendMsgToChar(buf, ch);
	if (IS_MANA_CASTER(k)) {
		snprintf(buf, sizeof(buf), " Мана :[%s%d/%d+%d%s]\r\n",
				kColorGrn, k->mem_queue.stored, mana[MIN(50, GetRealWis(k))], CalcManaGain(k), kColorNrm);
	} else {
		snprintf(buf, sizeof(buf), "\r\n");
	}
	SendMsgToChar(buf, ch);

	snprintf(buf, sizeof(buf),
			"Glory: [%d], ConstGlory: [%d], AC: [%d/%d(%d)], Броня: [%d], Попадания: [%2d/%2d/%d], Повреждения: [%2d/%2d/%d]\r\n",
			Glory::get_glory(k->get_uid()),
			GloryConst::get_glory(k->get_uid()),
			GET_AC(k),
			GetRealAc(k),
			CalcBaseAc(k),
			GET_ARMOUR(k),
			GET_HR(k),
			GET_REAL_HR(k),
			GET_REAL_HR(k) + str_bonus(GetRealStr(k), STR_TO_HIT),
			GET_DR(k),
			GetRealDamroll(k),
			GetRealDamroll(k) + str_bonus(GetRealStr(k), STR_TO_DAM));
	SendMsgToChar(buf, ch);
	snprintf(buf, sizeof(buf),
			"Защитн.аффекты: (базовые) [Will:%d/Crit.:%d/Stab.:%d/Reflex:%d], (полные) Поглощ: [%d], Воля: [%d], Здор.: [%d], Стойк.: [%d], Реакц.: [%d]\r\n",
			GetBasicSave(k, ESaving::kWill),
			GetBasicSave(k, ESaving::kCritical),
			GetBasicSave(k, ESaving::kStability),
			GetBasicSave(k, ESaving::kReflex),
			GET_ABSORBE(k),
			CalcSaving(k, k, ESaving::kWill, 0),
			CalcSaving(k, k, ESaving::kCritical, 0),
			CalcSaving(k, k, ESaving::kStability, 0),
			CalcSaving(k, k, ESaving::kReflex, 0));
	SendMsgToChar(buf, ch);
	snprintf(buf, sizeof(buf),
			"Резисты: [Огонь:%d/Воздух:%d/Вода:%d/Земля:%d/Жизнь:%d/Разум:%d/Иммунитет:%d/Тьма:%d]\r\n",
			GET_RESIST(k, 0), GET_RESIST(k, 1), GET_RESIST(k, 2), GET_RESIST(k, 3),
			GET_RESIST(k, 4), GET_RESIST(k, 5), GET_RESIST(k, 6), GET_RESIST(k, 7));
	SendMsgToChar(buf, ch);
	snprintf(buf, sizeof(buf),
			"Защита от маг. аффектов: [%d], Защита от маг. урона: [%d], Защита от физ. урона: [%d], Маг.урон: [%d], Физ.урон: [%d]\r\n",
			GET_AR(k),
			GET_MR(k),
			GET_PR(k),
			k->add_abils.percent_spellpower_add,
			k->add_abils.percent_physdam_add);
	SendMsgToChar(buf, ch);
	snprintf(buf, sizeof(buf),
			"Запом: [%d], УспехКолд: [%d], ВоссЖиз: [%d], ВоссСил: [%d], Поглощ: [%d], Удача: [%d], Иниц: [%d]\r\n",
			GET_MANAREG(k),
			GET_CAST_SUCCESS(k),
			k->get_hitreg(),
			k->get_movereg(),
			GET_ABSORBE(k),
			k->calc_morale(),
			GET_INITIATIVE(k));
	SendMsgToChar(buf, ch);

	sprinttype(static_cast<int>(k->GetPosition()), position_types, smallBuf);
	snprintf(buf, sizeof(buf), "Положение: %s, Сражается: %s, Экипирован в металл: %s",
			smallBuf, (k->GetEnemy() ? GET_NAME(k->GetEnemy()) : "Нет"), (IsEquipInMetall(k) ? "Да" : "Нет"));

	if (k->IsNpc()) {
		strncat(buf, ", Тип атаки: ", sizeof(buf) - strlen(buf) - 1);
		strncat(buf, attack_hit_text[k->mob_specials.attack_type].singular, sizeof(buf) - strlen(buf) - 1);
	}
	if (k->desc) {
		snprintf(buf2, sizeof(buf2), "%s", GetConDescription(k->desc->state));
		strncat(buf, ", Соединение: ", sizeof(buf) - strlen(buf) - 1);
		strncat(buf, buf2, sizeof(buf) - strlen(buf) - 1);
	}
	strncat(buf, "\r\n", sizeof(buf) - strlen(buf) - 1);
	SendMsgToChar(buf, ch);

	snprintf(buf, sizeof(buf), "Позиция по умолчанию: ");
	sprinttype(static_cast<int>(k->mob_specials.default_pos), position_types, buf2);
	strncat(buf, buf2, sizeof(buf) - strlen(buf) - 1);
	if (k->char_specials.timer > 0) {
		snprintf(buf2, sizeof(buf2), ", Таймер отсоединения (тиков) [%d]\r\n", k->char_specials.timer);
		strncat(buf, buf2, sizeof(buf) - strlen(buf) - 1);
	} else if (k->extract_timer > 0) {
		snprintf(buf2, sizeof(buf2), ", Extract timer [%d]\r\n", k->extract_timer);
		strncat(buf, buf2, sizeof(buf) - strlen(buf) - 1);
	} else {
		snprintf(buf2, sizeof(buf2), "\r\n");
		strncat(buf, buf2, sizeof(buf) - strlen(buf) - 1);
	}
	SendMsgToChar(buf, ch);

	if (k->IsNpc()) {
		k->char_specials.saved.act.sprintbits(action_bits, smallBuf, sizeof(smallBuf), ",", 4);
		snprintf(buf, sizeof(buf), "MOB флаги: %s%s%s\r\n", kColorCyn, smallBuf, kColorNrm);
		SendMsgToChar(buf, ch);
		k->mob_specials.npc_flags.sprintbits(function_bits, smallBuf, sizeof(smallBuf), ",", 4);
		snprintf(buf, sizeof(buf), "NPC флаги: %s%s%s\r\n", kColorCyn, smallBuf, kColorNrm);
		SendMsgToChar(buf, ch);
		SendMsgToChar(ch,
					  "Количество атак: %s%d%s. ",
					  kColorCyn,
					  k->mob_specials.extra_attack + 1,
					  kColorNrm);
		SendMsgToChar(ch,
					  "Вероятность использования умений: %s%d%%%s. ",
					  kColorCyn,
					  k->mob_specials.like_work,
					  kColorNrm);
		SendMsgToChar(ch,
					  "Убить до начала замакса: %s%d%s\r\n",
					  kColorCyn,
					  k->mob_specials.MaxFactor,
					  kColorNrm);
		SendMsgToChar(ch, "&GУмения:&c");
		for (const auto &skill : MUD::Skills()) {
			if (skill.IsValid() && k->GetSkill(skill.GetId())) {
				SendMsgToChar(ch, " %s:[%d]", skill.GetName(), k->GetSkill(skill.GetId()));
			}
		}
		SendMsgToChar(ch, "\r\n");
		if (!k->mob_specials.have_spell) {
			SendMsgToChar(ch, "&GЗаклинания: &Rнет&n");
		} else {
			SendMsgToChar(ch, "&GЗаклинания:&c");
			for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
				if (MUD::Spell(spell_id).IsUnavailable()) {
					continue;
				}
				if (GET_SPELL_MEM(k, spell_id)) {
					SendMsgToChar(ch, " %s:[%d]", MUD::Spell(spell_id).GetCName(), GET_SPELL_MEM(k, spell_id));
				}
			}
		}
		SendMsgToChar(ch, "\r\n");
		SendMsgToChar(ch, "&GСпособности:&c");
		for (const auto &feat : MUD::Feats()) {
			if (feat.IsInvalid()) {
				continue;
			}
			if (k->HaveFeat(feat.GetId())) {
				SendMsgToChar(ch, " '%s'", feat.GetCName());
			}
		}
		SendMsgToChar(kColorNrm, ch);
		SendMsgToChar(ch, "\r\n");
		// информация о маршруте моба
		if (k->mob_specials.dest_count > 0 && k->in_room != kNowhere) {
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
			RoomVnum current_room = world[k->in_room]->vnum;
			while (current_room != GET_DEST(k) && predictive_path_vnum_list.size() < max_path_size && current_room > kNowhere) {
				const auto direction = find_first_step(GetRoomRnum(current_room), GetRoomRnum(GET_DEST(k)), k);
				if (direction >= 0) {
					const auto exit_room_rnum = world[GetRoomRnum(current_room)]->dir_option[direction]->to_room();
					current_room = world[exit_room_rnum]->vnum;
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
						  kColorCyn,
						  str_dest_list.str().c_str(),
						  kColorNrm);
			if (!virt) {
				SendMsgToChar(ch,
							  "Предполагаемый маршрут: %s%s%s\r\n",
							  kColorCyn,
							  str_predictive_path.str().c_str(),
							  kColorNrm);
			}
		}
	} else {
		k->char_specials.saved.act.sprintbits(player_bits, smallBuf, sizeof(smallBuf), ",", 4);
		snprintf(buf, sizeof(buf), "PLR: %s%s%s\r\n", kColorCyn, smallBuf, kColorNrm);
		SendMsgToChar(buf, ch);

		k->player_specials->saved.pref.sprintbits(preference_bits, smallBuf, sizeof(smallBuf), ",", 4);
		snprintf(buf, sizeof(buf), "PRF: %s%s%s\r\n", kColorGrn, smallBuf, kColorNrm);
		SendMsgToChar(buf, ch);

		if (ch->IsImpl()) {
			sprintbitwd(k->player_specials->saved.GodsLike, godslike_bits, smallBuf, sizeof(smallBuf), ",");
			snprintf(buf, sizeof(buf), "GFL: %s%s%s\r\n", kColorCyn, smallBuf, kColorNrm);
			SendMsgToChar(buf, ch);
		}
	}

	if (k->IsNpc()) {

		snprintf(buf, sizeof(buf), "Mob СпецПроц: &R%s&n, NPC сила удара: %dd%d\r\n",
				print_special(k).c_str(),
				k->mob_specials.damnodice, k->mob_specials.damsizedice);
		SendMsgToChar(buf, ch);
	}
	snprintf(buf, sizeof(buf), "Несет - вес %d, предметов %d; ", k->GetCarryingWeight(), k->GetCarryingQuantity());

	for (i = 0, j = k->carrying; j; j = j->get_next_content(), i++);
	size_t buf_len = strlen(buf);
	snprintf(buf + buf_len, sizeof(buf) - buf_len, "(в инвентаре) : %d, ", i);

	for (i = 0, i2 = 0; i < EEquipPos::kNumEquipPos; i++) {
		if (GET_EQ(k, i)) {
			i2++;
		}
	}
	snprintf(buf2, sizeof(buf2), "(надето): %d\r\n", i2);
	strncat(buf, buf2, sizeof(buf) - strlen(buf) - 1);
	SendMsgToChar(buf, ch);

	if (!k->IsNpc()) {
		snprintf(buf, sizeof(buf), "Голод: %d, Жажда: %d, Опьянение: %d\r\n",
				GET_COND(k, FULL), GET_COND(k, THIRST), GET_COND(k, DRUNK));
		SendMsgToChar(buf, ch);
	}

	if (god_level >= kLvlGreatGod) {
		std::string fl_str;

		SendMsgToChar(ch, "Ведущий: %s, Ведомые: ", (k->has_master() ? GET_NAME(k->get_master()) : "<нет>"));
		for (auto &it : k->followers) {
			if (!it->IsNpc()) { 
				fl_str += " " + it->get_name();
			} else {
				fl_str += " " + it->get_name() + "_#" + std::to_string(GET_MOB_VNUM(it)); 
			}
		}
		SendMsgToChar(ch, "%s\r\n", utils::OutWordsList(fl_str, ch->player_specials->saved.stringLength - 30).c_str());
		SendMsgToChar(ch, "Помогают: ");
		if (!k->summon_helpers.empty()) {
			fl_str.clear();
			for (auto &helper : k->summon_helpers) {
				fl_str += " " +  std::to_string(helper);
			}
			SendMsgToChar(ch, "%s\r\n", utils::OutWordsList(fl_str, ch->player_specials->saved.stringLength - 30).c_str());
		}
	}
	// Showing the bitvector
	k->char_specials.saved.affected_by.sprintbits(affected_bits, smallBuf, sizeof(smallBuf), ",", 4);
	snprintf(buf, sizeof(buf), "Аффекты: %s%s%s\r\n", kColorYel, smallBuf, kColorNrm);
	SendMsgToChar(buf, ch);
	snprintf(buf, sizeof(buf), "&GПеревоплощений: %d\r\n&n", GetRealRemort(k));
	SendMsgToChar(buf, ch);
	// Routine to show what spells a char is affected by
	if (!k->affected.empty()) {
		for (const auto &aff : k->affected) {
			*buf2 = '\0';
			snprintf(buf, sizeof(buf), "Заклинания: (%3d%s|%s) %s%-21s%s ", aff->duration + 1,
					(aff->battleflag & kAfPulsedec) || (aff->battleflag & kAfSameTime) ? "плс" : "мин",
					(aff->battleflag & kAfBattledec) || (aff->battleflag & kAfSameTime) ? "рнд" : "мин",
					kColorCyn, MUD::Spell(aff->type).GetCName(), kColorNrm);
			if (aff->modifier) {
				snprintf(buf2, sizeof(buf2), "%+d to %s", aff->modifier, apply_types[(int) aff->location]);
				strncat(buf, buf2, sizeof(buf) - strlen(buf) - 1);
			}
			if (aff->bitvector) {
				if (*buf2) {
					strncat(buf, ", sets ", sizeof(buf) - strlen(buf) - 1);
				} else {
					strncat(buf, "sets ", sizeof(buf) - strlen(buf) - 1);
				}
				sprintbit(aff->bitvector, affected_bits, buf2, sizeof(buf2));
				strncat(buf, buf2, sizeof(buf) - strlen(buf) - 1);
			}
			strncat(buf, "\r\n", sizeof(buf) - strlen(buf) - 1);
			SendMsgToChar(buf, ch);
		}
	}

	// check mobiles for a script
	if (k->IsNpc() && god_level >= kLvlBuilder) {
		do_sstat_character(ch, k);
		if (MEMORY(k)) {
			struct mob_ai::MemoryRecord *memchar;
			SendMsgToChar("Помнит:\r\n", ch);
			for (memchar = MEMORY(k); memchar; memchar = memchar->next) {
				snprintf(buf, sizeof(buf), "%10ld - %10ld\r\n",
						static_cast<long>(memchar->id),
						static_cast<long>(memchar->time - time(nullptr)));
				SendMsgToChar(buf, ch);
			}
		}
	} else        // this is a PC, display their global variables
	{
		if (!SCRIPT(k)->global_vars.empty()) {
			char name[kMaxInputLength];
			void find_uid_name(const char *uid, char *name, size_t name_size);
			SendMsgToChar("Глобальные переменные:\r\n", ch);
			// currently, variable context for players is always 0, so it is
			// not displayed here. in the future, this might change
			for (auto tv : k->script->global_vars) {
				if (tv.value[0] == UID_CHAR) {
					find_uid_name(tv.value.c_str(), name, sizeof(name));
					snprintf(buf, sizeof(buf), "    %10s:  [CharUID]: %s\r\n", tv.name.c_str(), name);
				} else if (tv.value[0] == UID_OBJ) {
					find_uid_name(tv.value.c_str(), name, sizeof(name));
					snprintf(buf, sizeof(buf), "    %10s:  [ObjUID]: %s\r\n", tv.name.c_str(), name);
				} else if (tv.value[0] == UID_ROOM) {
					find_uid_name(tv.value.c_str(), name, sizeof(name));
					snprintf(buf, sizeof(buf), "    %10s:  [RoomUID]: %s\r\n", tv.name.c_str(), name);
				} else
					snprintf(buf, sizeof(buf), "    %10s:  %s\r\n", tv.name.c_str(), tv.value.c_str());
				SendMsgToChar(buf, ch);
			}
		}

		std::string quested(k->quested_print());
		if (!quested.empty())
			SendMsgToChar(ch, "Выполнил квесты:\r\n%s\r\n", quested.c_str());

		if (NORENTABLE(k)) {
			snprintf(buf, sizeof(buf), "Не может уйти на постой %ld\r\n",
					static_cast<long int>(NORENTABLE(k) - time(nullptr)));
			SendMsgToChar(buf, ch);
		}
		if (AGRO(k)) {
			snprintf(buf, sizeof(buf), "Агрессор %ld\r\n", static_cast<long int>(AGRO(k) - time(nullptr)));
			SendMsgToChar(buf, ch);
		}
		pk_list_sprintf(k, buf);
		SendMsgToChar(buf, ch);
	}
}

void do_stat_object(CharData *ch, ObjData *j, const int virt = 0) {
	int i, found;
	ObjVnum rnum, vnum;
	ObjData *j2;
	long int li;
	bool is_grgod = (ch->IsGrGod() || ch->IsFlagged(EPrf::kCoderinfo)) ? true : false;

	vnum = GET_OBJ_VNUM(j);
	rnum = j->get_rnum();

	snprintf(buf, sizeof(buf), "Название: '%s%s%s',\r\nСинонимы: '&c%s&n',",
			kColorYel,
			(!j->get_short_description().empty() ? j->get_short_description().c_str() : "<None>"),
			kColorNrm,
			j->get_aliases().c_str());
	SendMsgToChar(buf, ch);
	if (j->get_custom_label() && j->get_custom_label()->text_label) {
		snprintf(buf, sizeof(buf), " нацарапано: '&c%s&n',", j->get_custom_label()->text_label);
		SendMsgToChar(buf, ch);
	}
	snprintf(buf, sizeof(buf), "\r\n");
	SendMsgToChar(buf, ch);
	sprinttype(j->get_type(), item_types, buf1);
	if (rnum >= 0) {
		snprintf(buf2, sizeof(buf2), "%s", (obj_proto.func(j->get_rnum()) ? "Есть" : "Нет"));
	} else {
		snprintf(buf2, sizeof(buf2), "None");
	}

	SendMsgToChar(ch, "VNum: [%s%5d%s], RNum: [%5d], UniqueID: [%ld], Id: [%ld]\r\n",
				  kColorGrn, vnum, kColorNrm, j->get_rnum(), j->get_unique_id(), j->get_id());

	SendMsgToChar(ch, "Расчет критерия: %f, мортов: (%f) \r\n", j->show_koef_obj(), j->show_mort_req());
	SendMsgToChar(ch, "Тип: %s, СпецПроцедура: %s", buf1, buf2);

	if (j->get_owner()) {
		auto tmpstr = GetPlayerNameByUnique(j->get_owner());
		SendMsgToChar(ch, ", Владелец : %s", tmpstr.empty() ? "УДАЛЕН": tmpstr.c_str());
	}
//	if (GET_OBJ_ZONE(j))
	SendMsgToChar(ch, ", Принадлежит зоне VNUM : %d", j->get_vnum_zone_from());
	if (j->get_crafter_uid()) {
		auto to_name = GetPlayerNameByUnique(j->get_crafter_uid());
		if (to_name.empty()) {
			SendMsgToChar(ch, ", Создатель : не найден");
		} else {
			SendMsgToChar(ch, ", Создатель : %s", to_name.c_str());
		}
	}
	if (obj_proto[j->get_rnum()]->get_parent_rnum() > -1) {
		SendMsgToChar(ch, ", Родитель(VNum) : [%d]", obj_proto[j->get_rnum()]->get_parent_vnum());
	}
	if (j->get_craft_timer() > 0) {
		SendMsgToChar(ch, ", &Yскрафчена с таймером : [%d]&n", j->get_craft_timer());
	}
	if (j->get_is_rename()) // изменены падежи
	{
		SendMsgToChar(ch, ", &Gпадежи отличны от прототипа&n");
	}
	SendMsgToChar(ch, "\r\n%s", diag_weapon_to_char(j, 2));
	snprintf(buf, sizeof(buf), "L-Des: %s\r\n%s",
			!j->get_description().empty() ? j->get_description().c_str() : "Нет",
			kColorNrm);
	SendMsgToChar(buf, ch);

	if (j->get_ex_description()) {
		snprintf(buf, sizeof(buf), "Экстра описание:%s", kColorCyn);
		for (auto desc = j->get_ex_description(); desc; desc = desc->next) {
			strncat(buf, " ", sizeof(buf) - strlen(buf) - 1);
			strncat(buf, desc->keyword, sizeof(buf) - strlen(buf) - 1);
		}
		strncat(buf, kColorNrm, sizeof(buf) - strlen(buf) - 1);
		strncat(buf, "\r\n", sizeof(buf) - strlen(buf) - 1);
		SendMsgToChar(buf, ch);
	}
	SendMsgToChar("Может быть надет : ", ch);
	sprintbit(j->get_wear_flags(), wear_bits, buf, sizeof(buf));
	strncat(buf, "\r\n", sizeof(buf) - strlen(buf) - 1);
	SendMsgToChar(buf, ch);
	sprinttype(j->get_material(), material_name, buf2);
	snprintf(buf, sizeof(buf), "Материал : %s, макс.прочность : %d, тек.прочность : %d\r\n",
			 buf2, j->get_maximum_durability(), j->get_current_durability());
	SendMsgToChar(buf, ch);

	SendMsgToChar("Неудобства : ", ch);
	j->get_no_flags().sprintbits(no_bits, buf, sizeof(buf), ",", 4);
	strncat(buf, "\r\n", sizeof(buf) - strlen(buf) - 1);
	SendMsgToChar(buf, ch);

	SendMsgToChar("Запреты : ", ch);
	j->get_anti_flags().sprintbits(anti_bits, buf, sizeof(buf), ",", 4);
	strncat(buf, "\r\n", sizeof(buf) - strlen(buf) - 1);
	SendMsgToChar(buf, ch);

	SendMsgToChar("Устанавливает аффекты : ", ch);
	j->get_affect_flags().sprintbits(weapon_affects, buf, sizeof(buf), ",", 4);
	strncat(buf, "\r\n", sizeof(buf) - strlen(buf) - 1);
	SendMsgToChar(buf, ch);

	SendMsgToChar("Дополнительные флаги  : ", ch);
	j->get_extra_flags().sprintbits(extra_bits, buf, sizeof(buf), ",", 4);
	strncat(buf, "\r\n", sizeof(buf) - strlen(buf) - 1);
	SendMsgToChar(buf, ch);

	snprintf(buf, sizeof(buf), "Вес: %d, Цена: %d, Рента(eq): %d, Рента(inv): %d, ",
			j->get_weight(), j->get_cost(), j->get_rent_on(), j->get_rent_off());
	SendMsgToChar(buf, ch);
	if (stable_objs::IsTimerUnlimited(j))
		snprintf(buf, sizeof(buf), "Таймер: нерушимо, ");
	else
		snprintf(buf, sizeof(buf), "Таймер: %d, ", j->get_timer());
	SendMsgToChar(buf, ch);
	snprintf(buf, sizeof(buf), "Таймер на земле: %d\r\n", j->get_destroyer());
	SendMsgToChar(buf, ch);
	std::string str;

	str = Parcel::FindParcelObj(j);
	if (str.empty()) {
		str = Clan::print_imm_where_obj(j);
	}
	if (str.empty()) {
		str = Depot::print_imm_where_obj(j);
	}
	if (str.empty()) {
		for (ExchangeItem *tmp_obj = exchange_item_list; tmp_obj; tmp_obj = tmp_obj->next) {
			if (GET_EXCHANGE_ITEM(tmp_obj)->get_unique_id() == j->get_unique_id()) {
				str =  fmt::format("продается на базаре, лот #{}\r\n", GET_EXCHANGE_ITEM_LOT(tmp_obj));
				break;
			}
		}
	}
	if (str.empty()) {
		for (const auto &shop : GlobalObjects::Shops()) {
			const auto tmp_obj = shop->GetObjFromShop(j->get_unique_id());
			if (!tmp_obj) {
				continue;
			}
			str = fmt::format("можно купить в магазине: {}\r\n", shop->GetDictionaryName());
		}
	}
	if (!str.empty()) {
		str[0] = UPPER(str[0]);
		SendMsgToChar(ch, "&C%s&n", str.c_str());
	} else {
		auto room = get_room_where_obj(j);

		snprintf(buf, sizeof(buf), "%s", "&CНаходится в комнате : ");
		if (room == kNowhere || !is_grgod) {
			strncat(buf, "нигде", sizeof(buf) - strlen(buf) - 1);
		} else {
			snprintf(buf2, sizeof(buf2), "%d", room);
			strncat(buf, buf2, sizeof(buf) - strlen(buf) - 1);
		}
		strncat(buf, ", В контейнере: ", sizeof(buf) - strlen(buf) - 1);
		if (j->get_in_obj() && is_grgod) {
			snprintf(buf2, sizeof(buf2), "[%d] %s", GET_OBJ_VNUM(j->get_in_obj()), j->get_in_obj()->get_short_description().c_str());
			strncat(buf, buf2, sizeof(buf) - strlen(buf) - 1);
		} else {
			strncat(buf, "Нет", sizeof(buf) - strlen(buf) - 1);
		}
		strncat(buf, ", В инвентаре: ", sizeof(buf) - strlen(buf) - 1);
		if (j->get_carried_by() && is_grgod) {
			strncat(buf, GET_NAME(j->get_carried_by()), sizeof(buf) - strlen(buf) - 1);
		} else if (j->get_in_obj() && j->get_in_obj()->get_carried_by() && is_grgod) {
			strncat(buf, GET_NAME(j->get_in_obj()->get_carried_by()), sizeof(buf) - strlen(buf) - 1);
		} else {
			strncat(buf, "Нет", sizeof(buf) - strlen(buf) - 1);
		}
		strncat(buf, ", Надет: ", sizeof(buf) - strlen(buf) - 1);
		if (j->get_worn_by() && is_grgod) {
			strncat(buf, GET_NAME(j->get_worn_by()), sizeof(buf) - strlen(buf) - 1);
		} else if (j->get_in_obj() && j->get_in_obj()->get_worn_by() && is_grgod) {
			strncat(buf, GET_NAME(j->get_in_obj()->get_worn_by()), sizeof(buf) - strlen(buf) - 1);
		} else {
			strncat(buf, "Нет", sizeof(buf) - strlen(buf) - 1);
		}
		strncat(buf, "\r\n&n", sizeof(buf) - strlen(buf) - 1);
		SendMsgToChar(buf, ch);
	}

	switch (j->get_type()) {
		case EObjType::kBook:

			switch (GET_OBJ_VAL(j, 0)) {
				case EBook::kSpell: {
					auto spell_id = static_cast<ESpell>(GET_OBJ_VAL(j, 1));
					if (spell_id >= ESpell::kFirst && spell_id <= ESpell::kLast) {
						snprintf(buf, sizeof(buf), "содержит заклинание        : \"%s\"", MUD::Spell(spell_id).GetCName());
					} else
						snprintf(buf, sizeof(buf), "неверный номер заклинания");
					break;
				}
				case EBook::kSkill: {
					auto skill_id = static_cast<ESkill>(GET_OBJ_VAL(j, 1));
					if (MUD::Skill(skill_id).IsValid()) {
						snprintf(buf, sizeof(buf), "содержит секрет умения     : \"%s\"", MUD::Skill(skill_id).GetName());
					} else
						snprintf(buf, sizeof(buf), "неверный номер умения");
					break;
				}
				case EBook::kSkillUpgrade: {
					auto skill_id = static_cast<ESkill>(GET_OBJ_VAL(j, 1));
					if (MUD::Skills().IsValid(skill_id)) {
						if (GET_OBJ_VAL(j, 3) > 0) {
							snprintf(buf, sizeof(buf), "повышает умение \"%s\" (максимум %d)",
									MUD::Skill(skill_id).GetName(), GET_OBJ_VAL(j, 3));
						} else {
							snprintf(buf, sizeof(buf), "повышает умение \"%s\" (не больше максимума текущего перевоплощения)",
									MUD::Skill(skill_id).GetName());
						}
					} else {
						snprintf(buf, sizeof(buf), "неверный номер повышаемого умения");
					}
				}
					break;
				case EBook::kFeat: {
					const auto feat_id = static_cast<EFeat>(GET_OBJ_VAL(j, 1));
					if (MUD::Feat(feat_id).IsValid()) {
						snprintf(buf, sizeof(buf), "содержит секрет способности : \"%s\"", MUD::Feat(feat_id).GetCName());
					} else {
						snprintf(buf, sizeof(buf), "неверный номер способности");
					}
				}
					break;
				case EBook::kReceipt: {
					const auto recipe = im_get_recipe(GET_OBJ_VAL(j, 1));
					if (recipe >= 0) {
						const auto recipelevel = std::max(GET_OBJ_VAL(j, 2), imrecipes[recipe].level);
						const auto recipemort = imrecipes[recipe].remort;
						if ((recipelevel >= 0) && (recipemort >= 0)) {
							snprintf(buf, sizeof(buf),
									"содержит рецепт отвара     : \"%s\", уровень изучения: %d, количество ремортов: %d",
									imrecipes[recipe].name,
									recipelevel,
									recipemort);
						} else {
							snprintf(buf, sizeof(buf), "Некорректная запись рецепта (нет уровня или реморта)");
						}
					} else
						snprintf(buf, sizeof(buf), "Некорректная запись рецепта");
				}
					break;
				default:snprintf(buf, sizeof(buf), "НЕВЕРНО УКАЗАН ТИП КНИГИ");
					break;
			}
			break;
		case EObjType::kLightSource:
			if (GET_OBJ_VAL(j, 2) < 0) {
				snprintf(buf, sizeof(buf), "Вечный свет!");
			} else {
				snprintf(buf, sizeof(buf), "Осталось светить: [%d]", GET_OBJ_VAL(j, 2));
			}
			break;

		case EObjType::kScroll:
		case EObjType::kPotion: {
			std::ostringstream out;
			out << "Заклинания: (Уровень - " << GET_OBJ_VAL(j, 0) << ") ";
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
			snprintf(buf, sizeof(buf), "%s", out.str().c_str());
			break;
		}
		case EObjType::kWand:
		case EObjType::kStaff:
			snprintf(buf, sizeof(buf), "Заклинание: %s уровень %d, %d (из %d) зарядов осталось",
					MUD::Spell(static_cast<ESpell>(GET_OBJ_VAL(j, 3))).GetCName(),
					GET_OBJ_VAL(j, 0),
					GET_OBJ_VAL(j, 2),
					GET_OBJ_VAL(j, 1));
			break;

		case EObjType::kWeapon:
			snprintf(buf, sizeof(buf), "Повреждения: %dd%d, Тип повреждения: %d",
					GET_OBJ_VAL(j, 1),
					GET_OBJ_VAL(j, 2),
					GET_OBJ_VAL(j, 3));
			break;

		case EObjType::kArmor:
		case EObjType::kLightArmor:
		case EObjType::kMediumArmor:
		case EObjType::kHeavyArmor:snprintf(buf, sizeof(buf), "AC: [%d]  Броня: [%d]", GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1));
			break;

		case EObjType::kTrap:snprintf(buf, sizeof(buf), "Spell: %d, - Hitpoints: %d", GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1));
			break;

		case EObjType::kContainer:sprintbit(GET_OBJ_VAL(j, 1), container_bits, smallBuf, sizeof(smallBuf));
			if (IS_CORPSE(j)) {
				snprintf(buf, sizeof(buf), "Объем: %d, Тип ключа: %s, VNUM моба: %d, Труп: да",
						GET_OBJ_VAL(j, 0), smallBuf, GET_OBJ_VAL(j, 2));
			} else {
				snprintf(buf, sizeof(buf), "Объем: %d, Тип ключа: %s, Номер ключа: %d, Сложность замка: %d",
						GET_OBJ_VAL(j, 0), smallBuf, GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
			}
			break;

		case EObjType::kLiquidContainer:
		case EObjType::kFountain:sprinttype(GET_OBJ_VAL(j, 2), drinks, smallBuf);
			{
				std::string spells = drinkcon::print_spells(j);
				utils::Trim(spells);
				snprintf(buf, sizeof(buf), "Обьем: %d, Содержит: %d, Таймер (если 1 отравлено): %d, Жидкость: %s\r\n%s",
						GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 3), smallBuf, spells.c_str());
			}
			break;

		case EObjType::kNote:snprintf(buf, sizeof(buf), "Tongue: %d", GET_OBJ_VAL(j, 0));
			break;

		case EObjType::kKey:*buf = '\0';
			break;

		case EObjType::kFood:
			snprintf(buf, sizeof(buf),
					"Насыщает(час): %d, Таймер (если 1 отравлено): %d",
					GET_OBJ_VAL(j, 0),
					GET_OBJ_VAL(j, 3));
			break;

		case EObjType::kMoney:
			snprintf(buf, sizeof(buf), "Сумма: %d\r\nВалюта: %s", GET_OBJ_VAL(j, 0),
					GET_OBJ_VAL(j, 1) == currency::GOLD ? "куны" :
					GET_OBJ_VAL(j, 1) == currency::ICE ? "искристые снежинки" :
					"что-то другое"
			);
			break;

		case EObjType::kMagicIngredient:sprintbit(j->get_spec_param(), ingradient_bits, smallBuf, sizeof(smallBuf));
			snprintf(buf, sizeof(buf), "ingr bits %s", smallBuf);

			if (IS_SET(j->get_spec_param(), kItemCheckUses)) {
				size_t buf_len = strlen(buf);
				snprintf(buf + buf_len, sizeof(buf) - buf_len, "\r\nможно применить %d раз", GET_OBJ_VAL(j, 2));
			}

			if (IS_SET(j->get_spec_param(), kItemCheckLag)) {
				size_t buf_len = strlen(buf);
				snprintf(buf + buf_len, sizeof(buf) - buf_len, "\r\nможно применить 1 раз в %d сек", (i = GET_OBJ_VAL(j, 0) & 0xFF));
				buf_len = strlen(buf);
				if (GET_OBJ_VAL(j, 3) == 0 || GET_OBJ_VAL(j, 3) + i < time(nullptr)) {
					snprintf(buf + buf_len, sizeof(buf) - buf_len, "(можно применять).");
				} else {
					li = GET_OBJ_VAL(j, 3) + i - time(nullptr);
					snprintf(buf + buf_len, sizeof(buf) - buf_len, "(осталось %ld сек).", li);
				}
			}

			if (IS_SET(j->get_spec_param(), kItemCheckLevel)) {
				size_t buf_len = strlen(buf);
				snprintf(buf + buf_len, sizeof(buf) - buf_len, "\r\nможно применить с %d уровня.", (GET_OBJ_VAL(j, 0) >> 8) & 0x1F);
			}

			if ((i = GetObjRnum(GET_OBJ_VAL(j, 1))) >= 0) {
				size_t buf_len = strlen(buf);
				snprintf(buf + buf_len, sizeof(buf) - buf_len, "\r\nпрототип %s%s%s.",
						kColorBoldCyn, obj_proto[i]->get_PName(ECase::kNom).c_str(), kColorNrm);
			}
			break;
		case EObjType::kMagicContaner:
		case EObjType::kMagicArrow:
			snprintf(buf, sizeof(buf), "Заклинание: [%s]. Объем [%d]. Осталось стрел[%d].",
					MUD::Spell(static_cast<ESpell>(GET_OBJ_VAL(j, 0))).GetCName(),
					GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2));
			break;

		default:
			snprintf(buf, sizeof(buf), "Values 0-3: [%d] [%d] [%d] [%d]",
					GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
			break;
	}
	strncat(buf, "\r\n", sizeof(buf) - strlen(buf) - 1);
	SendMsgToChar(buf, ch);

	// * I deleted the "equipment status" code from here because it seemed
	// * more or less useless and just takes up valuable screen space.

	if (j->get_contains()) {
		snprintf(buf, sizeof(buf), "\r\nСодержит:%s", kColorGrn);
		for (found = 0, j2 = j->get_contains(); j2; j2 = j2->get_next_content()) {
			snprintf(buf2, sizeof(buf2), "%s %s", found++ ? "," : "", j2->get_short_description().c_str());
			strncat(buf, buf2, sizeof(buf) - strlen(buf) - 1);
			if (strlen(buf) >= 62) {
				if (j2->get_next_content())
					strncat(buf, ",\r\n", sizeof(buf) - strlen(buf) - 1);
				else
					strncat(buf, "\r\n", sizeof(buf) - strlen(buf) - 1);
				SendMsgToChar(buf, ch);
				*buf = found = 0;
			}
		}

		if (*buf) {
			strncat(buf, "\r\n", sizeof(buf) - strlen(buf) - 1);
			SendMsgToChar(buf, ch);
		}
		SendMsgToChar(kColorNrm, ch);
	}
	found = 0;
	SendMsgToChar("Аффекты:", ch);
	for (i = 0; i < kMaxObjAffect; i++) {
		if (j->get_affected(i).modifier) {
			sprinttype(j->get_affected(i).location, apply_types, smallBuf);
			snprintf(buf, sizeof(buf), "%s %+d to %s", found++ ? "," : "", j->get_affected(i).modifier, smallBuf);
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
			snprintf(buf, sizeof(buf), " %+d%% to %s", it.second, MUD::Skill(it.first).GetName());
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
	SendMsgToChar(ch, "Сохраненные переменные из DGScript: %s\r\n", j->get_dgscript_field().empty() ? "ничего" : j->get_dgscript_field().c_str());
	if (is_grgod) {
		snprintf(buf, sizeof(buf),
				"Сейчас в мире : %d. На постое : %d. Макс в мире: %d\r\n",
				rnum >= 0 ? obj_proto.total_online(rnum) - (virt ? 1 : 0) : -1,
				rnum >= 0 ? obj_proto.stored(rnum) : -1,
				GetObjMIW(j->get_rnum()));
		SendMsgToChar(buf, ch);
		// check the object for a script
		do_sstat_object(ch, j);
	}
}

void do_stat_room(CharData *ch, const int rnum = 0) {
	RoomData *rm = world[ch->in_room];
	int i, found;
	CharData *k;
	char tmpBuf[255];

	if (rnum != 0) {
		rm = world[rnum];
	}

	snprintf(buf, sizeof(buf), "Комната : %s%s%s\r\n", kColorCyn, rm->name, kColorNrm);
	SendMsgToChar(buf, ch);

	sprinttype(rm->sector_type, sector_types, smallBuf);
	snprintf(buf, sizeof(buf),
			"Зона: [%3d], VNum: [%s%5d%s], RNum: [%5d], Тип  сектора: %s\r\n",
			zone_table[rm->zone_rn].vnum, kColorGrn, rm->vnum, kColorNrm, rnum, smallBuf);
	SendMsgToChar(buf, ch);

	rm->flags_sprint(smallBuf, sizeof(smallBuf), ",");
	snprintf(buf, sizeof(buf), "СпецПроцедура: %s, Флаги: %s\r\n", (rm->func == nullptr) ? "None" : "Exists", smallBuf);
	SendMsgToChar(buf, ch);

	SendMsgToChar("Описание:\r\n", ch);
	SendMsgToChar(GlobalObjects::descriptions().get(rm->description_num), ch);

	if (rm->ex_description) {
		snprintf(buf, sizeof(buf), "Доп. описание:%s", kColorCyn);
		for (auto desc = rm->ex_description; desc; desc = desc->next) {
			strncat(buf, " ", sizeof(buf) - strlen(buf) - 1);
			strncat(buf, desc->keyword, sizeof(buf) - strlen(buf) - 1);
		}
		strncat(buf, kColorNrm, sizeof(buf) - strlen(buf) - 1);
		strncat(buf, "\r\n", sizeof(buf) - strlen(buf) - 1);
		SendMsgToChar(buf, ch);
	}
	snprintf(buf, sizeof(buf), "Живые существа:%s", kColorYel);
	found = 0;
	size_t counter = 0;
	for (auto k_i = rm->people.begin(); k_i != rm->people.end(); ++k_i) {
		const auto k = *k_i;
		++counter;

		if (!CAN_SEE(ch, k)) {
			continue;
		}
		snprintf(buf2, sizeof(buf2), "%s %s(%s)", found++ ? "," : "", GET_NAME(k),
				(!k->IsNpc() ? "PC" : (!k->IsNpc() ? "NPC" : "MOB")));
		strncat(buf, buf2, sizeof(buf) - strlen(buf) - 1);
		if (strlen(buf) >= 62) {
			if (counter != rm->people.size()) {
				strncat(buf, ",\r\n", sizeof(buf) - strlen(buf) - 1);
			} else {
				strncat(buf, "\r\n", sizeof(buf) - strlen(buf) - 1);
			}
			SendMsgToChar(buf, ch);
			*buf = found = 0;
		}
	}

	if (*buf) {
		strncat(buf, "\r\n", sizeof(buf) - strlen(buf) - 1);
		SendMsgToChar(buf, ch);
	}
	SendMsgToChar(kColorNrm, ch);

	if (!rm->contents.empty()) {
		snprintf(buf, sizeof(buf), "Предметы:%s", kColorGrn);
		found = 0;
		for (auto it = rm->contents.begin(); it != rm->contents.end(); ++it) {
			auto j = *it;
			if (!CAN_SEE_OBJ(ch, j))
				continue;
			snprintf(buf2, sizeof(buf2), "%s %s", found++ ? "," : "", j->get_short_description().c_str());
			strncat(buf, buf2, sizeof(buf) - strlen(buf) - 1);
			if (strlen(buf) >= 62) {
				if (std::next(it) != rm->contents.end()) {
					strncat(buf, ",\r\n", sizeof(buf) - strlen(buf) - 1);
				} else {
					strncat(buf, "\r\n", sizeof(buf) - strlen(buf) - 1);
				}
				SendMsgToChar(buf, ch);
				*buf = found = 0;
			}
		}

		if (*buf) {
			strncat(buf, "\r\n", sizeof(buf) - strlen(buf) - 1);
			SendMsgToChar(buf, ch);
		}
		SendMsgToChar(kColorNrm, ch);
	}
	for (i = 0; i < EDirection::kMaxDirNum; i++) {
		if (rm->dir_option[i]) {
			if (rm->dir_option[i]->to_room() == kNowhere)
				snprintf(smallBuf, sizeof(smallBuf), " %sNONE%s", kColorCyn, kColorNrm);
			else
				snprintf(smallBuf, sizeof(smallBuf), "%s%5d%s", kColorCyn,
						GET_ROOM_VNUM(rm->dir_option[i]->to_room()), kColorNrm);
			sprintbit(rm->dir_option[i]->exit_info, exit_bits, tmpBuf, sizeof(tmpBuf));
			snprintf(buf, sizeof(buf),
					"Выход %s%-5s%s:  Ведет в : [%s], Ключ: [%5d], Название: %s (%s), Тип: %s\r\n",
					kColorCyn, dirs[i], kColorNrm, smallBuf,
					rm->dir_option[i]->key,
					rm->dir_option[i]->keyword ? rm->dir_option[i]->keyword : "Нет(дверь)",
					rm->dir_option[i]->vkeyword ? rm->dir_option[i]->vkeyword : "Нет(дверь)", tmpBuf);
			SendMsgToChar(buf, ch);
			if (!rm->dir_option[i]->general_description.empty()) {
				snprintf(buf, sizeof(buf), "  %s\r\n", rm->dir_option[i]->general_description.c_str());
			} else {
				snprintf(buf, sizeof(buf), "  Нет описания выхода.\r\n");
			}
			SendMsgToChar(buf, ch);
		}
	}

	if (!rm->affected.empty()) {
		snprintf(buf1, sizeof(buf1), "&GАффекты на комнате:\r\n&n");
		for (const auto &aff : rm->affected) {
			size_t buf1_len = strlen(buf1);
			snprintf(buf1 + buf1_len, sizeof(buf1) - buf1_len, "       Заклинание \"%s\" (длит: %d, модиф: %d) - %s.\r\n",
					MUD::Spell(aff->type).GetCName(),
					aff->duration,
					aff->type == ESpell::kPortalTimer ? world[aff->modifier]->vnum : aff->modifier,
					(k = find_char(aff->caster_id)) ? GET_NAME(k) : "неизвестно");
		}
		SendMsgToChar(buf1, ch);
	}
	// check the room for a script
	do_sstat_room(rm, ch);
}

void do_stat(CharData *ch, char *argument, int cmd, int/* subcmd*/) {
	CharData *victim;
	ObjData *object;
	int tmp;
	int level = ch->IsFlagged(EPrf::kCoderinfo) ? kLvlImplementator : GetRealLevel(ch);

	half_chop(argument, buf1, buf2);

	if (!(privilege::HasPrivilege(ch, std::string(cmd_info[cmd].command), 0, 0, false)) && (GET_OLC_ZONE(ch) <=0)) {
		SendMsgToChar("Чаво?\r\n", ch);
		return;
	}
	if (!*buf1) {
		SendMsgToChar("Состояние КОГО или ЧЕГО?\r\n", ch);
		return;
	}
	if (*buf1 && ch->IsImmortal()) {
		if (utils::IsAbbr(buf1, "room") && level >= kLvlBuilder) {
			int vnum, rnum = kNowhere;
			if (*buf2 && (vnum = atoi(buf2))) {
				if ((rnum = GetRoomRnum(vnum)) != kNowhere)
					do_stat_room(ch, rnum);
				else
					SendMsgToChar("Состояние какой комнаты?\r\n", ch);
			}
			if (!*buf2)
				do_stat_room(ch);
			return;
		}
		if (utils::IsAbbr(buf1, "mob") && level >= kLvlBuilder) {
			if (!*buf2)
				SendMsgToChar("Состояние какого создания?\r\n", ch);
			else {
				if ((victim = get_char_vis(ch, buf2, EFind::kCharInWorld)) != nullptr)
					do_stat_character(ch, victim, 0);
				else
					SendMsgToChar("Нет такого создания в этом МАДе.\r\n", ch);
			}
			return;
		} 
		if (utils::IsAbbr(buf1, "player")) {
			if (!*buf2) {
				SendMsgToChar("Состояние какого игрока?\r\n", ch);
			} else {
				if ((victim = get_player_vis(ch, buf2, EFind::kCharInWorld)) != nullptr)
					do_stat_character(ch, victim);
				else
					SendMsgToChar("Этого персонажа сейчас нет в игре.\r\n", ch);
			}
			return;
		}
		if (utils::IsAbbr(buf1, "ip")) {
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
				if (LoadPlayerCharacter(buf2, &t_vict, ELoadCharFlags::kFindId) > -1) {
					do_statip(ch, &t_vict);
				} else {
					SendMsgToChar("Такого игрока нет ВООБЩЕ.\r\n", ch);
				}
			}
			return;
		}
		if (utils::IsAbbr(buf1, "karma") || utils::IsAbbr(buf1, "карма")) {
			if (!*buf2) {
				SendMsgToChar("Карму какого игрока?\r\n", ch);
			} else {
				if ((victim = get_player_vis(ch, buf2, EFind::kCharInWorld)) != nullptr) {
					DoStatKarma(ch, victim);
					return;
				} else {
					SendMsgToChar("Этого персонажа сейчас нет в игре, смотрим пфайл.\r\n", ch);
				}
				Player t_vict;
				if (LoadPlayerCharacter(buf2, &t_vict, ELoadCharFlags::kFindId) > -1) {
					DoStatKarma(ch, &t_vict);
				} else {
					SendMsgToChar("Такого игрока нет ВООБЩЕ.\r\n", ch);
				}
			}
			return;
		}
		if (utils::IsAbbr(buf1, "file")) {
			if (!*buf2) {
				SendMsgToChar("Состояние какого игрока(из файла)?\r\n", ch);
			} else {
				Player t_vict;
				if (LoadPlayerCharacter(buf2, &t_vict, ELoadCharFlags::kFindId) > -1) {
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
			return;
		}
		if (utils::IsAbbr(buf1, "object") && level >= kLvlBuilder) {
			if (!*buf2)
				SendMsgToChar("Состояние какого предмета?\r\n", ch);
			else {
				if ((object = get_obj_vis(ch, buf2)) != nullptr)
					do_stat_object(ch, object);
				else
					SendMsgToChar("Нет такого предмета в игре.\r\n", ch);
			}
			return;
		}
	}
	if (ch->IsImmortal()) {
		if ((object = get_object_in_equip_vis(ch, buf1, ch->equipment, &tmp)) != nullptr) {
			do_stat_object(ch, object);
			return;
		}
		if ((object = get_obj_in_list_vis(ch, buf1, ch->carrying)) != nullptr) {
			do_stat_object(ch, object);
			return;
		}
		if ((victim = get_char_vis(ch, buf1, EFind::kCharInRoom)) != nullptr) {
			do_stat_character(ch, victim);
			return;
		}
		if ((object = get_obj_in_list_vis(ch, buf1, world[ch->in_room]->contents)) != nullptr) {
			do_stat_object(ch, object);
			return;
		}
		if ((victim = get_char_vis(ch, buf1, EFind::kCharInWorld)) != nullptr) {
			do_stat_character(ch, victim);
			return;
		}
		if ((object = get_obj_vis(ch, buf1)) != nullptr) {
			do_stat_object(ch, object);
			return;
		}
	} 
	if (GET_OLC_ZONE(ch) == zone_table[world[ch->in_room]->zone_rn].vnum) {
		if ((object = get_object_in_equip_vis(ch, buf1, ch->equipment, &tmp)) != nullptr) {
			do_stat_object(ch, object);
			return;
		}
		if ((object = get_obj_in_list_vis(ch, buf1, world[ch->in_room]->contents)) != nullptr) {
			do_stat_object(ch, object);
			return;
		}
		if ((victim = get_char_vis(ch, buf1, EFind::kCharInRoom)) != nullptr) {
			do_stat_character(ch, victim);
			return;
		}
	}
	SendMsgToChar("Ничего похожего с этим именем нет.\r\n", ch);
}
