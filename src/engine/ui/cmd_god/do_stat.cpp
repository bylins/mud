#include "gameplay/mechanics/equipment.h"
#include "gameplay/affects/obj_affects.h"   // issue.obj-affects: Diag
#include "utils/native_text.h"
#include "gameplay/affects/affect_messages.h"
#include "do_stat.h"
#include "utils/utils_string.h"
#include "gameplay/core/experience.h"
#include "gameplay/economics/currencies.h"
#include "gameplay/mechanics/condition.h"
#include "gameplay/mechanics/magic_item.h"
#include "gameplay/magic/magic_utils.h"
#include "gameplay/fight/fight_messages.h"

#include "administration/ban.h"
#include "engine/entities/char_player.h"
#include "gameplay/mechanics/player_races.h"
#include "engine/core/utils_char_obj.inl"
#include "engine/core/target_resolver.h"
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
#include "utils/grammar/declensions.h"
#include "gameplay/mechanics/depot.h"
#include "gameplay/magic/magic.h"
#include "gameplay/mechanics/noob.h"
#include "administration/privilege.h"
#include "gameplay/mechanics/stable_objs.h"
#include "administration/proxy.h"
#include "gameplay/ai/spec_procs.h"
#include "gameplay/mechanics/weather.h"
#include "gameplay/core/game_limits.h"
#include "gameplay/economics/exchange.h"
#include "gameplay/communication/parcel.h"
#include "gameplay/mechanics/armor.h"
#include "engine/db/player_index.h"
#include "gameplay/core/remort.h"

#include <fmt/format.h>
#include <fmt/printf.h>
#include "gameplay/mechanics/sight.h"


std::string print_special(CharData *mob) {
	// issue.specials: identity via the registry; a mob may carry several specials (comma-joined).
	std::string out;
	for (const auto s : specials::MobSpecials(GET_MOB_VNUM(mob))) {
		const char *name = "глюк";
		switch (s) {
			case specials::ESpecial::kShop: name = "торговец"; break;
			case specials::ESpecial::kRent: name = "рентер"; break;
			case specials::ESpecial::kMail: name = "почтальон"; break;
			case specials::ESpecial::kBank: name = "банкир"; break;
			case specials::ESpecial::kExchange: name = "зазывала"; break;
			case specials::ESpecial::kHorse: name = "конюх"; break;
			case specials::ESpecial::kGuild: name = "учитель"; break;
			case specials::ESpecial::kTorc: name = "меняла"; break;
			case specials::ESpecial::kOutfit: name = "нубхелпер"; break;
			case specials::ESpecial::kMercenary: name = "ватажник"; break;
			default: break;
		}
		if (!out.empty()) {
			out += ", ";
		}
		out += name;
	}
	return out.empty() ? "нет" : out;
}


void do_statip(CharData *ch, CharData *k) {
	log("Start logon list stat");

	// Отображаем список ip-адресов с которых персонаж входил
	if (!LOGON_LIST(k).empty()) {
		// update: логон-лист может быть капитально большим, поэтому пишем это в свой дин.буфер, а не в buf2
		// заодно будет постраничный вывод ип, чтобы имма не посылало на йух с **OVERFLOW**
		// Заголовок именно append: конструктор ostringstream ставит позицию записи в начало,
		// и первая же строка списка затирала его -- бог видел хвост "ресов:" (#3814).
		std::ostringstream out;
		out << "Персонаж заходил с IP-адресов:\r\n";
		for (const auto &logon : LOGON_LIST(k)) {
			out << fmt::format("{:>16} {:>5} {:>20}{}\r\n",
							   logon.ip,
							   logon.count,
							   rustime(localtime(&logon.lasttime)),
							   logon.is_first ? " (создание)" : "");
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

void do_stat_character(CharData *ch, CharData *k, const int virt) {
	int i, i2;
	ObjData *j;
	int god_level = ch->IsFlagged(EPrf::kCoderinfo) ? kLvlImplementator : GetRealLevel(ch);
	int k_room = -1;
	if (!virt && (god_level == kLvlImplementator || (god_level == kLvlGreatGod && !k->IsNpc()))) {
		k_room = GET_ROOM_VNUM(k->in_room);
	}

	{
		std::string sline;
		if (k->IsNpc()) {
			sline = fmt::format("{} {} ", utils::sprintGender(to_underlying(k->get_sex())),
								GetTypeName(GET_RACE(k) - ENpcRace::kBasic, npc_race_types));
		}
		sline += fmt::format("{} '{}' В комнате [{}] Текущий UID:[{}]",
							 (!k->IsNpc() ? "PC" : "MOB"), GET_NAME(k), k_room, k->get_uid());
		SendMsgToChar(sline, ch);
	}
	SendMsgToChar(ch, " ЛАГ: [%d]\r\n", k->get_wait());
	if (k->IsNpc()) {
		SendMsgToChar(fmt::format("Синонимы: &S{}&s, VNum: [{:7}], RNum: [{:7}]\r\n", k->GetCharAliases(), GET_MOB_VNUM(k), k->get_rnum()), ch);
	}

	SendMsgToChar(fmt::format("Падежи: {}/{}/{}/{}/{}/{} ", GET_PAD(k, 0), GET_PAD(k, 1), GET_PAD(k, 2), GET_PAD(k, 3), GET_PAD(k, 4), GET_PAD(k, 5)), ch);

	if (!k->IsNpc()) {

		if (!(k)->player_specials->saved.NameGod) {
			SendMsgToChar("Имя никем не одобрено!\r\n", ch);
		} else if ((k)->player_specials->saved.NameGod < 1000) {
			SendMsgToChar(fmt::format("Имя запрещено! - {}\r\n", GetNameById((k)->player_specials->saved.NameIDGod)), ch);
		} else {
			SendMsgToChar(fmt::format("Имя одобрено! - {}\r\n", GetNameById((k)->player_specials->saved.NameIDGod)), ch);
		}

		SendMsgToChar(fmt::format("Вероисповедание: {}\r\n", religion_name[(int) GET_RELIGION(k)][(int) k->get_sex()]), ch);

		std::string file_name = GET_NAME(k);
		CreateFileName(file_name);
		SendMsgToChar(fmt::format("E-mail: &S{}&s File: {}\r\n", GET_EMAIL(k), file_name), ch);

		std::string text = RegisterSystem::ShowComment(GET_EMAIL(k));
		if (!text.empty())
			SendMsgToChar(ch, "Registered by email from %s\r\n", text.c_str());

		if (k->player_specials->saved.telegram_id != 0)
			SendMsgToChar(ch, "Подключен Телеграм, chat_id: %lu\r\n", k->player_specials->saved.telegram_id);

		if (k->IsFlagged(EPlrFlag::kFrozen) && punishments::Get(k, punishments::EType::kFreeze).duration) {
			SendMsgToChar(fmt::format("Заморожен : {} час [{}].\r\n", static_cast<long>((punishments::Get(k, punishments::EType::kFreeze).duration - time(nullptr)) / 3600), punishments::Get(k, punishments::EType::kFreeze).reason.empty() ? "-" : punishments::Get(k, punishments::EType::kFreeze).reason), ch);
		}
		if (k->IsFlagged(EPlrFlag::kHelled) && punishments::Get(k, punishments::EType::kHell).duration) {
			SendMsgToChar(fmt::format("Находится в темнице : {} час [{}].\r\n", static_cast<long>((punishments::Get(k, punishments::EType::kHell).duration - time(nullptr)) / 3600), punishments::Get(k, punishments::EType::kHell).reason.empty() ? "-" : punishments::Get(k, punishments::EType::kHell).reason), ch);
		}
		if (k->IsFlagged(EPlrFlag::kNameDenied) && punishments::Get(k, punishments::EType::kName).duration) {
			SendMsgToChar(fmt::format("Находится в комнате имени : {} час.\r\n", static_cast<long>((punishments::Get(k, punishments::EType::kName).duration - time(nullptr)) / 3600)), ch);
		}
		if (k->IsFlagged(EPlrFlag::kMuted) && punishments::Get(k, punishments::EType::kMute).duration) {
			SendMsgToChar(fmt::format("Будет молчать : {} час [{}].\r\n", static_cast<long>((punishments::Get(k, punishments::EType::kMute).duration - time(nullptr)) / 3600), punishments::Get(k, punishments::EType::kMute).reason.empty() ? "-" : punishments::Get(k, punishments::EType::kMute).reason), ch);
		}
		if (k->IsFlagged(EPlrFlag::kDumbed) && punishments::Get(k, punishments::EType::kDumb).duration) {
			SendMsgToChar(fmt::format("Будет нем : {} мин [{}].\r\n", static_cast<long>((punishments::Get(k, punishments::EType::kDumb).duration - time(nullptr)) / 60), punishments::Get(k, punishments::EType::kDumb).reason.empty() ? "-" : punishments::Get(k, punishments::EType::kDumb).reason), ch);
		}
		if (!k->IsFlagged(EPlrFlag::kRegistred) && punishments::Get(k, punishments::EType::kUnreg).duration) {
			SendMsgToChar(fmt::format("Не будет зарегистрирован : {} час [{}].\r\n", static_cast<long>((punishments::Get(k, punishments::EType::kUnreg).duration - time(nullptr)) / 3600), punishments::Get(k, punishments::EType::kUnreg).reason.empty() ? "-" : punishments::Get(k, punishments::EType::kUnreg).reason), ch);
		}

		if (GET_GOD_FLAG(k, EGf::kGodsLike) && punishments::Get(k, punishments::EType::kGcurse).duration) {
			SendMsgToChar(fmt::format("Под защитой Богов : {} час.\r\n", static_cast<long>((punishments::Get(k, punishments::EType::kGcurse).duration - time(nullptr)) / 3600)), ch);
		}
		if (GET_GOD_FLAG(k, EGf::kGodscurse) && punishments::Get(k, punishments::EType::kGcurse).duration) {
			SendMsgToChar(fmt::format("Проклят Богами : {} час.\r\n", static_cast<long>((punishments::Get(k, punishments::EType::kGcurse).duration - time(nullptr)) / 3600)), ch);
		}
	}

	const auto &title = k->GetTitleStr();
	SendMsgToChar(fmt::format("Титул: {}\r\n", (title.empty() ? "<Нет>" : title)), ch);
	if (k->IsNpc())
		SendMsgToChar(fmt::format("L-Des: {}",
								  (!k->player_data.long_descr.empty() ? k->player_data.long_descr : "<Нет>\r\n")), ch);
	else
		SendMsgToChar(fmt::format("L-Des: {}", (!k->player_data.description.empty() ? k->player_data.description : "<Нет>\r\n")), ch);

	if (!k->IsNpc()) {
		SendMsgToChar(fmt::format("Род: {}, Профессия: {}",
								  MUD::RaceMessages().GetMessage(GET_RACE(k), k->get_sex()),
								  MUD::Class(k->GetClass()).GetCName()), ch);
	} else {
		std::string str;
		if (k->get_role_bits().any()) {
			print_bitset(k->get_role_bits(), npc_role_types, ",", str);
		} else {
			str += "нет";
		}
		SendMsgToChar(ch, "Роли NPC: %s%s%s", kColorCyn, str.c_str(), kColorNrm);
	}

	std::string group_exp;
	if (experience::GetZoneGroup(k) > 1) {
		group_exp = fmt::format(" : групповой {}x{}",
								k->get_exp() / experience::GetZoneGroup(k), experience::GetZoneGroup(k));
	}

	SendMsgToChar(fmt::format(", Уровень: [&Y{:2}&n], Опыт: [&Y{:10}&n]{}, Наклонности: [{:4}]\r\n",
							  GetRealLevel(k), k->get_exp(), group_exp, alignment::GetAlignment(k)), ch);

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

		SendMsgToChar(fmt::format("Создан: [{}] Последний вход: [{}] Играл: [{}h {}m] Возраст: [{}]\r\n", t1, t2, k->player_data.time.played / 3600, ((k->player_data.time.played % 3600) / 60), CalcCharAge(k)->year), ch);

		{
			// сегменты без запятых -- разделитель ", " и перенос по ширине добавит OutWordsList
			std::vector<std::string> parts;
			parts.push_back(fmt::sprintf("Рента: [%d], Денег: [%9ld], В банке: [%9ld] (Всего: %ld)",
					GET_LOADROOM(k), currencies::GetHand(*k, currencies::kGold), currencies::GetBank(*k, currencies::kGold), currencies::GetTotal(*k, currencies::kGold)));
			for (const auto &cur : MUD::Currencies()) {
				if (cur.GetId() < 0 || cur.GetTextId() == currencies::kGold) { continue; }
				const long cur_total = currencies::GetTotal(*k, cur.GetTextId());
				if (cur_total != 0) {
					parts.push_back(fmt::sprintf("%s: %ld", cur.GetName(grammar::ECase::kNom).c_str(), cur_total));
				}
			}
			if (GetRealLevel(ch) >= kLvlImmortal) {
				parts.push_back(fmt::sprintf("%sOLC[%d]%s", kColorGrn, GET_OLC_ZONE(k), kColorNrm));
			}
			const size_t width = (!ch->IsNpc() && ch->player_specials->saved.stringLength > 0)
					? ch->player_specials->saved.stringLength : 120;
			SendMsgToChar(utils::OutWordsList(parts, width, ", ") + "\r\n", ch);
		}
	} else {
		int mob_online = mob_index[k->get_rnum()].total_online - (virt ? 1 : 0);
		SendMsgToChar(fmt::format("Сейчас в мире : {}, макс {}. ", mob_online, mob_index[k->get_rnum()].stored), ch);
		std::string stats;
		mob_stat::GetLastMobKill(k, stats);
		SendMsgToChar(fmt::format("Последний раз убит: &r{}&n", stats), ch);
	}
	SendMsgToChar(fmt::format("Сила: [{}{}/{}{}]  Инт : [{}{}/{}{}]  Мудр : [{}{}/{}{}] \r\n"
			"Ловк: [{}{}/{}{}]  Тело:[{}{}/{}{}]  Обаян:[{}{}/{}{}] Размер: [{}{}/{}{}]\r\n", kColorCyn, k->GetInbornStr(), GetRealStr(k), kColorNrm, kColorCyn, k->GetInbornInt(), GetRealInt(k), kColorNrm, kColorCyn, k->GetInbornWis(), GetRealWis(k), kColorNrm, kColorCyn, k->GetInbornDex(), GetRealDex(k), kColorNrm, kColorCyn, k->GetInbornCon(), GetRealCon(k), kColorNrm, kColorCyn, k->GetInbornCha(), GetRealCha(k), kColorNrm, kColorCyn, GET_SIZE(k), GET_REAL_SIZE(k), kColorNrm), ch);

	SendMsgToChar(fmt::format("Жизни :[{}{}/{}+{}{}]  Энергии :[{}{}/{}+{}{}]", kColorGrn, k->get_hit(), k->get_real_max_hit(), hit_gain(k), kColorNrm, kColorGrn, k->get_move(), k->get_real_max_move(), move_gain(k), kColorNrm), ch);
	if (IS_MANA_CASTER(k)) {
		SendMsgToChar(fmt::format(" Мана :[&G{}/{}+{}&n]\r\n",
								  k->mem_queue.stored, Mana(GetRealWis(k)), CalcManaGain(k)), ch);
	} else {
		SendMsgToChar("\r\n", ch);
	}

	SendMsgToChar(fmt::format("Glory: [{}], ConstGlory: [{}], AC: [{}/{}({})], Броня: [{}], Попадания: [{:2}/{:2}/{}], Повреждения: [{:2}/{:2}/{}]\r\n", Glory::get_glory(k->get_uid()), currencies::GetHand(*k, currencies::kGlory), GET_AC(k), GetRealAc(k), CalcBaseAc(k), GET_ARMOUR(k), GET_HR(k), GET_REAL_HR(k), GET_REAL_HR(k) + str_bonus(GetRealStr(k), STR_TO_HIT), GET_DR(k), GetRealDamroll(k), GetRealDamroll(k) + str_bonus(GetRealStr(k), STR_TO_DAM)), ch);
	SendMsgToChar(fmt::format("Защитн.аффекты: (базовые) [Will:{}/Crit.:{}/Stab.:{}/Reflex:{}], (полные) Поглощ: [{}], Воля: [{}], Здор.: [{}], Стойк.: [{}], Реакц.: [{}]\r\n", GetBasicSave(k, ESaving::kWill), GetBasicSave(k, ESaving::kCritical), GetBasicSave(k, ESaving::kStability), GetBasicSave(k, ESaving::kReflex), GET_ABSORBE(k), CalcSaving(k, k, ESaving::kWill, 0), CalcSaving(k, k, ESaving::kCritical, 0), CalcSaving(k, k, ESaving::kStability, 0), CalcSaving(k, k, ESaving::kReflex, 0)), ch);
	SendMsgToChar(fmt::format("Резисты: [Огонь:{}/Воздух:{}/Вода:{}/Земля:{}/Жизнь:{}/Разум:{}/Иммунитет:{}/Тьма:{}]\r\n", GET_RESIST(k, 0), GET_RESIST(k, 1), GET_RESIST(k, 2), GET_RESIST(k, 3), GET_RESIST(k, 4), GET_RESIST(k, 5), GET_RESIST(k, 6), GET_RESIST(k, 7)), ch);
	SendMsgToChar(fmt::format("Защита от маг. аффектов: [{}], Защита от маг. урона: [{}], Защита от физ. урона: [{}], Маг.урон: [{}], Физ.урон: [{}]\r\n", GET_AR(k), GET_MR(k), GET_PR(k), k->add_abils.percent_spellpower_add, k->add_abils.percent_physdam_add), ch);
	SendMsgToChar(fmt::format("Запом: [{}], УспехКолд: [{}], ВоссЖиз: [{}], ВоссСил: [{}], Поглощ: [{}], Удача: [{}], Иниц: [{}]\r\n", GET_MANAREG(k), GET_CAST_SUCCESS(k), k->get_hitreg(), k->get_movereg(), GET_ABSORBE(k), k->calc_morale(), GET_INITIATIVE(k)), ch);

	{
		std::string sline = fmt::format("Положение: {}, Сражается: {}, Экипирован в металл: {}",
										GetTypeName(static_cast<int>(k->GetPosition()), position_types),
										(k->GetEnemy() ? GET_NAME(k->GetEnemy()) : "Нет"),
										(IsEquipInMetall(k) ? "Да" : "Нет"));
		if (k->IsNpc()) {
			sline += ", Тип атаки: ";
			sline += fight::GetAttackTypeDescription(k->mob_specials.attack_type);
		}
		if (k->desc) {
			sline += ", Соединение: ";
			sline += GetConDescription(k->desc->state);
		}
		sline += "\r\n";
		SendMsgToChar(sline, ch);
	}
	{
		std::string sline = std::string("Позиция по умолчанию: ")
			+ GetTypeName(static_cast<int>(k->mob_specials.default_pos), position_types);
		if (k->char_specials.timer > 0) {
			sline += fmt::sprintf(", Таймер отсоединения (тиков) [%d]\r\n", k->char_specials.timer);
		} else if (k->extract_timer > 0) {
			sline += fmt::sprintf(", Extract timer [%d]\r\n", k->extract_timer);
		} else {
			sline += "\r\n";
		}
		SendMsgToChar(sline, ch);
	}

	if (k->IsNpc()) {
		SendMsgToChar(fmt::format("MOB флаги: &c{}&n\r\n",
								  k->char_specials.saved.mob_flags.sprintbits(action_bits, ",", 4)), ch);
		SendMsgToChar(fmt::format("NPC флаги: &c{}&n\r\n",
								  k->mob_specials.npc_flags.sprintbits(function_bits, ",", 4)), ch);
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
		// issue #3429: длинные списки умений/заклинаний/способностей переносим
		// по ширине экрана игрока через utils::OutWordsList (цвет метки/элементов
		// зашит в префикс, ширину считаем без цветокодов).
		const size_t list_width = (!ch->IsNpc() && ch->player_specials->saved.stringLength > 0)
				? ch->player_specials->saved.stringLength : 120;
		{
			std::vector<std::string> parts;
			for (const auto &skill : MUD::Skills()) {
				if (skill.IsValid() && GetSkill(k, skill.GetId())) {
					parts.push_back(fmt::sprintf("%s:[%d]", skill.GetName(), GetSkill(k, skill.GetId())));
				}
			}
			SendMsgToChar(utils::OutWordsList(parts, list_width, ", ", "&GУмения:&c ") + "&n\r\n", ch);
		}
		if (!k->mob_specials.have_spell) {
			SendMsgToChar(ch, "&GЗаклинания: &Rнет&n\r\n");
		} else {
			std::vector<std::string> parts;
			for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
				if (MUD::Spell(spell_id).IsUnavailable()) {
					continue;
				}
				if (GET_SPELL_MEM(k, spell_id)) {
					parts.push_back(fmt::sprintf("%s:[%d]", MUD::Spell(spell_id).GetCName(), GET_SPELL_MEM(k, spell_id)));
				}
			}
			SendMsgToChar(utils::OutWordsList(parts, list_width, ", ", "&GЗаклинания:&c ") + "&n\r\n", ch);
		}
		{
			std::vector<std::string> parts;
			for (const auto &feat : MUD::Feats()) {
				if (feat.IsInvalid()) {
					continue;
				}
				if (k->HaveFeat(feat.GetId())) {
					parts.push_back(feat.GetCName());
				}
			}
			SendMsgToChar(utils::OutWordsList(parts, list_width, ", ", "&GСпособности:&c ") + "&n\r\n", ch);
		}
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

			SendMsgToChar(ch,
						  "Заданные путевые точки: %s%s%s\r\n",
						  kColorCyn,
						  str_dest_list.str().c_str(),
						  kColorNrm);

			// Предполагаемый маршрут считаем и показываем только для живого моба
			// (не vstat): vstat ставит временный прототип в комнату rnum 1 чужой
			// зоны, а kStayZone-моб не строит путь между зонами. В лог это теперь
			// не сыплется в любом случае -- find_first_step молчит, пока его не
			// попросят жаловаться (issue #3384).
			if (!virt) {
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
							  "Предполагаемый маршрут: %s%s%s\r\n",
							  kColorCyn,
							  str_predictive_path.str().c_str(),
							  kColorNrm);
			}
		}
	} else {
		std::vector<std::string> out_str =
			utils::Split(k->char_specials.saved.plr_flags.sprintbits(player_bits, ", ", 4), ',');
		SendMsgToChar(fmt::format("{}{}{}\r\n", kColorCyn, utils::OutWordsList(out_str, ch->player_specials->saved.stringLength, ", ", "PLR: "), kColorNrm), ch);

		out_str = utils::Split(k->player_specials->saved.pref.sprintbits(preference_bits, ", ", 4), ',');
		SendMsgToChar(fmt::format("{}{}{}\r\n", kColorGrn, utils::OutWordsList(out_str, ch->player_specials->saved.stringLength, ", ", "PRF: "), kColorNrm), ch);

		if (privilege::IsImpl(ch)) {
			char godslike[kMaxStringLength];
			sprintbitwd(k->player_specials->saved.GodsLike, godslike_bits, godslike, sizeof(godslike), ", ");
			// sprintbitwd no longer substitutes the "nothing" word; do_stat is an
			// immortal-only command, so a plain English literal is fine
			out_str = utils::Split(*godslike ? godslike : "nothing", ',');
			SendMsgToChar(fmt::format("{}{}{}\r\n", kColorCyn, utils::OutWordsList(out_str, ch->player_specials->saved.stringLength, ", ", "GFL: "), kColorNrm), ch);
		}
	}

	if (k->IsNpc()) {

		SendMsgToChar(fmt::format("Mob СпецПроц: &R{}&n, NPC сила удара: {}d{}\r\n", print_special(k), k->mob_specials.damnodice, k->mob_specials.damsizedice), ch);
	}
	{
		std::string sline = fmt::sprintf("Несет - вес %d, предметов %d; ", k->GetCarryingWeight(), k->GetCarryingQuantity());
		for (i = 0, j = k->carrying; j; j = j->get_next_content(), i++);
		sline += fmt::sprintf("(в инвентаре) : %d, ", i);
		for (i = 0, i2 = 0; i < EEquipPos::kNumEquipPos; i++) {
			if (GET_EQ(k, i)) {
				i2++;
			}
		}
		sline += fmt::sprintf("(надето): %d\r\n", i2);
		SendMsgToChar(sline, ch);
	}

	if (!k->IsNpc()) {
		SendMsgToChar(fmt::format("Голод: {}, Жажда: {}, Опьянение: {}\r\n", GET_COND(k, condition::kFull), GET_COND(k, condition::kThirst), GET_COND(k, condition::kDrunk)), ch);
	}

	if (god_level >= kLvlGreatGod) {
		std::string fl_str;

		// "Ведущий: X, Ведомые: " передаём префиксом -- его длина (с именем
		// ведущего) учитывается в ширине строки автоматически, без магического -30.
		const std::string lead_prefix =
			fmt::format("Ведущий: {}, Ведомые: ", (k->has_master() ? GET_NAME(k->get_master()) : "<нет>"));
		for (auto &it : k->followers) {
			if (!it->IsNpc()) {
				fl_str += " " + it->get_name();
			} else {
				fl_str += " " + it->get_name() + "_#" + std::to_string(GET_MOB_VNUM(it));
			}
		}
		SendMsgToChar(utils::OutWordsList(fl_str, ch->player_specials->saved.stringLength, ", ", lead_prefix) + "\r\n", ch);
		if (ch->IsNpc()) {
			fl_str.clear();
			for (auto &helper : k->summon_helpers) {
				fl_str += " " +  std::to_string(helper);
			}
			SendMsgToChar(utils::OutWordsList(fl_str, ch->player_specials->saved.stringLength, ", ", "Помогают: ") + "\r\n", ch);
		}
	}
	// Showing the bitvector
	std::vector<std::string> out_str =
		utils::Split(affects::DescribeActive(k->char_specials.saved.affected_by, ", "), ',');
	SendMsgToChar(fmt::format("Аффекты: &Y{}&n\r\n",
							  utils::OutWordsList(out_str, ch->player_specials->saved.stringLength - 10)), ch);
	SendMsgToChar(fmt::format("&GПеревоплощений: {}\r\n&n", remort::GetRealRemort(k)), ch);
	// Routine to show what spells a char is affected by
	if (!k->affected.empty()) {
		for (const auto &aff : k->affected) {
			std::string sline = fmt::sprintf("Заклинания: (%3d%s|%s) %s%s%s ",
					aff->duration + 1,
					(aff->battleflag.get(kAfPulsedec)) || (aff->battleflag.get(kAfSameTime)) ? "плс" : "мин",
					(aff->battleflag.get(kAfBattledec)) || (aff->battleflag.get(kAfSameTime)) ? "рнд" : "мин",
					kColorCyn,
					// issue.affect-migration: affect name by its own identity (affect_type), spell fallback.
					// Ширина поля -- в символах: printf меряет %-21s в байтах (issue #3681).
					native_text::pad_right(
						affects::AffectMsg(aff->affect_type, affects::EAffectMsgType::kShortDesc), 21).c_str(),
					kColorNrm);
			bool has_modifier = aff->modifier != 0;
			if (has_modifier) {
				sline += fmt::sprintf("%+d to %s", aff->modifier, apply_types[(int) aff->location]);
			}
			if (aff->affect_type != EAffect::kUndefined) {
				sline += has_modifier ? ", sets " : "sets ";
				sline += affects::AffectMsg(aff->affect_type, affects::EAffectMsgType::kShortDesc);
			}
			if (aff->potency != 0.0f) {
				const auto bk = affects::AffectBuffKind(aff->affect_type);
				const char *kind = bk == affects::EBuff::kYes ? "buff"
						: bk == affects::EBuff::kNo ? "debuff" : "ambiguous";
				sline += fmt::sprintf(" [p: %.1f %s]", aff->potency, kind);
			}
			sline += "\r\n";
			SendMsgToChar(sline, ch);
		}
	}

	// check mobiles for a script
	if (k->IsNpc() && god_level >= kLvlBuilder) {
		do_sstat_character(ch, k);
		if (MEMORY(k)) {
			struct mob_ai::MemoryRecord *memchar;
			SendMsgToChar("Помнит:\r\n", ch);
			for (memchar = MEMORY(k); memchar; memchar = memchar->next) {
				SendMsgToChar(fmt::format("{:10} - {:10}\r\n", static_cast<long>(memchar->id), static_cast<long>(memchar->time - time(nullptr))), ch);
			}
		}
	} else        // this is a PC, display their global variables
	{
		if (!SCRIPT(k)->global_vars.empty()) {
			SendMsgToChar("Глобальные переменные:\r\n", ch);
			// currently, variable context for players is always 0, so it is
			// not displayed here. in the future, this might change
			for (auto tv : k->script->global_vars) {
				if (tv.value[0] == UID_CHAR) {
					// Ширину колонки имени считает fmt: printf меряет её в байтах (issue #3797).
					SendMsgToChar(fmt::format("    {:>10}:  [CharUID]: {}\r\n",
											  tv.name, find_uid_name(tv.value.c_str())), ch);
				} else if (tv.value[0] == UID_OBJ) {
					SendMsgToChar(fmt::format("    {:>10}:  [ObjUID]: {}\r\n",
											  tv.name, find_uid_name(tv.value.c_str())), ch);
				} else if (tv.value[0] == UID_ROOM) {
					SendMsgToChar(fmt::format("    {:>10}:  [RoomUID]: {}\r\n",
											  tv.name, find_uid_name(tv.value.c_str())), ch);
				} else {
					SendMsgToChar(fmt::format("    {:>10}:  {}\r\n", tv.name, tv.value), ch);
				}
			}
		}

		std::string quested(k->quested_print());
		if (!quested.empty()) {
			// issue #3429: список выполненных квестов бывает очень длинным --
			// переносим каждую строку (зону) по ширине экрана через WrapText.
			const size_t width = (!ch->IsNpc() && ch->player_specials->saved.stringLength > 0)
					? ch->player_specials->saved.stringLength : 120;
			SendMsgToChar(ch, "Выполнил квесты:\r\n%s\r\n", utils::WrapText(quested, width).c_str());
		}

		if (NORENTABLE(k)) {
			SendMsgToChar(fmt::format("Не может уйти на постой {}\r\n", static_cast<long int>(NORENTABLE(k) - time(nullptr))), ch);
		}
		if (AGRO(k)) {
			SendMsgToChar(fmt::format("Агрессор {}\r\n", static_cast<long int>(AGRO(k) - time(nullptr))), ch);
		}
		SendMsgToChar(pk_list_sprintf(k), ch);
	}
}

void do_stat_object(CharData *ch, ObjData *j, const int virt = 0) {
	int i, found;
	ObjVnum rnum, vnum;
	ObjData *j2;
	long int li;
	bool is_grgod = (privilege::IsGrGod(ch) || ch->IsFlagged(EPrf::kCoderinfo)) ? true : false;

	vnum = GET_OBJ_VNUM(j);
	rnum = j->get_rnum();

	SendMsgToChar(fmt::format("Название: '{}{}{}',\r\nСинонимы: '&c{}&n',", kColorYel, (!j->get_short_description().empty() ? j->get_short_description() : "<None>"), kColorNrm, j->get_aliases()), ch);
	if (j->get_custom_label() && !j->get_custom_label()->text_label.empty()) {
		SendMsgToChar(fmt::format(" нацарапано: '&c{}&n',", j->get_custom_label()->text_label), ch);
	}
	SendMsgToChar("\r\n", ch);

	const char *spec_proc = "None";
	if (rnum >= 0) {
		spec_proc = obj_proto.func(j->get_rnum()) ? "Есть" : "Нет";
	}

	SendMsgToChar(ch, "VNum: [%s%7d%s], RNum: [%7d], UniqueID: [%ld], Id: [%ld]\r\n",
				  kColorGrn, vnum, kColorNrm, j->get_rnum(), j->get_unique_id(), j->get_id());

	SendMsgToChar(ch, "Расчет критерия: %f, мортов: (%f) \r\n", j->show_koef_obj(), j->show_mort_req());
	SendMsgToChar(fmt::format("Тип: {}, СпецПроцедура: {}",
							  GetTypeName(j->get_type(), item_types), spec_proc), ch);

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
	SendMsgToChar(ch, "\r\n%s", sight::diag_weapon_to_char(j, 2));
	SendMsgToChar(fmt::format("L-Des: {}\r\n{}", !j->get_description().empty() ? j->get_description() : "Нет", kColorNrm), ch);

	if (!j->get_ex_description().empty()) {
		std::string sline = fmt::sprintf("Экстра описание:%s", kColorCyn);
		for (const auto &desc : j->get_ex_description()) {
			sline += " ";
			sline += desc.keyword;
		}
		sline += kColorNrm;
		sline += "\r\n";
		SendMsgToChar(sline, ch);
	}
	SendMsgToChar("Может быть надет : ", ch);
	SendMsgToChar(sprintbit(j->get_wear_flags(), wear_bits) + "\r\n", ch);
	SendMsgToChar(fmt::format("Материал : {}, макс.прочность : {}, тек.прочность : {}\r\n",
							  GetTypeName(j->get_material(), material_name),
							  j->get_maximum_durability(), j->get_current_durability()), ch);

	SendMsgToChar("Неудобства : ", ch);
	SendMsgToChar(j->get_no_flags().sprintbits(no_bits, ",", 4) + "\r\n", ch);

	SendMsgToChar("Запреты : ", ch);
	SendMsgToChar(j->get_anti_flags().sprintbits(anti_bits, ",", 4) + "\r\n", ch);

	SendMsgToChar("Устанавливает аффекты : ", ch);
	SendMsgToChar(j->get_affect_flags().sprintbits(equipment_affects, ",", 4) + "\r\n", ch);
	if (j->has_suppressed_affects()) {
		SendMsgToChar("Подавленные аффекты   : ", ch);
		std::string sup;
		for (const auto &pr : j->suppressed_equip_affects()) {
			char one[128];
			snprintf(one, sizeof(one), "%s%s (%d %s)", sup.empty() ? "" : ", ",
					affects::AffectMsg(pr.first, affects::EAffectMsgType::kShortDesc).c_str(), pr.second,
					grammar::GetDeclensionInNumber(pr.second, grammar::EWhat::kHour));
			sup += one;
		}
		SendMsgToChar(ch, "%s\r\n", sup.c_str());
	}

	SendMsgToChar("Дополнительные флаги  : ", ch);
	SendMsgToChar(j->get_extra_flags().sprintbits(extra_bits, ",", 4) + "\r\n", ch);

	SendMsgToChar(fmt::format("Вес: {}, Цена: {}, Рента(eq): {}, Рента(inv): {}, ", j->get_weight(), j->get_cost(), j->get_rent_on(), j->get_rent_off()), ch);
	// "Таймер: нерушимо" уходил в общий буфер, который тут же затирала строка про таймер на
	// земле, и бог этой пометки не видел вовсе (#3814).
	if (stable_objs::IsTimerUnlimited(j)) {
		SendMsgToChar("Таймер: нерушимо, ", ch);
	} else {
		SendMsgToChar(fmt::format("Таймер: {}, ", j->get_timer()), ch);
	}
	// Таймер на земле тикает только пока вещь лежит в комнате, и выставляется при попадании
	// туда (PlaceObjToRoom). У вещи в инвентаре в этом поле лежит умолчание конструктора (60),
	// которое выглядело настоящим таймером, хотя таким никогда не станет.
	if (j->get_in_room() != kNowhere) {
		SendMsgToChar(fmt::format("Таймер на земле: {}\r\n", j->get_destroyer()), ch);
	} else {
		SendMsgToChar("\r\n", ch);
	}
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
		native_text::capitalize_first(str);
		SendMsgToChar(ch, "&C%s&n", str.c_str());
	} else {
		auto room = get_room_where_obj(j);

		std::string sline = "&CНаходится в комнате : ";
		if (room == kNowhere || !is_grgod) {
			sline += "нигде";
		} else {
			sline += std::to_string(room);
		}
		sline += ", В контейнере: ";
		if (j->get_in_obj() && is_grgod) {
			sline += fmt::sprintf("[%d] %s", GET_OBJ_VNUM(j->get_in_obj()), j->get_in_obj()->get_short_description().c_str());
		} else {
			sline += "Нет";
		}
		sline += ", В инвентаре: ";
		if (j->get_carried_by() && is_grgod) {
			sline += GET_NAME(j->get_carried_by());
		} else if (j->get_in_obj() && j->get_in_obj()->get_carried_by() && is_grgod) {
			sline += GET_NAME(j->get_in_obj()->get_carried_by());
		} else {
			sline += "Нет";
		}
		sline += ", Надет: ";
		if (j->get_worn_by() && is_grgod) {
			sline += GET_NAME(j->get_worn_by());
		} else if (j->get_in_obj() && j->get_in_obj()->get_worn_by() && is_grgod) {
			sline += GET_NAME(j->get_in_obj()->get_worn_by());
		} else {
			sline += "Нет";
		}
		sline += "\r\n&n";
		SendMsgToChar(sline, ch);
	}

	std::string line;
	switch (j->get_type()) {
		case EObjType::kBook:

			switch (GET_OBJ_VAL(j, 0)) {
				case EBook::kSpell: {
					auto spell_id = static_cast<ESpell>(GET_OBJ_VAL(j, 1));
					if (spell_id >= ESpell::kFirst && spell_id <= ESpell::kLast) {
						line = fmt::format("содержит заклинание        : \"{}\"", MUD::Spell(spell_id).GetCName());
					} else
						line = fmt::format("неверный номер заклинания");
					break;
				}
				case EBook::kSkill: {
					auto skill_id = static_cast<ESkill>(GET_OBJ_VAL(j, 1));
					if (MUD::Skill(skill_id).IsValid()) {
						line = fmt::format("содержит секрет умения     : \"{}\"", MUD::Skill(skill_id).GetName());
					} else
						line = fmt::format("неверный номер умения");
					break;
				}
				case EBook::kSkillUpgrade: {
					auto skill_id = static_cast<ESkill>(GET_OBJ_VAL(j, 1));
					if (MUD::Skills().IsValid(skill_id)) {
						if (GET_OBJ_VAL(j, 3) > 0) {
							line = fmt::format("повышает умение \"{}\" (максимум {})", MUD::Skill(skill_id).GetName(), GET_OBJ_VAL(j, 3));
						} else {
							line = fmt::format("повышает умение \"{}\" (не больше максимума текущего перевоплощения)", MUD::Skill(skill_id).GetName());
						}
					} else {
						line = fmt::format("неверный номер повышаемого умения");
					}
				}
					break;
				case EBook::kFeat: {
					const auto feat_id = static_cast<EFeat>(GET_OBJ_VAL(j, 1));
					if (MUD::Feat(feat_id).IsValid()) {
						line = fmt::format("содержит секрет способности : \"{}\"", MUD::Feat(feat_id).GetCName());
					} else {
						line = fmt::format("неверный номер способности");
					}
				}
					break;
				case EBook::kReceipt: {
					const auto recipe = im_get_recipe(GET_OBJ_VAL(j, 1));
					if (recipe >= 0) {
						// issue.class-recipes: требования теперь per-class; класса тут нет, поэтому
						// показываем минимальные уровень/реморт среди владеющих классов.
						const auto [minlevel, minremort] = classes::GetRecipeMinRequirements(imrecipes[recipe].str_id);
						if ((minlevel >= 0) && (minremort >= 0)) {
							const auto recipelevel = std::max(GET_OBJ_VAL(j, 2), minlevel);
							line = fmt::format("содержит рецепт отвара     : \"{}\", мин. уровень изучения: {}, мин. количество ремортов: {}", imrecipes[recipe].name, recipelevel, minremort);
						} else {
							line = fmt::format("содержит рецепт отвара     : \"{}\" (не доступен ни одному классу)", imrecipes[recipe].name);
						}
					} else
						line = fmt::format("Некорректная запись рецепта");
				}
					break;
				default:line = fmt::format("НЕВЕРНО УКАЗАН ТИП КНИГИ");
					break;
			}
			break;
		case EObjType::kLightSource:
			if (GET_OBJ_VAL(j, 2) < 0) {
				line = fmt::format("Вечный свет!");
			} else {
				line = fmt::format("Осталось светить: [{}]", GET_OBJ_VAL(j, 2));
			}
			break;

		case EObjType::kScroll: {
			// issue.magic-items: заклинания свитка лежат в extra_values, сила -- умение мастера
			line = fmt::format("{}", utils::OutWordsList(SpellItemSpellsWithPotency(j), ch->player_specials->saved.stringLength, ", ", std::string(kColorGrn) + "Заклинания:" + kColorNrm + " "));
			break;
		}
		case EObjType::kPotion: {
			line = fmt::format("{}", utils::OutWordsList(SpellItemSpellsWithPotency(j), ch->player_specials->saved.stringLength, ", ", std::string(kColorGrn) + "Заклинания:" + kColorNrm + " "));
			break;
		}
		case EObjType::kWand:
		case EObjType::kStaff:
			// issue.magic-items: заклинание и заряды берем из extra_values, val[] у посохов нулевые
			{
				const auto staff_spell = static_cast<ESpell>(j->GetSpellItemSpellNum(1));
				const int potency = static_cast<int>(MagicItemPotency(j, staff_spell) + 0.5f);
				// issue #3611: у вещи из прототипа сила посчитана по зашитым умолчаниям, а не по
				// умению мастера -- помечаем так же, как в перечне заклинаний свитков и зелий.
				line = fmt::format("{}Заклинание:{} {}{}{} (сила {}{}), {} (из {}) зарядов осталось", kColorGrn, kColorNrm, kColorCyn, MUD::Spell(staff_spell).GetCName(), kColorNrm, potency, IsPotencyFromProto(j) ? ", базовая" : "", j->GetPotionValueKey(ObjVal::EValueKey::kCurCharges), j->GetPotionValueKey(ObjVal::EValueKey::kMaxCharges));
			}
			break;

		case EObjType::kWeapon:
			line = fmt::format("Повреждения: {}d{}, Тип повреждения: {}", GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
			break;

		case EObjType::kArmor:
		case EObjType::kLightArmor:
		case EObjType::kMediumArmor:
		case EObjType::kHeavyArmor:line = fmt::format("AC: [{}]  Броня: [{}]", GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1));
			break;

		case EObjType::kTrap:line = fmt::format("Spell: {}, - Hitpoints: {}", GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1));
			break;

		case EObjType::kContainer: {
			const std::string key_type = sprintbit(GET_OBJ_VAL(j, 1), container_bits);
			if (IS_CORPSE(j)) {
				line = fmt::format("Объем: {}, Тип ключа: {}, VNUM моба: {}, Труп: да", GET_OBJ_VAL(j, 0), key_type, GET_OBJ_VAL(j, 2));
			} else {
				line = fmt::format("Объем: {}, Тип ключа: {}, Номер ключа: {}, Сложность замка: {}", GET_OBJ_VAL(j, 0), key_type, GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
			}
			break;
		}

		case EObjType::kLiquidContainer:
		case EObjType::kFountain:
			{
				std::string spells = drinkcon::print_spells(j);
				utils::Trim(spells);
				line = fmt::format("Объем: {}, Содержит: {}, Свежесть: {}, Отрава: {}, Жидкость: {}\r\n{}",
								   GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1),
								   j->GetPotionValueKey(ObjVal::EValueKey::kLiquidTimer),
								   j->GetPotionValueKey(ObjVal::EValueKey::kLiquidPoison),
								   GetTypeName(GET_OBJ_VAL(j, 2), drinks), spells);
			}
			break;

		case EObjType::kNote:line = fmt::format("Tongue: {}", GET_OBJ_VAL(j, 0));
			break;

		case EObjType::kKey:line.clear();
			break;

		case EObjType::kFood:
			line = fmt::format("Насыщает(час): {}, Свежесть: {}, Отрава: {}", GET_OBJ_VAL(j, 0), j->GetPotionValueKey(ObjVal::EValueKey::kLiquidTimer), j->GetPotionValueKey(ObjVal::EValueKey::kLiquidPoison));
			break;

		case EObjType::kMoney:
			line = fmt::format("Сумма: {}\r\nВалюта: {}", GET_OBJ_VAL(j, 0), MUD::Currency(GET_OBJ_VAL(j, 1)).GetCName(grammar::ECase::kNom));
			break;

		case EObjType::kMagicIngredient:
			line = fmt::format("ingr bits {}", sprintbit(j->get_spec_param(), ingradient_bits));

			if (IS_SET(j->get_spec_param(), kItemCheckUses)) {
				line += fmt::format("\r\nможно применить {} раз", GET_OBJ_VAL(j, 2));
			}

			if (IS_SET(j->get_spec_param(), kItemCheckLag)) {
				line += fmt::format("\r\nможно применить 1 раз в {} сек", (i = GET_OBJ_VAL(j, 0) & 0xFF));
				if (GET_OBJ_VAL(j, 3) == 0 || GET_OBJ_VAL(j, 3) + i < time(nullptr)) {
					line += fmt::format("(можно применять).");
				} else {
					li = GET_OBJ_VAL(j, 3) + i - time(nullptr);
					line += fmt::format("(осталось {} сек).", li);
				}
			}

			if (IS_SET(j->get_spec_param(), kItemCheckLevel)) {
				line += fmt::format("\r\nможно применить с {} уровня.", (GET_OBJ_VAL(j, 0) >> 8) & 0x1F);
			}

			if ((i = GetObjRnum(GET_OBJ_VAL(j, 1))) >= 0) {
				line += fmt::format("\r\nпрототип {}{}{}.", kColorBoldCyn, obj_proto[i]->get_PName(grammar::ECase::kNom), kColorNrm);
			}
			break;
		case EObjType::kMagicContaner:
		case EObjType::kMagicArrow:
			line = fmt::format("Заклинание: [{}]. Объем [{}]. Осталось стрел[{}].", MUD::Spell(static_cast<ESpell>(GET_OBJ_VAL(j, 0))).GetCName(), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2));
			break;

		default:
			line = fmt::format("Values 0-3: [{}] [{}] [{}] [{}]", GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
			break;
	}
	SendMsgToChar(line + "\r\n", ch);

	// * I deleted the "equipment status" code from here because it seemed
	// * more or less useless and just takes up valuable screen space.

	if (j->get_contains()) {
		std::string sline = fmt::sprintf("\r\nСодержит:%s", kColorGrn);
		for (found = 0, j2 = j->get_contains(); j2; j2 = j2->get_next_content()) {
			sline += fmt::sprintf("%s %s", found++ ? "," : "", j2->get_short_description().c_str());
			if (sline.size() >= 62) {
				sline += j2->get_next_content() ? ",\r\n" : "\r\n";
				SendMsgToChar(sline, ch);
				sline.clear();
				found = 0;
			}
		}
		if (!sline.empty()) {
			sline += "\r\n";
			SendMsgToChar(sline, ch);
		}
		SendMsgToChar(kColorNrm, ch);
	}
	found = 0;
	SendMsgToChar("Аффекты:", ch);
	for (i = 0; i < kMaxObjAffect; i++) {
		if (j->get_affected(i).modifier) {
			SendMsgToChar(fmt::format("{} {:+} to {}", found++ ? "," : "", j->get_affected(i).modifier,
									  GetTypeName(j->get_affected(i).location, apply_types)), ch);
		}
	}
	if (!found) {
		SendMsgToChar(" Нет", ch);
	}

	// issue.obj-affects: obj affects on the item (gods see everything -> nullptr viewer).
	{
		const std::string oaff = obj_affects::Diag(j, nullptr);
		if (!oaff.empty()) {
			SendMsgToChar("\r\nОбъектные аффекты:\r\n", ch);
			SendMsgToChar(oaff, ch);
		}
	}

	if (j->has_skills()) {
		CObjectPrototype::skills_t skills;
		j->get_skills(skills);

		SendMsgToChar("\r\nУмения :", ch);
		for (const auto &it : skills) {
			if (it.second == 0) {
				continue;
			}
			SendMsgToChar(fmt::format(" %+d% to {}", it.second, MUD::Skill(it.first).GetName()), ch);
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
		SendMsgToChar(fmt::format("Сейчас в мире : {}. На постое : {}. Макс в мире: {}\r\n", rnum >= 0 ? obj_proto.total_online(rnum) - (virt ? 1 : 0) : -1, rnum >= 0 ? obj_proto.stored(rnum) : -1, GetObjMIW(j->get_rnum())), ch);
		// check the object for a script
		do_sstat_object(ch, j);
	}
}

void do_stat_room(CharData *ch, const int rnum = 0) {
	RoomData *rm = world[ch->in_room];
	int i, found;
	CharData *k;

	if (rnum != 0) {
		rm = world[rnum];
	}

	SendMsgToChar(fmt::format("Комната : {}{}{}\r\n", kColorCyn, rm->name, kColorNrm), ch);

	SendMsgToChar(fmt::format("Зона: [{:3}], VNum: [&g{:7}&n], RNum: [{:7}], Тип  сектора: {}\r\n",
							  zone_table[rm->zone_rn].vnum, rm->vnum, rnum,
							  GetTypeName(rm->sector_type, sector_types)), ch);

	char room_flags[kMaxStringLength];
	rm->flags_sprint(room_flags, sizeof(room_flags), ",");
	SendMsgToChar(fmt::format("СпецПроцедура: {}, Флаги: {}\r\n",
							  (rm->func == nullptr) ? "None" : "Exists", room_flags), ch);

	SendMsgToChar("Описание:\r\n", ch);
	SendMsgToChar(GlobalObjects::descriptions().get(rm->description_num), ch);

	if (!rm->ex_description.empty()) {
		std::string sline = fmt::sprintf("Доп. описание:%s", kColorCyn);
		for (const auto &desc : rm->ex_description) {
			sline += " ";
			sline += desc.keyword;
		}
		sline += kColorNrm;
		sline += "\r\n";
		SendMsgToChar(sline, ch);
	}
	{
		std::string sline = fmt::sprintf("Живые существа:%s", kColorYel);
		found = 0;
		size_t counter = 0;
		for (auto k_i = rm->people.begin(); k_i != rm->people.end(); ++k_i) {
			const auto k = *k_i;
			++counter;
			if (!sight::CanSee(ch, k)) {
				continue;
			}
			sline += fmt::sprintf("%s %s(%s)", found++ ? "," : "", GET_NAME(k),
					(!k->IsNpc() ? "PC" : "MOB"));
			if (sline.size() >= 62) {
				sline += (counter != rm->people.size()) ? ",\r\n" : "\r\n";
				SendMsgToChar(sline, ch);
				sline.clear();
				found = 0;
			}
		}
		if (!sline.empty()) {
			sline += "\r\n";
			SendMsgToChar(sline, ch);
		}
		SendMsgToChar(kColorNrm, ch);
	}
	if (!rm->contents.empty()) {
		std::string sline = fmt::sprintf("Предметы:%s", kColorGrn);
		found = 0;
		for (auto it = rm->contents.begin(); it != rm->contents.end(); ++it) {
			auto j = *it;
			if (!sight::CanSeeObj(ch, j))
				continue;
			sline += fmt::sprintf("%s %s", found++ ? "," : "", j->get_short_description().c_str());
			if (sline.size() >= 62) {
				sline += (std::next(it) != rm->contents.end()) ? ",\r\n" : "\r\n";
				SendMsgToChar(sline, ch);
				sline.clear();
				found = 0;
			}
		}
		if (!sline.empty()) {
			sline += "\r\n";
			SendMsgToChar(sline, ch);
		}
		SendMsgToChar(kColorNrm, ch);
	}
	for (i = 0; i < EDirection::kMaxDirNum; i++) {
		if (rm->dir_option[i]) {
			const std::string leads_to = rm->dir_option[i]->to_room() == kNowhere
				? std::string(" &cNONE&n")
				: fmt::format("&c{:7}&n", GET_ROOM_VNUM(rm->dir_option[i]->to_room()));
			SendMsgToChar(fmt::format("Выход &c{}&n:  Ведет в : [{}], Ключ: [{:5}], Название: {} ({}), Тип: {}\r\n",
									  native_text::pad_right(dirs[i], 5), leads_to, rm->dir_option[i]->key,
									  rm->dir_option[i]->keyword ? rm->dir_option[i]->keyword : "Нет(дверь)",
									  rm->dir_option[i]->vkeyword ? rm->dir_option[i]->vkeyword : "Нет(дверь)",
									  sprintbit(rm->dir_option[i]->exit_info.get_plane(0), exit_bits)), ch);
			SendMsgToChar(rm->dir_option[i]->general_description.empty()
						  ? std::string("  Нет описания выхода.\r\n")
						  : fmt::format("  {}\r\n", rm->dir_option[i]->general_description), ch);
		}
	}

	if (!rm->affected.empty()) {
		std::string affects_line("&GАффекты на комнате:\r\n&n");
		for (const auto &aff : rm->affected) {
			affects_line += fmt::format("       Заклинание \"{}\" (длит: {}, модиф: {}, сила: {:.1f}) - {}.\r\n",
										NAME_BY_ITEM<room_spells::ERoomAffect>(aff->affect_type),
										aff->duration,
										room_spells::IsPortalAffect(aff->affect_type)
											? world[aff->modifier]->vnum : aff->modifier,
										aff->potency,
										(k = find_char(aff->caster_id)) ? GET_NAME(k) : "неизвестно");
		}
		SendMsgToChar(affects_line, ch);
	}

	// issue.room-affect-trigger-improve (door affects): affects hosted on this room's exits/doors.
	for (int d = 0; d < EDirection::kMaxDirNum; ++d) {
		const auto ex = rm->dir_option[d];
		if (!ex || ex->affected.empty()) {
			continue;
		}
		std::string exit_affects = fmt::format("&GАффекты на выходе ({}):\r\n&n", dirs_rus[d]);
		for (const auto &aff : ex->affected) {
			exit_affects += fmt::format("       Заклинание \"{}\" (длит: {}, модиф: {}, сила: {:.1f}, "
										"заряды: {}) - {}.\r\n",
										NAME_BY_ITEM<room_spells::ERoomAffect>(aff->affect_type),
										aff->duration, aff->modifier, aff->potency,
										(aff->charges == -1 ? "беск" : std::to_string(aff->charges)),
										(k = find_char(aff->caster_id)) ? GET_NAME(k) : "неизвестно");
		}
		SendMsgToChar(exit_affects, ch);
	}

	// check the room for a script
	do_sstat_room(rm, ch);
}

void do_stat(CharData *ch, char *argument, int cmd, int/* subcmd*/) {
	CharData *victim;
	ObjData *object;
	int tmp;
	int level = ch->IsFlagged(EPrf::kCoderinfo) ? kLvlImplementator : GetRealLevel(ch);

	std::string target;
	const std::string what = utils::ExtractFirstArgumentLower(argument, target);

	if (!(privilege::HasPrivilege(ch, std::string(cmd_info[cmd].command), 0, 0, false)) && (GET_OLC_ZONE(ch) <=0)) {
		SendMsgToChar("Чаво?\r\n", ch);
		return;
	}
	if (what.empty()) {
		SendMsgToChar("Состояние КОГО или ЧЕГО?\r\n", ch);
		return;
	}
	if (privilege::IsImmortal(ch)) {
		if (utils::IsAbbr(what, "room") && level >= kLvlBuilder) {
			int vnum, rnum = kNowhere;
			if (!target.empty() && (vnum = atoi(target.c_str()))) {
				if ((rnum = GetRoomRnum(vnum)) != kNowhere)
					do_stat_room(ch, rnum);
				else
					SendMsgToChar("Состояние какой комнаты?\r\n", ch);
			}
			if (target.empty())
				do_stat_room(ch);
			return;
		}
		if (utils::IsAbbr(what, "mob") && level >= kLvlBuilder) {
			if (target.empty())
				SendMsgToChar("Состояние какого создания?\r\n", ch);
			else {
				victim = target_resolver::FindCharInWorld(ch, target);
				if ((victim != nullptr))
					do_stat_character(ch, victim, 0);
				else
					SendMsgToChar("Нет такого создания в этом МАДе.\r\n", ch);
			}
			return;
		} 
		if (utils::IsAbbr(what, "player")) {
			if (target.empty()) {
				SendMsgToChar("Состояние какого игрока?\r\n", ch);
			} else {
				if ((victim = target_resolver::FindPlayerVis(ch, target)) != nullptr)
					do_stat_character(ch, victim);
				else
					SendMsgToChar("Этого персонажа сейчас нет в игре.\r\n", ch);
			}
			return;
		}
		if (utils::IsAbbr(what, "ip")) {
			if (target.empty()) {
				SendMsgToChar("Состояние ip какого игрока?\r\n", ch);
			} else {
				if ((victim = target_resolver::FindPlayerVis(ch, target)) != nullptr) {
					do_statip(ch, victim);
					return;
				} else {
					SendMsgToChar("Этого персонажа сейчас нет в игре, смотрим пфайл.\r\n", ch);
				}
				Player t_vict;
				if (LoadPlayerCharacter(target.c_str(), &t_vict, ELoadCharFlags::kFindId) > -1) {
					do_statip(ch, &t_vict);
				} else {
					SendMsgToChar("Такого игрока нет ВООБЩЕ.\r\n", ch);
				}
			}
			return;
		}
		if (utils::IsAbbr(what, "karma") || utils::IsAbbr(what, "карма")) {
			if (target.empty()) {
				SendMsgToChar("Карму какого игрока?\r\n", ch);
			} else {
				if ((victim = target_resolver::FindPlayerVis(ch, target)) != nullptr) {
					DoStatKarma(ch, victim);
					return;
				} else {
					SendMsgToChar("Этого персонажа сейчас нет в игре, смотрим пфайл.\r\n", ch);
				}
				Player t_vict;
				if (LoadPlayerCharacter(target.c_str(), &t_vict, ELoadCharFlags::kFindId) > -1) {
					DoStatKarma(ch, &t_vict);
				} else {
					SendMsgToChar("Такого игрока нет ВООБЩЕ.\r\n", ch);
				}
			}
			return;
		}
		if (utils::IsAbbr(what, "file")) {
			if (target.empty()) {
				SendMsgToChar("Состояние какого игрока(из файла)?\r\n", ch);
			} else {
				Player t_vict;
				if (LoadPlayerCharacter(target.c_str(), &t_vict, ELoadCharFlags::kFindId) > -1) {
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
		if (utils::IsAbbr(what, "object") && level >= kLvlBuilder) {
			if (target.empty())
				SendMsgToChar("Состояние какого предмета?\r\n", ch);
			else {
				if ((object = target_resolver::FindObjInWorld(ch, target)) != nullptr)
					do_stat_object(ch, object);
				else
					SendMsgToChar("Нет такого предмета в игре.\r\n", ch);
			}
			return;
		}
	}
	if (privilege::IsImmortal(ch)) {
		if ((object = get_object_in_equip_vis(ch, what, ch->equipment, &tmp)) != nullptr) {
			do_stat_object(ch, object);
			return;
		}
		if ((object = get_obj_in_list_vis(ch, what, ch->carrying)) != nullptr) {
			do_stat_object(ch, object);
			return;
		}
		victim = target_resolver::FindCharInRoom(ch, what);
		if ((victim != nullptr)) {
			do_stat_character(ch, victim);
			return;
		}
		if ((object = get_obj_in_list_vis(ch, what, world[ch->in_room]->contents)) != nullptr) {
			do_stat_object(ch, object);
			return;
		}
		victim = target_resolver::FindCharInWorld(ch, what);
		if ((victim != nullptr)) {
			do_stat_character(ch, victim);
			return;
		}
		{
			object = target_resolver::FindObjInWorld(ch, what);
		}
		if (object != nullptr) {
			do_stat_object(ch, object);
			return;
		}
	} 
	if (GET_OLC_ZONE(ch) == zone_table[world[ch->in_room]->zone_rn].vnum) {
		if ((object = get_object_in_equip_vis(ch, what, ch->equipment, &tmp)) != nullptr) {
			do_stat_object(ch, object);
			return;
		}
		if ((object = get_obj_in_list_vis(ch, what, world[ch->in_room]->contents)) != nullptr) {
			do_stat_object(ch, object);
			return;
		}
		victim = target_resolver::FindCharInRoom(ch, what);
		if ((victim != nullptr)) {
			do_stat_character(ch, victim);
			return;
		}
	}
	SendMsgToChar("Ничего похожего с этим именем нет.\r\n", ch);
}
