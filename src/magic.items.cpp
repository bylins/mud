#include "magic.items.h"

#include "obj.hpp"
#include "room.hpp"
#include "chars/character.h"
#include "handler.h"
#include "object.prototypes.hpp"
#include "spells.info.h"
#include "magic.utils.hpp"

const short DEFAULT_STAFF_LVL = 12;
const short DEFAULT_WAND_LVL = 12;

extern char cast_argument[MAX_INPUT_LENGTH];

void employMagicItem(CHAR_DATA * ch, OBJ_DATA * obj, const char *argument)
{
	int i, spellnum;
	int level;
	CHAR_DATA *tch = nullptr;
	OBJ_DATA *tobj = nullptr;
	ROOM_DATA *troom = nullptr;

	one_argument(argument, cast_argument);
	level = GET_OBJ_VAL(obj, 0);
	if (level == 0)
	{
		if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_STAFF)
		{
			level = DEFAULT_STAFF_LVL;
		}
		else if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_WAND)
		{
			level = DEFAULT_WAND_LVL;
		}
	}

	if (obj->get_extra_flag(EExtraFlag::ITEM_TIMEDLVL))
	{
		int proto_timer = obj_proto[GET_OBJ_RNUM(obj)]->get_timer();
		if (proto_timer != 0)
		{
			level -= level * (proto_timer - obj->get_timer()) / proto_timer;
		}
	}

	switch (GET_OBJ_TYPE(obj))
	{
	case OBJ_DATA::ITEM_STAFF:
		if (!obj->get_action_description().empty())
		{
			act(obj->get_action_description().c_str(), FALSE, ch, obj, 0, TO_CHAR);
			act(obj->get_action_description().c_str(), FALSE, ch, obj, 0, TO_ROOM | TO_ARENA_LISTEN);
		}
		else
		{
			act("Вы ударили $o4 о землю.", FALSE, ch, obj, 0, TO_CHAR);
			act("$n ударил$g $o4 о землю.", FALSE, ch, obj, 0, TO_ROOM | TO_ARENA_LISTEN);
		}

		if (GET_OBJ_VAL(obj, 2) <= 0)
		{
			send_to_char("Похоже, кончились заряды :)\r\n", ch);
			act("И ничего не случилось.", FALSE, ch, obj, 0, TO_ROOM | TO_ARENA_LISTEN);
		}
		else
		{
			obj->dec_val(2);
			WAIT_STATE(ch, PULSE_VIOLENCE);
			if (HAS_SPELL_ROUTINE(GET_OBJ_VAL(obj, 3), MAG_MASSES | MAG_AREAS))
			{
				call_magic(ch, nullptr, nullptr, world[ch->in_room], GET_OBJ_VAL(obj, 3), level);
			}
			else
			{
				const auto people_copy = world[ch->in_room]->people;
				for (const auto tch : people_copy)
				{
					if (ch != tch)
					{
						call_magic(ch, tch, nullptr, world[ch->in_room], GET_OBJ_VAL(obj, 3), level);
					}
				}
			}
		}

		break;

	case OBJ_DATA::ITEM_WAND:
		spellnum = GET_OBJ_VAL(obj, 3);

		if (GET_OBJ_VAL(obj, 2) <= 0) {
			send_to_char("Похоже, магия кончилась.\r\n", ch);
			return;
		}

		if (!*argument) {
			if (!IS_SET(spell_info[GET_OBJ_VAL(obj, 3)].routines, MAG_AREAS | MAG_MASSES)) {
				tch = ch;
			}
		} else {
			if (!find_cast_target(spellnum, argument, ch, &tch, &tobj, &troom)) {
				return;
			}
		}

		if (tch) {
			if (tch == ch) {
				if (!obj->get_action_description().empty()) {
					act(obj->get_action_description().c_str(), FALSE, ch, obj, tch, TO_CHAR);
					act(obj->get_action_description().c_str(), FALSE, ch, obj, tch, TO_ROOM | TO_ARENA_LISTEN);
				} else {
					act("Вы указали $o4 на себя.", FALSE, ch, obj, 0, TO_CHAR);
					act("$n указал$g $o4 на себя.", FALSE, ch, obj, 0, TO_ROOM | TO_ARENA_LISTEN);
				}
			} else {
				if (!obj->get_action_description().empty()) {
					act(obj->get_action_description().c_str(), FALSE, ch, obj, tch, TO_CHAR);
					act(obj->get_action_description().c_str(), FALSE, ch, obj, tch, TO_ROOM | TO_ARENA_LISTEN);
				} else {
					act("Вы ткнули $o4 в $N3.", FALSE, ch, obj, tch, TO_CHAR);
					act("$N указал$G $o4 на вас.", FALSE, tch, obj, ch, TO_CHAR);
					act("$n ткнул$g $o4 в $N3.", TRUE, ch, obj, tch, TO_NOTVICT | TO_ARENA_LISTEN);
				}
			}
		}
		else if (tobj) {
			if (!obj->get_action_description().empty()) {
				act(obj->get_action_description().c_str(), FALSE, ch, obj, tobj, TO_CHAR);
				act(obj->get_action_description().c_str(), FALSE, ch, obj, tobj, TO_ROOM | TO_ARENA_LISTEN);
			} else {
				act("Вы прикоснулись $o4 к $O2.", FALSE, ch, obj, tobj, TO_CHAR);
				act("$n прикоснул$u $o4 к $O2.", TRUE, ch, obj, tobj, TO_ROOM | TO_ARENA_LISTEN);
			}
		}
		else {
			if (!obj->get_action_description().empty()) {
				act(obj->get_action_description().c_str(), FALSE, ch, obj, tch, TO_CHAR);
				act(obj->get_action_description().c_str(), FALSE, ch, obj, tch, TO_ROOM | TO_ARENA_LISTEN);
			}
			else {
				act("Вы обвели $o4 вокруг комнаты.", FALSE, ch, obj, nullptr, TO_CHAR);
				act("$n обвел$g $o4 вокруг комнаты.", TRUE, ch, obj, nullptr, TO_ROOM | TO_ARENA_LISTEN);
			}
		}

		obj->dec_val(2);
		WAIT_STATE(ch, PULSE_VIOLENCE);
		call_magic(ch, tch, tobj, world[ch->in_room], GET_OBJ_VAL(obj, 3), level);
		break;

	case OBJ_DATA::ITEM_SCROLL:
		if (AFF_FLAGGED(ch, EAffectFlag::AFF_SILENCE) || AFF_FLAGGED(ch, EAffectFlag::AFF_STRANGLED))
		{
			send_to_char("Вы немы, как рыба.\r\n", ch);
			return;
		}
		if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND))
		{
			send_to_char("Вы ослеплены.\r\n", ch);
			return;
		}

		spellnum = GET_OBJ_VAL(obj, 1);
		if (!*argument)
			tch = ch;
		else if (!find_cast_target(spellnum, argument, ch, &tch, &tobj, &troom))
			return;

		if (!obj->get_action_description().empty())
		{
			act(obj->get_action_description().c_str(), FALSE, ch, obj, nullptr, TO_CHAR);
			act(obj->get_action_description().c_str(), FALSE, ch, obj, nullptr, TO_ROOM | TO_ARENA_LISTEN);
		}
		else
		{
			act("Вы зачитали $o3, котор$W рассыпался в прах.", TRUE, ch, obj, 0, TO_CHAR);
			act("$n зачитал$g $o3.", FALSE, ch, obj, nullptr, TO_ROOM | TO_ARENA_LISTEN);
		}

		WAIT_STATE(ch, PULSE_VIOLENCE);
		for (i = 1; i <= 3; i++)
		{
			if (call_magic(ch, tch, tobj, world[ch->in_room], GET_OBJ_VAL(obj, i), level) <= 0)
			{
				break;
			}
		}

		if (obj != nullptr)
		{
			extract_obj(obj);
		}
		break;

	case OBJ_DATA::ITEM_POTION:
		if (AFF_FLAGGED(ch, EAffectFlag::AFF_STRANGLED))
		{
			send_to_char("Да вам сейчас и глоток воздуха не проглотить!\r\n", ch);
			return;
		}
		tch = ch;
		if (!obj->get_action_description().empty())
		{
			act(obj->get_action_description().c_str(), TRUE, ch, obj, nullptr, TO_CHAR);
			act(obj->get_action_description().c_str(), FALSE, ch, obj, nullptr, TO_ROOM | TO_ARENA_LISTEN);
		}
		else
		{
			act("Вы осушили $o3.", FALSE, ch, obj, nullptr, TO_CHAR);
			act("$n осушил$g $o3.", TRUE, ch, obj, nullptr, TO_ROOM | TO_ARENA_LISTEN);
		}

		WAIT_STATE(ch, PULSE_VIOLENCE);
		for (i = 1; i <= 3; i++)
		{
			if (call_magic(ch, ch, nullptr, world[ch->in_room], GET_OBJ_VAL(obj, i), level) <= 0)
			{
				break;
			}
		}

		if (obj != nullptr)
		{
			extract_obj(obj);
		}
		break;

	default:
		log("SYSERR: Unknown object_type %d in employMagicItem.", GET_OBJ_TYPE(obj));
		break;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
