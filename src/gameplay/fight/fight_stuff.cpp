// Part of Bylins http://www.mud.ru

#include <random>
#include "administration/privilege.h"
#include "utils/logger.h"
#include "gameplay/core/experience.h"
#include "engine/entities/char_data.h"
#include "gameplay/affects/affect_handler.h"
#include "gameplay/mechanics/alignment.h"
#include "utils/grammar/declensions.h"
#include "gameplay/mechanics/minions.h"
#include "gameplay/mechanics/mount.h"
#include "gameplay/economics/currencies.h"

#include "gameplay/affects/affect_data.h"
#include "gameplay/magic/magic.h"
#include "gameplay/mechanics/dead_load.h"
#include "gameplay/mechanics/dungeons.h"
#include "gameplay/ai/mobact.h"
#include "engine/entities/obj_data.h"
#include "engine/ui/cmd/do_flee.h"
#include "engine/db/world_characters.h"
#include "fight.h"
#include "fight_stuff.h"
#include "gameplay/economics/currencies.h"  // currencies:: gold-on-death (was relied on transitively)
#include "gameplay/skills/spell_capable.h"   // check_spell_capable
#include "utils/grammar/gender.h"             // NumberForm
#include "gameplay/clans/house_exp.h"        // change_rep
#include "fight_penalties.h"
#include "fight_hit.h"
#include "engine/core/char_handler.h"
#include "engine/core/target_resolver.h"
#include "utils/utils_parse.h"
#include "gameplay/mechanics/corpse.h"
#include "gameplay/clans/house.h"
#include "pk.h"
#include "gameplay/mechanics/stuff.h"
#include "engine/ui/color.h"
#include "gameplay/statistics/mob_stat.h"
#include "gameplay/mechanics/bonus.h"
#include "utils/backtrace.h"
#include "gameplay/magic/magic_utils.h"
#include "engine/entities/char_player.h"
#include "engine/core/utils_char_obj.inl"
#include "gameplay/mechanics/sight.h"
#include "gameplay/ai/mob_memory.h"
#include "engine/entities/zone.h"
#include "engine/observability/metrics.h"
#include "gameplay/core/game_limits.h"
#include "gameplay/mechanics/illumination.h"
#include "utils/utils_time.h"
#include "gameplay/skills/leadership.h"
#include "gameplay/core/remort.h"

// extern
void PerformDropGold(CharData *ch, int amount);
void get_from_container(CharData *ch, ObjData *cont, char *local_arg, int mode, int amount, bool autoloot);
void SetWait(CharData *ch, int waittime, int victim_in_room);

extern int max_exp_gain_npc;
extern std::list<combat_list_element> combat_list;

//интервал в секундах между восстановлением кругов после рипа
void process_mobmax(CharData *ch, CharData *killer) {
	bool leader_partner = false;
	int partner_feat = 0;
	int total_group_members = 1;
	CharData *partner = nullptr;

	CharData *master = nullptr;
	if (killer->IsNpc()
		&& (killer->IsFlagged(EMobFlag::kCompanion))
		&& killer->has_master()) {
		master = killer->get_master();
	} else if (!killer->IsNpc()) {
		master = killer;
	}

	// На этот момент master - PC
	if (master) {
		int cnt = 0;
		if (AFF_FLAGGED(master, EAffect::kGroup)) {

			// master - член группы, переходим на лидера группы
			if (master->has_master()) {
				master = master->get_master();
			}

			if (master->in_room == killer->in_room) {
				// лидер группы в тойже комнате, что и убивец
				cnt = 1;
				if (CanUseFeat(master, EFeat::kPartner)) {
					leader_partner = true;
				}
			}

			for (auto *f : master->followers) {
				if (AFF_FLAGGED(f, EAffect::kGroup)) ++total_group_members;
				if (AFF_FLAGGED(f, EAffect::kGroup)
					&& f->in_room == killer->in_room) {
					++cnt;
					if (leader_partner) {
						if (!f->IsNpc()) {
							partner_feat++;
							partner = f;
						}
					}
				}
			}
		}

		// обоим замакс, если способность напарник работает
		// получается замакс идет в 2 раза быстрее, чем без способности в той же группе
		if (leader_partner
			&& partner_feat == 1 && total_group_members == 2) {
			master->mobmax_add(master, GET_MOB_VNUM(ch), 1, GetRealLevel(ch));
			partner->mobmax_add(partner, GET_MOB_VNUM(ch), 1, GetRealLevel(ch));
		} else if (total_group_members > 12) {
			// динамический штраф к замаксу на большие группы (с полководцем >12 человек)
			// +1 человек к замаксу за каждые 4 мембера свыше 12
			// итого: (13-15)->2 (16-19)->3 20->4
			const int above_limit = total_group_members - 12;
			const int mobmax_members = (above_limit / 4) + 2;

			// выбираем всех мемберов в группе в комнате с убивецом
			std::vector<CharData *> members_to_mobmax;
			if (master->in_room == killer->in_room) {
				members_to_mobmax.push_back(master);
			}
			for (auto *f : master->followers) {
				if (AFF_FLAGGED(f, EAffect::kGroup)
					&& f->in_room == killer->in_room) {
					members_to_mobmax.push_back(f);
				}
			}

			// случайно выбираем на замакс
			std::shuffle(members_to_mobmax.begin(), members_to_mobmax.end(), std::mt19937(std::random_device()()));
			const int actual_size_to_mobmax = std::min(static_cast<int>(members_to_mobmax.size()), mobmax_members);
			members_to_mobmax.resize(actual_size_to_mobmax);

			for (const auto &member : members_to_mobmax) {
				member->mobmax_add(member, GET_MOB_VNUM(ch), 1, GetRealLevel(ch));
			}
		} else {
			// выберем случайным образом мембера группы для замакса
			auto n = number(0, cnt);
			int i = 0;
			for (auto *f : master->followers) {
				if (i >= n) break;
				if (AFF_FLAGGED(f, EAffect::kGroup)
					&& f->in_room == killer->in_room) {
					++i;
					master = f;
				}
			}
			master->mobmax_add(master, GET_MOB_VNUM(ch), 1, GetRealLevel(ch));
		}
	}
}

bool stone_rebirth(CharData *ch, CharData *killer) {
	RoomRnum rnum_start, rnum_stop;
	if (ch->IsNpc() && !IsCharmice(ch)){
		return false;
	}
	if (killer && (!killer->IsNpc() || IsCharmice(killer)) && (ch != killer)) { //не нычка в ПК
		return false;
	}
	GetZoneRooms(world[ch->in_room]->zone_rn, &rnum_start, &rnum_stop);
	for (; rnum_start <= rnum_stop; rnum_start++) {
		RoomData *rm = world[rnum_start];
		if (!rm->contents.empty()) {
			for (auto j : rm->contents) {
				if (j->get_vnum() == 1000) { // камень возрождения
					act("$n погиб$q смертью храбрых.", false, ch, nullptr, nullptr, kToRoom);
					if (ch->IsNpc()) {
						mob_ai::extract_charmice(ch, false);
						return true;
					}
					SendMsgToChar("Божественная сила спасла вашу жизнь!\r\n", ch);
					RemoveCharFromRoom(ch);
					PlaceCharToRoom(ch, rnum_start);
					mount::Dismount(ch);
					ch->set_hit(1);
					update_pos(ch);
					while (!ch->affected.empty()) {
						RemoveAffect(ch, ch->affected.begin());
					}
					affect_total(ch);
					ch->SetPosition(EPosition::kStand);
					sight::look_at_room(ch, 0);
					greet_mtrigger(ch, -1);
					greet_otrigger(ch, -1);
					act("$n медленно появил$u откуда-то.", false, ch, nullptr, nullptr, kToRoom);
					SetBattleLag(ch, 10);
					return true;
				}
			}
		}
	}
	return false;
}

bool check_tester_death(CharData *ch, CharData *killer) {
	const bool player_died = !ch->IsNpc();
	const bool zone_is_under_construction = InTestZone(ch);

	if (!player_died || !zone_is_under_construction) {
		return false;
	}

	if (killer && (!killer->IsNpc() || IsCharmice(killer))
		&& (ch != killer)) // рип в тестовой зоне от моба но не чармиса
	{
		return false;
	}

	// Сюда попадают только тестеры на волоске от смерти. Для инх функция должна вернуть true.
	// Теоретически ожидается, что вызывающая функция в этом случае не убъёт игрока-тестера.
	act("$n погиб$q смертью храбрых.", false, ch, nullptr, nullptr, kToRoom);
	const int rent_room = GetRoomRnum(GET_LOADROOM(ch));
	if (rent_room == kNowhere) {
		SendMsgToChar("Вам некуда возвращаться!\r\n", ch);
		return true;
	}
//	enter_wtrigger(world[rent_room], ch, -1);
	SendMsgToChar("Божественная сила спасла вашу жизнь.!\r\n", ch);
	RemoveCharFromRoom(ch);
	PlaceCharToRoom(ch, rent_room);
	mount::Dismount(ch);
	ch->set_hit(1);
	update_pos(ch);
	act("$n медленно появил$u откуда-то.", false, ch, nullptr, nullptr, kToRoom);
	while (!ch->affected.empty()) {
		RemoveAffect(ch, ch->affected.begin());
	}
	affect_total(ch);
	ch->SetPosition(EPosition::kStand);
	sight::look_at_room(ch, 0);
	greet_mtrigger(ch, -1);
	greet_otrigger(ch, -1);
	return true;
}

void die(CharData *ch, CharData *killer) {
	long dec_exp = 0;
	auto char_exp = ch->get_exp();

	if (!ch->IsNpc() && (ch->in_room == kNowhere)) {
		log("SYSERR: %s is dying in room kNowhere.", GET_NAME(ch));
		return;
	}
	if (check_tester_death(ch, killer)) {
		return;
	}
	if (stone_rebirth(ch, killer)) {
		return;
	}
	// issue.character-affect-triggers: kDeath -- an affect may PREVENT this death (a <trigger return="0"/>
	// that also heals via <points>). Like stone_rebirth, we abort die() before raw_kill; the killer still
	// keeps any XP already credited by ProcessDeath (same as the rebirth stone).
	if (RunCharDeathTriggers(ch, killer)) {
		return;
	}

	if (ch->IsNpc()
		|| !ROOM_FLAGGED(ch->in_room, ERoomFlag::kArena)
		|| NORENTABLE(ch)) {
		if (!(ch->IsNpc()
			|| privilege::IsImmortal(ch)
			|| GET_GOD_FLAG(ch, EGf::kGodsLike)
			|| (killer && killer->IsFlagged(EPrf::kExecutor))))//если убил не палач
		{
			if (!NORENTABLE(ch))
				dec_exp =
					(experience::GetExpUntilNextLvl(ch, GetRealLevel(ch) + 1) - experience::GetExpUntilNextLvl(ch, GetRealLevel(ch))) / (3 + std::min(3, remort::GetRealRemort(ch) / 5))
						/ ch->death_player_count();
			else
				dec_exp = (experience::GetExpUntilNextLvl(ch, GetRealLevel(ch) + 1) - experience::GetExpUntilNextLvl(ch, GetRealLevel(ch)))
					/ (3 + std::min(3, remort::GetRealRemort(ch) / 5));
			experience::EndowExpToChar(ch, -dec_exp);
			dec_exp = char_exp - ch->get_exp();
			sprintf(buf, "Вы потеряли %ld %s опыта.\r\n", dec_exp, grammar::GetDeclensionInNumber(dec_exp, grammar::EWhat::kPoint));
			SendMsgToChar(buf, ch);
		}

		// Вычисляем замакс по мобам
		// Решил немножко переделать, чтобы короче получилось,
		// кроме того, исправил ошибку с присутствием лидера в комнате
		if (ch->IsNpc() && killer) {
			process_mobmax(ch, killer);
		}
		if (killer) {
			UpdateLeadership(ch, killer);
		}
	}
	CharStat::UpdateOnKill(ch, killer, dec_exp);
	raw_kill(ch, killer);
}

/* Функция используемая при "автограбеже" и "автолуте",
   чтобы не было лута под холдом или в слепи             */
int can_loot(CharData *ch) {
	if (ch != nullptr) {
		if (!ch->IsNpc()
			&& AFF_FLAGGED(ch, EAffect::kHold) == 0 // если под холдом
			&& !AFF_FLAGGED(ch, EAffect::kStopFight) // парализован точкой
			&& !AFF_FLAGGED(ch, EAffect::kBlind)    // слеп
			&& (ch->GetPosition() >= EPosition::kRest)) // мертв, умирает, без сознания, спит
		{
			return true;
		}
	}
	return false;
}

void death_cry(CharData *ch, CharData *killer) {
	int door;
	if (killer) {
		if (IsCharmice(killer)) {
			if (killer->get_master() && killer->get_master()->in_room == killer->in_room) {
				act("Кровушка стынет в жилах от предсмертного крика $N1.",
					false, killer->get_master(), nullptr, ch, kToRoom | kToNotDeaf);
			}
		} else {
			act("Кровушка стынет в жилах от предсмертного крика $N1.",
				false, killer, nullptr, ch, kToRoom | kToNotDeaf);
		}
	}

	for (door = 0; door < EDirection::kMaxDirNum; door++) {
		if (CAN_GO(ch, door)) {
			const auto room_number = world[ch->in_room]->dir_option[door]->to_room();
			const auto room = world[room_number];
			if (!room->people.empty()) {
				act("Кровушка стынет в жилах от чьего-то предсмертного крика.",
					false, room->first_character(), nullptr, nullptr, kToChar | kToNotDeaf);
				act("Кровушка стынет в жилах от чьего-то предсмертного крика.",
					false, room->first_character(), nullptr, nullptr, kToRoom | kToNotDeaf);
			}
		}
	}
}
void arena_kill(CharData *ch, CharData *killer) {
	make_arena_corpse(ch, killer);
	//Если убил палач то все деньги перекачивают к нему
	if (killer && killer->IsFlagged(EPrf::kExecutor)) {
		currencies::SetHand(*killer, currencies::kGold, currencies::GetHand(*ch, currencies::kGold) + currencies::GetHand(*killer, currencies::kGold));
		currencies::SetHand(*ch, currencies::kGold, 0);
	}
	ChangeFighting(ch, true);
	ch->set_hit(1);
	mount::DropFromHorse(ch);
	ch->SetPosition(EPosition::kSit);
	int to_room = GetRoomRnum(GET_LOADROOM(ch));
	// тут придется ручками тащить чара за ворота, если ему в замке не рады
	if (!Clan::MayEnter(ch, to_room, kHousePortal)) {
		to_room = Clan::CloseRent(to_room);
	}
	if (to_room == kNowhere) {
		ch->SetFlag(EPlrFlag::kHelled);
		punishments::Get(ch, punishments::EType::kHell).duration = time(nullptr) + 6;
		to_room = r_helled_start_room;
	}
	for (auto *f : ch->followers) {
		if (IsCharmice(f)) {
			RemoveCharFromRoom(f);
			PlaceCharToRoom(f, to_room);
		}
	}
	// issue.affect-migration: strip every curable affect (kAfCurable); replaces the old
	// GetRemovableSpellId sweep. This already removes the nemo-poison clusters (every poison slot
	// carries kAfCurable), so the explicit per-poison RemoveAffectFromChar(ESpell::k*Poison) calls
	// that used to follow are redundant and were dropped.
	RemoveCurableAffects(ch);
	affect_total(ch);
	RemoveCharFromRoom(ch);
	PlaceCharToRoom(ch, to_room);
	sight::look_at_room(ch, to_room);
	act("$n со стонами упал$g с небес...", false, ch, nullptr, nullptr, kToRoom);
	enter_wtrigger(world[ch->in_room], ch, -1);
}

void auto_loot(CharData *ch, CharData *killer, ObjData *corpse, int local_gold) {
	char obj[256];

	if (is_dark(killer->in_room)
		&& !(sight::CanSeeInDark(killer) || CanUseFeat(killer, EFeat::kDarkReading))
		&& !(killer->IsNpc()
			&& AFF_FLAGGED(killer, EAffect::kCharmed)
			&& killer->has_master()
			&& CanUseFeat(killer->get_master(), EFeat::kDarkReading))) {
		return;
	}

	if (ch->IsNpc()
		&& !killer->IsNpc()
		&& killer->IsFlagged(EPrf::kAutoloot)
		&& (corpse != nullptr)
		&& can_loot(killer)) {
		sprintf(obj, "all");
		get_from_container(killer, corpse, obj, EFind::kObjInventory, 1, true);
	} else if (ch->IsNpc()
		&& !killer->IsNpc()
		&& local_gold
		&& killer->IsFlagged(EPrf::kAutomoney)
		&& (corpse != nullptr)
		&& can_loot(killer)) {
		sprintf(obj, "all.coin");
		get_from_container(killer, corpse, obj, EFind::kObjInventory, 1, false);
	} else if (ch->IsNpc()
		&& killer->IsNpc()
		&& (killer->IsFlagged(EMobFlag::kCompanion))
		&& (corpse != nullptr)
		&& killer->has_master()
		&& killer->in_room == killer->get_master()->in_room
		&& killer->get_master()->IsFlagged(EPrf::kAutoloot)
		&& can_loot(killer->get_master())) {
		sprintf(obj, "all");
		get_from_container(killer->get_master(), corpse, obj, EFind::kObjInventory, 1, true);
	} else if (ch->IsNpc()
		&& killer->IsNpc()
		&& local_gold
		&& (killer->IsFlagged(EMobFlag::kCompanion))
		&& (corpse != nullptr)
		&& killer->has_master()
		&& killer->in_room == killer->get_master()->in_room
		&& killer->get_master()->IsFlagged(EPrf::kAutomoney)
		&& can_loot(killer->get_master())) {
		sprintf(obj, "all.coin");
		get_from_container(killer->get_master(), corpse, obj, EFind::kObjInventory, 1, false);
	}
}

void clear_mobs_memory(CharData *ch) {
	for (const auto &hitter : character_list) {
		if (hitter->IsNpc() && MEMORY(hitter)) {
			mob_ai::mobForget(hitter.get(), ch);
		}
	}
}

void real_kill(CharData *ch, CharData *killer) {
	const long local_gold = currencies::GetHand(*ch, currencies::kGold);
	ObjData *corpse = make_corpse(ch, killer);

	bloody::handle_corpse(corpse, ch, killer);
	// Перенес вызов pk_revenge_action из die, чтобы на момент создания
	// трупа месть на убийцу была еще жива
	if (ch->IsNpc() || !ROOM_FLAGGED(ch->in_room, ERoomFlag::kArena) || NORENTABLE(ch)) {
		pk_revenge_action(killer, ch);
	}
	if (!ch->IsNpc()) {
		forget_all_spells(ch);
		clear_mobs_memory(ch);
		// Если убит в бою - то может выйти из игры
		ch->player_specials->may_rent = 0;
		AGRESSOR(ch) = 0;
		AGRO(ch) = 0;
		ch->agrobd = false;
#if defined WITH_SCRIPTING
		//scripting::on_pc_dead(ch, killer, corpse);
#endif
	} else {
		if (killer && (!killer->IsNpc() || IsCharmice(killer))) {
			log("Killed: %d %d %ld", GetRealLevel(ch), ch->get_max_hit(), ch->get_exp());
			obj_load_on_death(corpse, ch);
		}
		if (ch->IsFlagged(EMobFlag::kCorpse)) {
			PerformDropGold(ch, local_gold);
			currencies::SetHand(*ch, currencies::kGold, 0);
		}
		dead_load::LoadObjFromDeadLoad(corpse, ch, nullptr, dead_load::kOrdinary);
#if defined WITH_SCRIPTING
		//scripting::on_npc_dead(ch, killer, corpse);
#endif
	}
/*	до будущих времен
	if (!ch->IsNpc() && remort::GetRealRemort(ch) > 7 && (GetRealLevel(ch) == 29 || GetRealLevel(ch) == 30))
	{
		// лоадим свиток с экспой
		const auto rnum = GetObjRnum(100);
		if (rnum >= 0)
		{
			const auto o = world_objects.create_from_prototype_by_rnum(rnum);
			o->set_owner(ch->get_uid());
			obj_to_obj(o.get(), corpse);
		}

	}
*/
	// Теперь реализация режимов "автограбеж" и "брать куны" происходит не в damage,
	// а здесь, после создания соответствующего трупа. Кроме того,
	// если убил чармис и хозяин в комнате, то автолут происходит хозяину
	if ((ch != nullptr) && (killer != nullptr)) {
		auto_loot(ch, killer, corpse, local_gold);
	}
}

void raw_kill(CharData *ch, CharData *killer) {
	check_spell_capable(ch, killer);
	if (ch->GetEnemy()) {
		stop_fighting(ch, true);
	}
	for (auto &hitter : combat_list) {
		if (hitter.deleted)
			continue;
		if (hitter.ch->GetEnemy() == ch) {
			hitter.ch->zero_wait();
		}
	}
	if (!ch || ch->purged()) {
//		debug::coredump();
		debug::backtrace(runtime_config.logs(ERRLOG).handle());
		mudlog("SYSERR: Опять где-то кто-то спуржился не в то в время, не в том месте. Сброшен текущий стек и кора.",
				NRM, kLvlGod, ERRLOG, true);
		return;
	}
	// OpenTelemetry: count player deaths
	if (!ch->IsNpc()) {
		std::string death_type = "pve";
		if (killer && !killer->IsNpc() && !IsCharmice(killer)) {
			death_type = "pvp";
		} else if (!killer) {
			death_type = "other";
		}
		std::map<std::string, std::string> death_attrs;
		death_attrs["death_type"] = death_type;
		observability::OtelMetrics::RecordCounter("player.deaths.total", 1, death_attrs);
	}

	if (!ROOM_FLAGGED(ch->in_room, ERoomFlag::kDominationArena)) {
		if (!ch->IsNpc()) {
			reset_affects_no_recalc(ch);
		}
	}
	// для начала проверяем, активны ли евенты
	if ((!killer || death_mtrigger(ch, killer)) && ch->in_room != kNowhere) {
		death_cry(ch, killer);
	}

	if (killer && killer->IsNpc() && !ch->IsNpc() && kill_mtrigger(killer, ch)) {
		const auto it = std::find(killer->kill_list.begin(), killer->kill_list.end(), ch->get_uid());
		if (it != killer->kill_list.end()) {
			killer->kill_list.erase(it);
		}
		killer->kill_list.push_back(ch->get_uid());
	}

	// добавляем одну душу киллеру
	if (ch->IsNpc() && killer) {
		if (CanUseFeat(killer, EFeat::kSoulsCollector)) {
			if (GetRealLevel(ch) >= GetRealLevel(killer)) {
				if (killer->get_souls() < (remort::GetRealRemort(killer) + 1)) {
					act("&GВы забрали душу $N1 себе!&n", false, killer, nullptr, ch, kToChar);
					act("$n забрал душу $N1 себе!", false, killer, nullptr, ch, kToNotVict | kToArenaListen);
					killer->inc_souls();
				}
			}
		}
	}
	if (ch->in_room != kNowhere) {
		if (killer && (!killer->IsNpc() || IsCharmice(killer)) && !ch->IsNpc())
 			kill_pc_wtrigger(killer, ch);
		if (!ch->IsNpc() && (!NORENTABLE(ch) && ROOM_FLAGGED(ch->in_room, ERoomFlag::kArena))) {
			//Если убили на арене
			arena_kill(ch, killer);
		} else if (change_rep(ch, killer)) {
			// клановые не теряют вещи
			arena_kill(ch, killer);
		} else {
			real_kill(ch, killer);
			character_list.AddToExtractedList(ch);
//			ExtractCharFromWorld(ch, true);
		}
	}
}

bool check_valid_chars(CharData *ch, CharData *victim, const char *fname, int line) {
	if (!ch || ch->purged() || !victim || victim->purged()) {
		log("SYSERROR: ch = %s, victim = %s (%s:%d)",
			ch ? (ch->purged() ? "purged" : "true") : "false",
			victim ? (victim->purged() ? "purged" : "true") : "false",
			fname, line);
		return false;
	}
	return true;
}

/*
 * Alert: As of bpl14, this function returns the following codes:
 *	< 0	Victim  died.
 *	= 0	No damage.
 *	> 0	How much damage done.
 */

void char_dam_message(int dam, CharData *ch, CharData *victim, bool noflee) {
	if (ch->in_room == kNowhere)
		return;
	if (!victim || victim->purged())
		return;
	char msg[256];
	switch (victim->GetPosition()) {
		case EPosition::kPerish:
			snprintf(msg, sizeof(msg), "$n смертельно ранен$a и умр%s, если $m не помогут.",
					grammar::NumberForm(IsPoly(victim), "ет", "ут"));
			act(msg, true, victim, nullptr, nullptr, kToRoom | kToArenaListen);
			SendMsgToChar("Вы смертельно ранены и умрете, если вам не помогут.\r\n", victim);
			break;
		case EPosition::kIncap:
			snprintf(msg, sizeof(msg), "$n без сознания и медленно умира%s. Помогите же $m.",
					grammar::NumberForm(IsPoly(victim), "ет", "ют"));
			act(msg, true, victim, nullptr, nullptr, kToRoom | kToArenaListen);
			SendMsgToChar("Вы без сознания и медленно умираете, брошенные без помощи.\r\n", victim);
			break;
		case EPosition::kStun:
			snprintf(msg, sizeof(msg), "$n без сознания, но возможно $e еще повою%s (попозже :).",
					grammar::NumberForm(IsPoly(victim), "ет", "ют"));
			act(msg, true, victim, nullptr, nullptr, kToRoom | kToArenaListen);
			SendMsgToChar("Сознание покинуло вас. В битве от вас пока проку мало.\r\n", victim);
			break;
		case EPosition::kDead:
			if (victim->IsNpc() && (victim->IsFlagged(EMobFlag::kCorpse))) {
				act("$n вспыхнул$g и рассыпал$u в прах.", false, victim, nullptr, nullptr, kToRoom | kToArenaListen);
				SendMsgToChar("Похоже вас убили и даже тела не оставили!\r\n", victim);
			} else {
				snprintf(msg, sizeof(msg), "$n мертв$a, $s душ%s медленно подыма%s в небеса.",
						grammar::NumberForm(IsPoly(victim), "а", "и"),
						grammar::NumberForm(IsPoly(victim), "ется", "ются"));
				act(msg, false, victim, nullptr, nullptr, kToRoom | kToArenaListen);
				SendMsgToChar("Вы мертвы! Нам очень жаль...\r\n", victim);
			}
			break;
		default:        // >= POSITION SLEEPING
			if (dam > (victim->get_real_max_hit() / 4)) {
				SendMsgToChar("Это действительно БОЛЬНО!\r\n", victim);
			}

			if (dam > 0
				&& victim->get_hit() < (victim->get_real_max_hit() / 4)) {
				sprintf(buf2, "%s Вы желаете, чтобы ваши раны не кровоточили так сильно! %s\r\n",
						kColorRed, kColorNrm);
				SendMsgToChar(buf2, victim);
			}

			if (ch != victim
				&& victim->IsNpc()
				&& victim->get_hit() < (victim->get_real_max_hit() / 4)
				&& victim->IsFlagged(EMobFlag::kWimpy)
				&& !noflee
				&& victim->GetPosition() > EPosition::kSit) {
				DoFlee(victim, nullptr, 0, 0);
			}

			if (ch != victim
				&& victim->GetPosition() > EPosition::kSit
				&& !victim->IsNpc()
				&& HERE(victim)
				&& GET_WIMP_LEV(victim)
				&& victim->get_hit() < GET_WIMP_LEV(victim)
				&& !noflee) {
				SendMsgToChar("Вы запаниковали и попытались убежать!\r\n", victim);
				DoFlee(victim, nullptr, 0, 0);
			}

			break;
	}
}

void do_show_mobmax(CharData *ch, char *, int, int) {
	const auto player = dynamic_cast<Player *>(ch);
	if (nullptr == player) {
		// написать в лог, что show_mobmax была вызвана не игроком
		return;
	}
	SendMsgToChar(ch, "&BВ стадии тестирования!!!&n\n");
	player->show_mobmax();
}

void ChangeFighting(CharData *ch, int need_stop) {
//	utils::CExecutionTimer time;
//	log("ChangeFighting start %f vnum %d", time.delta().count(), GET_MOB_VNUM(ch));
	//Loop for all entities is necessary for unprotecting
//	for (const auto &k : character_list) {
	for (const auto &k : world[ch->in_room]->people) {

		if (k->get_protecting() == ch) {
			k->remove_protecting();
		}
//		log("ChangeFighting protecting %f", time.delta().count());
		if (k->get_touching() == ch) {
			k->set_touching(0);
		}
//		log("ChangeFighting touching %f", time.delta().count());
		if (k->GetExtraVictim() == ch) {
			k->SetExtraAttack(kExtraAttackUnused, 0);
		}

		if (k->GetCastChar() == ch) {
			k->SetCast(ESpell::kUndefined, ESpell::kUndefined, 0, 0, 0);
		}
//		log("ChangeFighting set cast %f", time.delta().count());

		if (k->GetEnemy() == ch && k->in_room != kNowhere) {
//			log("ChangeFighting Change victim %f", time.delta().count());
			bool found = false;
			for (const auto j : world[ch->in_room]->people) {
				if (j->GetEnemy() == k) {
					act("Вы переключили внимание на $N3.", false, k, 0, j, kToChar);
					act("$n переключил$u на вас!", false, k, 0, j, kToVict);
					k->SetEnemy(j);
					found = true;
//					log("ChangeFighting Change victim %f", time.delta().count());
					break;
				}
			}

			if (!found && need_stop) {
//				log("ChangeFighting stop fighting %f", time.delta().count());
				stop_fighting(k, false);
			}
		}
	}
//	log("ChangeFighting stop %f", time.delta().count());
}

bool MayAttack(const CharData *sub) {
	return (!AFF_FLAGGED((sub), EAffect::kCharmed)
		&& !mount::IsHorse((sub))
		&& !AFF_FLAGGED((sub), EAffect::kStopFight)
		&& !AFF_FLAGGED((sub), EAffect::kMagicStopFight)
		&& !AFF_FLAGGED((sub), EAffect::kHold)
		&& !AFF_FLAGGED((sub), EAffect::kSleep)
		&& !(sub)->IsFlagged(EMobFlag::kNoFight)
		&& sub->get_wait() <= 0
		&& !sub->GetEnemy()
		&& sub->GetPosition() >= EPosition::kRest);
}

// issue.chardata-cleaning: was CharData::HasWeapon.
bool HasWeapon(const CharData *ch) {
	if ((GET_EQ(ch, EEquipPos::kWield)
	  && GET_EQ(ch, EEquipPos::kWield)->get_type() != EObjType::kLightSource)
	  || (GET_EQ(ch, EEquipPos::kHold)
	  && GET_EQ(ch, EEquipPos::kHold)->get_type() != EObjType::kLightSource)
	  || (GET_EQ(ch, EEquipPos::kBoths)
	  && GET_EQ(ch, EEquipPos::kBoths)->get_type() != EObjType::kLightSource)) {
		return true;
	}
	return false;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
