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

#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <boost/shared_ptr.hpp>

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "screen.h"

extern const char *genders[];

// Check if name agree (name must be parsed)
int was_agree_name(DESCRIPTOR_DATA * d)
{
	FILE *fp;
	char temp[MAX_INPUT_LENGTH];
	char immname[MAX_INPUT_LENGTH];
	char mortname[6][MAX_INPUT_LENGTH];
	int immlev;
	int i;

//1. Load list

	if (!(fp = fopen(ANAME_FILE, "r"))) {
		perror("SYSERR: Unable to open '" ANAME_FILE "' for reading");
		return (1);
	}
//2. Find name in list ...
	while (get_line(fp, temp)) {
		// Format First name/pad1/pad2/pad3/pad4/pad5/ immname immlev
		sscanf(temp, "%s %s %s %s %s %s %s %d", mortname[0],
		       mortname[1], mortname[2], mortname[3], mortname[4], mortname[5], immname, &immlev);
		if (!strcmp(mortname[0], GET_NAME(d->character))) {
			// We find char ...
			for (i = 1; i < 6; i++) {
				GET_PAD(d->character, i) = (char *) malloc(strlen(mortname[i]));
				strcpy(GET_PAD(d->character, i), mortname[i]);
			}
			// Auto-Agree char ...
			NAME_GOD(d->character) = immlev + 1000;
			NAME_ID_GOD(d->character) = get_id_by_name(immname);
			sprintf(buf, "\r\nВаше имя одобрено Богом %s!!!\r\n", immname);
			SEND_TO_Q(buf, d);
			sprintf(buf, "AUTOAGREE: %s was agreed by %s", GET_PC_NAME(d->character), immname);
			log(buf, d);

			fclose(fp);
			return (0);
		}
	}
	fclose(fp);
	return (1);
}

int was_disagree_name(DESCRIPTOR_DATA * d)
{
	FILE *fp;
	char temp[MAX_INPUT_LENGTH];
	char mortname[MAX_INPUT_LENGTH];
	char immname[MAX_INPUT_LENGTH];
	int immlev;

	if (!(fp = fopen(DNAME_FILE, "r"))) {
		perror("SYSERR: Unable to open '" DNAME_FILE "' for reading");
		return (1);
	}
	//1. Load list
	//2. Find name in list ...
	//3. Compare names ... get next
	while (get_line(fp, temp)) {
		// Extract chars and
		sscanf(temp, "%s %s %d", mortname, immname, &immlev);
		if (!strcmp(mortname, GET_NAME(d->character))) {
			// Char found all ok;

			sprintf(buf, "\r\nВаше имя запрещено Богом %s!!!\r\n", immname);
			SEND_TO_Q(buf, d);
			sprintf(buf, "AUTOAGREE: %s was disagreed by %s", GET_PC_NAME(d->character), immname);
			log(buf, d);

			fclose(fp);
			return (0);
		};
	}
	fclose(fp);
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

	// 1. Find name ...
	if (!(fin = fopen(ANAME_FILE, "r"))) {
		perror("SYSERR: Unable to open '" ANAME_FILE "' for read");
		return;
	}
	if (!(fout = fopen("" ANAME_FILE ".tmp", "w"))) {
		perror("SYSERR: Unable to open '" ANAME_FILE ".tmp' for writing");
		fclose(fin);
		return;
	}
	while (get_line(fin, temp)) {
		// Get name ...
		sscanf(temp, "%s %s %s %s %s %s %s %d", mortname[0],
		       mortname[1], mortname[2], mortname[3], mortname[4], mortname[5], immname, &immlev);
		if (strcmp(mortname[0], GET_NAME(d))) {
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

struct NewName {
	std::string name0; // падежи
	std::string name1; // --//--
	std::string name2; // --//--
	std::string name3; // --//--
	std::string name4; // --//--
	std::string name5; // --//--
	std::string email; // мыло
	short sex;         // часто не ясно, для какоо пола падежи вообще
};

typedef boost::shared_ptr<NewName> NewNamePtr;
typedef std::map<std::string, NewNamePtr> NewNameListType;

static NewNameListType NewNameList;

// сохранение списка в файл
void NewNameSave()
{
	std::ofstream file(NNAME_FILE);
	if (!file.is_open()) {
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

	name->name0 = GET_PAD(ch, 0);
	name->name1 = GET_PAD(ch, 1);
	name->name2 = GET_PAD(ch, 2);
	name->name3 = GET_PAD(ch, 3);
	name->name4 = GET_PAD(ch, 4);
	name->name5 = GET_PAD(ch, 5);
	name->email = GET_EMAIL(ch);
	name->sex = GET_SEX(ch);

	NewNameList[GET_NAME(ch)] = name;
	if (save)
		NewNameSave();
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
void NewNameRemove(const char * name, CHAR_DATA * ch)
{
	NewNameListType::iterator it;
	std::string buffer = name;
	it = NewNameList.find(buffer);
	if (it != NewNameList.end()) {
		NewNameList.erase(it);
		send_to_char("Запись удалена.\r\n", ch);
	} else
		send_to_char("В списке нет такого имени.\r\n", ch);

	NewNameSave();
}

// лоад списка неодобренных имен
void NewNameLoad()
{
	std::ifstream file(NNAME_FILE);
	if (!file.is_open()) {
		log("Error open file: %s! (%s %s %d)", NNAME_FILE, __FILE__, __func__, __LINE__);
		return;
	}

	CHAR_DATA *tch;
	std::string buffer;
	while(file >> buffer) {
		// сразу проверяем не сделетился ли уже персонаж
		CREATE(tch, CHAR_DATA, 1);
		clear_char(tch);
		if (load_char(buffer.c_str(), tch) < 0) {
			free(tch);
			continue;
		}
		// не сделетился...
		NewNameAdd(tch, 0);
		free_char(tch);
	}

	file.close();
	NewNameSave();
}

// вывод списка неодобренных имму
void NewNameShow(CHAR_DATA * ch)
{
	if (NewNameList.empty()) return;

	std::ostringstream buffer;
	buffer << "\r\nСписок игроков, ждущих одобрения имени (имя <игрок> одобрить/запретить):\r\n"
		<< "Для удаления из списка без одобрения/запрета наберите 'имя удалить <игрок>'\r\n"
		<< CCWHT(ch, C_NRM);
	for (NewNameListType::const_iterator it = NewNameList.begin(); it != NewNameList.end(); ++it)
		buffer << "Имя: " << it->first << " " << it->second->name0 << "/" << it->second->name1
			<< "/" << it->second->name2 << "/" << it->second->name3 << "/" << it->second->name4
			<< "/" << it->second->name5 << " Email: " << it->second->email << " Пол: "
			<< genders[it->second->sex] << "\r\n";
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
	if (!(fin = fopen(DNAME_FILE, "r"))) {
		perror("SYSERR: Unable to open '" DNAME_FILE "' for read");
		return;
	}
	if (!(fout = fopen("" DNAME_FILE ".tmp", "w"))) {
		perror("SYSERR: Unable to open '" DNAME_FILE ".tmp' for writing");
		fclose(fin);
		return;
	}
	while (get_line(fin, temp)) {
		// Get name ...
		sscanf(temp, "%s %s %d", mortname, immname, &immlev);
		if (strcmp(mortname, GET_NAME(d))) {
			// Name un matches ... do copy ...
			fprintf(fout, "%s\n", temp);
		};
	}
	fclose(fin);
	fclose(fout);
	rename(DNAME_FILE ".tmp", DNAME_FILE);
	return;

}

void add_agree_name(CHAR_DATA * d, char *immname, int immlev)
{
	FILE *fl;
	if (!(fl = fopen(ANAME_FILE, "a"))) {
		perror("SYSERR: Unable to open '" ANAME_FILE "' for writing");
		return;
	}
	// Pos to the end ...
	fprintf(fl, "%s %s %s %s %s %s %s %d\r\n", GET_NAME(d),
		GET_PAD(d, 1), GET_PAD(d, 2), GET_PAD(d, 3), GET_PAD(d, 4), GET_PAD(d, 5), immname, immlev);
	fclose(fl);
	return;
}

void add_disagree_name(CHAR_DATA * d, char *immname, int immlev)
{
	FILE *fl;
	if (!(fl = fopen(DNAME_FILE, "a"))) {
		perror("SYSERR: Unable to open '" DNAME_FILE "' for writing");
		return;
	}
	// Pos to the end ...
	fprintf(fl, "%s %s %d\r\n", GET_NAME(d), immname, immlev);
	fclose(fl);
	return;
}

void disagree_name(CHAR_DATA * d, char *immname, int immlev)
{
	// Clean record from agreed if present ...
	rm_agree_name(d);
	rm_disagree_name(d);
	NewNameRemove(d);
	// Add record to disagreed if not present ...
	add_disagree_name(d, immname, immlev);
}

void agree_name(CHAR_DATA * d, char *immname, int immlev)
{
	// Clean record from disgreed if present ...
	rm_agree_name(d);
	rm_disagree_name(d);
	NewNameRemove(d);
	// Add record to agreed if not present ...
	add_agree_name(d, immname, immlev);
}
