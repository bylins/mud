#include "game_magic/magic_temp_spells.h"

#include "entities/world_characters.h"
#include "color.h"
#include "spells_info.h"
#include "structs/global_objects.h"

namespace temporary_spells {
void AddSpell(CharData *ch, ESpell spell_id, time_t set_time, time_t duration) {
	TemporarySpell sp;
	sp.spell = spell_id;
	sp.set_time = set_time;
	sp.duration = duration;
	auto it = ch->temp_spells.find(spell_id);
	if (it != ch->temp_spells.end()) {
		if ((it->second.set_time + it->second.duration) < (set_time + duration)) {
			it->second.set_time = set_time;
			it->second.duration = duration;
		}
	} else {
		SET_BIT(GET_SPELL_TYPE(ch, spell_id), ESpellType::kTemp);
		ch->temp_spells[spell_id] = sp;
	}
}

time_t GetSpellLeftTime(CharData *ch, ESpell spell_id) {
	auto it = ch->temp_spells.find(spell_id);
	if (it != ch->temp_spells.end()) {
		return (it->second.set_time + it->second.duration) - time(nullptr);
	}

	return -1;
}

void update_times() {
	time_t now = time(nullptr);
	for (const auto &ch : character_list) {
		if (ch->IsNpc()
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
			REMOVE_BIT(GET_SPELL_TYPE(ch, it->first), ESpellType::kTemp);

			//Если заклинание за это время не стало постоянным, то удалим из мема
			if (!IS_SET(GET_SPELL_TYPE(ch, it->first), ESpellType::kKnow)) {
				//Удаляем из мема
				for (i = &ch->mem_queue.queue; *i;) {
					if (i[0]->spell_id == it->first) {
						if (i == &ch->mem_queue.queue)
							ch->mem_queue.stored = 0;
						ch->mem_queue.total = std::max(0, ch->mem_queue.total - CalcSpellManacost(ch, it->first));
						ptr = i[0];
						i[0] = i[0]->next;
						free(ptr);
					} else {
						i = &(i[0]->next);
					}
				}

				//Удаляем из заученных
				GET_SPELL_MEM(ch, it->first) = 0;

				sprintf(buf,
						"Вы забыли заклинание \"%s%s%s\".\r\n",
						CCIMAG(ch, C_NRM), MUD::Spell(it->first).GetCName(), CCNRM(ch, C_NRM));
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
