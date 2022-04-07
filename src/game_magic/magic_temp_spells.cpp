#include "game_magic/magic_temp_spells.h"

#include "entities/world_characters.h"
#include "color.h"
#include "handler.h"
#include "spells_info.h"

namespace Temporary_Spells {
void add_spell(CharData *ch, int spellnum, time_t set_time, time_t duration) {
	temporary_spell_data sp;
	sp.spell = spellnum;
	sp.set_time = set_time;
	sp.duration = duration;
	auto it = ch->temp_spells.find(spellnum);
	if (it != ch->temp_spells.end()) {
		if ((it->second.set_time + it->second.duration) < (set_time + duration)) {
			it->second.set_time = set_time;
			it->second.duration = duration;
		}
	} else {
		SET_BIT(GET_SPELL_TYPE(ch, spellnum), kSpellTemp);
		ch->temp_spells[spellnum] = sp;
	}
}

time_t spell_left_time(CharData *ch, int spellnum) {
	auto it = ch->temp_spells.find(spellnum);
	if (it != ch->temp_spells.end()) {
		return (it->second.set_time + it->second.duration) - time(0);
	}

	return -1;
}

void update_times() {
	time_t now = time(0);
	for (const auto &ch : character_list) {
		if (ch->is_npc()
			|| IS_IMMORTAL(ch)) {
			continue;
		}

		update_char_times(ch.get(), now);
	}
}

void update_char_times(CharData *ch, time_t now) {
	struct SpellMemQueueItem **i, *ptr;

	for (auto it = ch->temp_spells.begin(); it != ch->temp_spells.end();) {
		if ((it->second.set_time + it->second.duration) < now) {
			REMOVE_BIT(GET_SPELL_TYPE(ch, it->first), kSpellTemp);

			//Если заклинание за это время не стало постоянным, то удалим из мема
			if (!IS_SET(GET_SPELL_TYPE(ch, it->first), kSpellKnow)) {
				//Удаляем из мема
				for (i = &ch->mem_queue.queue; *i;) {
					if (i[0]->spellnum == it->first) {
						if (i == &ch->mem_queue.queue)
							ch->mem_queue.stored = 0;
						ch->mem_queue.total = std::max(0, ch->mem_queue.total - mag_manacost(ch, it->first));
						ptr = i[0];
						i[0] = i[0]->link;
						free(ptr);
					} else {
						i = &(i[0]->link);
					}
				}

				//Удаляем из заученных
				GET_SPELL_MEM(ch, it->first) = 0;

				sprintf(buf,
						"Вы забыли заклинание \"%s%s%s\".\r\n",
						CCIMAG(ch, C_NRM), spell_info[it->first].name, CCNRM(ch, C_NRM));
				SendMsgToChar(buf, ch);
			}

			it = ch->temp_spells.erase(it);
		} else {
			++it;
		}
	}
}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
