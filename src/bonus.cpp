// Copyright (c) 2016 bodrich
// Part of Bylins http://www.bylins.su

#include "bonus.h"

#include "bonus.command.parser.hpp"
#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "modify.h"
#include "chars/char_player.hpp"

#include <iostream>

namespace Bonus
{
	const size_t MAXIMUM_BONUS_RECORDS = 10;

	// время бонуса, в неактивном состоянии -1
	int time_bonus = -1;

	// множитель бонуса
	int mult_bonus = 2;

	// типа бонуса
	// 0 - оружейный
	// 1 - опыт
	// 2 - дамаг
	EBonusType type_bonus = BONUS_EXP;

	// история бонусов
	typedef std::list<std::string> bonus_log_t;
	bonus_log_t bonus_log;

	void do_bonus_info(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
	{
		show_log(ch);
	}

	void setup_bonus(const int duration, const int multilpier, EBonusType type)
	{
		time_bonus = duration;
		mult_bonus = multilpier;
		type_bonus = type;
	}

	void bonus_log_add(const std::string& name)
	{
		time_t nt = time(nullptr);
		std::stringstream ss;
		ss << rustime(localtime(&nt)) << " " << name;
		std::string buf = ss.str();
		boost::replace_all(buf, "\r\n", "");
		bonus_log.push_back(buf );

		std::ofstream fout("../log/bonus.log", std::ios_base::app);
		fout << buf << std::endl;
		fout.close();
	}

	class AbstractErrorReporter
	{
	public:
		using shared_ptr = std::shared_ptr<AbstractErrorReporter>;

		virtual ~AbstractErrorReporter() {}
		virtual void report(const std::string& message) = 0;
	};

	class CharacterReporter: public AbstractErrorReporter
	{
	public:
		CharacterReporter(CHAR_DATA* character) : m_character(character) {}

		virtual void report(const std::string& message) override;

		static shared_ptr create(CHAR_DATA* character) { return std::make_shared<CharacterReporter>(character); }

	private:
		CHAR_DATA* m_character;
	};

	void CharacterReporter::report(const std::string& message)
	{
		send_to_char(message.c_str(), m_character);
	}

	class MudlogReporter : public AbstractErrorReporter
	{
	public:
		virtual void report(const std::string& message) override;

		static shared_ptr create() { return std::make_shared<MudlogReporter>(); }
	};

	void MudlogReporter::report(const std::string& message)
	{
		mudlog(message.c_str(), DEF, LVL_BUILDER, ERRLOG, TRUE);
	}

	void do_bonus(const AbstractErrorReporter::shared_ptr& reporter, const char *argument)
	{
		ArgumentsParser bonus(argument, type_bonus, time_bonus);

		bonus.parse();

		const auto result = bonus.result();
		switch (result)
		{
		case ArgumentsParser::ER_ERROR:
			reporter->report(bonus.error_message().c_str());
			break;

		case ArgumentsParser::ER_START:
			switch (bonus.type())
			{
			case BONUS_DAMAGE:
				reporter->report("Режим бонуса \"урон\" в настоящее время отключен.");
				break;

			default:
				{
					const std::string& message = bonus.broadcast_message();
					send_to_all(message.c_str());
					std::stringstream ss;
					ss << "&W" << message << "&n\r\n";
					bonus_log_add(ss.str());
					setup_bonus(bonus.time(), bonus.multiplier(), bonus.type());
				}
			}
			break;

		case ArgumentsParser::ER_STOP:
			send_to_all(bonus.broadcast_message().c_str());
			time_bonus = -1;
			break;
		}
	}

	void do_bonus_by_character(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
	{
		const auto reporter = CharacterReporter::create(ch);
		do_bonus(reporter, argument);
	}

	// вызывает функцию do_bonus
	void dg_do_bonus(char *cmd)
	{
		const auto reporter = MudlogReporter::create();
		do_bonus(reporter, cmd);
	}

	// записывает в буффер сколько осталось до конца бонуса
	std::string bonus_end()
	{
		std::stringstream ss;
		if (time_bonus > 4)
		{
			ss << "До конца бонуса осталось " << time_bonus << " часов.";
		}
		else if (time_bonus == 4)
		{
			ss << "До конца бонуса осталось четыре часа.";
		}
		else if (time_bonus == 3)
		{
			ss << "До конца бонуса осталось три часа.";
		}
		else if (time_bonus == 2)
		{
			ss << "До конца бонуса осталось два часа.";
		}
		else if (time_bonus == 1)
		{
			ss << "До конца бонуса остался последний час!";
		}
		else
		{
			ss << "Бонуса нет.";
		}
		return ss.str();
	}

	// Записывает в буфер тип бонуса
	std::string str_type_bonus()
	{
		switch (type_bonus)
		{
		case BONUS_DAMAGE:
			return "Сейчас идет бонус: повышенный урон.";

		case BONUS_EXP:
			return "Сейчас идет бонус: повышенный опыт за убийство моба.";

		case BONUS_WEAPON_EXP:
			return "Сейчас идет бонус: повышенный опыт от урона оружия.";

		default:
			return "";
		}
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
		std::string bonus_str = "&W" + bonus_end() + "&n\r\n";
		send_to_all(bonus_str.c_str());
	}

	// проверка на тип бонуса
	bool is_bonus(int type)
	{
	    if ((type == 0 || type == type_bonus) && time_bonus > -1)
        {
	        return true;
        }

		return false;
	}

	// загружает лог бонуса из файла
	void bonus_log_load()
	{
		std::ifstream fin("../log/bonus.log");
		if (!fin.is_open())
		{
			return;
		}
		std::string temp_buf;
		bonus_log.clear();
		while (std::getline(fin, temp_buf))
		{
			bonus_log.push_back(temp_buf);
		}
		fin.close();
	}

	// выводит весь лог в обратном порядке
	void show_log(CHAR_DATA *ch)
	{
		if (bonus_log.empty())
		{
			send_to_char(ch, "Лог пустой!\r\n");
			return;
		}

		std::stringstream buf_str;

		for (auto [it, count] = std::tuple(bonus_log.rbegin(), 0ul); it != bonus_log.rend() && count <= MAXIMUM_BONUS_RECORDS; ++it, ++count)
		{
            buf_str << "&G" << count << ". &W" << *it << "&n\r\n";
		}

		page_string(ch->desc, buf_str.str());
	}

	// возвращает множитель бонуса
	int get_mult_bonus()
	{
		return mult_bonus;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
