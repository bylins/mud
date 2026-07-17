#include "magic_items.h"

#include <algorithm>
#include <limits>

#include "engine/entities/char_data.h"
#include "engine/core/obj_handler.h"
#include "engine/db/obj_prototypes.h"
#include "spells_info.h"
#include "magic_utils.h"
#include "engine/db/global_objects.h"

// issue.magic-items: map a scroll spell slot (1..3) to its shared ObjVal key.
static ObjVal::EValueKey spell_item_slot_key(int slot) {
	switch (slot) {
		case 2: return ObjVal::EValueKey::kSpell2Num;
		case 3: return ObjVal::EValueKey::kSpell3Num;
		default: return ObjVal::EValueKey::kSpell1Num;
	}
}

extern char cast_argument[kMaxInputLength];

void EmployMagicItem(CharData *ch, ObjData *obj, const char *argument) {
	int i;
	CharData *tch = nullptr;
	ObjData *tobj = nullptr;
	RoomData *troom = nullptr;

	one_argument(argument, cast_argument);
	// issue.magic-items: scroll/wand/staff no longer cast at a stored caster-level -- their strength
	// is the maker competence (MagicItemPotency), like a potion. A kTimedLvl item still "wears out":
	// the fraction of its timer remaining scales the effective potency/skill down as it ages.
	double timed_factor = 1.0;
	if (obj->has_flag(EObjFlag::kTimedLvl)) {
		const int proto_timer = obj_proto[obj->get_rnum()]->get_timer();
		if (proto_timer != 0) {
			timed_factor = std::clamp(static_cast<double>(obj->get_timer()) / proto_timer, 0.0, 1.0);
		}
	}

	// Cast one spell from this item at its maker competence (potency); noise drawn fresh (not brewed).
	auto cast_item_spell = [&](CharData *tgt_ch, ObjData *tgt_obj, ESpell sp) -> ECastResult {
		const float potency = MagicItemPotency(obj, sp) * static_cast<float>(timed_factor);
		const int skill = static_cast<int>(MagicItemSkill(obj) * timed_factor);
		// issue.magic-items: pass a real 0.0 (deterministic -- no noise spread), never NaN. Under the
		// fasttest -Ofast (-ffast-math) build std::isnan() folds to false, so a NaN sentinel would leak
		// into the affect <apply> modifier (noise_dev) and yield INT_MIN. Authored items cast at their
		// exact maker potency.
		return CallMagic(ch, tgt_ch, tgt_obj, world[ch->in_room], sp, 0, potency,
						 /*noise z*/ 0.0, skill);
	};

	auto spell_id = static_cast<ESpell>(obj->GetPotionValueKey(ObjVal::EValueKey::kSpell1Num));
	switch (obj->get_type()) {
		case EObjType::kStaff:
			if (!obj->get_action_description().empty()) {
				act(obj->get_action_description().c_str(), false, ch, obj, nullptr, kToChar);
				act(obj->get_action_description().c_str(), false, ch, obj, nullptr, kToRoom | kToArenaListen);
			} else {
				act("Вы ударили $o4 о землю.", false, ch, obj, nullptr, kToChar);
				act("$n ударил$g $o4 о землю.", false, ch, obj, nullptr, kToRoom | kToArenaListen);
			}

			if (obj->GetPotionValueKey(ObjVal::EValueKey::kCurCharges) <= 0) {
				SendMsgToChar("Похоже, кончились заряды :)\r\n", ch);
				act("И ничего не случилось.", false, ch, obj, nullptr, kToRoom | kToArenaListen);
			} else {
				obj->SetPotionValueKey(ObjVal::EValueKey::kCurCharges,
						obj->GetPotionValueKey(ObjVal::EValueKey::kCurCharges) - 1);
				SetBattleLag(ch, 1);
				if (MUD::Spell(spell_id).IsFlagged(kMagMasses | kMagAreas)) {
					cast_item_spell(nullptr, nullptr, spell_id);
				} else  if (MUD::Spell(spell_id).IsFlagged(kMagGroups | kMagManual)) {
					cast_item_spell(ch, nullptr, spell_id);
				} else {
					const auto people_copy = world[ch->in_room]->people;
					for (const auto target : people_copy) {
						if (ch != target) {
							cast_item_spell(target, nullptr, spell_id);
						}
					}
				}
			}
			break;

		case EObjType::kWand:
			if (obj->GetPotionValueKey(ObjVal::EValueKey::kCurCharges) <= 0) {
				SendMsgToChar("Похоже, магия кончилась.\r\n", ch);
				return;
			}

			if (!*argument) {
				if (!MUD::Spell(spell_id).IsFlagged(kMagAreas | kMagMasses)) {
					tch = ch;
				}
			} else {
				if (!FindCastTarget(spell_id, argument, ch, &tch, &tobj, &troom)) {
					return;
				}
			}

			if (tch) {
				if (tch == ch) {
					if (!obj->get_action_description().empty()) {
						act(obj->get_action_description().c_str(), false, ch, obj, tch, kToChar);
						act(obj->get_action_description().c_str(), false, ch, obj, tch, kToRoom | kToArenaListen);
					} else {
						act("Вы указали $o4 на себя.", false, ch, obj, nullptr, kToChar);
						act("$n указал$g $o4 на себя.", false, ch, obj, nullptr, kToRoom | kToArenaListen);
					}
				} else {
					if (!obj->get_action_description().empty()) {
						act(obj->get_action_description().c_str(), false, ch, obj, tch, kToChar);
						act(obj->get_action_description().c_str(), false, ch, obj, tch, kToRoom | kToArenaListen);
					} else {
						act("Вы ткнули $o4 в $N3.", false, ch, obj, tch, kToChar);
						act("$N указал$G $o4 на вас.", false, tch, obj, ch, kToChar);
						act("$n ткнул$g $o4 в $N3.", true, ch, obj, tch, kToNotVict | kToArenaListen);
					}
				}
			} else if (tobj) {
				if (!obj->get_action_description().empty()) {
					act(obj->get_action_description().c_str(), false, ch, obj, tobj, kToChar);
					act(obj->get_action_description().c_str(), false, ch, obj, tobj, kToRoom | kToArenaListen);
				} else {
					act("Вы прикоснулись $o4 к $O2.", false, ch, obj, tobj, kToChar);
					act("$n прикоснул$u $o4 к $O2.", true, ch, obj, tobj, kToRoom | kToArenaListen);
				}
			} else {
				if (!obj->get_action_description().empty()) {
					act(obj->get_action_description().c_str(), false, ch, obj, tch, kToChar);
					act(obj->get_action_description().c_str(), false, ch, obj, tch, kToRoom | kToArenaListen);
				} else {
					act("Вы обвели $o4 вокруг комнаты.", false, ch, obj, nullptr, kToChar);
					act("$n обвел$g $o4 вокруг комнаты.", true, ch, obj, nullptr, kToRoom | kToArenaListen);
				}
			}

			obj->SetPotionValueKey(ObjVal::EValueKey::kCurCharges,
					obj->GetPotionValueKey(ObjVal::EValueKey::kCurCharges) - 1);
			SetBattleLag(ch, 1);
			cast_item_spell(tch, tobj, spell_id);
			break;

		case EObjType::kScroll:
			if (AFF_FLAGGED(ch, EAffect::kSilence)) {
				SendMsgToChar("Вы немы, как рыба.\r\n", ch);
				return;
			}
			if (AFF_FLAGGED(ch, EAffect::kBlind)) {
				SendMsgToChar("Вы ослеплены.\r\n", ch);
				return;
			}

			spell_id = static_cast<ESpell>(obj->GetPotionValueKey(ObjVal::EValueKey::kSpell1Num));
			if (!*argument) {
				for (int slot = 1; slot < 4; slot++) {
					if (MUD::Spell(static_cast<ESpell>(obj->GetPotionValueKey(spell_item_slot_key(slot)))).IsFlagged(kMagAreas | kMagMasses)) {
						break;
					}
					tch = ch;
				}
			} else if (!FindCastTarget(spell_id, argument, ch, &tch, &tobj, &troom)) {
				return;
			}

			if (!obj->get_action_description().empty()) {
				act(obj->get_action_description().c_str(), false, ch, obj, nullptr, kToChar);
				act(obj->get_action_description().c_str(), false, ch, obj, nullptr, kToRoom | kToArenaListen);
			} else {
				act("Вы зачитали $o3, котор$W рассыпался в прах.", true, ch, obj, nullptr, kToChar);
				act("$n зачитал$g $o3.", false, ch, obj, nullptr, kToRoom | kToArenaListen);
			}

			SetBattleLag(ch, 1);
			for (i = 1; i <= 3; i++) {
				if (cast_item_spell(tch, tobj, static_cast<ESpell>(obj->GetPotionValueKey(spell_item_slot_key(i)))) != ECastResult::kSuccess) {
					break;
				}
			}

			/*if (obj != nullptr) {
				extract_obj(obj);
			}*/
			ExtractObjFromWorld(obj);
			break;

		case EObjType::kPotion:
			if (AFF_FLAGGED(ch, EAffect::kStrangled) && AFF_FLAGGED(ch, EAffect::kSilence)) {
				SendMsgToChar("Да вам сейчас и глоток воздуха не проглотить!\r\n", ch);
				return;
			}
			tch = ch;
			if (!obj->get_action_description().empty()) {
				act(obj->get_action_description().c_str(), true, ch, obj, nullptr, kToChar);
				act(obj->get_action_description().c_str(), false, ch, obj, nullptr, kToRoom | kToArenaListen);
			} else {
				act("Вы осушили $o3.", false, ch, obj, nullptr, kToChar);
				act("$n осушил$g $o3.", true, ch, obj, nullptr, kToRoom | kToArenaListen);
			}

			SetBattleLag(ch, 1);
			{
				// issue.potion-hotfix P3: strength is BREWED IN -- the crafted kPotionPotency, or (non-
				// crafted) the value a fixed-skill potion-maker would brew (MagicItemPotency). Spells,
				// potency and the frozen noise roll all live in the ObjVal keys (the boot/load converter
				// migrated every potion off m_vals), so we read keys ONLY. brew_roll absent (non-crafted)
				// -> NaN -> the noise is drawn at cast, like a fresh cast.
				const int brew_roll = obj->GetPotionValueKey(ObjVal::EValueKey::kPotionBrewRoll);
				const double noise_z = (brew_roll > 0)
					? static_cast<double>(brew_roll) / ObjVal::kBrewRollScale - ObjVal::kBrewRollBias
					: std::numeric_limits<double>::quiet_NaN();
				for (i = 1; i <= 3; i++) {
					const ObjVal::EValueKey spell_key = (i == 1) ? ObjVal::EValueKey::kSpell1Num
						: (i == 2) ? ObjVal::EValueKey::kSpell2Num : ObjVal::EValueKey::kSpell3Num;
					const int spell_num = obj->GetPotionValueKey(spell_key);
					if (spell_num <= 0) {
						continue;
					}
					const float potency = MagicItemPotency(obj, static_cast<ESpell>(spell_num));
					if (CallMagic(ch, ch, nullptr, world[ch->in_room], static_cast<ESpell>(spell_num), 0,
								  potency, noise_z, MagicItemSkill(obj)) != ECastResult::kSuccess) {
						break;
					}
				}
			}

			/*if (obj != nullptr) {
				extract_obj(obj);
			}*/
			ExtractObjFromWorld(obj);
			break;

		default: log("SYSERR: Unknown object_type %d in EmployMagicItem.", obj->get_type());
			break;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
