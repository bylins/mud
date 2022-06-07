#include "magic_items.h"

#include "entities/char_data.h"
#include "handler.h"
#include "obj_prototypes.h"
#include "spells_info.h"
#include "magic_utils.h"
#include "structs/global_objects.h"

const short kDefaultStaffLvl = 12;
const short kDefaultWandLvl = 12;

extern char cast_argument[kMaxInputLength];

void EmployMagicItem(CharData *ch, ObjData *obj, const char *argument) {
	int i;
	int level;
	CharData *tch = nullptr;
	ObjData *tobj = nullptr;
	RoomData *troom = nullptr;

	one_argument(argument, cast_argument);
	level = GET_OBJ_VAL(obj, 0);
	if (level == 0) {
		if (GET_OBJ_TYPE(obj) == EObjType::kStaff) {
			level = kDefaultStaffLvl;
		} else if (GET_OBJ_TYPE(obj) == EObjType::kWand) {
			level = kDefaultWandLvl;
		}
	}

	if (obj->has_flag(EObjFlag::kTimedLvl)) {
		int proto_timer = obj_proto[GET_OBJ_RNUM(obj)]->get_timer();
		if (proto_timer != 0) {
			level -= level * (proto_timer - obj->get_timer()) / proto_timer;
		}
	}

	auto spell_id = static_cast<ESpell>(GET_OBJ_VAL(obj, 3));
	switch (GET_OBJ_TYPE(obj)) {
		case EObjType::kStaff:
			if (!obj->get_action_description().empty()) {
				act(obj->get_action_description().c_str(), false, ch, obj, nullptr, kToChar);
				act(obj->get_action_description().c_str(), false, ch, obj, nullptr, kToRoom | kToArenaListen);
			} else {
				act("Вы ударили $o4 о землю.", false, ch, obj, nullptr, kToChar);
				act("$n ударил$g $o4 о землю.", false, ch, obj, nullptr, kToRoom | kToArenaListen);
			}

			if (GET_OBJ_VAL(obj, 2) <= 0) {
				SendMsgToChar("Похоже, кончились заряды :)\r\n", ch);
				act("И ничего не случилось.", false, ch, obj, nullptr, kToRoom | kToArenaListen);
			} else {
				obj->dec_val(2);
				SetWaitState(ch, kPulseViolence);
				if (MUD::Spell(spell_id).IsFlagged(kMagMasses | kMagAreas)) {
					CallMagic(ch, nullptr, nullptr, world[ch->in_room], spell_id, level);
				} else {
					const auto people_copy = world[ch->in_room]->people;
					for (const auto target : people_copy) {
						if (ch != target) {
							CallMagic(ch, target, nullptr, world[ch->in_room], spell_id, level);
						}
					}
				}
			}
			break;

		case EObjType::kWand:
			if (GET_OBJ_VAL(obj, 2) <= 0) {
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

			obj->dec_val(2);
			SetWaitState(ch, kPulseViolence);
			CallMagic(ch, tch, tobj, world[ch->in_room], spell_id, level);
			break;

		case EObjType::kScroll:
			if (AFF_FLAGGED(ch, EAffect::kSilence) || AFF_FLAGGED(ch, EAffect::kStrangled)) {
				SendMsgToChar("Вы немы, как рыба.\r\n", ch);
				return;
			}
			if (AFF_FLAGGED(ch, EAffect::kBlind)) {
				SendMsgToChar("Вы ослеплены.\r\n", ch);
				return;
			}

			spell_id = static_cast<ESpell>(GET_OBJ_VAL(obj, 1));
			if (!*argument) {
				for (int slot = 1; slot < 4; slot++) {
					if (MUD::Spell(static_cast<ESpell>(GET_OBJ_VAL(obj, slot))).IsFlagged(kMagAreas | kMagMasses)) {
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

			SetWaitState(ch, kPulseViolence);
			for (i = 1; i <= 3; i++) {
				if (CallMagic(ch, tch, tobj, world[ch->in_room], static_cast<ESpell>(GET_OBJ_VAL(obj, i)), level) <= 0) {
					break;
				}
			}

			/*if (obj != nullptr) {
				extract_obj(obj);
			}*/
			ExtractObjFromWorld(obj);
			break;

		case EObjType::kPotion:
			if (AFF_FLAGGED(ch, EAffect::kStrangled)) {
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

			SetWaitState(ch, kPulseViolence);
			for (i = 1; i <= 3; i++) {
				if (CallMagic(ch, ch, nullptr, world[ch->in_room], static_cast<ESpell>(GET_OBJ_VAL(obj, i)), level) <= 0) {
					break;
				}
			}

			/*if (obj != nullptr) {
				extract_obj(obj);
			}*/
			ExtractObjFromWorld(obj);
			break;

		default: log("SYSERR: Unknown object_type %d in EmployMagicItem.", GET_OBJ_TYPE(obj));
			break;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
