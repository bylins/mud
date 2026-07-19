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

#include "engine/core/char_handler.h"
#include "gameplay/mechanics/equipment.h"
#include "gameplay/ai/spec_procs.h"
#include "administration/privilege.h"
#include "gameplay/economics/currencies.h"
#include "utils/grammar/gender.h"
#include "utils/grammar/declensions.h"
#include "gameplay/mechanics/follow.h"
#include "gameplay/ai/special_messages.h"

#include <fmt/format.h>
#include "gameplay/ai/subcmd_resolver.h"

#include "engine/core/char_movement.h"
#include "engine/core/utils_char_obj.inl"
#include "engine/core/target_resolver.h"
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
#include "gameplay/mechanics/sight.h"

extern CharData *get_player_of_name(const char *name);

// extern functions
void DoDrop(CharData *ch, char *argument, int, int);
void do_say(CharData *ch, char *argument, int cmd, int subcmd);
int find_first_step(RoomRnum src, RoomRnum target, CharData *ch);

// local functions
int mayor(CharData *ch, void *me, int cmd, char *argument);

// ********************************************************************
// *  Special procedures for mobiles                                  *
// ********************************************************************

namespace {
// issue.specials: horse-keeper subcommands. лошадь with no arg offers a horse (depends on state,
// handled in the wrapper); лошадь купить/продать go through the resolver.
enum class EHorseCmd { kBuy, kSell };

int HorseBuy(CharData *ch, void *me, char * /*rest*/) {
	CharData *victim = (CharData *) me, *horse = nullptr;
		if (mount::HasHorse(ch, false)) {
			act(specials::HorseMsg(specials::EHorseMsg::kBuyHaveAlready), false, ch, 0, victim, kToChar);
			return (true);
		}
		if (currencies::GetHand(*ch, currencies::kGold) < mount::kHorseCost) {
			act(specials::HorseMsg(specials::EHorseMsg::kBuyNoMoney), false, ch, 0, victim, kToChar);
			return (true);
		}
		if (!(horse = ReadMobile(mount::kHorseVnum, kVirtual))) {
			act(specials::HorseMsg(specials::EHorseMsg::kBuyNoHorse), false, ch, 0, victim, kToChar);
			return (true);
		}
		// Сначала поместить коня в комнату, иначе add_follower внутри
		// make_horse видит лошадь в kNowhere и логирует "попытка
		// загрупить игроков в разных комнатах" (#3207).
		PlaceCharToRoom(horse, ch->in_room);
		mount::MakeHorse(horse, ch);
		act(fmt::format(fmt::runtime(specials::HorseMsg(specials::EHorseMsg::kBuyGiveChar)),
				fmt::arg("horse", GET_PAD(horse, 3)), fmt::arg("pronoun", grammar::PossessivePronoun((horse)->get_sex()))),
			false, ch, 0, victim, kToChar);
		act(fmt::format(fmt::runtime(specials::HorseMsg(specials::EHorseMsg::kBuyGiveRoom)),
				fmt::arg("horse", GET_PAD(horse, 3)), fmt::arg("pronoun", grammar::PossessivePronoun((horse)->get_sex()))),
			false, ch, 0, victim, kToRoom);
		currencies::RemoveHand(*ch, currencies::kGold, mount::kHorseCost);
		ch->SetFlag(EPlrFlag::kCrashSave);
		return (true);
	return (1);
}

int HorseSell(CharData *ch, void *me, char * /*rest*/) {
	CharData *victim = (CharData *) me, *horse = nullptr;
		if (!mount::HasHorse(ch, true)) {
			act(specials::HorseMsg(specials::EHorseMsg::kSellNoHorse), false, ch, 0, victim, kToChar);
			return (true);
		}
		if (mount::IsOnHorse(ch)) {
			act(specials::HorseMsg(specials::EHorseMsg::kSellOnHorse), false, ch, 0, victim, kToChar);
			return (true);
		}

		if (!(horse = mount::GetHorse(ch)) || GET_MOB_VNUM(horse) != mount::kHorseVnum) {
			act(specials::HorseMsg(specials::EHorseMsg::kSellWrongHorse), false, ch, 0, victim, kToChar);
			return (true);
		}

		if (horse->in_room != victim->in_room) {
			act(specials::HorseMsg(specials::EHorseMsg::kSellHorseAway), false, ch, 0, victim, kToChar);
			return (true);
		}

		act(fmt::format(fmt::runtime(specials::HorseMsg(specials::EHorseMsg::kSellTaken)),
				fmt::arg("horse", GET_PAD(horse, 3)), fmt::arg("pronoun", grammar::PossessivePronoun((horse)->get_sex()))),
			false, ch, 0, victim, kToChar);
		act(fmt::format(fmt::runtime(specials::HorseMsg(specials::EHorseMsg::kSellTaken)),
				fmt::arg("horse", GET_PAD(horse, 3)), fmt::arg("pronoun", grammar::PossessivePronoun((horse)->get_sex()))),
			false, ch, 0, victim, kToRoom);
		ExtractCharFromWorld(horse, false);
		currencies::AddHand(*ch, currencies::kGold, (mount::kHorseCost >> 1));
		ch->SetFlag(EPlrFlag::kCrashSave);
		return (true);
	return (1);
}

const SubCmdResolver kHorseCmds([] { return specials::HorseMsg(specials::EHorseMsg::kGreeting); }, {
	{{"купить", "buy"}, static_cast<int>(EHorseCmd::kBuy), HorseBuy},
	{{"продать", "sell"}, static_cast<int>(EHorseCmd::kSell), HorseSell},
});
} // namespace

int horse_keeper(CharData *ch, void *me, int /*cmd*/, char *argument) {
	if (ch->IsNpc()) {
		return (0);
	}
	CharData *victim = (CharData *) me;
	skip_spaces(&argument);
	if (!*argument) {
			if (mount::HasHorse(ch, false)) {
				act(specials::HorseMsg(specials::EHorseMsg::kAlreadyHave), false, ch, nullptr, victim, kToChar);
				return (true);
			}
			act(fmt::format(fmt::runtime(specials::HorseMsg(specials::EHorseMsg::kForSale)),
					fmt::arg("amount", mount::kHorseCost),
					fmt::arg("currency", MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(mount::kHorseCost, grammar::ECase::kNom).c_str())),
				false, ch, nullptr, victim, kToChar);
			return (true);
		return (1);
	}
	return kHorseCmds.Dispatch(ch, me, argument);
}

bool item_nouse(ObjData *obj) {
	switch (obj->get_type()) {
		case EObjType::kLightSource:
			if (GET_OBJ_VAL(obj, 2) == 0) {
				return true;
			}
			break;

		// issue.magic-items: заклинания и заряды переехали в extra_values, val[] у этих типов нулевые
		case EObjType::kScroll:
		case EObjType::kPotion:
			if (obj->GetPotionValueKey(ObjVal::EValueKey::kSpell1Num) <= 0
				&& obj->GetPotionValueKey(ObjVal::EValueKey::kSpell2Num) <= 0
				&& obj->GetPotionValueKey(ObjVal::EValueKey::kSpell3Num) <= 0) {
				return true;
			}
			break;

		case EObjType::kStaff:
		case EObjType::kWand:
			if (obj->GetPotionValueKey(ObjVal::EValueKey::kCurCharges) <= 0) {
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
		case EObjType::kMagicComponent: return true;

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
	ObjData *best_obj, *cont, *best_cont, *cobj;

	if (!ch->IsFlagged(EMobFlag::kScavenger)) {
		return (false);
	}

	if (specials::IsShopkeeper(ch)) {
		return (false);
	}

	npc_dropunuse(ch);
	if (!world[ch->in_room]->contents.empty() && number(0, 25) <= GetRealInt(ch)) {
		max = 1;
		best_obj = nullptr;
		cont = nullptr;
		best_cont = nullptr;
		for (auto obj : world[ch->in_room]->contents) {
			if (obj->get_type() == EObjType::kMagicComponent
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
				if (IS_SET(GET_OBJ_VAL((obj), 1), (EContainerFlag::kLockedUp))
					&& HasKey(ch, GET_OBJ_VAL(obj, 2))) {
					do_doorcmd(ch, obj, 0, kScmdUnlock);
				}

				// Заперто, взламываем, если умеем
				if (IS_SET(GET_OBJ_VAL((obj), 1), (EContainerFlag::kLockedUp))
					&& GetSkill(ch, ESkill::kPickLock)
					&& IsPickLockSucessdul(ch, 0, obj, EDirection::kUndefinedDir, kScmdPick)) {
					do_doorcmd(ch, obj, EDirection::kUndefinedDir, kScmdPick);
				}
				// Все равно заперто, ну тогда фиг с ним
				if (IS_SET(GET_OBJ_VAL((obj), 1), (EContainerFlag::kLockedUp))) {
					continue;
				}

				if (IS_SET(GET_OBJ_VAL((obj), 1), (EContainerFlag::kShutted))) {
					do_doorcmd(ch, obj, 0, kScmdOpen);
				}

				if (IS_SET(GET_OBJ_VAL((obj), 1), (EContainerFlag::kShutted))) {
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
					currencies::AddHand(*ch, currencies::kGold, GET_OBJ_VAL(best_obj, 0));
					ExtractObjFromWorld(best_obj);
				} else {
					RemoveObjFromRoom(best_obj);
					PlaceObjToInventory(best_obj, ch);
				}
			} else {
				sprintf(buf, "$n достал$g $o3 из %s.", cont->get_PName(grammar::ECase::kGen).c_str());
				act(buf, false, ch, best_obj, 0, kToRoom);
				if (best_obj->get_type() == EObjType::kMoney) {
					currencies::AddHand(*ch, currencies::kGold, GET_OBJ_VAL(best_obj, 0));
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
	ObjData *loot_obj, *next_loot, *cobj, *cnext_obj;

	if (!ch->IsFlagged(EMobFlag::kLooter))
		return (false);
	if (specials::IsShopkeeper(ch))
		return (false);
	npc_dropunuse(ch);
	if (!world[ch->in_room]->contents.empty() && number(0, GetRealInt(ch)) > 10) {
		for (auto obj : world[ch->in_room]->contents) {
			if (sight::CanSeeObj(ch, obj) && IS_CORPSE(obj)) {
				// Сначала лутим то, что не в контейнерах
				for (loot_obj = obj->get_contains(); loot_obj; loot_obj = next_loot) {
					next_loot = loot_obj->get_next_content();
					if ((loot_obj->get_type() != EObjType::kContainer
						|| system_obj::is_purse(loot_obj))
						&& CAN_GET_OBJ(ch, loot_obj)
						&& !item_nouse(loot_obj)) {
						sprintf(buf, "$n вытащил$g $o3 из %s.", obj->get_PName(grammar::ECase::kGen).c_str());
						act(buf, false, ch, loot_obj, 0, kToRoom);
						if (loot_obj->get_type() == EObjType::kMoney) {
							currencies::AddHand(*ch, currencies::kGold, GET_OBJ_VAL(loot_obj, 0));
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
							|| IS_SET(GET_OBJ_VAL((loot_obj), 1), (EContainerFlag::kLockedUp))
							|| system_obj::is_purse(loot_obj)) {
							continue;
						}
						auto value = loot_obj->get_val(1); //откроем контейнер
						REMOVE_BIT(value, EContainerFlag::kShutted);
						loot_obj->set_val(1, value);
						for (cobj = loot_obj->get_contains(); cobj; cobj = cnext_obj) {
							cnext_obj = cobj->get_next_content();
							if (CAN_GET_OBJ(ch, cobj) && !item_nouse(cobj)) {
								sprintf(buf, "$n вытащил$g $o3 из %s.", obj->get_PName(grammar::ECase::kGen).c_str());
								act(buf, false, ch, cobj, 0, kToRoom);
								if (cobj->get_type() == EObjType::kMoney) {
									currencies::AddHand(*ch, currencies::kGold, GET_OBJ_VAL(cobj, 0));
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
							|| !IS_SET(GET_OBJ_VAL((loot_obj), 1), (EContainerFlag::kLockedUp))
							|| system_obj::is_purse(loot_obj)) {
							continue;
						}

						// Есть ключ?
						if (IS_SET(GET_OBJ_VAL((loot_obj), 1), (EContainerFlag::kLockedUp))
							&& HasKey(ch, GET_OBJ_VAL(loot_obj, 2))) {
							loot_obj->toggle_val_bit(1, EContainerFlag::kLockedUp);
						}

						// ...или взломаем?
						if (IS_SET(GET_OBJ_VAL((loot_obj), 1), (EContainerFlag::kLockedUp))
							&& GetSkill(ch, ESkill::kPickLock)
							&& IsPickLockSucessdul(ch, 0, loot_obj, EDirection::kUndefinedDir, kScmdPick)) {
							loot_obj->toggle_val_bit(1, EContainerFlag::kLockedUp);
						}

						// Эх, не открыть. Ну ладно.
						if (IS_SET(GET_OBJ_VAL((loot_obj), 1), (EContainerFlag::kLockedUp))) {
							continue;
						}

						loot_obj->toggle_val_bit(1, EContainerFlag::kShutted);
						for (cobj = loot_obj->get_contains(); cobj; cobj = cnext_obj) {
							cnext_obj = cobj->get_next_content();
							if (CAN_GET_OBJ(ch, cobj) && !item_nouse(cobj)) {
								sprintf(buf, "$n вытащил$g $o3 из %s.", obj->get_PName(grammar::ECase::kGen).c_str());
								act(buf, false, ch, cobj, 0, kToRoom);
								if (cobj->get_type() == EObjType::kMoney) {
									currencies::AddHand(*ch, currencies::kGold, GET_OBJ_VAL(cobj, 0));
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

	if (GetSkill(ch, ESkill::kHammer) > 0
		&& GetSkill(ch, ESkill::kOverwhelm) < GetSkill(ch, ESkill::kHammer)) {
		return;
	}

	if (GetRealInt(ch) < 10 || specials::IsShopkeeper(ch))
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

	if (GetRealInt(ch) < 10 || specials::IsShopkeeper(ch))
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

	if (GetRealInt(ch) < 10 || specials::IsShopkeeper(ch))
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

	if (!ch->IsFlagged(EMobFlag::kScavenger))
		return (false);

	if (specials::IsShopkeeper(ch))
		return (false);

	if (!world[ch->in_room]->contents.empty() && number(0, GetRealInt(ch)) > 10)
		for (auto it = world[ch->in_room]->contents.begin(); it != world[ch->in_room]->contents.end(); ) {
			auto obj = *it; ++it;
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

	door = find_first_step(ch->in_room, rnum, ch);

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

	if (victim->IsNpc() || specials::IsShopkeeper(ch) || victim->GetEnemy())
		return (false);

	if (GetRealLevel(victim) >= kLvlImmortal)
		return (false);

	if (!sight::CanSee(ch, victim))
		return (false);

	if (AWAKE(victim) && (number(0, std::max(0, GetRealLevel(ch) - IntApp(GetRealInt(victim)).observation)) == 0)) {
		act("Вы обнаружили руку $n1 в своем кармане.", false, ch, 0, victim, kToVict);
		act("$n пытал$u обокрасть $N3.", true, ch, 0, victim, kToNotVict);
	} else        // Steal some gold coins
	{
		gold = (int) ((currencies::GetHand(*victim, currencies::kGold) * number(1, 10)) / 100);
		if (gold > 0) {
			currencies::AddHand(*ch, currencies::kGold, gold);
			currencies::RemoveHand(*victim, currencies::kGold, gold);
		}
		// Steal something from equipment
		if (ch->GetCarryingQuantity() < CAN_CARRY_N(ch) && CalcCurrentSkill(ch, ESkill::kSteal, victim)
			>= number(1, 100) - (AWAKE(victim) ? 100 : 0)) {
			for (obj = victim->carrying; obj; obj = obj->get_next_content())
				if (sight::CanSeeObj(ch, obj) && ch->GetCarryingWeight() + obj->get_weight()
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

	if (ch->GetPosition() != EPosition::kStand || specials::IsShopkeeper(ch) || ch->GetEnemy())
		return (false);

	for (const auto cons : world[ch->in_room]->people) {
		if (!cons->IsNpc()
			&& !privilege::IsImmortal(cons)
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
			follow::StopFollower(ch, follow::kSfEmpty);
		}

		return;
	}

	if (leader->has_master()) {
		follow::StopFollower(leader, follow::kSfEmpty);
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
			follow::AddFollower(leader, vict);
		} else if (vict->get_master() != leader) {
			follow::StopFollower(vict, follow::kSfEmpty);
			follow::AddFollower(leader, vict);
		}
		AFF_FLAGS(vict).set(EAffect::kGroup);
	}
}

void npc_groupbattle(CharData *ch) {
	if (!ch->IsNpc()
		|| !ch->GetEnemy()
		|| AFF_FLAGGED(ch, EAffect::kCharmed)
		|| !ch->has_master()
		|| ch->in_room == kNowhere
		|| ch->followers.empty()) {
		return;
	}

	auto *leader = ch->has_master() ? ch->get_master() : ch;
	// Check the leader first, then iterate through their followers
	auto check_helper = [&](CharData *helper) {
		if (ch->in_room == helper->in_room
			&& !helper->GetEnemy()
			&& !helper->IsNpc()
			&& helper->GetPosition() > EPosition::kStun) {
			helper->SetPosition(EPosition::kStand);
			SetFighting(helper, ch->GetEnemy());
			act("$n вступил$u за $N3.", false, helper, 0, ch, kToRoom);
		}
	};
	check_helper(leader);
	for (auto *f : leader->followers) {
		check_helper(f);
	}
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
		if (alignment::IsEvil(ch)) {
			CastSpell(ch, target, nullptr, nullptr, ESpell::kEnergyDrain, ESpell::kEnergyDrain);
		} else if (alignment::IsGood(ch)) {
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
	ObjData *temp, *next_obj;

	if (cmd || !AWAKE(ch))
		return (false);

	for (auto i : world[ch->in_room]->contents) {
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
	if (cmd || !AWAKE(ch))
		return (false);

	for (auto i : world[ch->in_room]->contents) {
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
		if (!tch->IsNpc() && sight::CanSee(ch, tch) && tch->IsFlagged(EPlrFlag::kKiller)) {
			act("$n screams 'HEY!!!  You're one of those PLAYER KILLERS!!!!!!'", false, ch, 0, 0, kToRoom);
			hit(ch, tch, ESkill::kUndefined, fight::kMainHand);

			return (true);
		}
	}

	for (const auto tch : world[ch->in_room]->people) {
		if (!tch->IsNpc() && sight::CanSee(ch, tch) && tch->IsFlagged(EPlrFlag::kBurglar)) {
			act("$n screams 'HEY!!!  You're one of those PLAYER THIEVES!!!!!!'", false, ch, 0, 0, kToRoom);
			hit(ch, tch, ESkill::kUndefined, fight::kMainHand);

			return (true);
		}
	}

	for (const auto tch : world[ch->in_room]->people) {
		if (sight::CanSee(ch, tch) && tch->GetEnemy()) {
			if ((alignment::GetAlignment(tch) < max_evil) && (tch->IsNpc() || tch->GetEnemy()->IsNpc())) {
				max_evil = alignment::GetAlignment(tch);
				evil = tch;
			}
		}
	}

	if (evil
		&& (alignment::GetAlignment(evil->GetEnemy()) >= 0)) {
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

		target_resolver::Query q;
		q.scopes = {target_resolver::Scope::kRoom};
		q.name = buf;
		q.room_override = pet_room;
		q.visible_only = false;
		if (!(pet = target_resolver::ResolveChar(ch, q))) {
			SendMsgToChar("There is no such pet!\r\n", ch);
			return (true);
		}
		if (currencies::GetHand(*ch, currencies::kGold) < PET_PRICE(pet)) {
			SendMsgToChar("You don't have enough gold!\r\n", ch);
			return (true);
		}
		currencies::RemoveHand(*ch, currencies::kGold, PET_PRICE(pet));

		pet = ReadMobile(pet->get_rnum(), kReal);
		pet->set_exp(0);
		AFF_FLAGS(pet).set(EAffect::kCharmed);
		pet->SetFlag(EMobFlag::kCompanion);	// any NPC ally

		if (*pet_name) {
			sprintf(buf, "%s %s", pet->GetCharAliases().c_str(), pet_name);
			// free(pet->GetCharAliases()); don't free the prototype!
			pet->SetCharAliases(buf);

			sprintf(buf,
					"%sA small sign on a chain around the neck says 'My name is %s'\r\n",
					pet->player_data.description.c_str(), pet_name);
			// player_data.description -- std::string со своим владением: прямое
			// присваивание заменяет собственную копию, прототип не затрагивается.
			pet->player_data.description = buf;
		}
		PlaceCharToRoom(pet, ch->in_room);
		follow::AddFollower(ch, pet);
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
namespace {
// issue.specials: bank subcommands. One function per subcommand; the resolver maps the synonym
// words to these handlers and auto-generates the "чего изволите" tooltip from the same list.
enum class EBankCmd { kBalance, kDeposit, kWithdraw, kTransfer, kTreasury };

// Parse "<amount> [<currency>]" for deposit/withdraw. An omitted currency means gold; a named one
// must resolve and permit banking (IsStorable). Returns false (after messaging) on a bad value.
bool ParseBankArg(CharData *ch, char *argument, specials::EBankMsg how_much, int &amount, int &currency) {
	char amount_arg[kMaxInputLength], curr_arg[kMaxInputLength];
	argument = one_argument(argument, amount_arg);
	one_argument(argument, curr_arg);
	amount = atoi(amount_arg);
	if (amount <= 0) {
		SendMsgToChar(specials::BankMsg(how_much) + "\r\n", ch);
		return false;
	}
	currency = currencies::kGoldVnum;
	if (*curr_arg) {
		const auto *cur = currencies::FindBySearch(curr_arg);
		if (!cur || !cur->IsStorable()) {
			SendMsgToChar(specials::BankMsg(specials::EBankMsg::kCantBank) + "\r\n", ch);
			return false;
		}
		currency = cur->GetId();
	}
	return true;
}

int BankBalance(CharData *ch, void * /*me*/, char * /*argument*/) {
	bool any = false;
	for (const auto &[id, amounts] : currencies::HeldByChar(*ch)) {
		if (amounts.bank <= 0) {
			continue;
		}
		const auto &info = MUD::Currencies().FindAvailableItem(id);
		if (info.GetId() < 0) {
			continue;
		}
		SendMsgToChar(fmt::format(fmt::runtime(specials::BankMsg(specials::EBankMsg::kBalance)),
				fmt::arg("amount", amounts.bank),
				fmt::arg("currency", info.GetNameWithAmount(amounts.bank, grammar::ECase::kNom).c_str())) + "\r\n", ch);
		any = true;
	}
	if (!any) {
		SendMsgToChar(specials::BankMsg(specials::EBankMsg::kNoMoney) + "\r\n", ch);
	}
	return (1);
}

int BankDeposit(CharData *ch, void * /*me*/, char *argument) {
	int amount, currency;
	if (!ParseBankArg(ch, argument, specials::EBankMsg::kDepositHowMuch, amount, currency)) {
		return (1);
	}
	if (currencies::GetHand(*ch, currency) < amount) {
		SendMsgToChar(specials::BankMsg(specials::EBankMsg::kCantAfford) + "\r\n", ch);
		return (1);
	}
	currencies::RemoveHand(*ch, currency, amount, false);
	currencies::AddBank(*ch, currency, amount, false, false);
	SendMsgToChar(fmt::format(fmt::runtime(specials::BankMsg(specials::EBankMsg::kDeposited)),
			fmt::arg("amount", amount),
			fmt::arg("currency", MUD::Currency(currency).GetNameWithAmount(amount, grammar::ECase::kNom).c_str())) + "\r\n", ch);
	act(specials::BankMsg(specials::EBankMsg::kFinancialOp), true, ch, nullptr, nullptr, kToRoom);
	return (1);
}

int BankWithdraw(CharData *ch, void * /*me*/, char *argument) {
	int amount, currency;
	if (!ParseBankArg(ch, argument, specials::EBankMsg::kWithdrawHowMuch, amount, currency)) {
		return (1);
	}
	if (currencies::GetBank(*ch, currency) < amount) {
		SendMsgToChar(specials::BankMsg(specials::EBankMsg::kNeverHadThatMuch) + "\r\n", ch);
		return (1);
	}
	currencies::AddHand(*ch, currency, amount, false, false);
	currencies::RemoveBank(*ch, currency, amount, false);
	SendMsgToChar(fmt::format(fmt::runtime(specials::BankMsg(specials::EBankMsg::kWithdrawn)),
			fmt::arg("amount", amount),
			fmt::arg("currency", MUD::Currency(currency).GetNameWithAmount(amount, grammar::ECase::kNom).c_str())) + "\r\n", ch);
	act(specials::BankMsg(specials::EBankMsg::kFinancialOp), true, ch, nullptr, nullptr, kToRoom);
	return (1);
}

int BankTransfer(CharData *ch, void * /*me*/, char *argument) {
	int amount;
	CharData *vict;
	argument = one_argument(argument, arg);
	amount = atoi(argument);
	if (privilege::IsGod(ch) && !privilege::IsImpl(ch)) {
		SendMsgToChar(specials::BankMsg(specials::EBankMsg::kImmCant) + "\r\n", ch);
		return (1);

	}
	if (!*arg) {
		SendMsgToChar(specials::BankMsg(specials::EBankMsg::kTransferToWhom) + "\r\n", ch);
		return (1);
	}
	if (amount <= 0) {
		SendMsgToChar(specials::BankMsg(specials::EBankMsg::kWithdrawHowMuch) + "\r\n", ch);
		return (1);
	}
	if (amount <= 100) {
		if (currencies::GetBank(*ch, currencies::kGold) < (amount + 5)) {
			SendMsgToChar(specials::BankMsg(specials::EBankMsg::kNoTaxMoney) + "\r\n", ch);
			return (1);
		}
	}

	if (currencies::GetBank(*ch, currencies::kGold) < amount) {
		SendMsgToChar(specials::BankMsg(specials::EBankMsg::kNeverHadThatMuch) + "\r\n", ch);
		return (1);
	}
	if (currencies::GetBank(*ch, currencies::kGold) < amount + ((amount * 5) / 100)) {
		SendMsgToChar(specials::BankMsg(specials::EBankMsg::kNoTaxMoney) + "\r\n", ch);
		return (1);
	}

	if ((vict = get_player_of_name(arg))) {
		currencies::RemoveBank(*ch, currencies::kGold, amount);
		if (amount <= 100) currencies::RemoveBank(*ch, currencies::kGold, 5);
		else currencies::RemoveBank(*ch, currencies::kGold, ((amount * 5) / 100));
		SendMsgToChar(fmt::format(fmt::runtime(specials::BankMsg(specials::EBankMsg::kTransferSent)),
				fmt::arg("color", kColorWht), fmt::arg("amount", amount),
				fmt::arg("recipient", GET_PAD(vict, 2)), fmt::arg("nocolor", kColorNrm)) + "\r\n", ch);
		currencies::AddBank(*vict, currencies::kGold, amount);
		SendMsgToChar(fmt::format(fmt::runtime(specials::BankMsg(specials::EBankMsg::kTransferReceived)),
				fmt::arg("color", kColorWht), fmt::arg("amount", amount),
				fmt::arg("sender", GET_PAD(ch, 1)), fmt::arg("nocolor", kColorNrm)) + "\r\n", vict);
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
			SendMsgToChar(specials::BankMsg(specials::EBankMsg::kNoSuchPlayer) + "\r\n", ch);
			delete vict;
			return (1);
		}

		currencies::RemoveBank(*ch, currencies::kGold, amount);
		if (amount <= 100) currencies::RemoveBank(*ch, currencies::kGold, 5);
		else currencies::RemoveBank(*ch, currencies::kGold, ((amount * 5) / 100));
		SendMsgToChar(fmt::format(fmt::runtime(specials::BankMsg(specials::EBankMsg::kTransferSent)),
				fmt::arg("color", kColorWht), fmt::arg("amount", amount),
				fmt::arg("recipient", GET_PAD(vict, 2)), fmt::arg("nocolor", kColorNrm)) + "\r\n", ch);
		sprintf(buf,
				"<%s> {%d} перевел %d кун банковским переводом %s.",
				ch->get_name().c_str(),
				GET_ROOM_VNUM(ch->in_room),
				amount,
				GET_PAD(vict, 2));
		mudlog(buf, NRM, kLvlGreatGod, MONEY_LOG, true);
		currencies::AddBank(*vict, currencies::kGold, amount);
		Depot::add_offline_money(vict->get_uid(), amount);
		vict->save_char();

		delete vict;
		return (1);
	}
}

int BankTreasury(CharData *ch, void * /*me*/, char *argument) {
	// казна = clan treasury; BankManage returns false only when the char is not in a clan.
	if (!Clan::BankManage(ch, argument)) {
		SendMsgToChar(specials::BankMsg(specials::EBankMsg::kNotInClan) + "\r\n", ch);
	}
	return (1);
}

const SubCmdResolver kBankCmds([] { return specials::BankMsg(specials::EBankMsg::kGreeting); }, {
	{{"баланс", "сальдо", "balance"}, static_cast<int>(EBankCmd::kBalance), BankBalance},
	{{"вложить", "вклад", "deposit"}, static_cast<int>(EBankCmd::kDeposit), BankDeposit},
	{{"получить", "withdraw"}, static_cast<int>(EBankCmd::kWithdraw), BankWithdraw},
	{{"перевести", "transfer"}, static_cast<int>(EBankCmd::kTransfer), BankTransfer},
	{{"казна"}, static_cast<int>(EBankCmd::kTreasury), BankTreasury},
});
} // namespace

int bank(CharData *ch, void *me, int /*cmd*/, char *argument) {
	return kBankCmds.Dispatch(ch, me, argument);
}

bool is_post(RoomRnum room) {
	for (const auto ch : world[room]->people) {
		if (ch->IsNpc() && specials::IsPostkeeper(ch)) {
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
		if (ch->IsNpc() && specials::IsRentkeeper(ch)) {
			return true;
		}
	}
	return false;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
