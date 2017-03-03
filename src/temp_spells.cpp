#include "temp_spells.hpp"

#include "screen.h"
#include "utils.h"
#include "handler.h"

#include <vector>

extern int mag_manacost(CHAR_DATA * ch, int spellnum);

namespace Temporary_Spells
{
	void add_spell(CHAR_DATA* ch, int spell, time_t set_time, time_t duration)
	{
		temporary_spell_data sp;
		sp.spell = spell;
		sp.set_time = set_time;
		sp.duration = duration;

		for (auto it = ch->temp_spells.begin(); it != ch->temp_spells.end(); ++it)
		{
			//Если заклинание уже в списке, то обновим время при необходимости 
			if (it->spell == spell && ((it->set_time + it->duration) < (set_time + duration)))
			{
				it->set_time = set_time;
				it->duration = duration;
				return;
			}
		}

		//Если заклинание уже известно, то не добавляем
		if (!IS_SET(GET_SPELL_TYPE(ch, spell), SPELL_KNOW))
		{
			SET_BIT(GET_SPELL_TYPE(ch, spell), SPELL_TEMP);
			ch->temp_spells.push_back(sp);
		}
	}

	time_t spell_left_time(CHAR_DATA* ch, int spell)
	{
		for (auto it = ch->temp_spells.begin(); it != ch->temp_spells.end(); ++it)
		{
			if (it->spell == spell)
			{
				return (it->set_time + it->duration) - time(0);
			}
		}

		return -1;
	}

	void update_times()
	{
		time_t now = time(0);

		for (CHAR_DATA *ch = character_list; ch; ch = ch->get_next())
		{                                 
			if (IS_NPC(ch) || IS_IMMORTAL(ch))
				continue;

			update_char_times(ch, now);
		}
	}

	void update_char_times(CHAR_DATA *ch, time_t now)
	{
		struct spell_mem_queue_item **i, *ptr;

		for (auto it = ch->temp_spells.begin(); it != ch->temp_spells.end();)
		{
			if ((it->set_time + it->duration) < now)
			{
				REMOVE_BIT(GET_SPELL_TYPE(ch, it->spell), SPELL_TEMP);

				//Если заклинание за это время не стало постоянным, то удалим из мема
				if (!IS_SET(GET_SPELL_TYPE(ch, it->spell), SPELL_KNOW))
				{
					//Удаляем из мема
					for (i = &ch->MemQueue.queue; *i;)
					{
						if (i[0]->spellnum == it->spell)
						{
							if (i == &ch->MemQueue.queue)
								GET_MEM_COMPLETED(ch) = 0;
							GET_MEM_TOTAL(ch) = MAX(0, GET_MEM_TOTAL(ch) - mag_manacost(ch, it->spell));
							ptr = i[0];
							i[0] = i[0]->link;
							free(ptr);
						}
						else
						{
							i = &(i[0]->link);
						}
					}

					//Удаляем из заученных
					GET_SPELL_MEM(ch, it->spell) = 0;

					sprintf(buf,
						"Вы забыли заклинание \"%s%s%s\".\r\n",
						CCIMAG(ch, C_NRM), spell_info[it->spell].name, CCNRM(ch, C_NRM));
					send_to_char(buf, ch);
				}

				it = ch->temp_spells.erase(it);
			}
			else
			{
				++it;
			}
		}
	}
}