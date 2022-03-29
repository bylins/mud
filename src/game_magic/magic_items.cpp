#include "magic_items.h"

#include "entities/char_data.h"
#include "handler.h"
#include "obj_prototypes.h"
#include "spells_info.h"
#include "magic_utils.h"

const short DEFAULT_STAFF_LVL = 12;
const short DEFAULT_WAND_LVL = 12;

extern char cast_argument[kMaxInputLength];

void EmployMagicItem(CharData *ch, ObjData *obj, const char *argument) {
	int i, spellnum;
	int level;
	CharData *tch = nullptr;
	ObjData *tobj = nullptr;
	RoomData *troom = nullptr;

	one_argument(argument, cast_argument);
	level = GET_OBJ_VAL(obj, 0);
	if (level == 0) {
		if (GET_OBJ_TYPE(obj) == ObjData::ITEM_STAFF) {
			level = DEFAULT_STAFF_LVL;
		} else if (GET_OBJ_TYPE(obj) == ObjData::ITEM_WAND) {
			level = DEFAULT_WAND_LVL;
		}
	}

	if (obj->has_flag(EObjFlag::kTimedLvl)) {
		int proto_timer = obj_proto[GET_OBJ_RNUM(obj)]->get_timer();
		if (proto_timer != 0) {
			level -= level * (proto_timer - obj->get_timer()) / proto_timer;
		}
	}

	switch (GET_OBJ_TYPE(obj)) {
		case ObjData::ITEM_STAFF:
			if (!obj->get_action_description().empty()) {
				act(obj->get_action_description().c_str(), false, ch, obj, nullptr, kToChar);
				act(obj->get_action_description().c_str(), false, ch, obj, nullptr, kToRoom | kToArenaListen);
			} else {
				act("Вы ударили $o4 о землю.", false, ch, obj, nullptr, kToChar);
				act("$n ударил$g $o4 о землю.", false, ch, obj, nullptr, kToRoom | kToArenaListen);
			}

			if (GET_OBJ_VAL(obj, 2) <= 0) {
				send_to_char("Похоже, кончились заряды :)\r\n", ch);
				act("И ничего не случилось.", false, ch, obj, nullptr, kToRoom | kToArenaListen);
			} else {
				obj->dec_val(2);
				WAIT_STATE(ch, kPulseViolence);
				if (HAS_SPELL_ROUTINE(GET_OBJ_VAL(obj, 3), kMagMasses | kMagAreas)) {
					CallMagic(ch, nullptr, nullptr, world[ch->in_room], GET_OBJ_VAL(obj, 3), level);
				} else {
					const auto people_copy = world[ch->in_room]->people;
					for (const auto target : people_copy) {
						if (ch != target) {
							CallMagic(ch, target, nullptr, world[ch->in_room], GET_OBJ_VAL(obj, 3), level);
						}
					}
				}
			}

			break;

		case ObjData::ITEM_WAND: spellnum = GET_OBJ_VAL(obj, 3);

			if (GET_OBJ_VAL(obj, 2) <= 0) {
				send_to_char("Похоже, магия кончилась.\r\n", ch);
				return;
			}

			if (!*argument) {
				if (!IS_SET(spell_info[GET_OBJ_VAL(obj, 3)].routines, kMagAreas | kMagMasses)) {
					tch = ch;
				}
			} else {
				if (!FindCastTarget(spellnum, argument, ch, &tch, &tobj, &troom)) {
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
			WAIT_STATE(ch, kPulseViolence);
			CallMagic(ch, tch, tobj, world[ch->in_room], GET_OBJ_VAL(obj, 3), level);
			break;

		case ObjData::ITEM_SCROLL:
			if (AFF_FLAGGED(ch, EAffect::kSilence) || AFF_FLAGGED(ch, EAffect::kStrangled)) {
				send_to_char("Вы немы, как рыба.\r\n", ch);
				return;
			}
			if (AFF_FLAGGED(ch, EAffect::kBlind)) {
				send_to_char("Вы ослеплены.\r\n", ch);
				return;
			}

			spellnum = GET_OBJ_VAL(obj, 1);
			if (!*argument) {
				for (int slot = 1; slot < 4; slot++) {
					if (IS_SET(spell_info[GET_OBJ_VAL(obj, slot)].routines, kMagAreas | kMagMasses)) {
						break;
					}
					tch = ch;
				}
			} else if (!FindCastTarget(spellnum, argument, ch, &tch, &tobj, &troom)) {
				return;
			}

			if (!obj->get_action_description().empty()) {
				act(obj->get_action_description().c_str(), false, ch, obj, nullptr, kToChar);
				act(obj->get_action_description().c_str(), false, ch, obj, nullptr, kToRoom | kToArenaListen);
			} else {
				act("Вы зачитали $o3, котор$W рассыпался в прах.", true, ch, obj, nullptr, kToChar);
				act("$n зачитал$g $o3.", false, ch, obj, nullptr, kToRoom | kToArenaListen);
			}

			WAIT_STATE(ch, kPulseViolence);
			for (i = 1; i <= 3; i++) {
				if (CallMagic(ch, tch, tobj, world[ch->in_room], GET_OBJ_VAL(obj, i), level) <= 0) {
					break;
				}
			}

			/*if (obj != nullptr) {
				extract_obj(obj);
			}*/
			extract_obj(obj);
			break;

		case ObjData::ITEM_POTION:
			if (AFF_FLAGGED(ch, EAffect::kStrangled)) {
				send_to_char("Да вам сейчас и глоток воздуха не проглотить!\r\n", ch);
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

			WAIT_STATE(ch, kPulseViolence);
			for (i = 1; i <= 3; i++) {
				if (CallMagic(ch, ch, nullptr, world[ch->in_room], GET_OBJ_VAL(obj, i), level) <= 0) {
					break;
				}
			}

			/*if (obj != nullptr) {
				extract_obj(obj);
			}*/
			extract_obj(obj);
			break;

		default: log("SYSERR: Unknown object_type %d in EmployMagicItem.", GET_OBJ_TYPE(obj));
			break;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
