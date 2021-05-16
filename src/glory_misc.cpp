// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2010 Krodo
// Part of Bylins http://www.mud.ru

#include "glory_misc.hpp"

#include "logger.hpp"
#include "utils.h"
#include "glory.hpp"
#include "glory_const.hpp"
#include "genchar.h"
#include "chars/character.h"
#include "db.h"
#include "screen.h"
#include "comm.h"
#include "chars/char_player.hpp"
#include "modify.h"

#include <boost/algorithm/string.hpp>

#include <map>

namespace GloryMisc
{

class GloryLog
{
public:
	GloryLog() : type(0), num(0) {};
	// тип записи (1 - плюс слава, 2 - минус слава, 3 - убирание стата, 4 - трансфер, 5 - hide)
	int type;
	// кол-во славы при +- славы
	int num;
	// что было записано в карму
	std::string karma;
};

typedef std::shared_ptr<GloryLog> GloryLogPtr;
typedef std::multimap<time_t /* время */, GloryLogPtr> GloryLogType;
// лог манипуляций со славой
GloryLogType glory_log;

// * Загрузка лога славы.
void load_log()
{
	const char *glory_file = "../log/glory.log";
	std::ifstream file(glory_file);
	if (!file.is_open())
	{
		log("GloryLog: не удалось открыть файл на чтение: %s", glory_file);
		return;
	}

	std::string buffer;
	while (std::getline(file, buffer))
	{
		std::string buffer2;
		GetOneParam(buffer, buffer2);
		time_t time = 0;
		try
		{
			time = std::stoi(buffer2, nullptr, 10);
		}
		catch (const std::invalid_argument &)
		{
			log("GloryLog: ошибка чтения time, buffer2: %s", buffer2.c_str());
			return;
		}
		GetOneParam(buffer, buffer2);
		int type = 0;
		try
		{
			type = std::stoi(buffer2, nullptr, 10);
		}
		catch (const std::invalid_argument &)
		{
			log("GloryLog: ошибка чтения type, buffer2: %s", buffer2.c_str());
			return;
		}
		GetOneParam(buffer, buffer2);
		int num = 0;
		try
		{
			num = std::stoi(buffer2, nullptr, 10);
		}
		catch (const std::invalid_argument &)
		{
			log("GloryLog: ошибка чтения num, buffer2: %s", buffer2.c_str());
			return;
		}

		boost::trim(buffer);
		GloryLogPtr temp_node(new GloryLog);
		temp_node->type = type;
		temp_node->num = num;
		temp_node->karma = buffer;
		glory_log.insert(std::make_pair(time, temp_node));
	}
}

// * Сохранение лога славы.
void save_log()
{
	std::stringstream out;

	for (GloryLogType::const_iterator it = glory_log.begin(); it != glory_log.end(); ++it)
		out << it->first << " " << it->second->type << " " << it->second->num << " " << it->second->karma << "\n";

	const char *glory_file = "../log/glory.log";
	std::ofstream file(glory_file);
	if (!file.is_open())
	{
		log("GloryLog: не удалось открыть файл на запись: %s", glory_file);
		return;
	}
	file << out.rdbuf();
	file.close();
}

// * Добавление записи в лог славы (время, тип, кол-во, строка из кармы).
void add_log(int type, int num, std::string punish, std::string reason, CHAR_DATA *vict)
{
	if (!vict || vict->get_name().empty())
	{
		return;
	}

	GloryLogPtr temp_node(new GloryLog);
	temp_node->type = type;
	temp_node->num = num;
	std::stringstream out;
	out << GET_NAME(vict) << " : " << punish << " [" << reason << "]";
	temp_node->karma = out.str();
	glory_log.insert(std::make_pair(time(0), temp_node));
	save_log();
}

/**
* Показ лога славы (show glory), отсортированного по убыванию даты, с возможностью фильтрациии.
* Фильтры: show glory число|transfer|remove|hide
*/
void show_log(CHAR_DATA *ch , char const * const value)
{
	if (glory_log.empty())
	{
		send_to_char("Пусто, слава те господи!\r\n", ch);
		return;
	}

	int type = 0;
	int num = 0;
	std::string buffer;
	if (value && *value)
		buffer = value;
	if (CompareParam(buffer, "transfer"))
		type = TRANSFER_GLORY;
	else if (CompareParam(buffer, "remove"))
		type = REMOVE_GLORY;
	else if (CompareParam(buffer, "hide"))
		type = HIDE_GLORY;
	else
	{
		try
		{
			num = std::stoi(buffer, nullptr, 10);
		}
		catch (const std::invalid_argument &)
		{
			type = 0;
			num = 0;
		}
	}
	std::stringstream out;
	for (GloryLogType::reverse_iterator it = glory_log.rbegin(); it != glory_log.rend(); ++it)
	{
		if (type == it->second->type || (type == 0 && num == 0) || (type == 0 && num <= it->second->num))
		{
			char time_buf[17];
			strftime(time_buf, sizeof(time_buf), "%H:%M %d-%m-%Y", localtime(&it->first));
			out << time_buf << " " << it->second->karma << "\r\n";
		}
	}
	page_string(ch->desc, out.str());
}

// * Суммарное кол-во стартовых статов у чара (должно совпадать с SUM_ALL_STATS)
int start_stats_count(CHAR_DATA *ch)
{
	int count = 0;
	for (int i = 0; i < START_STATS_TOTAL; ++i)
	{
		count += ch->get_start_stat(i);
	}
	return count;
}

/**
* Стартовые статы при любых условиях должны соответствовать границам ролла.
* В случае старого ролла тут это всплывет из-за нулевых статов.
*/
bool bad_start_stats(CHAR_DATA *ch)
{
	if (ch->get_start_stat(G_STR) > MAX_STR(ch)
			|| ch->get_start_stat(G_STR) < MIN_STR(ch)
			|| ch->get_start_stat(G_DEX) > MAX_DEX(ch)
			|| ch->get_start_stat(G_DEX) < MIN_DEX(ch)
			|| ch->get_start_stat(G_INT) > MAX_INT(ch)
			|| ch->get_start_stat(G_INT) < MIN_INT(ch)
			|| ch->get_start_stat(G_WIS) > MAX_WIS(ch)
			|| ch->get_start_stat(G_WIS) < MIN_WIS(ch)
			|| ch->get_start_stat(G_CON) > MAX_CON(ch)
			|| ch->get_start_stat(G_CON) < MIN_CON(ch)
			|| ch->get_start_stat(G_CHA) > MAX_CHA(ch)
			|| ch->get_start_stat(G_CHA) < MIN_CHA(ch)
			|| start_stats_count(ch) != SUM_ALL_STATS)
	{
		return 1;
	}
	return 0;
}

/**
* Считаем реальные статы с учетом мортов и влитой славы.
* \return 0 - все ок, любое другое число - все плохо
*/
int bad_real_stats(CHAR_DATA *ch, int check)
{
	check -= SUM_ALL_STATS; // стартовые статы у всех по 95
	check -= 6 * GET_REMORT(ch); // реморты
	// влитая слава
	check -= Glory::get_spend_glory(ch);
	check -= GloryConst::main_stats_count(ch);
	return check;
}

/**
* Проверка стартовых и итоговых статов.
* Если невалидные стартовые статы - чар отправляется на реролл.
* Если невалидные только итоговые статы - идет перезапись со стартовых с учетом мортов и славы.
*/
bool check_stats(CHAR_DATA *ch)
{
	// иммов травмировать не стоит
	if (IS_IMMORTAL(ch))
	{
		return 1;
	}

	int have_stats = ch->get_inborn_str() + ch->get_inborn_dex() + ch->get_inborn_int() + ch->get_inborn_wis()
		+ ch->get_inborn_con() + ch->get_inborn_cha();

	// чар со старым роллом статов или после попыток поправить статы в файле
	if (bad_start_stats(ch))
	{
		snprintf(buf, MAX_STRING_LENGTH, "\r\n%sВаши параметры за вычетом перевоплощений:\r\n"
			"Сила: %d, Ловкость: %d, Ум: %d, Мудрость: %d, Телосложение: %d, Обаяние: %d\r\n"
			"Просим вас заново распределить основные параметры персонажа.%s\r\n",
			CCIGRN(ch, C_SPR),
			ch->get_inborn_str() - GET_REMORT(ch),
			ch->get_inborn_dex() - GET_REMORT(ch),
			ch->get_inborn_int() - GET_REMORT(ch),
			ch->get_inborn_wis() - GET_REMORT(ch),
			ch->get_inborn_con() - GET_REMORT(ch),
			ch->get_inborn_cha() - GET_REMORT(ch),
			CCNRM(ch, C_SPR));
		SEND_TO_Q(buf, ch->desc);

		// данную фигню мы делаем для того, чтобы из ролла нельзя было случайно так просто выйти
		// сразу, не раскидав статы, а то много любителей тригов и просто нажатий не глядя
		ch->set_str(MIN_STR(ch));
		ch->set_dex(MIN_DEX(ch));
		ch->set_int(MIN_INT(ch));
		ch->set_wis(MIN_WIS(ch));
		ch->set_con(MIN_CON(ch));
		ch->set_cha(MIN_CHA(ch));

		genchar_disp_menu(ch);
		STATE(ch->desc) = CON_RESET_STATS;
		return 0;
	}
	// стартовые статы в поряде, но слава не сходится (снялась по времени или иммом)
	if (bad_real_stats(ch, have_stats))
	{
		recalculate_stats(ch);
	}
	return 1;
}

// * Пересчет статов чара на основании стартовых статов, ремортов и славы.
void recalculate_stats(CHAR_DATA *ch)
{
	// стартовые статы
	ch->set_str(ch->get_start_stat(G_STR));
	ch->set_dex(ch->get_start_stat(G_DEX));
	ch->set_con(ch->get_start_stat(G_CON));
	ch->set_int(ch->get_start_stat(G_INT));
	ch->set_wis(ch->get_start_stat(G_WIS));
	ch->set_cha(ch->get_start_stat(G_CHA));
	// морты
	if (GET_REMORT(ch))
	{
		ch->inc_str(GET_REMORT(ch));
		ch->inc_dex(GET_REMORT(ch));
		ch->inc_con(GET_REMORT(ch));
		ch->inc_wis(GET_REMORT(ch));
		ch->inc_int(GET_REMORT(ch));
		ch->inc_cha(GET_REMORT(ch));
	}
	// влитая слава
	Glory::set_stats(ch);
	GloryConst::set_stats(ch);
}

} // namespace GloryMisc

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
