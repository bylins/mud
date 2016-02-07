// Copyright (c) 2016 bodrich
// Part of Bylins http://www.bylins.su

#include "bonus.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "modify.h"
#include "char.hpp"
#include "char_player.hpp"

#include <iostream>
#include <fstream>
#include <boost/lexical_cast.hpp>

namespace Bonus
{
	// время бонуса, в неактивном состоянии -1
	int time_bonus = -1;
	// множитель бонуса
	int mult_bonus = 2;
	// типа бонуса
	// 0 - оружейный
	// 1 - опыт
	// 2 - дамаг
	int type_bonus = BONUS_EXP;;
	// история бонусов
	std::vector<std::string> bonus_log;


	ACMD(do_bonus_info)
	{
		show_log(ch);
	}

	ACMD(do_bonus)
	{
		argument = two_arguments(argument, buf, buf2);
		std::string out = "&W*** Объявляется ";

		if (!isname(buf, "двойной тройной отменить"))
		{
			send_to_char("Синтаксис команды:\r\nбонус <двойной|тройной|отменить> [оружейный|опыт] [время]\r\n", ch);
			return;
		}
		if (is_abbrev(buf, "отменить"))
		{
			sprintf(buf, "Бонус был отменен.\r\n");
			send_to_all(buf);
			time_bonus = -1;
			return;
		}
		if (!*buf || !*buf2 || !a_isascii(*buf2))
		{
			send_to_char("Синтаксис команды:\r\nбонус <двойной|тройной|отменить> [оружейный|опыт|урон] [время]\r\n", ch);
			return;
		}
		if (!isname(buf2, "оружейный опыт урон"))
		{
			send_to_char("Тип бонуса может быть &Wоружейный&n, &Wопыт&n или &Wурон&n.\r\n", ch);
			return;
		}
		if (*argument) time_bonus = atol(argument);

		if ((time_bonus < 1) || (time_bonus > 60))
		{
			send_to_char("Возможный временной интервал: от 1 до 60 игровых часов.\r\n", ch);
			return;
		}
		if (is_abbrev(buf, "двойной"))
		{
			out += "двойной бонус ";
			mult_bonus = 2;
		}
		else if (is_abbrev(buf, "тройной"))
		{
			out += "тройной бонус ";
			mult_bonus = 3;
		}
		else
		{
			return;
		}
		if (is_abbrev(buf2, "оружейный"))
		{
			out += "оружейного опыта ";
			type_bonus = 0;
		}
		else if (is_abbrev(buf2, "опыт"))
		{
			out += "опыта ";
			type_bonus = 1;
		}
		else if (is_abbrev(buf2, "уронт"))
		{
			out += "увеличенного урона";
			type_bonus = 2;
		}
		else
		{
			return;
		}
		out += "на " + boost::lexical_cast<string>(time_bonus) + " часов. ***&n\r\n";
		bonus_log_add(out);
		send_to_all(out.c_str());
	}

	// таймер бонуса
	void timer_bonus()
	{
		if (time_bonus <= -1)
		{
			return;
		}
		time_bonus--;
		if (time_bonus < 1)
		{
			send_to_all("&WБонус закончился...&n\r\n");
			time_bonus = -1;
			return;
		}
		if (time_bonus > 4)
			sprintf(buf, "&WДо конца бонуса осталось %d часов.&n\r\n", time_bonus);
		else if (time_bonus == 4)
			sprintf(buf, "&WДо конца бонуса осталось четыре часа.&n\r\n");
		else if (time_bonus == 3)
			sprintf(buf, "&WДо конца бонуса осталось три часа.&n\r\n");
		else if (time_bonus == 2)
			sprintf(buf, "&WДо конца бонуса осталось два часа.&n\r\n");
		else
			sprintf(buf, "&WДо конца бонуса остался последний час!&n\r\n");
		send_to_all(buf);
	}

	// проверка на тип бонуса
	bool is_bonus(int type)
	{
		if (time_bonus <= -1) return false;
		if (type == type_bonus) return true;
		return false;
	}

	// добавляет строку в лог
	void bonus_log_add(std::string name)
	{
		time_t nt = time(NULL);
		bonus_log.push_back(std::string(rustime(localtime(&nt))) + " " + name);
		std::ofstream fout("../log/bonus.log", std::ios_base::app);
		fout << std::string(rustime(localtime(&nt))) + " " + name;
		fout.close();
	}

	// загружает лог бонуса из файла
	void bonus_log_load()
	{
		std::ifstream fin("../log/bonus.log");
		if (!fin.is_open())
			return;
		std::string temp_buf;
		while (std::getline(fin, temp_buf))
			bonus_log.push_back(temp_buf);
		fin.close();
	}

	// выводит весь лог в обратном порядке
	void show_log(CHAR_DATA *ch)
	{
		if (bonus_log.size() == 0)
		{
			send_to_char(ch, "Лог пустой!\r\n");
			return;
		}
		std::string buf_str = "";
		for (size_t i = bonus_log.size(); i >= 0; i--)
		{
			buf_str += bonus_log[i];
		}
		page_string(ch->desc, buf_str);
	}

	// возвращает множитель бонуса
	int get_mult_bonus()
	{
		return mult_bonus;
	}
	


}