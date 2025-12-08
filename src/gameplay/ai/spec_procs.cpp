/* ************************************************************************
*   File: spec_procs.cpp                                Part of Bylins    *
*  Usage: implementation of special procedures for mobiles/objects/rooms  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
* 									  *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
************************************************************************ */

#include "gameplay/ai/spec_procs.h"

#include "engine/core/char_movement.h"
#include "engine/core/utils_char_obj.inl"
#include "engine/entities/char_player.h"
#include "gameplay/mechanics/mount.h"
#include "gameplay/mechanics/depot.h"
#include "gameplay/fight/fight.h"
#include "gameplay/fight/fight_hit.h"
#include "gameplay/clans/house.h"
#include "gameplay/magic/magic.h"
#include "engine/ui/color.h"
#include "gameplay/magic/magic_utils.h"
#include "engine/db/global_objects.h"
#include "gameplay/core/game_limits.h"
#include "engine/ui/cmd/do_equip.h"
#include "gameplay/mechanics/illumination.h"
#include "gameplay/mechanics/doors.h"

extern CharData *get_player_of_name(const char *name);

// extern functions
void DoDrop(CharData *ch, char *argument, int, int);
void do_say(CharData *ch, char *argument, int cmd, int subcmd);
int find_first_step(RoomRnum src, RoomRnum target, CharData *ch);

// local functions
int dump(CharData *ch, void *me, int cmd, char *argument);
int mayor(CharData *ch, void *me, int cmd, char *argument);

// ********************************************************************
// *  Special procedures for mobiles                                  *
// ********************************************************************

int horse_keeper(CharData *ch, void *me, int cmd, char *argument) {
	CharData *victim = (CharData *) me, *horse = nullptr;

	if (ch->IsNpc())
		return (0);

	if (!CMD_IS("лошадь") && !CMD_IS("horse"))
		return (0);

	if (ch->is_morphed()) {
		SendMsgToChar("Лошадка испугается вас в таком виде... \r\n", ch);
		return (true);
	}

	skip_spaces(&argument);

	if (!*argument) {
		if (ch->has_horse(false)) {
			act("$N поинтересовал$U : \"$n, зачем тебе второй скакун? У тебя ведь одно седалище.\"",
				false, ch, nullptr, victim, kToChar);
			return (true);
		}
		sprintf(buf, "$N сказал$G : \"Я продам тебе скакуна за %d %s.\"",
				kHorseCost, GetDeclensionInNumber(kHorseCost, EWhat::kMoneyA));
		act(buf, false, ch, nullptr, victim, kToChar);
		return (true);
	}

	if (!strn_cmp(argument, "купить", strlen(argument)) || !strn_cmp(argument, "buy", strlen(argument))) {
		if (ch->has_horse(false)) {
			act("$N засмеял$U : \"$n, ты шутишь, у тебя же есть скакун.\"", false, ch, 0, victim, kToChar);
			return (true);
		}
		if (ch->get_gold() < kHorseCost) {
			act("\"Ступай отсюда, злыдень, у тебя нет таких денег!\"-заорал$G $N",
				false, ch, 0, victim, kToChar);
			return (true);
		}
		if (!(horse = ReadMobile(kHorseVnum, kVirtual))) {
			act("\"Извини, у меня нет для тебя скакуна.\"-смущенно произнес$Q $N",
				false, ch, 0, victim, kToChar);
			return (true);
		}
		make_horse(horse, ch);
		PlaceCharToRoom(horse, ch->in_room);
		sprintf(buf, "$N оседлал$G %s и отдал$G %s вам.", GET_PAD(horse, 3), HSHR(horse));
		act(buf, false, ch, 0, victim, kToChar);
		sprintf(buf, "$N оседлал$G %s и отдал$G %s $n2.", GET_PAD(horse, 3), HSHR(horse));
		act(buf, false, ch, 0, victim, kToRoom);
		ch->remove_gold(kHorseCost);
		ch->SetFlag(EPlrFlag::kCrashSave);
		return (true);
	}

	if (!strn_cmp(argument, "продать", strlen(argument)) || !strn_cmp(argument, "sell", strlen(argument))) {
		if (!ch->has_horse(true)) {
			act("$N засмеял$U : \"$n, ты не влезешь в мое стойло.\"", false, ch, 0, victim, kToChar);
			return (true);
		}
		if (ch->IsOnHorse()) {
			act("\"Я не собираюсь платить еще и за всадника.\"-усмехнул$U $N",
				false, ch, 0, victim, kToChar);
			return (true);
		}

		if (!(horse = ch->get_horse()) || GET_MOB_VNUM(horse) != kHorseVnum) {
			act("\"Извини, твой скакун мне не подходит.\"- заявил$G $N", false, ch, 0, victim, kToChar);
			return (true);
		}

		if (horse->in_room != victim->in_room) {
			act("\"Извини, твой скакун где-то бродит.\"- заявил$G $N", false, ch, 0, victim, kToChar);
			return (true);
		}

		sprintf(buf, "$N расседлал$G %s и отвел$G %s в стойло.", GET_PAD(horse, 3), HSHR(horse));
		act(buf, false, ch, 0, victim, kToChar);
		sprintf(buf, "$N расседлал$G %s и отвел$G %s в стойло.", GET_PAD(horse, 3), HSHR(horse));
		act(buf, false, ch, 0, victim, kToRoom);
		ExtractCharFromWorld(horse, false);
		ch->add_gold((kHorseCost >> 1));
		ch->SetFlag(EPlrFlag::kCrashSave);
		return (true);
	}

	return (0);
}

bool item_nouse(ObjData *obj) {
	switch (obj->get_type()) {
		case EObjType::kLightSource:
			if (GET_OBJ_VAL(obj, 2) == 0) {
				return true;
			}
			break;

		case EObjType::kScroll:
		case EObjType::kPotion:
			if (!GET_OBJ_VAL(obj, 1)
				&& !GET_OBJ_VAL(obj, 2)
				&& !GET_OBJ_VAL(obj, 3)) {
				return true;
			}
			break;

		case EObjType::kStaff:
		case EObjType::kWand:
			if (!GET_OBJ_VAL(obj, 2)) {
				return true;
			}
			break;

		case EObjType::kContainer:
			if (!system_obj::is_purse(obj)) {
				return true;
			}
			break;

		case EObjType::kOther:
		case EObjType::kTrash:
		case EObjType::kTrap:
		case EObjType::kNote:
		case EObjType::kLiquidContainer:
		case EObjType::kFood:
		case EObjType::kPen:
		case EObjType::kBoat:
		case EObjType::kFountain:
		case EObjType::kMagicIngredient: return true;

		default: break;
	}

	return false;
}

void npc_dropunuse(CharData *ch) {
	ObjData *obj, *nobj;
	for (obj = ch->carrying; obj; obj = nobj) {
		nobj = obj->get_next_content();
		if (item_nouse(obj)) {
			act("$n выбросил$g $o3.", false, ch, obj, 0, kToRoom);
			RemoveObjFromChar(obj);
			PlaceObjToRoom(obj, ch->in_room);
		}
	}
}

int npc_scavenge(CharData *ch) {
	int max = 1;
	ObjData *obj, *best_obj, *cont, *best_cont, *cobj;

	if (!ch->IsFlagged(EMobFlag::kScavenger)) {
		return (false);
	}

	if (IS_SHOPKEEPER(ch)) {
		return (false);
	}

	npc_dropunuse(ch);
	if (world[ch->in_room]->contents && number(0, 25) <= GetRealInt(ch)) {
		max = 1;
		best_obj = nullptr;
		cont = nullptr;
		best_cont = nullptr;
		for (obj = world[ch->in_room]->contents; obj; obj = obj->get_next_content()) {
			if (obj->get_type() == EObjType::kMagicIngredient
				|| Clan::is_clan_chest(obj)
				|| ClanSystem::is_ingr_chest(obj)) {
				continue;
			}

			if (obj->get_type() == EObjType::kContainer
				&& !system_obj::is_purse(obj)) {
				if (IS_CORPSE(obj)) {
					continue;
				}

				// Заперто, открываем, если есть ключ
				if (OBJVAL_FLAGGED(obj, EContainerFlag::kLockedUp)
					&& HasKey(ch, GET_OBJ_VAL(obj, 2))) {
					do_doorcmd(ch, obj, 0, kScmdUnlock);
				}

				// Заперто, взламываем, если умеем
				if (OBJVAL_FLAGGED(obj, EContainerFlag::kLockedUp)
					&& ch->GetSkill(ESkill::kPickLock)
					&& IsPickLockSucessdul(ch, 0, obj, EDirection::kUndefinedDir, kScmdPick)) {
					do_doorcmd(ch, obj, EDirection::kUndefinedDir, kScmdPick);
				}
				// Все равно заперто, ну тогда фиг с ним
				if (OBJVAL_FLAGGED(obj, EContainerFlag::kLockedUp)) {
					continue;
				}

				if (OBJVAL_FLAGGED(obj, EContainerFlag::kShutted)) {
					do_doorcmd(ch, obj, 0, kScmdOpen);
				}

				if (OBJVAL_FLAGGED(obj, EContainerFlag::kShutted)) {
					continue;
				}

				for (cobj = obj->get_contains(); cobj; cobj = cobj->get_next_content()) {
					if (CAN_GET_OBJ(ch, cobj) && !item_nouse(cobj) && cobj->get_cost() > max) {
						cont = obj;
						best_cont = best_obj = cobj;
						max = cobj->get_cost();
					}
				}
			} else if (!IS_CORPSE(obj) &&
				CAN_GET_OBJ(ch, obj) && obj->get_cost() > max && !item_nouse(obj)) {
				best_obj = obj;
				max = obj->get_cost();
			}
		}
		if (best_obj != nullptr) {
			if (best_obj != best_cont) {
				act("$n поднял$g $o3.", false, ch, best_obj, 0, kToRoom);
				if (best_obj->get_type() == EObjType::kMoney) {
					ch->add_gold(GET_OBJ_VAL(best_obj, 0));
					ExtractObjFromWorld(best_obj);
				} else {
					RemoveObjFromRoom(best_obj);
					PlaceObjToInventory(best_obj, ch);
				}
			} else {
				sprintf(buf, "$n достал$g $o3 из %s.", cont->get_PName(ECase::kGen).c_str());
				act(buf, false, ch, best_obj, 0, kToRoom);
				if (best_obj->get_type() == EObjType::kMoney) {
					ch->add_gold(GET_OBJ_VAL(best_obj, 0));
					ExtractObjFromWorld(best_obj);
				} else {
					RemoveObjFromObj(best_obj);
					PlaceObjToInventory(best_obj, ch);
				}
			}
		}
	}
	return (max > 1);
}

int npc_loot(CharData *ch) {
	int max = false;
	ObjData *obj, *loot_obj, *next_loot, *cobj, *cnext_obj;

	if (!ch->IsFlagged(EMobFlag::kLooter))
		return (false);
	if (IS_SHOPKEEPER(ch))
		return (false);
	npc_dropunuse(ch);
	if (world[ch->in_room]->contents && number(0, GetRealInt(ch)) > 10) {
		for (obj = world[ch->in_room]->contents; obj; obj = obj->get_next_content()) {
			if (CAN_SEE_OBJ(ch, obj) && IS_CORPSE(obj)) {
				// Сначала лутим то, что не в контейнерах
				for (loot_obj = obj->get_contains(); loot_obj; loot_obj = next_loot) {
					next_loot = loot_obj->get_next_content();
					if ((loot_obj->get_type() != EObjType::kContainer
						|| system_obj::is_purse(loot_obj))
						&& CAN_GET_OBJ(ch, loot_obj)
						&& !item_nouse(loot_obj)) {
						sprintf(buf, "$n вытащил$g $o3 из %s.", obj->get_PName(ECase::kGen).c_str());
						act(buf, false, ch, loot_obj, 0, kToRoom);
						if (loot_obj->get_type() == EObjType::kMoney) {
							ch->add_gold(GET_OBJ_VAL(loot_obj, 0));
							ExtractObjFromWorld(loot_obj);
						} else {
							RemoveObjFromObj(loot_obj);
							PlaceObjToInventory(loot_obj, ch);
							max++;
						}
					}
				}
				// Теперь не запертые контейнеры
				for (loot_obj = obj->get_contains(); loot_obj; loot_obj = next_loot) {
					next_loot = loot_obj->get_next_content();
					if (loot_obj->get_type() == EObjType::kContainer) {
						if (IS_CORPSE(loot_obj)
							|| OBJVAL_FLAGGED(loot_obj, EContainerFlag::kLockedUp)
							|| system_obj::is_purse(loot_obj)) {
							continue;
						}
						auto value = loot_obj->get_val(1); //откроем контейнер
						REMOVE_BIT(value, EContainerFlag::kShutted);
						loot_obj->set_val(1, value);
						for (cobj = loot_obj->get_contains(); cobj; cobj = cnext_obj) {
							cnext_obj = cobj->get_next_content();
							if (CAN_GET_OBJ(ch, cobj) && !item_nouse(cobj)) {
								sprintf(buf, "$n вытащил$g $o3 из %s.", obj->get_PName(ECase::kGen).c_str());
								act(buf, false, ch, cobj, 0, kToRoom);
								if (cobj->get_type() == EObjType::kMoney) {
									ch->add_gold(GET_OBJ_VAL(cobj, 0));
									ExtractObjFromWorld(cobj);
								} else {
									RemoveObjFromObj(cobj);
									PlaceObjToInventory(cobj, ch);
									max++;
								}
							}
						}
					}
				}
				// И наконец, лутим запертые контейнеры если есть ключ или можем взломать
				for (loot_obj = obj->get_contains(); loot_obj; loot_obj = next_loot) {
					next_loot = loot_obj->get_next_content();
					if (loot_obj->get_type() == EObjType::kContainer) {
						if (IS_CORPSE(loot_obj)
							|| !OBJVAL_FLAGGED(loot_obj, EContainerFlag::kLockedUp)
							|| system_obj::is_purse(loot_obj)) {
							continue;
						}

						// Есть ключ?
						if (OBJVAL_FLAGGED(loot_obj, EContainerFlag::kLockedUp)
							&& HasKey(ch, GET_OBJ_VAL(loot_obj, 2))) {
							loot_obj->toggle_val_bit(1, EContainerFlag::kLockedUp);
						}

						// ...или взломаем?
						if (OBJVAL_FLAGGED(loot_obj, EContainerFlag::kLockedUp)
							&& ch->GetSkill(ESkill::kPickLock)
							&& IsPickLockSucessdul(ch, 0, loot_obj, EDirection::kUndefinedDir, kScmdPick)) {
							loot_obj->toggle_val_bit(1, EContainerFlag::kLockedUp);
						}

						// Эх, не открыть. Ну ладно.
						if (OBJVAL_FLAGGED(loot_obj, EContainerFlag::kLockedUp)) {
							continue;
						}

						loot_obj->toggle_val_bit(1, EContainerFlag::kShutted);
						for (cobj = loot_obj->get_contains(); cobj; cobj = cnext_obj) {
							cnext_obj = cobj->get_next_content();
							if (CAN_GET_OBJ(ch, cobj) && !item_nouse(cobj)) {
								sprintf(buf, "$n вытащил$g $o3 из %s.", obj->get_PName(ECase::kGen).c_str());
								act(buf, false, ch, cobj, 0, kToRoom);
								if (cobj->get_type() == EObjType::kMoney) {
									ch->add_gold(GET_OBJ_VAL(cobj, 0));
									ExtractObjFromWorld(cobj);
								} else {
									RemoveObjFromObj(cobj);
									PlaceObjToInventory(cobj, ch);
									max++;
								}
							}
						}
					}
				}
			}
		}
	}
	return (max);
}

int npc_move(CharData *ch, int dir, int/* need_specials_check*/) {
	int need_close = false, need_lock = false;
	int rev_dir[] = {EDirection::kSouth, EDirection::kWest, EDirection::kNorth, EDirection::kEast, EDirection::kDown, EDirection::kUp};
	int retval = false;

	if (ch == nullptr || dir < 0 || dir >= EDirection::kMaxDirNum || ch->GetEnemy()) {
		return (false);
	} else if (!EXIT(ch, dir) || EXIT(ch, dir)->to_room() == kNowhere) {
		return (false);
	} else if (ch->has_master() && ch->isInSameRoom(ch->get_master())) {
		return (false);
	} else if (EXIT_FLAGGED(EXIT(ch, dir), EExitFlag::kClosed)) {
		if (!EXIT_FLAGGED(EXIT(ch, dir), EExitFlag::kHasDoor)) {
			return (false);
		}

		const auto &rdata = EXIT(ch, dir);

		if (EXIT_FLAGGED(rdata, EExitFlag::kLocked)) {
			if (HasKey(ch, rdata->key)
				|| (!EXIT_FLAGGED(rdata, EExitFlag::kPickroof)
					&& !EXIT_FLAGGED(rdata, EExitFlag::kBrokenLock)
					&& CalcCurrentSkill(ch, ESkill::kPicks, 0) >= number(0, 100))) {
				do_doorcmd(ch, 0, dir, kScmdUnlock);
				need_lock = true;
			} else {
				return (false);
			}
		}
		if (EXIT_FLAGGED(rdata, EExitFlag::kClosed)) {
			if (GetRealInt(ch) >= 15
				|| GET_DEST(ch) != kNowhere
				|| ch->IsFlagged(EMobFlag::kOpensDoor)) {
				do_doorcmd(ch, 0, dir, kScmdOpen);
				need_close = true;
			}
		}
	}

	retval = PerformMove(ch, dir, 1, false, 0);

	if (need_close) {
		const int close_direction = retval ? rev_dir[dir] : dir;
		// закрываем за собой только существующую дверь
		if (EXIT(ch, close_direction) &&
			EXIT_FLAGGED(EXIT(ch, close_direction), EExitFlag::kHasDoor) &&
			EXIT(ch, close_direction)->to_room() != kNowhere) {
			do_doorcmd(ch, 0, close_direction, kScmdClose);
		}
	}

	if (need_lock) {
		const int lock_direction = retval ? rev_dir[dir] : dir;
		// запираем за собой только существующую дверь
		if (EXIT(ch, lock_direction) &&
			EXIT_FLAGGED(EXIT(ch, lock_direction), EExitFlag::kHasDoor) &&
			EXIT(ch, lock_direction)->to_room() != kNowhere) {
			do_doorcmd(ch, 0, lock_direction, kScmdLock);
		}
	}

	return (retval);
}

int has_curse(ObjData *obj) {
	for (const auto &i : weapon_affect) {
		// Замена выражения на макрос
		if (i.aff_spell <= ESpell::kUndefined || !obj->GetEWeaponAffect(i.aff_pos)) {
			continue;
		}
		if (MUD::Spell(i.aff_spell).AllowTarget(kNpcAffectPc | kNpcDamagePc)){
			return true;
		}
	}
	return false;
}

int calculate_weapon_class(CharData *ch, ObjData *weapon) {
	int damage = 0, hits = 0, i;

	if (!weapon
		|| weapon->get_type() != EObjType::kWeapon) {
		return 0;
	}

	hits = CalcCurrentSkill(ch, static_cast<ESkill>(weapon->get_spec_param()), 0);
	damage = (GET_OBJ_VAL(weapon, 1) + 1) * (GET_OBJ_VAL(weapon, 2)) / 2;
	for (i = 0; i < kMaxObjAffect; i++) {
		auto &affected = weapon->get_affected(i);
		if (affected.location == EApply::kDamroll) {
			damage += affected.modifier;
		}

		if (affected.location == EApply::kHitroll) {
			hits += affected.modifier * 10;
		}
	}

	if (has_curse(weapon))
		return (0);

	return (damage + (hits > 200 ? 10 : hits / 20));
}

void best_weapon(CharData *ch, ObjData *sweapon, ObjData **dweapon) {
	if (*dweapon == nullptr) {
		if (calculate_weapon_class(ch, sweapon) > 0) {
			*dweapon = sweapon;
		}
	} else if (calculate_weapon_class(ch, sweapon) > calculate_weapon_class(ch, *dweapon)) {
		*dweapon = sweapon;
	}
}

void npc_wield(CharData *ch) {
	ObjData *obj, *next, *right = nullptr, *left = nullptr, *both = nullptr;

	if (!NPC_FLAGGED(ch, ENpcFlag::kWielding))
		return;

	if (ch->GetSkill(ESkill::kHammer) > 0
		&& ch->GetSkill(ESkill::kOverwhelm) < ch->GetSkill(ESkill::kHammer)) {
		return;
	}

	if (GetRealInt(ch) < 10 || IS_SHOPKEEPER(ch))
		return;

	if (GET_EQ(ch, EEquipPos::kHold)
		&& GET_EQ(ch, EEquipPos::kHold)->get_type() == EObjType::kWeapon) {
		left = GET_EQ(ch, EEquipPos::kHold);
	}
	if (GET_EQ(ch, EEquipPos::kWield)
		&& GET_EQ(ch, EEquipPos::kWield)->get_type() == EObjType::kWeapon) {
		right = GET_EQ(ch, EEquipPos::kWield);
	}
	if (GET_EQ(ch, EEquipPos::kBoths)
		&& GET_EQ(ch, EEquipPos::kBoths)->get_type() == EObjType::kWeapon) {
		both = GET_EQ(ch, EEquipPos::kBoths);
	}

	if (GetRealInt(ch) < 15 && ((left && right) || (both)))
		return;

	for (obj = ch->carrying; obj; obj = next) {
		next = obj->get_next_content();
		if (obj->get_type() != EObjType::kWeapon || obj->get_unique_id() != 0) {
			continue;
		}

		if (CAN_WEAR(obj, EWearFlag::kHold) && CanBeTakenInMinorHand(ch, obj)) {
			best_weapon(ch, obj, &left);
		} else if (CAN_WEAR(obj, EWearFlag::kWield) && CanBeTakenInMajorHand(ch, obj)) {
			best_weapon(ch, obj, &right);
		} else if (CAN_WEAR(obj, EWearFlag::kBoth) && CanBeTakenInBothHands(ch, obj)) {
			best_weapon(ch, obj, &both);
		}
	}

	if (both
		&& calculate_weapon_class(ch, both) > calculate_weapon_class(ch, left) + calculate_weapon_class(ch, right)) {
		if (both == GET_EQ(ch, EEquipPos::kBoths)) {
			return;
		}
		if (GET_EQ(ch, EEquipPos::kBoths)) {
			act("$n прекратил$g использовать $o3.", false, ch, GET_EQ(ch, EEquipPos::kBoths), 0, kToRoom);
			PlaceObjToInventory(UnequipChar(ch, EEquipPos::kBoths, CharEquipFlag::show_msg), ch);
		}
		if (GET_EQ(ch, EEquipPos::kWield)) {
			act("$n прекратил$g использовать $o3.", false, ch, GET_EQ(ch, EEquipPos::kWield), 0, kToRoom);
			PlaceObjToInventory(UnequipChar(ch, EEquipPos::kWield, CharEquipFlag::show_msg), ch);
		}
		if (GET_EQ(ch, EEquipPos::kShield)) {
			act("$n прекратил$g использовать $o3.", false, ch, GET_EQ(ch, EEquipPos::kShield), 0, kToRoom);
			PlaceObjToInventory(UnequipChar(ch, EEquipPos::kShield, CharEquipFlag::show_msg), ch);
		}
		if (GET_EQ(ch, EEquipPos::kHold)) {
			act("$n прекратил$g использовать $o3.", false, ch, GET_EQ(ch, EEquipPos::kHold), 0, kToRoom);
			PlaceObjToInventory(UnequipChar(ch, EEquipPos::kHold, CharEquipFlag::show_msg), ch);
		}
		//obj_from_char(both);
		EquipObj(ch, both, EEquipPos::kBoths, CharEquipFlag::show_msg);
	} else {
		if (left && GET_EQ(ch, EEquipPos::kHold) != left) {
			if (GET_EQ(ch, EEquipPos::kBoths)) {
				act("$n прекратил$g использовать $o3.", false, ch, GET_EQ(ch, EEquipPos::kBoths), 0, kToRoom);
				PlaceObjToInventory(UnequipChar(ch, EEquipPos::kBoths, CharEquipFlag::show_msg), ch);
			}
			if (GET_EQ(ch, EEquipPos::kShield)) {
				act("$n прекратил$g использовать $o3.", false, ch, GET_EQ(ch, EEquipPos::kShield), 0, kToRoom);
				PlaceObjToInventory(UnequipChar(ch, EEquipPos::kShield, CharEquipFlag::show_msg), ch);
			}
			if (GET_EQ(ch, EEquipPos::kHold)) {
				act("$n прекратил$g использовать $o3.", false, ch, GET_EQ(ch, EEquipPos::kHold), 0, kToRoom);
				PlaceObjToInventory(UnequipChar(ch, EEquipPos::kHold, CharEquipFlag::show_msg), ch);
			}
			//obj_from_char(left);
			EquipObj(ch, left, EEquipPos::kHold, CharEquipFlag::show_msg);
		}
		if (right && GET_EQ(ch, EEquipPos::kWield) != right) {
			if (GET_EQ(ch, EEquipPos::kBoths)) {
				act("$n прекратил$g использовать $o3.", false, ch, GET_EQ(ch, EEquipPos::kBoths), 0, kToRoom);
				PlaceObjToInventory(UnequipChar(ch, EEquipPos::kBoths, CharEquipFlag::show_msg), ch);
			}
			if (GET_EQ(ch, EEquipPos::kWield)) {
				act("$n прекратил$g использовать $o3.", false, ch, GET_EQ(ch, EEquipPos::kWield), 0, kToRoom);
				PlaceObjToInventory(UnequipChar(ch, EEquipPos::kWield, CharEquipFlag::show_msg), ch);
			}
			//obj_from_char(right);
			EquipObj(ch, right, EEquipPos::kWield, CharEquipFlag::show_msg);
		}
	}
}

void npc_armor(CharData *ch) {
	ObjData *obj, *next;
	int where = 0;

	if (!NPC_FLAGGED(ch, ENpcFlag::kArmoring))
		return;

	if (GetRealInt(ch) < 10 || IS_SHOPKEEPER(ch))
		return;

	for (obj = ch->carrying; obj; obj = next) {
		next = obj->get_next_content();

		if (!ObjSystem::is_armor_type(obj)
			|| !no_bad_affects(obj)) {
			continue;
		}

		if (CAN_WEAR(obj, EWearFlag::kFinger)) {
			where = EEquipPos::kFingerR;
		}

		if (CAN_WEAR(obj, EWearFlag::kNeck)) {
			where = EEquipPos::kNeck;
		}

		if (CAN_WEAR(obj, EWearFlag::kBody)) {
			where = EEquipPos::kBody;
		}

		if (CAN_WEAR(obj, EWearFlag::kHead)) {
			where = EEquipPos::kHead;
		}

		if (CAN_WEAR(obj, EWearFlag::kLegs)) {
			where = EEquipPos::kLegs;
		}

		if (CAN_WEAR(obj, EWearFlag::kFeet)) {
			where = EEquipPos::kFeet;
		}

		if (CAN_WEAR(obj, EWearFlag::kHands)) {
			where = EEquipPos::kHands;
		}

		if (CAN_WEAR(obj, EWearFlag::kArms)) {
			where = EEquipPos::kArms;
		}

		if (CAN_WEAR(obj, EWearFlag::kShield)) {
			where = EEquipPos::kShield;
		}

		if (CAN_WEAR(obj, EWearFlag::kShoulders)) {
			where = EEquipPos::kShoulders;
		}

		if (CAN_WEAR(obj, EWearFlag::kWaist)) {
			where = EEquipPos::kWaist;
		}

		if (CAN_WEAR(obj, EWearFlag::kQuiver)) {
			where = EEquipPos::kQuiver;
		}

		if (CAN_WEAR(obj, EWearFlag::kWrist)) {
			where = EEquipPos::kWristR;
		}

		if (!where) {
			continue;
		}

		if ((where == EEquipPos::kFingerR) || (where == EEquipPos::kNeck) || (where == EEquipPos::kWristR)) {
			if (GET_EQ(ch, where)) {
				where++;
			}
		}

		if (where == EEquipPos::kShield && (GET_EQ(ch, EEquipPos::kBoths) || GET_EQ(ch, EEquipPos::kHold))) {
			continue;
		}

		if (GET_EQ(ch, where)) {
			if (GetRealInt(ch) < 15) {
				continue;
			}

			if (GET_OBJ_VAL(obj, 0) + GET_OBJ_VAL(obj, 1) * 3
				<= GET_OBJ_VAL(GET_EQ(ch, where), 0) + GET_OBJ_VAL(GET_EQ(ch, where), 1) * 3
				|| has_curse(obj)) {
				continue;
			}
			act("$n прекратил$g использовать $o3.", false, ch, GET_EQ(ch, where), 0, kToRoom);
			PlaceObjToInventory(UnequipChar(ch, where, CharEquipFlag::show_msg), ch);
		}
		//obj_from_char(obj);
		EquipObj(ch, obj, where, CharEquipFlag::show_msg);
		break;
	}
}

void npc_light(CharData *ch) {
	ObjData *obj, *next;

	if (GetRealInt(ch) < 10 || IS_SHOPKEEPER(ch))
		return;

	if (AFF_FLAGGED(ch, EAffect::kInfravision))
		return;

	if ((obj = GET_EQ(ch, EEquipPos::kLight)) && (GET_OBJ_VAL(obj, 2) == 0 || !is_dark(ch->in_room))) {
		act("$n прекратил$g использовать $o3.", false, ch, obj, 0, kToRoom);
		PlaceObjToInventory(UnequipChar(ch, EEquipPos::kLight, CharEquipFlag::show_msg), ch);
	}

	if (!GET_EQ(ch, EEquipPos::kLight) && is_dark(ch->in_room)) {
		for (obj = ch->carrying; obj; obj = next) {
			next = obj->get_next_content();
			if (obj->get_type() != EObjType::kLightSource) {
				continue;
			}
			if (GET_OBJ_VAL(obj, 2) == 0) {
				act("$n выбросил$g $o3.", false, ch, obj, 0, kToRoom);
				RemoveObjFromChar(obj);
				PlaceObjToRoom(obj, ch->in_room);
				continue;
			}
			//obj_from_char(obj);
			EquipObj(ch, obj, EEquipPos::kLight, CharEquipFlag::show_msg);
			return;
		}
	}
}

int npc_battle_scavenge(CharData *ch) {
	int max = false;
	ObjData *obj, *next_obj = nullptr;

	if (!ch->IsFlagged(EMobFlag::kScavenger))
		return (false);

	if (IS_SHOPKEEPER(ch))
		return (false);

	if (world[ch->in_room]->contents && number(0, GetRealInt(ch)) > 10)
		for (obj = world[ch->in_room]->contents; obj; obj = next_obj) {
			next_obj = obj->get_next_content();
			if (CAN_GET_OBJ(ch, obj)
				&& !has_curse(obj)
				&& (ObjSystem::is_armor_type(obj)
					|| obj->get_type() == EObjType::kWeapon)) {
				RemoveObjFromRoom(obj);
				PlaceObjToInventory(obj, ch);
				act("$n поднял$g $o3.", false, ch, obj, 0, kToRoom);
				max = true;
			}
		}
	return (max);
}

int npc_walk(CharData *ch) {
	int rnum, door = kBfsError;

	if (ch->in_room == kNowhere)
		return (kBfsError);

	if (GET_DEST(ch) == kNowhere || (rnum = GetRoomRnum(GET_DEST(ch))) == kNowhere)
		return (kBfsError);

	if (ch->in_room == rnum) {
		if (ch->mob_specials.dest_count == 1)
			return (kBfsAlreadyThere);
		if (ch->mob_specials.dest_pos == ch->mob_specials.dest_count - 1 && ch->mob_specials.dest_dir >= 0)
			ch->mob_specials.dest_dir = -1;
		if (!ch->mob_specials.dest_pos && ch->mob_specials.dest_dir < 0)
			ch->mob_specials.dest_dir = 0;
		ch->mob_specials.dest_pos += ch->mob_specials.dest_dir >= 0 ? 1 : -1;
		if (((rnum = GetRoomRnum(GET_DEST(ch))) == kNowhere)
			|| rnum == ch->in_room)
			return (kBfsError);
		else
			return (npc_walk(ch));
	}

	door = find_first_step(ch->in_room, GetRoomRnum(GET_DEST(ch)), ch);

	return (door);
}

int do_npc_steal(CharData *ch, CharData *victim) {
	ObjData *obj, *best = nullptr;
	int gold;
	int max = 0;

	if (!NPC_FLAGGED(ch, ENpcFlag::kStealing))
		return (false);

	if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kPeaceful))
		return (false);

	if (victim->IsNpc() || IS_SHOPKEEPER(ch) || victim->GetEnemy())
		return (false);

	if (GetRealLevel(victim) >= kLvlImmortal)
		return (false);

	if (!CAN_SEE(ch, victim))
		return (false);

	if (AWAKE(victim) && (number(0, std::max(0, GetRealLevel(ch) - int_app[GetRealInt(victim)].observation)) == 0)) {
		act("Вы обнаружили руку $n1 в своем кармане.", false, ch, 0, victim, kToVict);
		act("$n пытал$u обокрасть $N3.", true, ch, 0, victim, kToNotVict);
	} else        // Steal some gold coins
	{
		gold = (int) ((victim->get_gold() * number(1, 10)) / 100);
		if (gold > 0) {
			ch->add_gold(gold);
			victim->remove_gold(gold);
		}
		// Steal something from equipment
		if (ch->GetCarryingQuantity() < CAN_CARRY_N(ch) && CalcCurrentSkill(ch, ESkill::kSteal, victim)
			>= number(1, 100) - (AWAKE(victim) ? 100 : 0)) {
			for (obj = victim->carrying; obj; obj = obj->get_next_content())
				if (CAN_SEE_OBJ(ch, obj) && ch->GetCarryingWeight() + obj->get_weight()
					<= CAN_CARRY_W(ch) && (!best || obj->get_cost() > best->get_cost()))
					best = obj;
			if (best) {
				RemoveObjFromChar(best);
				PlaceObjToInventory(best, ch);
				max++;
			}
		}
	}
	return (max);
}

int npc_steal(CharData *ch) {
	if (!NPC_FLAGGED(ch, ENpcFlag::kStealing))
		return (false);

	if (ch->GetPosition() != EPosition::kStand || IS_SHOPKEEPER(ch) || ch->GetEnemy())
		return (false);

	for (const auto cons : world[ch->in_room]->people) {
		if (!cons->IsNpc()
			&& !IS_IMMORTAL(cons)
			&& (number(0, GetRealInt(ch)) > 10)) {
			return (do_npc_steal(ch, cons));
		}
	}

	return false;
}

#define ZONE(ch)  (GET_MOB_VNUM(ch) / 100)
#define GROUP(ch) ((GET_MOB_VNUM(ch) % 100) / 10)

void npc_group(CharData *ch) {
	CharData *leader = nullptr;
	int zone = ZONE(ch), group = GROUP(ch), members = 0;

	if (GET_DEST(ch) == kNowhere || ch->in_room == kNowhere)
		return;

	// ноугруп мобы не вступают в группу
	if (ch->IsFlagged(EMobFlag::kNoGroup)) {
		return;
	}

	if (ch->has_master() && ch->isInSameRoom(ch->get_master())) {
		leader = ch->get_master();
	}

	if (!ch->has_master()) {
		leader = ch;
	}

	if (leader
		&& (AFF_FLAGGED(leader, EAffect::kCharmed)
			|| leader->GetPosition() < EPosition::kSleep)) {
		leader = nullptr;
	}

	// ноугруп моб не может быть лидером
	if (leader && leader->IsFlagged(EMobFlag::kNoGroup)) {
		leader = nullptr;
	}

	// Find leader
	for (const auto vict : world[ch->in_room]->people) {
		if (!vict->IsNpc()
			|| GET_DEST(vict) != GET_DEST(ch)
			|| zone != ZONE(vict)
			|| group != GROUP(vict)
			|| vict->IsFlagged(EMobFlag::kNoGroup)
			|| AFF_FLAGGED(vict, EAffect::kCharmed)
			|| vict->GetPosition() < EPosition::kSleep) {
			continue;
		}

		members++;

		if (!leader
			|| GetRealInt(vict) > GetRealInt(leader)) {
			leader = vict;
		}
	}

	if (members <= 1) {
		if (ch->has_master()) {
			stop_follower(ch, kSfEmpty);
		}

		return;
	}

	if (leader->has_master()) {
		stop_follower(leader, kSfEmpty);
	}

	// Assign leader
	for (const auto vict : world[ch->in_room]->people) {
		if (!vict->IsNpc()
			|| GET_DEST(vict) != GET_DEST(ch)
			|| zone != ZONE(vict)
			|| group != GROUP(vict)
			|| AFF_FLAGGED(vict, EAffect::kCharmed)
			|| vict->GetPosition() < EPosition::kSleep) {
			continue;
		}

		if (vict == leader) {
			AFF_FLAGS(vict).set(EAffect::kGroup);
			continue;
		}

		if (!vict->has_master()) {
			leader->add_follower(vict);
		} else if (vict->get_master() != leader) {
			stop_follower(vict, kSfEmpty);
			leader->add_follower(vict);
		}
		AFF_FLAGS(vict).set(EAffect::kGroup);
	}
}

void npc_groupbattle(CharData *ch) {
	struct FollowerType *k;
	CharData *tch, *helper;

	if (!ch->IsNpc()
		|| !ch->GetEnemy()
		|| AFF_FLAGGED(ch, EAffect::kCharmed)
		|| !ch->has_master()
		|| ch->in_room == kNowhere
		|| !ch->followers) {
		return;
	}

	k = ch->has_master() ? ch->get_master()->followers : ch->followers;
	tch = ch->has_master() ? ch->get_master() : ch;
	for (; k; (k = tch ? k : k->next), tch = nullptr) {
		helper = tch ? tch : k->follower;
		if (ch->in_room == helper->in_room
			&& !helper->GetEnemy()
			&& !helper->IsNpc()
			&& helper->GetPosition() > EPosition::kStun) {
			helper->SetPosition(EPosition::kStand);
			SetFighting(helper, ch->GetEnemy());
			act("$n вступил$u за $N3.", false, helper, 0, ch, kToRoom);
		}
	}
}

int dump(CharData *ch, void * /*me*/, int cmd, char *argument) {
	ObjData *k;
	int value = 0;

	for (k = world[ch->in_room]->contents; k; k = world[ch->in_room]->contents) {
		act("$p рассыпал$U в прах!", false, 0, k, 0, kToRoom);
		ExtractObjFromWorld(k);
	}

	if (!CMD_IS("drop") || !CMD_IS("бросить"))
		return (0);

	DoDrop(ch, argument, cmd, 0);

	for (k = world[ch->in_room]->contents; k; k = world[ch->in_room]->contents) {
		act("$p рассыпал$U в прах!", false, 0, k, 0, kToRoom);
		value += MAX(1, MIN(1, k->get_cost() / 10));
		ExtractObjFromWorld(k);
	}

	if (value) {
		SendMsgToChar("Боги оценили вашу жертву.\r\n", ch);
		act("$n оценен$y Богами.", true, ch, 0, 0, kToRoom);
		if (GetRealLevel(ch) < 3)
			EndowExpToChar(ch, value);
		else
			ch->add_gold(value);
	}
	return (1);
}

#if 0
void mayor(CharacterData *ch, void *me, int cmd, char* argument)
{
const char open_path[] = "W3a3003b33000c111d0d111Oe333333Oe22c222112212111a1S.";
const char close_path[] = "W3a3003b33000c111d0d111CE333333CE22c222112212111a1S.";

static const char *path = nullptr;
static int index;
static bool move = false;

if (!move)
{
if (time_info.hours == 6)
{
move = true;
path = open_path;
index = 0;
}
else if (time_info.hours == 20)
{
move = true;
path = close_path;
index = 0;
}
}
if (cmd || !move || (ch->GetPosition() < EPosition::kSleep) || (ch->GetPosition() == EPosition::kFight))
return (false);

switch (path[index])
{
case '0':
case '1':
case '2':
case '3':
perform_move(ch, path[index] - '0', 1, false);
break;

case 'W':
ch->SetPosition(EPosition::kStand);
act("$n awakens and groans loudly.", false, ch, 0, 0, TO_ROOM);
break;

case 'S':
ch->SetPosition(EPosition::kSleep);
act("$n lies down and instantly falls asleep.", false, ch, 0, 0, TO_ROOM);
break;

case 'a':
act("$n says 'Hello Honey!'", false, ch, 0, 0, TO_ROOM);
act("$n smirks.", false, ch, 0, 0, TO_ROOM);
break;

case 'b':
act("$n says 'What a view!  I must get something done about that dump!'", false, ch, 0, 0, TO_ROOM);
break;

case 'c':
act("$n says 'Vandals!  Youngsters nowadays have no respect for anything!'", false, ch, 0, 0, TO_ROOM);
break;

case 'd':
act("$n says 'Good day, citizens!'", false, ch, 0, 0, TO_ROOM);
break;

case 'e':
act("$n says 'I hereby declare the bazaar open!'", false, ch, 0, 0, TO_ROOM);
break;

case 'E':
act("$n says 'I hereby declare Midgaard closed!'", false, ch, 0, 0, TO_ROOM);
break;

case 'O':
do_gen_door(ch, "gate", 0, SCMD_UNLOCK);
do_gen_door(ch, "gate", 0, SCMD_OPEN);
break;

case 'C':
do_gen_door(ch, "gate", 0, SCMD_CLOSE);
do_gen_door(ch, "gate", 0, SCMD_LOCK);
break;

case '.':
move = false;
break;

}

index++;
return (false);
}
#endif

// ********************************************************************
// *  General special procedures for mobiles                          *
// ********************************************************************

//int thief(CharacterData *ch, void* /*me*/, int cmd, char* /*argument*/)
/*
{
	if (cmd)
		return (false);

	if (ch->GetPosition() != EPosition::kStand)
		return (false);

	for (const auto cons : world[ch->in_room]->people)
	{
		if (!cons->IsNpc()->IsNpc()
			&& GetRealLevel(cons) < kLevelImmortal
			&& !number(0, 4))
		{
			do_npc_steal(ch, cons);

			return true;
		}
	}

	return false;
}
*/
int magic_user(CharData *ch, void * /*me*/, int cmd, char * /*argument*/) {
	if (cmd || ch->GetPosition() != EPosition::kFight) {
		return (false);
	}

	CharData *target = nullptr;
	// pseudo-randomly choose someone in the room who is fighting me //
	for (const auto vict : world[ch->in_room]->people) {
		if (vict->GetEnemy() == ch
			&& !number(0, 4)) {
			target = vict;

			break;
		}
	}

	// if I didn't pick any of those, then just slam the guy I'm fighting //
	if (target == nullptr
		&& ch->GetEnemy()->in_room == ch->in_room) {
		target = ch->GetEnemy();
	}

	// Hm...didn't pick anyone...I'll wait a round. //
	if (target == nullptr) {
		return (true);
	}

	if ((GetRealLevel(ch) > 13) && (number(0, 10) == 0)) {
		CastSpell(ch, target, nullptr, nullptr, ESpell::kSleep, ESpell::kSleep);
	}

	if ((GetRealLevel(ch) > 7) && (number(0, 8) == 0)) {
		CastSpell(ch, target, nullptr, nullptr, ESpell::kBlindness, ESpell::kBlindness);
	}

	if ((GetRealLevel(ch) > 12) && (number(0, 12) == 0)) {
		if (IS_EVIL(ch)) {
			CastSpell(ch, target, nullptr, nullptr, ESpell::kEnergyDrain, ESpell::kEnergyDrain);
		} else if (IS_GOOD(ch)) {
			CastSpell(ch, target, nullptr, nullptr, ESpell::kDispelEvil, ESpell::kDispelEvil);
		}
	}

	if (number(0, 4)) {
		return (true);
	}

	switch (GetRealLevel(ch)) {
		case 4:
		case 5: CastSpell(ch, target, nullptr, nullptr, ESpell::kMagicMissile, ESpell::kMagicMissile);
			break;
		case 6:
		case 7: CastSpell(ch, target, nullptr, nullptr, ESpell::kChillTouch, ESpell::kChillTouch);
			break;
		case 8:
		case 9: CastSpell(ch, target, nullptr, nullptr, ESpell::kBurningHands, ESpell::kBurningHands);
			break;
		case 10:
		case 11: CastSpell(ch, target, nullptr, nullptr, ESpell::kShockingGasp, ESpell::kShockingGasp);
			break;
		case 12:
		case 13: CastSpell(ch, target, nullptr, nullptr, ESpell::kLightingBolt, ESpell::kLightingBolt);
			break;
		case 14:
		case 15:
		case 16:
		case 17: CastSpell(ch, target, nullptr, nullptr, ESpell::kIceBolts, ESpell::kIceBolts);
			break;
		default: CastSpell(ch, target, nullptr, nullptr, ESpell::kFireball, ESpell::kFireball);
			break;
	}

	return (true);
}

// TODO: повырезать все это
int puff(CharData * /*ch*/, void * /*me*/, int/* cmd*/, char * /*argument*/) {
	return 0;
}

int fido(CharData *ch, void * /*me*/, int cmd, char * /*argument*/) {
	ObjData *i, *temp, *next_obj;

	if (cmd || !AWAKE(ch))
		return (false);

	for (i = world[ch->in_room]->contents; i; i = i->get_next_content()) {
		if (IS_CORPSE(i)) {
			act("$n savagely devours a corpse.", false, ch, 0, 0, kToRoom);
			for (temp = i->get_contains(); temp; temp = next_obj) {
				next_obj = temp->get_next_content();
				RemoveObjFromObj(temp);
				PlaceObjToRoom(temp, ch->in_room);
			}
			ExtractObjFromWorld(i);
			return (true);
		}
	}
	return (false);
}

int janitor(CharData *ch, void * /*me*/, int cmd, char * /*argument*/) {
	ObjData *i;

	if (cmd || !AWAKE(ch))
		return (false);

	for (i = world[ch->in_room]->contents; i; i = i->get_next_content()) {
		if (!CAN_WEAR(i, EWearFlag::kTake)) {
			continue;
		}

		if (i->get_type() != EObjType::kLiquidContainer
			&& i->get_cost() >= 15) {
			continue;
		}

		act("$n picks up some trash.", false, ch, 0, 0, kToRoom);
		RemoveObjFromRoom(i);
		PlaceObjToInventory(i, ch);

		return true;
	}

	return false;
}

int cityguard(CharData *ch, void * /*me*/, int cmd, char * /*argument*/) {
	CharData *evil;
	int max_evil;

	if (cmd
		|| !AWAKE(ch)
		|| ch->GetEnemy()) {
		return (false);
	}

	max_evil = 1000;
	evil = 0;

	for (const auto tch : world[ch->in_room]->people) {
		if (!tch->IsNpc() && CAN_SEE(ch, tch) && tch->IsFlagged(EPlrFlag::kKiller)) {
			act("$n screams 'HEY!!!  You're one of those PLAYER KILLERS!!!!!!'", false, ch, 0, 0, kToRoom);
			hit(ch, tch, ESkill::kUndefined, fight::kMainHand);

			return (true);
		}
	}

	for (const auto tch : world[ch->in_room]->people) {
		if (!tch->IsNpc() && CAN_SEE(ch, tch) && tch->IsFlagged(EPlrFlag::kBurglar)) {
			act("$n screams 'HEY!!!  You're one of those PLAYER THIEVES!!!!!!'", false, ch, 0, 0, kToRoom);
			hit(ch, tch, ESkill::kUndefined, fight::kMainHand);

			return (true);
		}
	}

	for (const auto tch : world[ch->in_room]->people) {
		if (CAN_SEE(ch, tch) && tch->GetEnemy()) {
			if ((GET_ALIGNMENT(tch) < max_evil) && (tch->IsNpc() || tch->GetEnemy()->IsNpc())) {
				max_evil = GET_ALIGNMENT(tch);
				evil = tch;
			}
		}
	}

	if (evil
		&& (GET_ALIGNMENT(evil->GetEnemy()) >= 0)) {
		act("$n screams 'PROTECT THE INNOCENT!  BANZAI!  CHARGE!  ARARARAGGGHH!'", false, ch, 0, 0, kToRoom);
		hit(ch, evil, ESkill::kUndefined, fight::kMainHand);

		return (true);
	}

	return (false);
}

#define PET_PRICE(pet) (GetRealLevel(pet) * 300)

int pet_shops(CharData *ch, void * /*me*/, int cmd, char *argument) {
	char buf[kMaxStringLength], pet_name[256];
	RoomRnum pet_room;
	CharData *pet;

	pet_room = ch->in_room + 1;

	if (CMD_IS("list")) {
		SendMsgToChar("Available pets are:\r\n", ch);
		for (const auto pet : world[pet_room]->people) {
			sprintf(buf, "%8d - %s\r\n", PET_PRICE(pet), GET_NAME(pet));
			SendMsgToChar(buf, ch);
		}

		return (true);
	} else if (CMD_IS("buy")) {
		two_arguments(argument, buf, pet_name);

		if (!(pet = SearchCharInRoomByName(buf, pet_room))) {
			SendMsgToChar("There is no such pet!\r\n", ch);
			return (true);
		}
		if (ch->get_gold() < PET_PRICE(pet)) {
			SendMsgToChar("You don't have enough gold!\r\n", ch);
			return (true);
		}
		ch->remove_gold(PET_PRICE(pet));

		pet = ReadMobile(GET_MOB_RNUM(pet), kReal);
		pet->set_exp(0);
		AFF_FLAGS(pet).set(EAffect::kCharmed);

		if (*pet_name) {
			sprintf(buf, "%s %s", pet->GetCharAliases().c_str(), pet_name);
			// free(pet->GetCharAliases()); don't free the prototype!
			pet->SetCharAliases(buf);

			sprintf(buf,
					"%sA small sign on a chain around the neck says 'My name is %s'\r\n",
					pet->player_data.description.c_str(), pet_name);
			// free(pet->player_data.description); don't free the prototype!
			pet->player_data.description = str_dup(buf);
		}
		PlaceCharToRoom(pet, ch->in_room);
		ch->add_follower(pet);
		load_mtrigger(pet);

		// Be certain that pets can't get/carry/use/wield/wear items
		IS_CARRYING_W(pet) = 1000;
		IS_CARRYING_N(pet) = 100;

		SendMsgToChar("May you enjoy your pet.\r\n", ch);
		act("$n buys $N as a pet.", false, ch, 0, pet, kToRoom);

		return (1);
	}

	// All commands except list and buy
	return (0);
}

// ********************************************************************
// *  Special procedures for objects                                  *
// ********************************************************************
int bank(CharData *ch, void * /*me*/, int cmd, char *argument) {
	int amount;
	CharData *vict;

	if (CMD_IS("balance") || CMD_IS("баланс") || CMD_IS("сальдо")) {
		if (ch->get_bank() > 0)
			sprintf(buf, "У вас на счету %ld %s.\r\n",
					ch->get_bank(), GetDeclensionInNumber(ch->get_bank(), EWhat::kMoneyA));
		else
			sprintf(buf, "У вас нет денег.\r\n");
		SendMsgToChar(buf, ch);
		return (1);
	} else if (CMD_IS("deposit") || CMD_IS("вложить") || CMD_IS("вклад")) {
		if ((amount = atoi(argument)) <= 0) {
			SendMsgToChar("Сколько вы хотите вложить?\r\n", ch);
			return (1);
		}
		if (ch->get_gold() < amount) {
			SendMsgToChar("О такой сумме вы можете только мечтать!\r\n", ch);
			return (1);
		}
		ch->remove_gold(amount, false);
		ch->add_bank(amount, false);
		sprintf(buf, "Вы вложили %d %s.\r\n", amount, GetDeclensionInNumber(amount, EWhat::kMoneyU));
		SendMsgToChar(buf, ch);
		act("$n произвел$g финансовую операцию.", true, ch, nullptr, nullptr, kToRoom);
		return (1);
	} else if (CMD_IS("withdraw") || CMD_IS("получить")) {
		if ((amount = atoi(argument)) <= 0) {
			SendMsgToChar("Уточните количество денег, которые вы хотите получить?\r\n", ch);
			return (1);
		}
		if (ch->get_bank() < amount) {
			SendMsgToChar("Да вы отродясь столько денег не видели!\r\n", ch);
			return (1);
		}
		ch->add_gold(amount, false);
		ch->remove_bank(amount, false);
		sprintf(buf, "Вы сняли %d %s.\r\n", amount, GetDeclensionInNumber(amount, EWhat::kMoneyU));
		SendMsgToChar(buf, ch);
		act("$n произвел$g финансовую операцию.", true, ch, nullptr, nullptr, kToRoom);
		return (1);
	} else if (CMD_IS("transfer") || CMD_IS("перевести")) {
		argument = one_argument(argument, arg);
		amount = atoi(argument);
		if (IS_GOD(ch) && !IS_IMPL(ch)) {
			SendMsgToChar("Почитить захотелось?\r\n", ch);
			return (1);

		}
		if (!*arg) {
			SendMsgToChar("Уточните кому вы хотите перевести?\r\n", ch);
			return (1);
		}
		if (amount <= 0) {
			SendMsgToChar("Уточните количество денег, которые вы хотите получить?\r\n", ch);
			return (1);
		}
		if (amount <= 100) {
			if (ch->get_bank() < (amount + 5)) {
				SendMsgToChar("У вас не хватит денег на налоги!\r\n", ch);
				return (1);
			}
		}

		if (ch->get_bank() < amount) {
			SendMsgToChar("Да вы отродясь столько денег не видели!\r\n", ch);
			return (1);
		}
		if (ch->get_bank() < amount + ((amount * 5) / 100)) {
			SendMsgToChar("У вас не хватит денег на налоги!\r\n", ch);
			return (1);
		}

		if ((vict = get_player_of_name(arg))) {
			ch->remove_bank(amount);
			if (amount <= 100) ch->remove_bank(5);
			else ch->remove_bank(((amount * 5) / 100));
			sprintf(buf, "%sВы перевели %d кун %s%s.\r\n", kColorWht, amount,
					GET_PAD(vict, 2), kColorNrm);
			SendMsgToChar(buf, ch);
			vict->add_bank(amount);
			sprintf(buf, "%sВы получили %d кун банковским переводом от %s%s.\r\n", kColorWht, amount,
					GET_PAD(ch, 1), kColorNrm);
			SendMsgToChar(buf, vict);
			sprintf(buf,
					"<%s> {%d} перевел %d кун банковским переводом %s.",
					ch->get_name().c_str(),
					GET_ROOM_VNUM(ch->in_room),
					amount,
					GET_PAD(vict, 2));
			mudlog(buf, NRM, kLvlGreatGod, MONEY_LOG, true);
			return (1);

		} else {
			vict = new Player; // TODO: переделать на стек
			if (LoadPlayerCharacter(arg, vict, ELoadCharFlags::kFindId) < 0) {
				SendMsgToChar("Такого персонажа не существует.\r\n", ch);
				delete vict;
				return (1);
			}

			ch->remove_bank(amount);
			if (amount <= 100) ch->remove_bank(5);
			else ch->remove_bank(((amount * 5) / 100));
			sprintf(buf, "%sВы перевели %d кун %s%s.\r\n", kColorWht, amount,
					GET_PAD(vict, 2), kColorNrm);
			SendMsgToChar(buf, ch);
			sprintf(buf,
					"<%s> {%d} перевел %d кун банковским переводом %s.",
					ch->get_name().c_str(),
					GET_ROOM_VNUM(ch->in_room),
					amount,
					GET_PAD(vict, 2));
			mudlog(buf, NRM, kLvlGreatGod, MONEY_LOG, true);
			vict->add_bank(amount);
			Depot::add_offline_money(vict->get_uid(), amount);
			vict->save_char();

			delete vict;
			return (1);
		}
	} else if (CMD_IS("казна"))
		return (Clan::BankManage(ch, argument));
	return 0;
}

bool is_post(RoomRnum room) {
	for (const auto ch : world[room]->people) {
		if (ch->IsNpc() && IS_POSTKEEPER(ch)) {
			return true;
		}
	}
	return false;
}

bool is_rent(RoomRnum room) {
	if (ROOM_FLAGGED(room, ERoomFlag::kHouse)) {
		const auto clan = Clan::GetClanByRoom(room);
		if (!clan) {
			return false;
		}
	}
	// комната без рентера в ней
	for (const auto ch : world[room]->people) {
		if (ch->IsNpc() && IS_RENTKEEPER(ch)) {
			return true;
		}
	}
	return false;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
