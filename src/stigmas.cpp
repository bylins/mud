#include "stigmas.h"
#include "entities/char_data.h"


/*void do_touch_stigma(CharacterData *ch, char*, int, int);

std::vector<Stigma> stigmas;

void do_touch_stigma(CharacterData *ch, char* argument, int, int)
{
	CharacterData *vict = NULL;

	one_argument(argument, buf);

	if (!*buf)
	{
		SendMsgToChar(ch, "Вы прикоснулись к себе. Приятно!\r\n");
		return;
	}

	if (!(vict = get_player_vis(ch, buf, EFind::kCharInWorld)))
	{
		ch->touch_stigma(buf);
	}
	else
	{
		sprintf(buf, "Вы прикоснулись к %s. Ничего не произошло.\r\n", vict->get_name().c_str());
		SendMsgToChar(buf, ch);
	}
}

std::string StigmaWear::get_name() const
{
	return this->stigma.name;
}

// стигма огненный дракон
void stigma_fire_dragon(CharacterData *ch)
{
	SendMsgToChar(ch, "Вы прикоснулись к стигме с изображением огненного дракона.\r\n");
	SendMsgToChar(ch, "Рисунок вспыхнул и вы почуствовали небольшую боль.");
}

// инициализация стигм
void init_stigmas()
{
	Stigma tmp;
	tmp.id = STIGMA_FIRE_DRAGON;
	tmp.name = "огненный дракон";
	tmp.activation_stigma = &stigma_fire_dragon;
	tmp.reload = 10;
	stigmas.push_back(tmp);
}*/
