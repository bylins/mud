/**
 \file handle_thunderstorm_tick.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: HandleThunderstormTick room-affect tick handler (extracted from magic_rooms.cpp).
*/

#include "gameplay/handlers/spell_handlers.h"
#include "gameplay/magic/magic_rooms.h"
#include "gameplay/magic/magic_internal.h"
#include "gameplay/magic/magic_utils.h"
#include "engine/entities/char_data.h"
#include "engine/core/comm.h"
#include "engine/db/global_objects.h"
#include "gameplay/magic/spell_messages.h"
#include "gameplay/mechanics/weather.h"
#include "gameplay/magic/spells.h"   // cast_argument

extern int what_sky;

using namespace room_spells;

// EmitThunderstormMsg: single-use helper for HandleThunderstormTick (kept file-local).
static void EmitThunderstormMsg(CharData *ch, ESpellMsg key) {
	const auto &msg = MUD::SpellMessages()[ESpell::kThunderstorm].GetMessage(key);
	if (!msg.empty()) {
		act(msg.c_str(), false, ch, nullptr, nullptr, kToRoom | kToChar | kToArenaListen);
	}
}

namespace handlers {

// issue.affect-migration: a room-affect manual_cast handler. It runs as an <action><manual_cast
// handler="HandleThunderstormTick"/></action> on the kThunderstorm affect, dispatched via the same
// kManualHandlers registry spells use. The per-tick phase is the affect's current duration, threaded
// in via ActionContext (SetTickDuration); writing it back (SetTickDuration) ends the storm early.
EStageResult HandleThunderstormTick(ActionContext &ctx) {
	CharData *ch = ctx.caster();
	if (ch == nullptr) {
		return EStageResult::kSuccess;
	}
	switch (ctx.GetTickDuration()) {
	case 7:
		cast_argument[0] = '\0';   // programmatic cast: SpellControlWeather must not parse a stale argument
		if (CallMagic(ch, nullptr, nullptr, nullptr, ESpell::kControlWeather, GetRealLevel(ch)) == ECastResult::kNotCast) {
			ctx.SetTickDuration(0);
			return EStageResult::kBreak;
		}
		what_sky = kSkyCloudy;
		EmitThunderstormMsg(ch, ESpellMsg::kCustomMsgOne);
		break;
	case 6:
		EmitThunderstormMsg(ch, ESpellMsg::kCustomMsgTwo);
		CastAreaInRoom(ch, ESpell::kDeafness, GetRealLevel(ch));
		break;
	case 5:
		EmitThunderstormMsg(ch, ESpellMsg::kCustomMsgThree);
		CastAreaInRoom(ch, ESpell::kColdWind, GetRealLevel(ch));
		break;
	case 4:
		EmitThunderstormMsg(ch, ESpellMsg::kCustomMsgFour);
		CastAreaInRoom(ch, ESpell::kAcid, GetRealLevel(ch));
		break;
	case 3:
		EmitThunderstormMsg(ch, ESpellMsg::kCustomMsgFive);
		CastAreaInRoom(ch, ESpell::kLightingBolt, GetRealLevel(ch));
		break;
	case 2:
		EmitThunderstormMsg(ch, ESpellMsg::kCustomMsgSix);
		CastAreaInRoom(ch, ESpell::kCallLighting, GetRealLevel(ch));
		break;
	case 1:
		EmitThunderstormMsg(ch, ESpellMsg::kCustomMsgSeven);
		CastAreaInRoom(ch, ESpell::kWhirlwind, GetRealLevel(ch));
		break;
	case 0:
	default:
		what_sky = kSkyCloudless;
		EmitThunderstormMsg(ch, ESpellMsg::kCustomMsgEight);
		break;
	}
	return EStageResult::kSuccess;
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
