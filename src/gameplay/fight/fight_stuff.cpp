// Part of Bylins http://www.mud.ru

#include <random>

#include "gameplay/affects/affect_data.h"
#include "gameplay/mechanics/dead_load.h"
#include "gameplay/mechanics/dungeons.h"
#include "gameplay/ai/mobact.h"
#include "engine/entities/obj_data.h"
#include "engine/ui/cmd/do_flee.h"
#include "engine/db/world_characters.h"
#include "fight.h"
#include "fight_penalties.h"
#include "fight_hit.h"
#include "engine/core/handler.h"
#include "gameplay/mechanics/corpse.h"
#include "gameplay/clans/house.h"
#include "pk.h"
#include "gameplay/mechanics/stuff.h"
#include "gameplay/statistics/top.h"
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
#include "gameplay/core/game_limits.h"
#include "gameplay/mechanics/illumination.h"
#include "utils/utils_time.h"
#include "gameplay/skills/leadership.h"

// extern
void PerformDropGold(CharData *ch, int amount);
int max_exp_gain_pc(CharData *ch);
int max_exp_loss_pc(CharData *ch);
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
		&& (AFF_FLAGGED(killer, EAffect::kCharmed)
			|| killer->IsFlagged(EMobFlag::kTutelar)
			|| killer->IsFlagged(EMobFlag::kMentalShadow))
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

			for (struct FollowerType *f = master->followers; f; f = f->next) {
				if (AFF_FLAGGED(f->follower, EAffect::kGroup)) ++total_group_members;
				if (AFF_FLAGGED(f->follower, EAffect::kGroup)
					&& f->follower->in_room == killer->in_room) {
					++cnt;
					if (leader_partner) {
						if (!f->follower->IsNpc()) {
							partner_feat++;
							partner = f->follower;
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
			for (struct FollowerType *f = master->followers; f; f = f->next) {
				if (AFF_FLAGGED(f->follower, EAffect::kGroup)
					&& f->follower->in_room == killer->in_room) {
					members_to_mobmax.push_back(f->follower);
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
			for (struct FollowerType *f = master->followers; f && i < n; f = f->next) {
				if (AFF_FLAGGED(f->follower, EAffect::kGroup)
					&& f->follower->in_room == killer->in_room) {
					++i;
					master = f->follower;
				}
			}
			master->mobmax_add(master, GET_MOB_VNUM(ch), 1, GetRealLevel(ch));
		}
	}
}

bool stone_rebirth(CharData *ch, CharData *killer) {
	RoomRnum rnum_start, rnum_stop;
	if (ch->IsNpc() && !IS_CHARMICE(ch)){
		return false;
	}
	if (killer && (!killer->IsNpc() || IS_CHARMICE(killer)) && (ch != killer)) { //не нычка в ПК
		return false;
	}
	GetZoneRooms(world[ch->in_room]->zone_rn, &rnum_start, &rnum_stop);
	for (; rnum_start <= rnum_stop; rnum_start++) {
		RoomData *rm = world[rnum_start];
		if (rm->contents) {
			for (ObjData *j = rm->contents; j; j = j->get_next_content()) {
				if (j->get_vnum() == 1000) { // камень возрождения
					act("$n погиб$q смертью храбрых.", false, ch, nullptr, nullptr, kToRoom);
					if (ch->IsNpc()) {
						mob_ai::extract_charmice(ch, false);
						return true;
					}
					SendMsgToChar("Божественная сила спасла вашу жизнь!\r\n", ch);
					RemoveCharFromRoom(ch);
					PlaceCharToRoom(ch, rnum_start);
					ch->dismount();
					ch->set_hit(1);
					update_pos(ch);
					while (!ch->affected.empty()) {
						ch->AffectRemove(ch->affected.begin());
					}
					affect_total(ch);
					ch->SetPosition(EPosition::kStand);
					look_at_room(ch, 0);
					greet_mtrigger(ch, -1);
					greet_otrigger(ch, -1);
					act("$n медленно появил$u откуда-то.", false, ch, nullptr, nullptr, kToRoom);
					SetWaitState(ch, 10 * kBattleRound);
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

	if (killer && (!killer->IsNpc() || IS_CHARMICE(killer))
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
	ch->dismount();
	ch->set_hit(1);
	update_pos(ch);
	act("$n медленно появил$u откуда-то.", false, ch, nullptr, nullptr, kToRoom);
	while (!ch->affected.empty()) {
		ch->AffectRemove(ch->affected.begin());
	}
	affect_total(ch);
	ch->SetPosition(EPosition::kStand);
	look_at_room(ch, 0);
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
	if (!ch->IsNpc() && (GetZoneVnumByCharPlace(ch) == 759)
		&& (GetRealLevel(ch) < 15)) //нуб помер в мадшколе
	{
		act("$n глупо погиб$q не закончив обучение.", false, ch, nullptr, nullptr, kToRoom);
		RemoveCharFromRoom(ch);
		PlaceCharToRoom(ch, GetRoomRnum(75989));
		ch->dismount();
		ch->set_hit(1);
		update_pos(ch);
		act("$n медленно появил$u откуда-то.", false, ch, nullptr, nullptr, kToRoom);
		look_at_room(ch, 0);
		greet_mtrigger(ch, -1);
		greet_otrigger(ch, -1);
//		WAIT_STATE(ch, 10 * kBattleRound); лаг лучше ставить триггерами
		return;
	}

	if (ch->IsNpc()
		|| !ROOM_FLAGGED(ch->in_room, ERoomFlag::kArena)
		|| NORENTABLE(ch)) {
		if (!(ch->IsNpc()
			|| IS_IMMORTAL(ch)
			|| GET_GOD_FLAG(ch, EGf::kGodsLike)
			|| (killer && killer->IsFlagged(EPrf::kExecutor))))//если убил не палач
		{
			if (!NORENTABLE(ch))
				dec_exp =
					(GetExpUntilNextLvl(ch, GetRealLevel(ch) + 1) - GetExpUntilNextLvl(ch, GetRealLevel(ch))) / (3 + std::min(3, GetRealRemort(ch) / 5))
						/ ch->death_player_count();
			else
				dec_exp = (GetExpUntilNextLvl(ch, GetRealLevel(ch) + 1) - GetExpUntilNextLvl(ch, GetRealLevel(ch)))
					/ (3 + std::min(3, GetRealRemort(ch) / 5));
			EndowExpToChar(ch, -dec_exp);
			dec_exp = char_exp - ch->get_exp();
			sprintf(buf, "Вы потеряли %ld %s опыта.\r\n", dec_exp, GetDeclensionInNumber(dec_exp, EWhat::kPoint));
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
		if (IS_CHARMICE(killer)) {
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
		killer->set_gold(ch->get_gold() + killer->get_gold());
		ch->set_gold(0);
	}
	change_fighting(ch, true);
	ch->set_hit(1);
	ch->DropFromHorse();
	ch->SetPosition(EPosition::kSit);
	int to_room = GetRoomRnum(GET_LOADROOM(ch));
	// тут придется ручками тащить чара за ворота, если ему в замке не рады
	if (!Clan::MayEnter(ch, to_room, kHousePortal)) {
		to_room = Clan::CloseRent(to_room);
	}
	if (to_room == kNowhere) {
		ch->SetFlag(EPlrFlag::kHelled);
		HELL_DURATION(ch) = time(nullptr) + 6;
		to_room = r_helled_start_room;
	}
	for (FollowerType *f = ch->followers; f; f = f->next) {
		if (IS_CHARMICE(f->follower)) {
			RemoveCharFromRoom(f->follower);
			PlaceCharToRoom(f->follower, to_room);
		}
	}
	for (int i=0; i < MAX_FIRSTAID_REMOVE; i++) {
		RemoveAffectFromChar(ch, GetRemovableSpellId(i));
	}
	// наемовские яды
	RemoveAffectFromChar(ch, ESpell::kAconitumPoison);
	RemoveAffectFromChar(ch, ESpell::kDaturaPoison);
	RemoveAffectFromChar(ch, ESpell::kScopolaPoison);
	RemoveAffectFromChar(ch, ESpell::kBelenaPoison);
	affect_total(ch);
	RemoveCharFromRoom(ch);
	PlaceCharToRoom(ch, to_room);
	look_at_room(ch, to_room);
	act("$n со стонами упал$g с небес...", false, ch, nullptr, nullptr, kToRoom);
	enter_wtrigger(world[ch->in_room], ch, -1);
}

void auto_loot(CharData *ch, CharData *killer, ObjData *corpse, int local_gold) {
	char obj[256];

	if (is_dark(killer->in_room)
		&& !(CAN_SEE_IN_DARK(killer) || CanUseFeat(killer, EFeat::kDarkReading))
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
		&& (AFF_FLAGGED(killer, EAffect::kCharmed)
			|| killer->IsFlagged(EMobFlag::kTutelar)
			|| killer->IsFlagged(EMobFlag::kMentalShadow))
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
		&& (AFF_FLAGGED(killer, EAffect::kCharmed)
			|| killer->IsFlagged(EMobFlag::kTutelar)
			|| killer->IsFlagged(EMobFlag::kMentalShadow))
		&& (corpse != nullptr)
		&& killer->has_master()
		&& killer->in_room == killer->get_master()->in_room
		&& killer->get_master()->IsFlagged(EPrf::kAutomoney)
		&& can_loot(killer->get_master())) {
		sprintf(obj, "all.coin");
		get_from_container(killer->get_master(), corpse, obj, EFind::kObjInventory, 1, false);
	}
}

void check_spell_capable(CharData *ch, CharData *killer) {
	if (ch->IsNpc()
		&& killer
		&& killer != ch
		&& ch->IsFlagged(EMobFlag::kClone)
		&& ch->has_master()
		&& IsAffectedBySpell(ch, ESpell::kCapable)) {
		RemoveAffectFromCharAndRecalculate(ch, ESpell::kCapable);
		act("Чары, наложенные на $n3, тускло засветились и стали превращаться в нечто опасное.",
			false, ch, nullptr, killer, kToRoom | kToArenaListen);
		auto pos = ch->GetPosition();
		ch->SetPosition(EPosition::kStand);
		CallMagic(ch, killer, nullptr, world[ch->in_room], ch->mob_specials.capable_spell, GetRealLevel(ch));
		ch->SetPosition(pos);
	}
}

void clear_mobs_memory(CharData *ch) {
	for (const auto &hitter : character_list) {
		if (hitter->IsNpc() && MEMORY(hitter)) {
			mob_ai::mobForget(hitter.get(), ch);
		}
	}
}

bool change_rep(CharData *ch, CharData *killer) {
	return false;
	// проверяем, в кланах ли оба игрока
	if ((!CLAN(ch)) || (!CLAN(killer)))
		return false;
	// кланы должны быть разные
	if (CLAN(ch) == CLAN(killer))
		return false;

	// 1/10 репутации замка уходит замку киллера
	int rep_ch = CLAN(ch)->get_rep() * 0.1 + 1;
	CLAN(ch)->set_rep(CLAN(ch)->get_rep() - rep_ch);
	CLAN(killer)->set_rep(CLAN(killer)->get_rep() + rep_ch);
	SendMsgToChar("Вы потеряли очко репутации своего клана! Стыд и позор вам.\r\n", ch);
	SendMsgToChar("Вы заработали очко репутации для своего клана! Честь и похвала вам.\r\n", killer);
	// проверяем репу клана у убитого
	if (CLAN(ch)->get_rep() < 1) {
		// сам распустится
		//CLAN(ch)->bank = 0;
	}
	return true;
}

void real_kill(CharData *ch, CharData *killer) {
	const long local_gold = ch->get_gold();
	ObjData *corpse = make_corpse(ch, killer);
	utils::CSteppedProfiler round_profiler(fmt::format("real_kill"), 0.01);

	round_profiler.next_step("handle_corpse");
	bloody::handle_corpse(corpse, ch, killer);
	// Перенес вызов pk_revenge_action из die, чтобы на момент создания
	// трупа месть на убийцу была еще жива
	if (ch->IsNpc() || !ROOM_FLAGGED(ch->in_room, ERoomFlag::kArena) || NORENTABLE(ch)) {
		round_profiler.next_step("pk_revenge_action");
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
		if (killer && (!killer->IsNpc() || IS_CHARMICE(killer))) {
			log("Killed: %d %d %ld", GetRealLevel(ch), ch->get_max_hit(), ch->get_exp());
			obj_load_on_death(corpse, ch);
		}
		if (ch->IsFlagged(EMobFlag::kCorpse)) {
			round_profiler.next_step("PerformDropGold");
			PerformDropGold(ch, local_gold);
			ch->set_gold(0);
		}
		round_profiler.next_step("LoadObjFromDeadLoad");
		dead_load::LoadObjFromDeadLoad(corpse, ch, nullptr, dead_load::kOrdinary);
#if defined WITH_SCRIPTING
		//scripting::on_npc_dead(ch, killer, corpse);
#endif
	}
/*	до будущих времен
	if (!ch->IsNpc() && GetRealRemort(ch) > 7 && (GetRealLevel(ch) == 29 || GetRealLevel(ch) == 30))
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
	round_profiler.next_step("auto_loot");
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
			SetWaitState(hitter.ch, 0);
		}
	}
	if (!ch || ch->purged()) {
//		debug::coredump();
		debug::backtrace(runtime_config.logs(ERRLOG).handle());
		mudlog("SYSERR: Опять где-то кто-то спуржился не в то в время, не в том месте. Сброшен текущий стек и кора.",
				NRM, kLvlGod, ERRLOG, true);
		return;
	}
	if (!ROOM_FLAGGED(ch->in_room, ERoomFlag::kDominationArena)) {
		reset_affects(ch);
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
				if (killer->get_souls() < (GetRealRemort(killer) + 1)) {
					act("&GВы забрали душу $N1 себе!&n", false, killer, nullptr, ch, kToChar);
					act("$n забрал душу $N1 себе!", false, killer, nullptr, ch, kToNotVict | kToArenaListen);
					killer->inc_souls();
				}
			}
		}
	}
	if (ch->in_room != kNowhere) {
		if (killer && (!killer->IsNpc() || IS_CHARMICE(killer)) && !ch->IsNpc())
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

int get_remort_mobmax(CharData *ch) {
	int remort = GetRealRemort(ch);
	if (remort >= 18)
		return 15;
	if (remort >= 14)
		return 7;
	if (remort >= 9)
		return 4;
	return 0;
}

// даем увеличенную экспу за давно не убитых мобов.
// за совсем неубитых мобов не даем, что бы новые зоны не давали x10 экспу.
int get_npc_long_live_exp_bounus(CharData *victim) {
	if (GET_MOB_VNUM(victim) == -1) {
		return 1;
	}
	if (GET_MOB_VNUM(victim) / 100 >= dungeons::kZoneStartDungeons) {
		return 1;
	}

	int exp_multiplier = 1;

	const auto last_kill_time = mob_stat::GetMobKilllastTime(GET_MOB_VNUM(victim));
	if (last_kill_time > 0) {
		const auto now_time = time(nullptr);
		if (now_time > last_kill_time) {
			const auto delta_time = now_time - last_kill_time;
			constexpr long delay = 60 * 60 * 24 * 30; // 30 days
			exp_multiplier = std::clamp(static_cast<int>(floor(delta_time / delay)), 1, 10);
		}
	}

	return exp_multiplier;
}

long long get_extend_exp(long long exp, CharData *ch, CharData *victim) {
	int base, diff;
	int koef;

	if (!victim->IsNpc() || ch->IsNpc())
		return (exp);
	MobVnum vnum  = GET_MOB_VNUM(victim);
	if (vnum >= dungeons::kZoneStartDungeons * 100) {
		ZoneVnum zvn = vnum / 100;
		MobVnum  mvn = vnum % 100;
		vnum = zone_table[GetZoneRnum(zvn)].copy_from_zone * 100 + mvn;
	}

	ch->send_to_TC(false, true, false,
				   "&RУ моба еще %d убийств без замакса, экспа %d, убито %d&n\r\n",
				   victim->mob_specials.MaxFactor,
				   exp,
				   ch->mobmax_get(vnum));

	exp += static_cast<int>(exp * (ch->add_abils.percent_exp_add) / 100.0);
	for (koef = 100, base = 0, diff =
		 ch->mobmax_get(vnum) - victim->mob_specials.MaxFactor;
		 base < diff && koef > 5; base++, koef = koef * (95 - get_remort_mobmax(ch)) / 100);
	// минимальный опыт при замаксе 15% от полного опыта
	exp = exp * std::max(15, koef) / 100;

	// делим на реморты
	exp /= std::max(1.0, 0.5 * (GetRealRemort(ch) - kMaxExpCoefficientsUsed));
	return (exp);
}

// When ch kills victim
void change_alignment(CharData *ch, CharData *victim) {
	/*
	 * new alignment change algorithm: if you kill a monster with alignment A,
	 * you move 1/16th of the way to having alignment -A.  Simple and fast.
	 */
	GET_ALIGNMENT(ch) += (-GET_ALIGNMENT(victim) - GET_ALIGNMENT(ch)) / 16;
}

/*++
   Функция начисления опыта
      ch - кому опыт начислять
           Вызов этой функции для NPC смысла не имеет, но все равно
           какие-то проверки внутри зачем то делаются
--*/
void perform_group_gain(CharData *ch, CharData *victim, int members, int koef) {


// Странно, но для NPC эта функция тоже должна работать
//  if (ch->IsNpc() || !OK_GAIN_EXP(ch,victim))
	if (!OK_GAIN_EXP(ch, victim)) {
		SendMsgToChar("Ваше деяние никто не оценил.\r\n", ch);
		return;
	}

	// 1. Опыт делится поровну на всех
	long long exp = victim->get_exp() / std::max(members, 1);
	int long_live_exp_bounus_miltiplier = 1;

	if (victim->get_zone_group() > 1 && members < victim->get_zone_group()) {
		// в случае груп-зоны своего рода планка на мин кол-во человек в группе
		exp = victim->get_exp() / victim->get_zone_group();
	}
	// 2. Учитывается коэффициент (лидерство, разность уровней)
	//    На мой взгляд его правильней использовать тут а не в конце процедуры,
	//    хотя в большинстве случаев это все равно
	exp = exp * koef / 100;
	// 3. Вычисление опыта для PC и NPC
	if (!NPC_FLAGGED(victim, ENpcFlag::kIgnoreRareKill)) {
		long_live_exp_bounus_miltiplier = get_npc_long_live_exp_bounus(victim);
	}
	if (ch->IsNpc()) {
		exp = std::min(static_cast<long long>(max_exp_gain_npc), exp);
		exp += std::max(static_cast<long long>(0), (exp * std::min(0, (GetRealLevel(victim) - GetRealLevel(ch)))) / 8);
	} else
		exp = std::min(static_cast<long long>(max_exp_gain_pc(ch)), get_extend_exp(exp, ch, victim) * long_live_exp_bounus_miltiplier);
	// 4. Последняя проверка
	exp = std::max(static_cast<long long>(1), exp);
	if (exp > 1) {
		if (Bonus::is_bonus_active(Bonus::EBonusType::BONUS_EXP) && Bonus::can_get_bonus_exp(ch)) {
			exp *= Bonus::get_mult_bonus();
		}

		if (!ch->IsNpc() && !ch->affected.empty() && Bonus::can_get_bonus_exp(ch)) {
			for (const auto &aff : ch->affected) {
				if (aff->location == EApply::kExpBonus) // скушал свиток с эксп бонусом
				{
					exp *= std::min(3, aff->modifier); // бонус макс тройной
				}
			}
		}

		if (long_live_exp_bounus_miltiplier > 1) {
			std::string mess;
			switch (long_live_exp_bounus_miltiplier) {
				case 2: mess = "Редкая удача! Опыт повышен!\r\n";
					break;
				case 3: mess = "Очень редкая удача! Опыт повышен!\r\n";
					break;
				case 4: mess = "Очень-очень редкая удача! Опыт повышен!\r\n";
					break;
				case 5: mess = "Вы везунчик! Опыт повышен!\r\n";
					break;
				case 6: mess = "Ваша удача велика! Опыт повышен!\r\n";
					break;
				case 7: mess = "Ваша удача достигла небес! Опыт повышен!\r\n";
					break;
				case 8: mess = "Ваша удача коснулась луны! Опыт повышен!\r\n";
					break;
				case 9: mess = "Ваша удача затмевает солнце! Опыт повышен!\r\n";
					break;
				default: mess = "Ваша удача выше звезд! Опыт повышен!\r\n";
					break;
			}
			SendMsgToChar(mess.c_str(), ch);
		}
		if (long_live_exp_bounus_miltiplier >= 10) {
			const CharData *ch_with_bonus = ch->IsNpc() ? ch->get_master() : ch;
			if (ch_with_bonus && !ch_with_bonus->IsNpc()) {
				std::stringstream str_log;
				str_log << "[INFO] " << ch_with_bonus->get_name() << " получил(а) x" << long_live_exp_bounus_miltiplier << " опыта за убийство моба: [";
				str_log << GET_MOB_VNUM(victim) << "] " << victim->get_name();
				mudlog(str_log.str(), NRM, kLvlImmortal, SYSLOG, true);
			}
		}

		exp = std::min(static_cast<long long>(max_exp_gain_pc(ch)), exp);
		SendMsgToChar(ch, "Ваш опыт повысился на %lld %s.\r\n", exp, GetDeclensionInNumber(exp, EWhat::kPoint));
	} else if (exp == 1) {
		SendMsgToChar("Ваш опыт повысился всего лишь на маленькую единичку.\r\n", ch);
	}
	if (!InTestZone(ch)) {
		EndowExpToChar(ch, exp);
		change_alignment(ch, victim);
		TopPlayer::Refresh(ch);
		if (!EXTRA_FLAGGED(victim, EXTRA_GRP_KILL_COUNT)
				&& !ch->IsNpc()
				&& !IS_IMMORTAL(ch)
				&& victim->IsNpc()
				&& !IS_CHARMICE(victim)
				&& !ROOM_FLAGGED(victim->in_room, ERoomFlag::kArena)) {
				mob_stat::AddMob(victim, members);
				EXTRA_FLAGS(victim).set(EXTRA_GRP_KILL_COUNT);
		} else if (ch->IsNpc() && !victim->IsNpc()
			&& !ROOM_FLAGGED(victim->in_room, ERoomFlag::kArena)) {
			mob_stat::AddMob(ch, 0);
		}
	}
}

/*++
   Функция расчитывает всякие бонусы для группы при получении опыта,
 после чего вызывает функцию получения опыта для всех членов группы
 Т.к. членом группы может быть только PC, то эта функция раздаст опыт только PC

   ch - обязательно член группы, из чего следует:
            1. Это не NPC
            2. Он находится в группе лидера (или сам лидер)

   Просто для PC-последователей эта функция не вызывается

--*/
void group_gain(CharData *killer, CharData *victim) {
	int inroom_members, koef = 100, maxlevel;
	struct FollowerType *f;
	int partner_count = 0;
	int total_group_members = 1;
	bool use_partner_exp = false;

	// если наем лидер, то тоже режем экспу
	if (CanUseFeat(killer, EFeat::kCynic)) {
		maxlevel = 300;
	} else {
		maxlevel = GetRealLevel(killer);
	}

	auto leader = killer->get_master();
	if (nullptr == leader) {
		leader = killer;
	}

	// k - подозрение на лидера группы
	const bool leader_inroom = AFF_FLAGGED(leader, EAffect::kGroup)
		&& leader->in_room == killer->in_room;

	// Количество согрупников в комнате
	if (leader_inroom) {
		inroom_members = 1;
		maxlevel = GetRealLevel(leader);
	} else {
		inroom_members = 0;
	}

	// Вычисляем максимальный уровень в группе
	for (f = leader->followers; f; f = f->next) {
		if (AFF_FLAGGED(f->follower, EAffect::kGroup)) ++total_group_members;
		if (AFF_FLAGGED(f->follower, EAffect::kGroup)
			&& f->follower->in_room == killer->in_room) {
			// если в группе наем, то режим опыт всей группе
			// дабы наема не выгодно было бы брать в группу
			// ставим 300, чтобы вообще под ноль резало
			if (CanUseFeat(f->follower, EFeat::kCynic)) {
				maxlevel = 300;
			}
			// просмотр членов группы в той же комнате
			// член группы => PC автоматически
			++inroom_members;
			maxlevel = std::max(maxlevel, GetRealLevel(f->follower));
			if (!f->follower->IsNpc()) {
				partner_count++;
			}
		}
	}

	GroupPenaltyCalculator group_penalty(killer, leader, maxlevel, grouping);
	koef -= group_penalty.get();

	koef = std::max(0, koef);

	if (leader_inroom) {
		koef += CalcLeadershipGroupExpKoeff(leader, inroom_members, koef);
	}

	// Раздача опыта
	// если групповой уровень зоны равняется единице
	if (zone_table[world[killer->in_room]->zone_rn].group < 2) {
		// чтобы не абьюзили на суммонах, когда в группе на самом деле больше
		// двух мемберов, но лишних реколят перед непосредственным рипом
		use_partner_exp = total_group_members == 2;
	}

	// если лидер группы в комнате
	if (leader_inroom) {
		// если у лидера группы есть способность напарник
		if (CanUseFeat(leader, EFeat::kPartner) && use_partner_exp) {
			// если в группе всего двое человек
			// k - лидер, и один последователь
			if (partner_count == 1) {
				// и если кожф. больше или равен 100
				if (koef >= 100) {
					if (leader->get_zone_group() < 2) {
						koef += 100;
					}
				}
			}
		}
		perform_group_gain(leader, victim, inroom_members, koef);
	}

	for (f = leader->followers; f; f = f->next) {
		if (AFF_FLAGGED(f->follower, EAffect::kGroup)
				&& f->follower->in_room == killer->in_room) {
			perform_group_gain(f->follower, victim, inroom_members, koef);
		}
	}
}

char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural) {
	static char buf[256];
	char *cp = buf;

	for (; *str; str++) {
		if (*str == '#') {
			switch (*(++str)) {
				case 'W':
					for (; *weapon_plural; *(cp++) = *(weapon_plural++)) {
					}
					break;
				case 'w':
					for (; *weapon_singular; *(cp++) = *(weapon_singular++)) {
					}
					break;
				default: *(cp++) = '#';
					break;
			}
		} else
			*(cp++) = *str;

		*cp = 0;
	}            // For

	return (buf);
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
	switch (victim->GetPosition()) {
		case EPosition::kPerish:
			if (IS_POLY(victim))
				act("$n смертельно ранены и умрут, если им не помогут.",
					true, victim, nullptr, nullptr, kToRoom | kToArenaListen);
			else
				act("$n смертельно ранен$a и умрет, если $m не помогут.",
					true, victim, nullptr, nullptr, kToRoom | kToArenaListen);
			SendMsgToChar("Вы смертельно ранены и умрете, если вам не помогут.\r\n", victim);
			break;
		case EPosition::kIncap:
			if (IS_POLY(victim))
				act("$n без сознания и медленно умирают. Помогите же им.",
					true, victim, nullptr, nullptr, kToRoom | kToArenaListen);
			else
				act("$n без сознания и медленно умирает. Помогите же $m.",
					true, victim, nullptr, nullptr, kToRoom | kToArenaListen);
			SendMsgToChar("Вы без сознания и медленно умираете, брошенные без помощи.\r\n", victim);
			break;
		case EPosition::kStun:
			if (IS_POLY(victim))
				act("$n без сознания, но возможно они еще повоюют (попозже :).",
					true, victim, nullptr, nullptr, kToRoom | kToArenaListen);
			else
				act("$n без сознания, но возможно $e еще повоюет (попозже :).",
					true, victim, nullptr, nullptr, kToRoom | kToArenaListen);
			SendMsgToChar("Сознание покинуло вас. В битве от вас пока проку мало.\r\n", victim);
			break;
		case EPosition::kDead:
			if (victim->IsNpc() && (victim->IsFlagged(EMobFlag::kCorpse))) {
				act("$n вспыхнул$g и рассыпал$u в прах.", false, victim, nullptr, nullptr, kToRoom | kToArenaListen);
				SendMsgToChar("Похоже вас убили и даже тела не оставили!\r\n", victim);
			} else {
				if (IS_POLY(victim))
					act("$n мертвы, их души медленно подымаются в небеса.",
						false, victim, nullptr, nullptr, kToRoom | kToArenaListen);
				else
					act("$n мертв$a, $s душа медленно подымается в небеса.",
						false, victim, nullptr, nullptr, kToRoom | kToArenaListen);
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
// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
