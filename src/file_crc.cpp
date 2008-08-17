// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#include <fstream>
#include <string>
#include <sstream>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/crc.hpp>
#include <boost/cstdint.hpp>
#include "conf.h"
#include "file_crc.hpp"
#include "db.h"
#include "utils.h"
#include "interpreter.h"
#include "comm.h"

namespace FileCRC
{

class PlayerCRC
{
public:
	std::string name; // имя игрока
	boost::uint32_t player; // crc .player
	// TODO: обж, таймеры, отдельно мб кланы
};

typedef boost::shared_ptr<PlayerCRC> CRCListPtr;
typedef std::map<long, CRCListPtr> CRCListType;
const char CRC_UID = '*';

CRCListType crc_list;
// лог иммам
std::string message;
// флажок необходимости записи файла чексумм
bool need_save = false;

/**
* Вывод сообщения в сислог, иммлог и лог show crc.
*/
void add_message(const char *text, ...)
{
	if (!text) return;
	va_list args;
	char out[MAX_STRING_LENGTH];

	va_start(args, text);
	vsprintf(out, text, args);
	va_end(args);

	mudlog(out, DEF, LVL_IMMORT, SYSLOG, TRUE);
	message += out + std::string("\r\n");
}

/**
* Подсчет crc для строки.
*/
boost::uint32_t calculate_str_crc(const std::string &text)
{
	boost::crc_32_type crc;
	crc = std::for_each(text.begin(), text.end(), crc);
	return crc();
}

/**
* Подсчет crc для файла.
*/
boost::uint32_t calculate_file_crc(const char *name)
{
	std::ifstream in(name, std::ios::binary);
	std::ostringstream t_out;
	t_out << in.rdbuf();
	std::string out;
	t_out.str().swap(out);
	return calculate_str_crc(out);
}

/**
* Загрузка глобального списка crc.
*/
void load()
{
	const char *file_name = LIB_PLRSTUFF"crc.lst";
	std::ifstream file(file_name);
	if (!file.is_open())
	{
		add_message("SYSERROR: не удалось открыть файл на чтение: %s", file_name);
		return;
	}

	std::string buffer;
	std::getline(file, buffer, CRC_UID);
	std::stringstream stream(buffer);

	long uid;
	while (stream >> uid)
	{
		if (!(stream >> buffer))
		{
			add_message("SYSERROR: не удалось прочитать файл: %s, last uid: %ld", file_name, uid);
			return;
		}

		std::string name = GetNameByUnique(uid);
		if (name.empty())
		{
			log("FileCRC: UID %ld - персонажа не существует.", uid);
			continue;
		}
		CRCListPtr tmp_crc(new PlayerCRC);
		tmp_crc->name = name;
		try
		{
			tmp_crc->player = boost::lexical_cast<boost::uint32_t>(buffer);
		}
		catch (boost::bad_lexical_cast &)
		{
			add_message("FileCrc: ошибка чтения crc (%s), uid: %ld", buffer.c_str(), uid);
			break;
		}
		crc_list[uid] = tmp_crc;
	}

	bool checked = false;
	if ((file >> buffer))
	{
		boost::uint32_t prev_crc;
		try
		{
			prev_crc = boost::lexical_cast<boost::uint32_t>(buffer);
		}
		catch (boost::bad_lexical_cast &)
		{
			add_message("SYSERROR: ошибка чтения total crc (%s)", buffer.c_str());
			return;
		}
		stream.clear();
		const boost::uint32_t crc = calculate_str_crc(stream.str());
		if (crc != prev_crc)
		{
			add_message("SYSERROR: несовпадение контрольной суммы файла: %s", file_name);
			return;
		}
		checked = true;
	}

	if (!checked)
		add_message("SYSERROR: не произведена сверка контрольной суммы файла: %s", file_name);
}

/**
* Запись глобального списка чек-сумм (дергается раз в секунду для проверки необходимости сейва).
* \param force_save - запись файла в любом случае (при шатдауне мада), по умолчанию false.
*/
void save(bool force_save)
{
	if (!need_save && !force_save) return;

	need_save = false;
	const char *file_name = LIB_PLRSTUFF"crc.backup";
	std::ofstream file(file_name);
	if (!file.is_open())
	{
		add_message("SYSERROR: не удалось открыть файл на запись: %s", file_name);
		return;
	}

	for (CRCListType::const_iterator it = crc_list.begin(); it != crc_list.end(); ++it)
		file << it->first << " " << it->second->player << "\r\n";
	file.close();

	const boost::uint32_t crc = calculate_file_crc(file_name);
	std::ofstream out(file_name, std::ios_base::app);
	if (!out.is_open())
	{
		add_message("SYSERROR: не удалось открыть файл на дозапись crc: %s", file_name);
		return;
	}
	out << CRC_UID << " " << crc << "\r\n";
	out.close();

	std::string buffer("cp "LIB_PLRSTUFF"crc.backup "LIB_PLRSTUFF"crc.lst");
	system(buffer.c_str());
}

/**
* Проверка crc файлоа плеера.
* \param mode - PLAYER для просто сверки, UPDATE_PLAYER - для сверки с перезаписью нового crc.
*/
void check_crc(const char *filename, int mode, long uid)
{
	CRCListType::const_iterator it = crc_list.find(uid);
	if (it != crc_list.end())
	{
		switch (mode)
		{
		case PLAYER:
		{
			const boost::uint32_t crc = calculate_file_crc(filename);
			if (it->second->player != crc)
			{
				char time_buf[20];
				time_t ct = time(0);
				strftime(time_buf, sizeof(time_buf), "%d-%m-%y %H:%M:%S", localtime(&ct));
				add_message("%s Несовпадение контрольной суммы player файла: %s", time_buf, it->second->name.c_str());
			}
			break;
		}
		case TEXTOBJS:
		case TIMEOBJS:
		case UPDATE_PLAYER:
			it->second->player = calculate_file_crc(filename);
			it->second->name = GetNameByUnique(uid);
			break;
		default:
			add_message("SYSERROR: мы не должны были сюда попасть, uid: %ld, mode: %d, func: %s",
						uid, mode, __func__);
			return;
		}
	}
	else if (mode == PLAYER || mode == UPDATE_PLAYER)
	{
		CRCListPtr tmp_crc(new PlayerCRC);
		tmp_crc->name = GetNameByUnique(uid);
		tmp_crc->player = calculate_file_crc(filename);
		crc_list[uid] = tmp_crc;
	}

	if (mode == UPDATE_PLAYER)
		need_save = true;
}

/**
* Вывод лога событий имму по show crc.
*/
void show(CHAR_DATA *ch)
{
	if (message.empty())
		send_to_char("Вроде ничего не происходило...\r\n", ch);
	else
		send_to_char(message.c_str(), ch);
}

} // namespace FileCRC
