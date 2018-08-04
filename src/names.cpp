/* ************************************************************************
*   File: names.cpp                                     Part of Bylins    *
*  Usage: saving\checked names , agreement and disagreement               *
*                                                                         *
*  17.06.2003 - made by Alez                                              *
* 									  *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "logger.hpp"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "screen.h"
#include "privilege.hpp"
#include "char.hpp"
#include "char_player.hpp"

#include <boost/algorithm/string.hpp>

#include <string>
#include <map>
#include <fstream>
#include <sstream>

extern const char *genders[];
extern void send_to_gods(char *text, bool demigod);

static const char *god_text = "Богом";
static const char *player_text = "привилегированным игроком";

const char * print_god_or_player(int level)
{
	return level < LVL_IMMORT ? player_text : god_text;
}

// Check if name agree (name must be parsed)
int was_agree_name(DESCRIPTOR_DATA * d)
{
	log("was_agree_name start");
	FILE *fp;
	char temp[MAX_INPUT_LENGTH];
	char immname[MAX_INPUT_LENGTH];
	char mortname[6][MAX_INPUT_LENGTH];
	int immlev;
	int sex;
	int i;

//1. Load list

	if (!(fp = fopen(ANAME_FILE, "r")))
	{
		perror("SYSERR: Unable to open '" ANAME_FILE "' for reading");
	log("was_agree_name end");
		return (1);
	}
//2. Find name in list ...
	while (get_line(fp, temp))
	{
		// Format First name/pad1/pad2/pad3/pad4/pad5/sex immname immlev
		sscanf(temp, "%s %s %s %s %s %s %d %s %d", mortname[0],
			   mortname[1], mortname[2], mortname[3], mortname[4], mortname[5], &sex, immname, &immlev);
		if (!strcmp(mortname[0], GET_NAME(d->character)))
		{
			// We find char ...
			for (i = 1; i < 6; i++)
			{
				d->character->player_data.PNames[i] = std::string(mortname[i]);
			}
			GET_SEX(d->character) = static_cast<ESex>(sex);
			// Auto-Agree char ...
			NAME_GOD(d->character) = immlev + 1000;
			NAME_ID_GOD(d->character) = get_id_by_name(immname);
			sprintf(buf, "\r\nВаше имя одобрено!\r\n");
			SEND_TO_Q(buf, d);
			sprintf(buf, "AUTOAGREE: %s was agreed by %s", GET_PC_NAME(d->character), immname);
			log(buf, d);
			fclose(fp);
	log("was_agree_name end");
			return (0);
		}
	}
	fclose(fp);
	log("was_agree_name end");
	return (1);
}

int was_disagree_name(DESCRIPTOR_DATA * d)
{
	log("was_disagree_name start");
	FILE *fp;
	char temp[MAX_INPUT_LENGTH];
	char mortname[MAX_INPUT_LENGTH];
	char immname[MAX_INPUT_LENGTH];
	int immlev;

	if (!(fp = fopen(DNAME_FILE, "r")))
	{
		perror("SYSERR: Unable to open '" DNAME_FILE "' for reading");
	log("was_disagree_name end");
		return (1);
	}
	//1. Load list
	//2. Find name in list ...
	//3. Compare names ... get next
	while (get_line(fp, temp))
	{
		// Extract chars and
		sscanf(temp, "%s %s %d", mortname, immname, &immlev);
		if (!strcmp(mortname, GET_NAME(d->character)))
		{
			// Char found all ok;

			sprintf(buf, "\r\nВаше имя запрещено!\r\n");
			SEND_TO_Q(buf, d);
			sprintf(buf, "AUTOAGREE: %s was disagreed by %s", GET_PC_NAME(d->character), immname);
			log(buf, d);

			fclose(fp);
	log("was_disagree_name end");
			return (0);
		};
	}
	fclose(fp);
	log("was_disagree_name end");
	return (1);
}

void rm_agree_name(CHAR_DATA * d)
{
	FILE *fin;
	FILE *fout;
	char temp[MAX_INPUT_LENGTH];
	char immname[MAX_INPUT_LENGTH];
	char mortname[6][MAX_INPUT_LENGTH];
	int immlev;
	int sex;

	// 1. Find name ...
	if (!(fin = fopen(ANAME_FILE, "r")))
	{
		perror("SYSERR: Unable to open '" ANAME_FILE "' for read");
		return;
	}
	if (!(fout = fopen("" ANAME_FILE ".tmp", "w")))
	{
		perror("SYSERR: Unable to open '" ANAME_FILE ".tmp' for writing");
		fclose(fin);
		return;
	}
	while (get_line(fin, temp))
	{
		// Get name ...
		sscanf(temp, "%s %s %s %s %s %s %d %s %d", mortname[0],
			   mortname[1], mortname[2], mortname[3], mortname[4], mortname[5], &sex, immname, &immlev);
		if (strcmp(mortname[0], GET_NAME(d)))
		{
			// Name un matches ... do copy ...
			fprintf(fout, "%s\n", temp);
		};
	}

	fclose(fin);
	fclose(fout);
	// Rewrite from tmp
	rename(ANAME_FILE ".tmp", ANAME_FILE);
	return;
}

// список неодобренных имен, дубль2
// на этот раз ничего перебирать не будем, держа все в памяти и обновляя по необходимости

struct NewName
{
	std::string name0; // падежи
	std::string name1; // --//--
	std::string name2; // --//--
	std::string name3; // --//--
	std::string name4; // --//--
	std::string name5; // --//--
	std::string email; // мыло
	ESex sex;         // часто не ясно, для какоо пола падежи вообще
};

typedef std::shared_ptr<NewName> NewNamePtr;
typedef std::map<std::string, NewNamePtr> NewNameListType;

static NewNameListType NewNameList;

// сохранение списка в файл
void NewNameSave()
{
	std::ofstream file(NNAME_FILE);
	if (!file.is_open())
	{
		log("Error open file: %s! (%s %s %d)", NNAME_FILE, __FILE__, __func__, __LINE__);
		return;
	}

	for (NewNameListType::const_iterator it = NewNameList.begin(); it != NewNameList.end(); ++it)
		file << it->first << "\n";

	file.close();
}

// добавление имени в список неодобренных для показа иммам
// флажок для более удобного лоада без перезаписи файла
void NewNameAdd(CHAR_DATA * ch, bool save = 1)
{
	NewNamePtr name(new NewName);

	name->name0 = ch->get_name();
	name->name1 = GET_PAD(ch, 1);
	name->name2 = GET_PAD(ch, 2);
	name->name3 = GET_PAD(ch, 3);
	name->name4 = GET_PAD(ch, 4);
	name->name5 = GET_PAD(ch, 5);
	name->email = GET_EMAIL(ch);
	name->sex = GET_SEX(ch);

	NewNameList[GET_NAME(ch)] = name;
	if (save)
	{
		NewNameSave();
	}
}

// поиск/удаление персонажа из списка неодобренных имен
void NewNameRemove(CHAR_DATA * ch)
{
	NewNameListType::iterator it;
	it = NewNameList.find(GET_NAME(ch));
	if (it != NewNameList.end())
		NewNameList.erase(it);
	NewNameSave();
}

// для удаление через команду имма
void NewNameRemove(const std::string& name, CHAR_DATA * ch)
{
	NewNameListType::iterator it = NewNameList.find(name);
	if (it != NewNameList.end())
	{
		NewNameList.erase(it);
		send_to_char("Запись удалена.\r\n", ch);
	}
	else
		send_to_char("В списке нет такого имени.\r\n", ch);

	NewNameSave();
}

// лоад списка неодобренных имен
void NewNameLoad()
{
	std::ifstream file(NNAME_FILE);
	if (!file.is_open())
	{
		log("Error open file: %s! (%s %s %d)", NNAME_FILE, __FILE__, __func__, __LINE__);
		return;
	}

	std::string buffer;
	while (file >> buffer)
	{
		// сразу проверяем не сделетился ли уже персонаж
		Player t_tch;
		Player *tch = &t_tch;
		if (load_char(buffer.c_str(), tch) < 0)
			continue;
		// не сделетился...
		NewNameAdd(tch, 0);
	}

	file.close();
	NewNameSave();
}

// вывод списка неодобренных имму
void NewNameShow(CHAR_DATA * ch)
{
	if (NewNameList.empty()) return;

	std::ostringstream buffer;
	buffer << "\r\nИгроки, ждущие одобрения имени (имя <игрок> одобрить/запретить/удалить):\r\n" << CCWHT(ch, C_NRM);
	for (NewNameListType::const_iterator it = NewNameList.begin(); it != NewNameList.end(); ++it)
	{
		const size_t sex = static_cast<size_t>(to_underlying(it->second->sex));
		buffer << "Имя: " << it->first << " " << it->second->name0 << "/" << it->second->name1
		<< "/" << it->second->name2 << "/" << it->second->name3 << "/" << it->second->name4
		<< "/" << it->second->name5 << " Email: &S" << (GET_GOD_FLAG(ch, GF_DEMIGOD) ? "неопределен" : it->second->email) 
		<< "&s Пол: " << genders[sex] << "\r\n";
	}
	buffer << CCNRM(ch, C_NRM);
	send_to_char(buffer.str(), ch);
}

int process_auto_agreement(DESCRIPTOR_DATA * d)
{
	// Check for name ...
	if (!was_agree_name(d))
		return 0;
	else if (!was_disagree_name(d))
		return 1;

	return 2;
}

void rm_disagree_name(CHAR_DATA * d)
{
	FILE *fin;
	FILE *fout;
	char temp[256];
	char immname[256];
	char mortname[256];
	int immlev;

	// 1. Find name ...
	if (!(fin = fopen(DNAME_FILE, "r")))
	{
		perror("SYSERR: Unable to open '" DNAME_FILE "' for read");
		return;
	}
	if (!(fout = fopen("" DNAME_FILE ".tmp", "w")))
	{
		perror("SYSERR: Unable to open '" DNAME_FILE ".tmp' for writing");
		fclose(fin);
		return;
	}
	while (get_line(fin, temp))
	{
		// Get name ...
		sscanf(temp, "%s %s %d", mortname, immname, &immlev);
		if (strcmp(mortname, GET_NAME(d)))
		{
			// Name un matches ... do copy ...
			fprintf(fout, "%s\n", temp);
		};
	}
	fclose(fin);
	fclose(fout);
	rename(DNAME_FILE ".tmp", DNAME_FILE);
	return;

}

void add_agree_name(CHAR_DATA * d, const char *immname, int immlev)
{
	FILE *fl;
	if (!(fl = fopen(ANAME_FILE, "a")))
	{
		perror("SYSERR: Unable to open '" ANAME_FILE "' for writing");
		return;
	}
	// Pos to the end ...
	fprintf(fl, "%s %s %s %s %s %s %d %s %d\r\n", GET_NAME(d),
			GET_PAD(d, 1), GET_PAD(d, 2), GET_PAD(d, 3), GET_PAD(d, 4), GET_PAD(d, 5), static_cast<int>(GET_SEX(d)), immname, immlev);
	fclose(fl);
	return;
}

void add_disagree_name(CHAR_DATA * d, const char *immname, int immlev)
{
	FILE *fl;
	if (!(fl = fopen(DNAME_FILE, "a")))
	{
		perror("SYSERR: Unable to open '" DNAME_FILE "' for writing");
		return;
	}
	// Pos to the end ...
	fprintf(fl, "%s %s %d\r\n", GET_NAME(d), immname, immlev);
	fclose(fl);
	return;
}

void disagree_name(CHAR_DATA * d, const char *immname, int immlev)
{
	// Clean record from agreed if present ...
	rm_agree_name(d);
	rm_disagree_name(d);
	NewNameRemove(d);
	// Add record to disagreed if not present ...
	add_disagree_name(d, immname, immlev);
}

void agree_name(CHAR_DATA * d, const char *immname, int immlev)
{
	// Clean record from disgreed if present ...
	rm_agree_name(d);
	rm_disagree_name(d);
	NewNameRemove(d);
	// Add record to agreed if not present ...
	add_agree_name(d, immname, immlev);
}

enum { NAME_AGREE, NAME_DISAGREE, NAME_DELETE };

void go_name(CHAR_DATA* ch, CHAR_DATA* vict, int action)
{
	int god_level = PRF_FLAGGED(ch, PRF_CODERINFO) ? LVL_IMPL : GET_LEVEL(ch);

	if (GET_LEVEL(vict) > god_level)
	{
		send_to_char("А он ведь старше вас...\r\n", ch);
		return;
	}

	// одобряем или нет
	int lev = NAME_GOD(vict);
	if (lev > 1000)
		lev = lev - 1000;
	if (lev > god_level)
	{
		send_to_char("Об этом имени уже позаботился бог старше вас.\r\n", ch);
		return;
	}

	if (lev == god_level)
		if (NAME_ID_GOD(vict) != GET_IDNUM(ch))
			send_to_char("Об этом имени уже позаботился другой бог вашего уровня.\r\n", ch);

	if (action == NAME_AGREE)
	{
		NAME_GOD(vict) = god_level + 1000;
		NAME_ID_GOD(vict) = GET_IDNUM(ch);
		//send_to_char("Имя одобрено!\r\n", ch);
		send_to_char(vict, "&GВаше имя одобрено!&n\r\n");
		agree_name(vict, GET_NAME(ch), god_level);
		switch (GET_SEX(ch))
		{
		case ESex::SEX_NEUTRAL:
			sprintf(buf, "&c%s одобрило имя игрока %s.&n\r\n", GET_NAME(ch), GET_NAME(vict));
			break;

		case ESex::SEX_MALE:
			sprintf(buf, "&c%s одобрил имя игрока %s.&n\r\n", GET_NAME(ch), GET_NAME(vict));
			break;

		case ESex::SEX_FEMALE:
			sprintf(buf, "&c%s одобрила имя игрока %s.&n\r\n", GET_NAME(ch), GET_NAME(vict));
			break;

		case ESex::SEX_POLY:
			sprintf(buf, "&c%s одобрили имя игрока %s.&n\r\n", GET_NAME(ch), GET_NAME(vict));
			break;
		}

		send_to_gods(buf, true);
		// В этом теперь нет смысла
		//mudlog(buf, CMP, LVL_GOD, SYSLOG, TRUE);

	}
	else
	{
		NAME_GOD(vict) = god_level;
		NAME_ID_GOD(vict) = GET_IDNUM(ch);
		//send_to_char("Имя запрещено!\r\n", ch);
		send_to_char(vict, "&RВаше имя запрещено!&n\r\n");
		disagree_name(vict, GET_NAME(ch), god_level);

		switch (GET_SEX(ch))
		{
		case ESex::SEX_NEUTRAL:
			sprintf(buf, "&c%s запретило имя игрока %s.&n\r\n", GET_NAME(ch), GET_NAME(vict));
			break;

		case ESex::SEX_MALE:
			sprintf(buf, "&c%s запретил имя игрока %s.&n\r\n", GET_NAME(ch), GET_NAME(vict));
			break;

		case ESex::SEX_FEMALE:
			sprintf(buf, "&c%s запретила имя игрока %s.&n\r\n", GET_NAME(ch), GET_NAME(vict));
			break;

		case ESex::SEX_POLY:
			sprintf(buf, "&c%s запретили имя игрока %s.&n\r\n", GET_NAME(ch), GET_NAME(vict));
			break;
		}

		send_to_gods(buf, true);
		//mudlog(buf, CMP, LVL_GOD, SYSLOG, TRUE);

	}

}

const char* MORTAL_DO_TITLE_FORMAT = "\r\n"
									 "имя - вывод списка имен, ждущих одобрения, если они есть\r\n"
									 "имя <игрок> одобрить - одобрить имя данного игрока\r\n"
									 "имя <игрок> запретить - запретить имя данного игрока\r\n"
									 "имя <игрок> удалить - удалить данное имя из списка без запрета или одобрения\r\n";

void do_name(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	std::string name, command = argument;
	GetOneParam(command, name);

	if (name.empty())
	{
		if (!NewNameList.empty())
			NewNameShow(ch);
		else
			send_to_char(MORTAL_DO_TITLE_FORMAT, ch);
		return;
	}

	boost::trim(command);
	int	action = -1;
	if (CompareParam(command, "одобрить"))
		action = NAME_AGREE;
	else if (CompareParam(command, "запретить"))
		action = NAME_DISAGREE;
	else if (CompareParam(command, "удалить"))
		action = NAME_DELETE;

	if (action < 0)
	{
		send_to_char(MORTAL_DO_TITLE_FORMAT, ch);
		return;
	}

	name_convert(name);
	if (action == NAME_DELETE)
	{
		NewNameRemove(name, ch);
		return;
	}

	CHAR_DATA* vict;
	if ((vict = get_player_vis(ch, name, FIND_CHAR_WORLD)) != NULL)
	{
		if (!(vict = get_player_pun(ch, name, FIND_CHAR_WORLD)))
		{
			send_to_char("Нет такого игрока.\r\n", ch);
			return;
		}
		go_name(ch, vict, action);
	}
	else
	{
		vict = new Player; // TODO: переделать на стек
		if (load_char(name.c_str(), vict) < 0)
		{
			send_to_char("Такого персонажа не существует.\r\n", ch);
			delete vict;
			return;
		}
		go_name(ch, vict, action);
		vict->save_char();
		delete vict;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
