#include "gameplay/mechanics/initiative.h"

#include "engine/entities/char_data.h"
#include "gameplay/core/constants.h"          // SizeApp
#include "gameplay/magic/spells_constants.h"  // battle-affect flags (kEaf*)
#include "gameplay/skills/leadership.h"        // CalcLeadership
#include "gameplay/mechanics/inventory.h"       // CAN_CARRY_W
#include "utils/utils.h"                        // GET_* accessors, MIN/MAX
#include "utils/random.h"                       // number()
#include "utils/logger.h"                       // log()

#include <algorithm>

// ============================================================================
// Combat initiative
// ----------------------------------------------------------------------------
// Initiative governs a single combat round (perform_violence, fight.cpp). It has
// two jobs:
//
//   1. Turn ORDER. At the top of the round every fighter rolls an initiative
//      value (roll_round_initiative). The round then steps a counter from the
//      round's highest value down to its lowest (max_init .. min_init); on each
//      step, fighters whose stored ch->initiative equals the counter act. So a
//      higher roll acts earlier.
//
//   2. Extra attack PASSES (players only). A fighter does not act just once: as
//      long as its remaining initiative is above the round floor (min_init), it
//      spends one point per action taken (try_consume_extra_pass) and yields.
//      Because the counter keeps descending, the fighter is revisited at lower
//      values, taking further attack passes. The higher a fighter rolled above
//      the floor, the more passes it gets in the round.
//
// A rolled 0 is special-cased to -100 ("act last", roughly a 1 in 201 outcome),
// and stop-fight states (hold/fear/sleep) that land after the roll are skipped
// during the ordered passes.
//
// The per-fighter value lives in the transient CharData::initiative field (reset
// each round in stuff_before_round). A persistent bonus from equipment/affects
// is added via GET_INITIATIVE(ch) (the add_abils.initiative_add apply).
// ============================================================================

int calc_initiative(CharData *ch, bool mode) {
	int initiative = SizeApp(GET_POS_SIZE(ch)).initiative;
	if (mode) //Добавим булевую переменную, чтобы счет все выдавал постоянное значение, а не каждый раз рандом
	{
		int i = number(1, 10);
		if (i == 10)
			initiative -= 1;
		else
			initiative += i;
	};

	initiative += GET_INITIATIVE(ch);

	if (!ch->IsNpc()) {
		switch (ch->GetCarryingWeight() * 10 / MAX(1, CAN_CARRY_W(ch))) {
			case 10:
			case 9:
			case 8: initiative -= 20;
				break;
			case 7:
			case 6:
			case 5: initiative -= 10;
				break;
		}
	}

	if (ch->battle_affects.get(kEafAwake))
		initiative -= 20;
	if (ch->battle_affects.get(kEafPunctual))
		initiative -= 10;
	if (ch->get_wait() > 0)
		initiative -= 1;
	if (CalcLeadership(ch))
		initiative += 5;
	if (ch->battle_affects.get(kEafSlow))
		initiative = 1;

// indra
// рраскомменчу, посмотрим
	initiative = std::max(initiative, 1);
	//Почему инициатива не может быть отрицательной?
	return initiative;
}

void roll_round_initiative(CharData *ch, int &min_init, int &max_init) {
	int initiative = calc_initiative(ch, true);
	if (initiative > 100) { //откуда больше 100??????
		log("initiative calc: %s (%d) == %d", GET_NAME(ch), GET_MOB_VNUM(ch), initiative);
	}
	initiative = std::clamp(initiative, -100, 100);
	if (initiative == 0) {
		ch->initiative = -100; //Если кубик выпал в 0 - бей последним шанс 1 из 201
		min_init = MIN(min_init, -100);
	} else {
		ch->initiative = initiative;
	}
	max_init = MAX(max_init, initiative);
	min_init = MIN(min_init, initiative);
}

bool try_consume_extra_pass(CharData *ch, int min_init) {
	if (ch->initiative > min_init) {
		--(ch->initiative);
		return true;
	}
	return false;
}
