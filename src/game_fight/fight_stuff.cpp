// Part of Bylins http://www.mud.ru

#include "game_affects/affect_data.h"
#include "mobact.h"
#include "entities/obj_data.h"
#include "cmd/flee.h"
#include "entities/world_characters.h"
#include "fight.h"
#include "fight_penalties.h"
#include "fight_hit.h"
#include "handler.h"
#include "corpse.h"
#include "house.h"
#include "pk.h"
#include "stuff.h"

#include <random>
#include "statistics/top.h"
#include "color.h"
#include "game_magic/magic.h"
#include "statistics/mob_stat.h"
#include "game_mechanics/bonus.h"
#include "backtrace.h"
#include "game_magic/magic_utils.h"
//#include "entities/zone.h"
#include "entities/char_player.h"
#include "structs/global_objects.h"

// extern
void perform_drop_gold(CharData *ch, int amount);
long GetExpUntilNextLvl(CharData *ch, int level);
int max_exp_gain_pc(CharData *ch);
int max_exp_loss_pc(CharData *ch);
void get_from_container(CharData *ch, ObjData *cont, char *local_arg, int mode, int amount, bool autoloot);
void SetWait(CharData *ch, int waittime, int victim_in_room);

extern int material_value[];
extern int max_exp_gain_npc;

//интервал в секундах между восстановлением кругов после рипа
extern const unsigned RECALL_SPELLS_INTERVAL;
const unsigned RECALL_SPELLS_INTERVAL = 28;
void process_mobmax(CharData *ch, CharData *killer) {
	bool leader_partner = false;
	int partner_feat = 0;
	int total_group_members = 1;
	CharData *partner = nullptr;

	CharData *master = nullptr;
	if (killer->IsNpc()
		&& (AFF_FLAGGED(killer, EAffect::kCharmed)
			|| MOB_FLAGGED(killer, EMobFlag::kTutelar)
			|| MOB_FLAGGED(killer, EMobFlag::kMentalShadow))
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

			if (IN_ROOM(master) == IN_ROOM(killer)) {
				// лидер группы в тойже комнате, что и убивец
				cnt = 1;
				if (IsAbleToUseFeat(master, EFeat::kPartner)) {
					leader_partner = true;
				}
			}

			for (struct Follower *f = master->followers; f; f = f->next) {
				if (AFF_FLAGGED(f->ch, EAffect::kGroup)) ++total_group_members;
				if (AFF_FLAGGED(f->ch, EAffect::kGroup)
					&& IN_ROOM(f->ch) == IN_ROOM(killer)) {
					++cnt;
					if (leader_partner) {
						if (!f->ch->IsNpc()) {
							partner_feat++;
							partner = f->ch;
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
			if (IN_ROOM(master) == IN_ROOM(killer)) {
				members_to_mobmax.push_back(master);
			}
			for (struct Follower *f = master->followers; f; f = f->next) {
				if (AFF_FLAGGED(f->ch, EAffect::kGroup)
					&& IN_ROOM(f->ch) == IN_ROOM(killer)) {
					members_to_mobmax.push_back(f->ch);
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
			for (struct Follower *f = master->followers; f && i < n; f = f->next) {
				if (AFF_FLAGGED(f->ch, EAffect::kGroup)
					&& IN_ROOM(f->ch) == IN_ROOM(killer)) {
					++i;
					master = f->ch;
				}
			}
			master->mobmax_add(master, GET_MOB_VNUM(ch), 1, GetRealLevel(ch));
		}
	}
}

void update_die_counts(CharData *ch, CharData *killer, int dec_exp) {
	//настоящий убийца мастер чармиса/коня/ангела
	CharData *rkiller = killer;

	if (rkiller
		&& rkiller->IsNpc()
		&& (IS_CHARMICE(rkiller)
			|| IS_HORSE(rkiller)
			|| MOB_FLAGGED(killer, EMobFlag::kTutelar)
			|| MOB_FLAGGED(killer, EMobFlag::kMentalShadow))) {
		if (rkiller->has_master()) {
			rkiller = rkiller->get_master();
		} else {
			snprintf(buf, kMaxStringLength,
					 "die: %s killed by %s (without master)",
					 GET_PAD(ch, 0), GET_PAD(rkiller, 0));
			mudlog(buf, LGH, kLvlImmortal, SYSLOG, true);
			rkiller = nullptr;
		}
	}
	if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kDominationArena)) {
		ch->player_specials->saved.rip_arena_dom++;
	}
	if (!ch->IsNpc()) {
		if (rkiller && rkiller != ch) {
			if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kArena)) {
				if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kDominationArena)) {
					rkiller->player_specials->saved.kill_arena_dom = rkiller->player_specials->saved.kill_arena_dom + 1;
				}
				else {
					GET_RIP_ARENA(ch) = GET_RIP_ARENA(ch) + 1;
					GET_WIN_ARENA(killer) = GET_WIN_ARENA(killer) + 1;
					if (dec_exp) {
						GET_EXP_ARENA(ch) = GET_EXP_ARENA(ch) + dec_exp; //Если чар в бд
					}
				}
			} else if (rkiller->IsNpc()) {
				//Рип от моба
				GET_RIP_MOB(ch) = GET_RIP_MOB(ch) + 1;
				GET_RIP_MOBTHIS(ch) = GET_RIP_MOBTHIS(ch) + 1;
				if (dec_exp) {
					GET_EXP_MOB(ch) = GET_EXP_MOB(ch) + dec_exp;
					GET_EXP_MOBTHIS(ch) = GET_EXP_MOBTHIS(ch) + dec_exp;
				}
			} else if (!rkiller->IsNpc()) {
				//Рип в ПК
				GET_RIP_PK(ch) = GET_RIP_PK(ch) + 1;
				GET_RIP_PKTHIS(ch) = GET_RIP_PKTHIS(ch) + 1;
				if (dec_exp) {
					GET_EXP_PK(ch) = GET_EXP_PK(ch) + dec_exp;
					GET_EXP_PKTHIS(ch) = GET_EXP_PKTHIS(ch) + dec_exp;
				}
			}
		} else if ((!rkiller || (rkiller && rkiller == ch)) &&
			(ROOM_FLAGGED(ch->in_room, ERoomFlag::kDeathTrap) ||
				ROOM_FLAGGED(ch->in_room, ERoomFlag::kSlowDeathTrap) ||
				ROOM_FLAGGED(ch->in_room, ERoomFlag::kIceTrap))) {
			//Рип в дт
			GET_RIP_DT(ch) = GET_RIP_DT(ch) + 1;
			GET_RIP_DTTHIS(ch) = GET_RIP_DTTHIS(ch) + 1;
			if (dec_exp) {
				GET_EXP_DT(ch) = GET_EXP_DT(ch) + dec_exp;
				GET_EXP_DTTHIS(ch) = GET_EXP_DTTHIS(ch) + dec_exp;
			}
		} else// if (!rkiller || (rkiller && rkiller == ch))
		{
			//Рип по стечению обстоятельств
			GET_RIP_OTHER(ch) = GET_RIP_OTHER(ch) + 1;
			GET_RIP_OTHERTHIS(ch) = GET_RIP_OTHERTHIS(ch) + 1;
			if (dec_exp) {
				GET_EXP_OTHER(ch) = GET_EXP_OTHER(ch) + dec_exp;
				GET_EXP_OTHERTHIS(ch) = GET_EXP_OTHERTHIS(ch) + dec_exp;
			}
		}
	}
}
//end by WorM
//конец правки (с) Василиса

void update_leadership(CharData *ch, CharData *killer) {
	// train LEADERSHIP
	if (ch->IsNpc() && killer) // Убили моба
	{
		if (!killer->IsNpc() // Убил загрупленный чар
			&& AFF_FLAGGED(killer, EAffect::kGroup)
			&& killer->has_master()
			&& killer->get_master()->get_skill(ESkill::kLeadership) > 0
			&& IN_ROOM(killer) == IN_ROOM(killer->get_master())) {
			ImproveSkill(killer->get_master(), ESkill::kLeadership, number(0, 1), ch);
		} else if (killer->IsNpc() // Убил чармис загрупленного чара
			&& IS_CHARMICE(killer)
			&& killer->has_master()
			&& AFF_FLAGGED(killer->get_master(), EAffect::kGroup)) {
			if (killer->get_master()->has_master() // Владелец чармиса НЕ лидер
				&& killer->get_master()->get_master()->get_skill(ESkill::kLeadership) > 0
				&& IN_ROOM(killer) == IN_ROOM(killer->get_master())
				&& IN_ROOM(killer) == IN_ROOM(killer->get_master()->get_master())) {
				ImproveSkill(killer->get_master()->get_master(), ESkill::kLeadership, number(0, 1), ch);
			}
		}
	}

	// decrease LEADERSHIP
	if (!ch->IsNpc() // Член группы убит мобом
		&& killer
		&& killer->IsNpc()
		&& AFF_FLAGGED(ch, EAffect::kGroup)
		&& ch->has_master()
		&& ch->in_room == IN_ROOM(ch->get_master())
		&& ch->get_master()->get_inborn_skill(ESkill::kLeadership) > 1) {
		const auto current_skill = ch->get_master()->get_trained_skill(ESkill::kLeadership);
		ch->get_master()->set_skill(ESkill::kLeadership, current_skill - 1);
	}
}

bool stone_rebirth(CharData *ch, CharData *killer) {
	RoomRnum rnum_start, rnum_stop;
	if (ch->IsNpc()){
		return false;
	}
	if (killer && (!killer->IsNpc() || IS_CHARMICE(killer)) && (ch != killer)) { //не нычка в ПК
		return false;
	}
	act("$n погиб$q смертью храбрых.", false, ch, nullptr, nullptr, kToRoom);
	get_zone_rooms(world[ch->in_room]->zone_rn, &rnum_start, &rnum_stop);
	for (; rnum_start <= rnum_stop; rnum_start++) {
		RoomData *rm = world[rnum_start];
		if (rm->contents) {
			for (ObjData *j = rm->contents; j; j = j->get_next_content()) {
				if (j->get_vnum() == 1000) { // камень возрождения
					SendMsgToChar("Божественная сила спасла вашу жизнь!\r\n", ch);
//					enter_wtrigger(world[rnum_start], ch, -1);
					ExtractCharFromRoom(ch);
					PlaceCharToRoom(ch, rnum_start);
					ch->dismount();
					GET_HIT(ch) = 1;
					update_pos(ch);
					if (!ch->affected.empty()) {
						while (!ch->affected.empty()) {
							ch->affect_remove(ch->affected.begin());
						}
					}
					GET_POS(ch) = EPosition::kStand;
					look_at_room(ch, 0);
					greet_mtrigger(ch, -1);
					greet_otrigger(ch, -1);
					act("$n медленно появил$u откуда-то.", false, ch, nullptr, nullptr, kToRoom);
					SetWaitState(ch, 10 * kPulseViolence);
					return true;
				}
			}
		}
	}
	return false;
}

bool check_tester_death(CharData *ch, CharData *killer) {
	const bool player_died = !ch->IsNpc();
	const bool zone_is_under_construction = 0 != zone_table[world[ch->in_room]->zone_rn].under_construction;

	if (!player_died
		|| !zone_is_under_construction) {
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
	const int rent_room = real_room(GET_LOADROOM(ch));
	if (rent_room == kNowhere) {
		SendMsgToChar("Вам некуда возвращаться!\r\n", ch);
		return true;
	}
//	enter_wtrigger(world[rent_room], ch, -1);
	SendMsgToChar("Божественная сила спасла вашу жизнь.!\r\n", ch);
	ExtractCharFromRoom(ch);
	PlaceCharToRoom(ch, rent_room);
	ch->dismount();
	GET_HIT(ch) = 1;
	update_pos(ch);
	act("$n медленно появил$u откуда-то.", false, ch, nullptr, nullptr, kToRoom);
	if (!ch->affected.empty()) {
		while (!ch->affected.empty()) {
			ch->affect_remove(ch->affected.begin());
		}
	}
	GET_POS(ch) = EPosition::kStand;
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
	if (!ch->IsNpc() && (zone_table[world[ch->in_room]->zone_rn].vnum == 759)
		&& (GetRealLevel(ch) < 15)) //нуб помер в мадшколе
	{
		act("$n глупо погиб$q не закончив обучение.", false, ch, nullptr, nullptr, kToRoom);
//		sprintf(buf, "Вы погибли смертью глупых в бою! Боги возродили вас, но вы пока не можете двигаться\r\n");
//		SendMsgToChar(buf, ch);  // все мессаги писать в грит триггере
//		enter_wtrigger(world[real_room(75989)], ch, -1);
		ExtractCharFromRoom(ch);
		PlaceCharToRoom(ch, real_room(75989));
		ch->dismount();
		GET_HIT(ch) = 1;
		update_pos(ch);
		act("$n медленно появил$u откуда-то.", false, ch, nullptr, nullptr, kToRoom);
		look_at_room(ch, 0);
		greet_mtrigger(ch, -1);
		greet_otrigger(ch, -1);
//		WAIT_STATE(ch, 10 * kPulseViolence); лаг лучше ставить триггерами
		return;
	}

	if (ch->IsNpc()
		|| !ROOM_FLAGGED(ch->in_room, ERoomFlag::kArena)
		|| NORENTABLE(ch)) {
		if (!(ch->IsNpc()
			|| IS_IMMORTAL(ch)
			|| GET_GOD_FLAG(ch, EGf::kGodsLike)
			|| (killer && PRF_FLAGGED(killer, EPrf::kExecutor))))//если убил не палач
		{
			if (!NORENTABLE(ch))
				dec_exp =
					(GetExpUntilNextLvl(ch, GetRealLevel(ch) + 1) - GetExpUntilNextLvl(ch, GetRealLevel(ch))) / (3 + MIN(3, GET_REAL_REMORT(ch) / 5))
						/ ch->death_player_count();
			else
				dec_exp = (GetExpUntilNextLvl(ch, GetRealLevel(ch) + 1) - GetExpUntilNextLvl(ch, GetRealLevel(ch)))
					/ (3 + MIN(3, GET_REAL_REMORT(ch) / 5));
			EndowExpToChar(ch, -dec_exp);
			dec_exp = char_exp - GET_EXP(ch);
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
			update_leadership(ch, killer);
		}
	}

	update_die_counts(ch, killer, dec_exp);
	raw_kill(ch, killer);
}

#include "game_classes/classes_spell_slots.h"
void forget_all_spells(CharData *ch) {
	using PlayerClass::CalcCircleSlotsAmount;

	ch->mem_queue.stored = 0;
	int slots[kMaxSlot];
	int max_slot = 0;
	for (unsigned i = 0; i < kMaxSlot; ++i) {
		slots[i] = CalcCircleSlotsAmount(ch, i + 1);
		if (slots[i]) max_slot = i + 1;
	}
	struct SpellMemQueueItem *qi_cur, **qi = &ch->mem_queue.queue;
	while (*qi) {
		--slots[spell_info[(*(qi))->spellnum].slot_forc[(int) GET_CLASS(ch)][(int) GET_KIN(ch)] - 1];
		qi = &((*qi)->link);
	}
	int slotn;

	for (int i = 0; i <= kSpellCount; i++) {
		if (PRF_FLAGGED(ch, EPrf::kAutomem) && ch->real_abils.SplMem[i]) {
			slotn = spell_info[i].slot_forc[(int) GET_CLASS(ch)][(int) GET_KIN(ch)] - 1;
			for (unsigned j = 0; (slots[slotn] > 0 && j < ch->real_abils.SplMem[i]); ++j, --slots[slotn]) {
				ch->mem_queue.total += mag_manacost(ch, i);
				CREATE(qi_cur, 1);
				*qi = qi_cur;
				qi_cur->spellnum = i;
				qi_cur->link = nullptr;
				qi = &qi_cur->link;
			}
		}
		ch->real_abils.SplMem[i] = 0;
	}
	if (max_slot) {
		Affect<EApply> af;
		af.type = kSpellRecallSpells;
		af.location = EApply::kNone;
		af.modifier = 1; // номер круга, который восстанавливаем
		//добавим 1 проход про запас, иначе неуспевает отмемиться последний круг -- аффект спадает раньше
		af.duration = CalcDuration(ch, max_slot * RECALL_SPELLS_INTERVAL + kSecsPerPlayerAffect, 0, 0, 0, 0);
		af.bitvector = to_underlying(EAffect::kMemorizeSpells);
		af.battleflag = kAfPulsedec | kAfDeadkeep;
		ImposeAffect(ch, af, false, false, false, false);
	}
}

/* Функция используемая при "автограбеже" и "автолуте",
   чтобы не было лута под холдом или в слепи             */
int can_loot(CharData *ch) {
	if (ch != nullptr) {
		if (!ch->IsNpc()
			&& AFF_FLAGGED(ch, EAffect::kHold) == 0 // если под холдом
			&& !AFF_FLAGGED(ch, EAffect::kStopFight) // парализован точкой
			&& !AFF_FLAGGED(ch, EAffect::kBlind)    // слеп
			&& (GET_POS(ch) >= EPosition::kRest)) // мертв, умирает, без сознания, спит
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
			act("Кровушка стынет в жилах от предсмертного крика $N1.",
				false, killer->get_master(), nullptr, ch, kToRoom | kToNotDeaf);
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
	if (killer && PRF_FLAGGED(killer, EPrf::kExecutor)) {
		killer->set_gold(ch->get_gold() + killer->get_gold());
		ch->set_gold(0);
	}
	change_fighting(ch, true);
	GET_HIT(ch) = 1;
	GET_POS(ch) = EPosition::kSit;
	int to_room = real_room(GET_LOADROOM(ch));
	// тут придется ручками тащить чара за ворота, если ему в замке не рады
	if (!Clan::MayEnter(ch, to_room, kHousePortal)) {
		to_room = Clan::CloseRent(to_room);
	}
	if (to_room == kNowhere) {
		PLR_FLAGS(ch).set(EPlrFlag::kHelled);
		HELL_DURATION(ch) = time(nullptr) + 6;
		to_room = r_helled_start_room;
	}
	for (Follower *f = ch->followers; f; f = f->next) {
		if (IS_CHARMICE(f->ch)) {
			ExtractCharFromRoom(f->ch);
			PlaceCharToRoom(f->ch, to_room);
		}
	}
	for (int i=0; i < MAX_FIRSTAID_REMOVE; i++) {
		affect_from_char(ch, RemoveSpell(i));
	}
	// наемовские яды
	affect_from_char(ch, kSpellAconitumPoison);
	affect_from_char(ch, kSpellDaturaPoison);
	affect_from_char(ch, kSpellScopolaPoison);
	affect_from_char(ch, kSpellBelenaPoison);

	ExtractCharFromRoom(ch);
	PlaceCharToRoom(ch, to_room);
	look_at_room(ch, to_room);
	act("$n со стонами упал$g с небес...", false, ch, nullptr, nullptr, kToRoom);
	enter_wtrigger(world[ch->in_room], ch, -1);
}

void auto_loot(CharData *ch, CharData *killer, ObjData *corpse, int local_gold) {
	char obj[256];

	if (is_dark(IN_ROOM(killer))
		&& !IsAbleToUseFeat(killer, EFeat::kDarkReading)
		&& !(killer->IsNpc()
			&& AFF_FLAGGED(killer, EAffect::kCharmed)
			&& killer->has_master()
			&& IsAbleToUseFeat(killer->get_master(), EFeat::kDarkReading))) {
		return;
	}

	if (ch->IsNpc()
		&& !killer->IsNpc()
		&& PRF_FLAGGED(killer, EPrf::kAutoloot)
		&& (corpse != nullptr)
		&& can_loot(killer)) {
		sprintf(obj, "all");
		get_from_container(killer, corpse, obj, EFind::kObjInventory, 1, true);
	} else if (ch->IsNpc()
		&& !killer->IsNpc()
		&& local_gold
		&& PRF_FLAGGED(killer, EPrf::kAutomoney)
		&& (corpse != nullptr)
		&& can_loot(killer)) {
		sprintf(obj, "all.coin");
		get_from_container(killer, corpse, obj, EFind::kObjInventory, 1, false);
	} else if (ch->IsNpc()
		&& killer->IsNpc()
		&& (AFF_FLAGGED(killer, EAffect::kCharmed)
			|| MOB_FLAGGED(killer, EMobFlag::kTutelar)
			|| MOB_FLAGGED(killer, EMobFlag::kMentalShadow))
		&& (corpse != nullptr)
		&& killer->has_master()
		&& killer->in_room == killer->get_master()->in_room
		&& PRF_FLAGGED(killer->get_master(), EPrf::kAutoloot)
		&& can_loot(killer->get_master())) {
		sprintf(obj, "all");
		get_from_container(killer->get_master(), corpse, obj, EFind::kObjInventory, 1, true);
	} else if (ch->IsNpc()
		&& killer->IsNpc()
		&& local_gold
		&& (AFF_FLAGGED(killer, EAffect::kCharmed)
			|| MOB_FLAGGED(killer, EMobFlag::kTutelar)
			|| MOB_FLAGGED(killer, EMobFlag::kMentalShadow))
		&& (corpse != nullptr)
		&& killer->has_master()
		&& killer->in_room == killer->get_master()->in_room
		&& PRF_FLAGGED(killer->get_master(), EPrf::kAutomoney)
		&& can_loot(killer->get_master())) {
		sprintf(obj, "all.coin");
		get_from_container(killer->get_master(), corpse, obj, EFind::kObjInventory, 1, false);
	}
}

void check_spell_capable(CharData *ch, CharData *killer) {
	if (ch->IsNpc()
		&& killer
		&& killer != ch
		&& MOB_FLAGGED(ch, EMobFlag::kClone)
		&& ch->has_master()
		&& IsAffectedBySpell(ch, kSpellCapable)) {
		affect_from_char(ch, kSpellCapable);
		act("Чары, наложенные на $n3, тускло засветились и стали превращаться в нечто опасное.",
			false, ch, nullptr, killer, kToRoom | kToArenaListen);
		auto pos = GET_POS(ch);
		GET_POS(ch) = EPosition::kStand;
		CallMagic(ch, killer, nullptr, world[ch->in_room], ch->mob_specials.capable_spell, GetRealLevel(ch));
		GET_POS(ch) = pos;
	}
}

void clear_mobs_memory(CharData *ch) {
	for (const auto &hitter : character_list) {
		if (hitter->IsNpc() && MEMORY(hitter)) {
			mobForget(hitter.get(), ch);
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
		NORENTABLE(ch) = 0;
		AGRESSOR(ch) = 0;
		AGRO(ch) = 0;
		ch->agrobd = false;
#if defined WITH_SCRIPTING
		//scripting::on_pc_dead(ch, killer, corpse);
#endif
	} else {
		if (killer && (!killer->IsNpc() || IS_CHARMICE(killer))) {
			log("Killed: %d %d %ld", GetRealLevel(ch), GET_MAX_HIT(ch), GET_EXP(ch));
			obj_load_on_death(corpse, ch);
		}
		if (MOB_FLAGGED(ch, EMobFlag::kCorpse)) {
			perform_drop_gold(ch, local_gold);
			ch->set_gold(0);
		}
		dl_load_obj(corpse, ch, nullptr, DL_ORDINARY);
//		dl_load_obj(corpse, ch, NULL, DL_PROGRESSION); вот зачем это неработающее?
#if defined WITH_SCRIPTING
		//scripting::on_npc_dead(ch, killer, corpse);
#endif
	}
/*	до будущих времен
	if (!ch->IsNpc() && GET_REAL_REMORT(ch) > 7 && (GetRealLevel(ch) == 29 || GetRealLevel(ch) == 30))
	{
		// лоадим свиток с экспой
		const auto rnum = real_object(100);
		if (rnum >= 0)
		{
			const auto o = world_objects.create_from_prototype_by_rnum(rnum);
			o->set_owner(GET_UNIQUE(ch));
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
	if (ch->GetEnemy())
		stop_fighting(ch, true);

	for (CharData *hitter = combat_list; hitter; hitter = hitter->next_fighting) {
		if (hitter->GetEnemy() == ch) {
			SetWaitState(hitter, 0);
		}
	}

	if (!ch || ch->purged()) {
//		debug::coredump();
		debug::backtrace(runtime_config.logs(ERRLOG).handle());
		mudlog("SYSERR: Опять где-то кто-то спуржился не в то в время, не в том месте. Сброшен текущий стек и кора.",
			   NRM,
			   kLvlGod,
			   ERRLOG,
			   true);
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
		const auto it = std::find(killer->kill_list.begin(), killer->kill_list.end(), GET_ID(ch));
		if (it != killer->kill_list.end()) {
			killer->kill_list.erase(it);
		}
		killer->kill_list.push_back(GET_ID(ch));
	}

	// добавляем одну душу киллеру
	if (ch->IsNpc() && killer) {
		if (IsAbleToUseFeat(killer, EFeat::kSoulsCollector)) {
			if (GetRealLevel(ch) >= GetRealLevel(killer)) {
				if (killer->get_souls() < (GET_REAL_REMORT(killer) + 1)) {
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
			ExtractCharFromWorld(ch, true);
		}
	}
}

int get_remort_mobmax(CharData *ch) {
	int remort = GET_REAL_REMORT(ch);
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

int get_extend_exp(int exp, CharData *ch, CharData *victim) {
	int base, diff;
	int koef;

	if (!victim->IsNpc() || ch->IsNpc())
		return (exp);

	ch->send_to_TC(false, true, false,
				   "&RУ моба еще %d убийств без замакса, экспа %d, убито %d&n\r\n",
				   mob_proto[victim->get_rnum()].mob_specials.MaxFactor,
				   exp,
				   ch->mobmax_get(GET_MOB_VNUM(victim)));

	exp += static_cast<int>(exp * (ch->add_abils.percent_exp_add) / 100.0);
	for (koef = 100, base = 0, diff =
		ch->mobmax_get(GET_MOB_VNUM(victim)) - mob_proto[victim->get_rnum()].mob_specials.MaxFactor;
		 base < diff && koef > 5; base++, koef = koef * (95 - get_remort_mobmax(ch)) / 100);
	// минимальный опыт при замаксе 15% от полного опыта
	exp = exp * MAX(15, koef) / 100;

	// делим на реморты
	exp /= std::max(1.0, 0.5 * (GET_REAL_REMORT(ch) - kMaxExpCoefficientsUsed));
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
	int exp = GET_EXP(victim) / MAX(members, 1);

	if (victim->get_zone_group() > 1 && members < victim->get_zone_group()) {
		// в случае груп-зоны своего рода планка на мин кол-во человек в группе
		exp = GET_EXP(victim) / victim->get_zone_group();
	}

	// 2. Учитывается коэффициент (лидерство, разность уровней)
	//    На мой взгляд его правильней использовать тут а не в конце процедуры,
	//    хотя в большинстве случаев это все равно
	exp = exp * koef / 100;

	// 3. Вычисление опыта для PC и NPC
	const int long_live_exp_bounus_miltiplier = get_npc_long_live_exp_bounus(victim);
	if (ch->IsNpc()) {
		exp = MIN(max_exp_gain_npc, exp);
		exp += MAX(0, (exp * MIN(4, (GetRealLevel(victim) - GetRealLevel(ch)))) / 8);
	} else
		exp = MIN(max_exp_gain_pc(ch), get_extend_exp(exp, ch, victim) * long_live_exp_bounus_miltiplier);
	// 4. Последняя проверка
	exp = MAX(1, exp);
	if (exp > 1) {
		if (Bonus::is_bonus_active(Bonus::EBonusType::BONUS_EXP) && Bonus::can_get_bonus_exp(ch)) {
			exp *= Bonus::get_mult_bonus();
		}

		if (!ch->IsNpc() && !ch->affected.empty() && Bonus::can_get_bonus_exp(ch)) {
			for (const auto &aff : ch->affected) {
				if (aff->location == EApply::kExpBonus) // скушал свиток с эксп бонусом
				{
					exp *= MIN(3, aff->modifier); // бонус макс тройной
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

		exp = MIN(max_exp_gain_pc(ch), exp);
		SendMsgToChar(ch, "Ваш опыт повысился на %d %s.\r\n", exp, GetDeclensionInNumber(exp, EWhat::kPoint));
	} else if (exp == 1) {
		SendMsgToChar("Ваш опыт повысился всего лишь на маленькую единичку.\r\n", ch);
	}
	EndowExpToChar(ch, exp);
	change_alignment(ch, victim);
	TopPlayer::Refresh(ch);

	if (!EXTRA_FLAGGED(victim, EXTRA_GRP_KILL_COUNT)
		&& !ch->IsNpc()
		&& !IS_IMMORTAL(ch)
		&& victim->IsNpc()
		&& !IS_CHARMICE(victim)
		&& !ROOM_FLAGGED(IN_ROOM(victim), ERoomFlag::kArena)) {
		mob_stat::AddMob(victim, members);
		EXTRA_FLAGS(victim).set(EXTRA_GRP_KILL_COUNT);
	} else if (ch->IsNpc() && !victim->IsNpc()
		&& !ROOM_FLAGGED(IN_ROOM(victim), ERoomFlag::kArena)) {
		mob_stat::AddMob(ch, 0);
	}
}

int grouping_koef(int player_class, int player_remort) {
	if ((player_class >= kNumPlayerClasses) || (player_class < 0))
		return 1;
	return grouping[player_class][player_remort];

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
	struct Follower *f;
	int partner_count = 0;
	int total_group_members = 1;
	bool use_partner_exp = false;

	// если наем лидер, то тоже режем экспу
	if (IsAbleToUseFeat(killer, EFeat::kCynic)) {
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
		&& leader->in_room == IN_ROOM(killer);

	// Количество согрупников в комнате
	if (leader_inroom) {
		inroom_members = 1;
		maxlevel = GetRealLevel(leader);
	} else {
		inroom_members = 0;
	}

	// Вычисляем максимальный уровень в группе
	for (f = leader->followers; f; f = f->next) {
		if (AFF_FLAGGED(f->ch, EAffect::kGroup)) ++total_group_members;
		if (AFF_FLAGGED(f->ch, EAffect::kGroup)
			&& f->ch->in_room == IN_ROOM(killer)) {
			// если в группе наем, то режим опыт всей группе
			// дабы наема не выгодно было бы брать в группу
			// ставим 300, чтобы вообще под ноль резало
			if (IsAbleToUseFeat(f->ch, EFeat::kCynic)) {
				maxlevel = 300;
			}
			// просмотр членов группы в той же комнате
			// член группы => PC автоматически
			++inroom_members;
			maxlevel = MAX(maxlevel, GetRealLevel(f->ch));
			if (!f->ch->IsNpc()) {
				partner_count++;
			}
		}
	}

	GroupPenaltyCalculator group_penalty(killer, leader, maxlevel, grouping);
	koef -= group_penalty.get();

	koef = MAX(0, koef);

	// Лидерство используется, если в комнате лидер и есть еще хоть кто-то
	// из группы из PC (последователи типа лошади или чармисов не считаются)
	if (koef >= 100 && leader_inroom && (inroom_members > 1) && calc_leadership(leader)) {
		koef += 20;
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
		if (IsAbleToUseFeat(leader, EFeat::kPartner) && use_partner_exp) {
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
		if (AFF_FLAGGED(f->ch, EAffect::kGroup)
			&& f->ch->in_room == IN_ROOM(killer)) {
			perform_group_gain(f->ch, victim, inroom_members, koef);
		}
	}
}

void gain_battle_exp(CharData *ch, CharData *victim, int dam) {
	// не даем получать батлу с себя по зеркалу?
	if (ch == victim) { return; }
	// не даем получать экспу с !эксп мобов
	if (MOB_FLAGGED(victim, EMobFlag::kNoBattleExp)) { return; }
	// если цель не нпс то тоже не даем экспы
	if (!victim->IsNpc()) { return; }
	// если цель под чармом не даем экспу
	if (AFF_FLAGGED(victim, EAffect::kCharmed)) { return; }

	// получение игроками экспы
	if (!ch->IsNpc() && OK_GAIN_EXP(ch, victim)) {
		int max_exp = MIN(max_exp_gain_pc(ch), (GetRealLevel(victim) * GET_MAX_HIT(victim) + 4) /
			(5 * MAX(1, GET_REAL_REMORT(ch) - kMaxExpCoefficientsUsed - 1)));
		double coeff = MIN(dam, GET_HIT(victim)) / static_cast<double>(GET_MAX_HIT(victim));
		int battle_exp = MAX(1, static_cast<int>(max_exp * coeff));
		if (Bonus::is_bonus_active(Bonus::EBonusType::BONUS_WEAPON_EXP) && Bonus::can_get_bonus_exp(ch)) {
			battle_exp *= Bonus::get_mult_bonus();
		}
		EndowExpToChar(ch, battle_exp);
		ch->dps_add_exp(battle_exp, true);
	}


	// перенаправляем батлэкспу чармиса в хозяина, цифры те же что и у файтеров.
	if (ch->IsNpc() && AFF_FLAGGED(ch, EAffect::kCharmed)) {
		CharData *master = ch->get_master();
		// проверяем что есть мастер и он может получать экспу с данной цели
		if (master && OK_GAIN_EXP(master, victim)) {
			int max_exp = MIN(max_exp_gain_pc(master), (GetRealLevel(victim) * GET_MAX_HIT(victim) + 4) /
				(5 * MAX(1, GET_REAL_REMORT(master) - kMaxExpCoefficientsUsed - 1)));

			double coeff = MIN(dam, GET_HIT(victim)) / static_cast<double>(GET_MAX_HIT(victim));
			int battle_exp = MAX(1, static_cast<int>(max_exp * coeff));
			if (Bonus::is_bonus_active(Bonus::EBonusType::BONUS_WEAPON_EXP) && Bonus::can_get_bonus_exp(master)) {
				battle_exp *= Bonus::get_mult_bonus();
			}
			EndowExpToChar(master, battle_exp);
			master->dps_add_exp(battle_exp, true);
		}
	}
}

// * Alterate equipment
void alterate_object(ObjData *obj, int dam, int chance) {
	if (!obj)
		return;
	dam = number(0, dam * (material_value[GET_OBJ_MATER(obj)] + 30) /
		MAX(1, GET_OBJ_MAX(obj) *
			(obj->has_flag(EObjFlag::kNodrop) ? 5 :
			 obj->has_flag(EObjFlag::kBless) ? 15 : 10)
			 * (static_cast<ESkill>(GET_OBJ_SKILL(obj)) == ESkill::kBows ? 3 : 1)));

	if (dam > 0 && chance >= number(1, 100)) {
		if (dam > 1 && obj->get_worn_by() && GET_EQ(obj->get_worn_by(), EEquipPos::kShield) == obj) {
			dam /= 2;
		}

		obj->sub_current(dam);
		if (obj->get_current_durability() <= 0) {
			if (obj->get_worn_by()) {
				snprintf(buf, kMaxStringLength, "$o%s рассыпал$U, не выдержав повреждений.",
						 char_get_custom_label(obj, obj->get_worn_by()).c_str());
				act(buf, false, obj->get_worn_by(), obj, nullptr, kToChar);
			} else if (obj->get_carried_by()) {
				snprintf(buf, kMaxStringLength, "$o%s рассыпал$U, не выдержав повреждений.",
						 char_get_custom_label(obj, obj->get_carried_by()).c_str());
				act(buf, false, obj->get_carried_by(), obj, nullptr, kToChar);
			}
			ExtractObjFromWorld(obj);
		}
	}
}

void alt_equip(CharData *ch, int pos, int dam, int chance) {
	// calculate drop_chance if
	if (pos == kNowhere) {
		pos = number(0, 100);
		if (pos < 3)
			pos = EEquipPos::kFingerR + number(0, 1);
		else if (pos < 6)
			pos = EEquipPos::kNeck + number(0, 1);
		else if (pos < 20)
			pos = EEquipPos::kBody;
		else if (pos < 30)
			pos = EEquipPos::kHead;
		else if (pos < 45)
			pos = EEquipPos::kLegs;
		else if (pos < 50)
			pos = EEquipPos::kFeet;
		else if (pos < 58)
			pos = EEquipPos::kHands;
		else if (pos < 66)
			pos = EEquipPos::kArms;
		else if (pos < 76)
			pos = EEquipPos::kShield;
		else if (pos < 86)
			pos = EEquipPos::kShoulders;
		else if (pos < 90)
			pos = EEquipPos::kWaist;
		else if (pos < 94)
			pos = EEquipPos::kWristR + number(0, 1);
		else
			pos = EEquipPos::kHold;
	}

	if (pos <= 0 || pos > EEquipPos::kBoths || !GET_EQ(ch, pos) || dam < 0 || AFF_FLAGGED(ch, EAffect::kShield))
		return; // Добавил: под "зб" не убивается стаф (Купала)
	alterate_object(GET_EQ(ch, pos), dam, chance);
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
	switch (GET_POS(victim)) {
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
			if (victim->IsNpc() && (MOB_FLAGGED(victim, EMobFlag::kCorpse))) {
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
			if (dam > (GET_REAL_MAX_HIT(victim) / 4)) {
				SendMsgToChar("Это действительно БОЛЬНО!\r\n", victim);
			}

			if (dam > 0
				&& GET_HIT(victim) < (GET_REAL_MAX_HIT(victim) / 4)) {
				sprintf(buf2, "%s Вы желаете, чтобы ваши раны не кровоточили так сильно! %s\r\n",
						CCRED(victim, C_SPR), CCNRM(victim, C_SPR));
				SendMsgToChar(buf2, victim);
			}

			if (ch != victim
				&& victim->IsNpc()
				&& GET_HIT(victim) < (GET_REAL_MAX_HIT(victim) / 4)
				&& MOB_FLAGGED(victim, EMobFlag::kWimpy)
				&& !noflee
				&& GET_POS(victim) > EPosition::kSit) {
				DoFlee(victim, nullptr, 0, 0);
			}

			if (ch != victim
				&& GET_POS(victim) > EPosition::kSit
				&& !victim->IsNpc()
				&& HERE(victim)
				&& GET_WIMP_LEV(victim)
				&& GET_HIT(victim) < GET_WIMP_LEV(victim)
				&& !noflee) {
				SendMsgToChar("Вы запаниковали и попытались убежать!\r\n", victim);
				DoFlee(victim, nullptr, 0, 0);
			}

			break;
	}
}

/**
 * Разный инит щитов у мобов и чаров.
 * У мобов работают все 3 щита, у чаров только 1 рандомный на текущий удар.
 */
void Damage::post_init_shields(CharData *victim) {
	if (victim->IsNpc() && !IS_CHARMICE(victim)) {
		if (AFF_FLAGGED(victim, EAffect::kFireShield)) {
			flags.set(fight::kVictimFireShield);
		}

		if (AFF_FLAGGED(victim, EAffect::kIceShield)) {
			flags.set(fight::kVictimIceShield);
		}

		if (AFF_FLAGGED(victim, EAffect::kAirShield)) {
			flags.set(fight::kVictimAirShield);
		}
	} else {
		enum { FIRESHIELD, ICESHIELD, AIRSHIELD };
		std::vector<int> shields;

		if (AFF_FLAGGED(victim, EAffect::kFireShield)) {
			shields.push_back(FIRESHIELD);
		}

		if (AFF_FLAGGED(victim, EAffect::kAirShield)) {
			shields.push_back(AIRSHIELD);
		}

		if (AFF_FLAGGED(victim, EAffect::kIceShield)) {
			shields.push_back(ICESHIELD);
		}

		if (shields.empty()) {
			return;
		}

		int shield_num = number(0, static_cast<int>(shields.size() - 1));

		if (shields[shield_num] == FIRESHIELD) {
			flags.set(fight::kVictimFireShield);
		} else if (shields[shield_num] == AIRSHIELD) {
			flags.set(fight::kVictimAirShield);
		} else if (shields[shield_num] == ICESHIELD) {
			flags.set(fight::kVictimIceShield);
		}
	}
}

void Damage::post_init(CharData *ch, CharData *victim) {
	if (msg_num == -1) {
		// ABYRVALG тут нужно переделать на взятие сообщения из структуры абилок
		if (MUD::Skills().IsValid(skill_id)) {
			msg_num = to_underlying(skill_id) + kTypeHit;
		} else if (spell_num >= 0) {
			msg_num = spell_num;
		} else if (hit_type >= 0) {
			msg_num = hit_type + kTypeHit;
		} else {
			msg_num = kTypeHit;
		}
	}

	if (ch_start_pos == EPosition::kIncorrect) {
		ch_start_pos = GET_POS(ch);
	}

	if (victim_start_pos == EPosition::kIncorrect) {
		victim_start_pos = GET_POS(victim);
	}

	post_init_shields(victim);
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

void Damage::zero_init() {
	dam = 0;
	dam_critic = 0;
	fs_damage = 0;
	magic_type = 0;
	dmg_type = -1;
	skill_id = ESkill::kUndefined;
	spell_num = -1;
	hit_type = -1;
	msg_num = -1;
	ch_start_pos = EPosition::kIncorrect;
	victim_start_pos = EPosition::kIncorrect;
	wielded = nullptr;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
