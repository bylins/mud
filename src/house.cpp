/* ****************************************************************************
* File: house.cpp                                              Part of Bylins *
* Usage: Handling of clan system                                              *
* (c) 2005 Krodo                                                              *
******************************************************************************/

#include "conf.h"
#include <fstream>
#include <sstream>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sys/stat.h>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>

#include "house.h"
#include "comm.h"
#include "handler.h"
#include "pk.h"
#include "screen.h"
#include "boards.h"
#include "skills.h"
#include "spells.h"
#include "privilege.hpp"
#include "char.hpp"
#include "char_player.hpp"

extern void list_obj_to_char(OBJ_DATA * list, CHAR_DATA * ch, int mode, int show);
extern void write_one_object(char **data, OBJ_DATA * object, int location);
extern OBJ_DATA *read_one_object_new(char **data, int *error);
extern int file_to_string_alloc(const char *name, char **buf);
extern struct player_index_element *player_table;
// TODO: думать надо с этим, или глобально следить за спамом, или игноров напихать на все случаи жизни, или так и оставить
extern void set_wait(CHAR_DATA * ch, int waittime, int victim_in_room);
extern int top_of_p_table;
extern const char *apply_types[];
extern const char *item_types[];
extern const char *wear_bits[];
extern const char *weapon_class[];
extern const char *weapon_affects[];
extern const char *show_obj_to_char(OBJ_DATA * object, CHAR_DATA * ch, int mode, int show_state, int how);
extern void imm_show_obj_values(OBJ_DATA * obj, CHAR_DATA * ch);
extern void mort_show_obj_values(OBJ_DATA * obj, CHAR_DATA * ch, int fullness);

// vnum кланового сундука
static const int CLAN_CHEST_VNUM = 330;
static int CLAN_CHEST_RNUM = -1;

long long clan_level_exp [MAX_CLANLEVEL+1] =
{
	0LL,
	100000000LL, /* 1 level, should be achieved fast, 100M imho is possible*/
	1000000000LL, /* 1bilion. OMG. */
	5000000000LL, /* 5bilions. */
	15000000000LL, /* 15billions. */
	1000000000000LL /* BIG NUMBER. */
};

inline bool Clan::SortRank::operator()(const CHAR_DATA * ch1, const CHAR_DATA * ch2)
{
	return CLAN_MEMBER(ch1)->rank_num < CLAN_MEMBER(ch2)->rank_num;
}

ClanListType Clan::ClanList;

// поиск to_room в зонах клан-замков, выставляет за замок, если найдено
room_rnum Clan::CloseRent(room_rnum to_room)
{
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
		if (world[to_room]->zone == world[real_room((*clan)->rent)]->zone)
			return real_room((*clan)->out_rent);
	return to_room;
}

// проверяет находится ли чар в зоне чужого клана
bool Clan::InEnemyZone(CHAR_DATA * ch)
{
	int zone = world[IN_ROOM(ch)]->zone;

	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
		if (zone == world[real_room((*clan)->rent)]->zone)
		{
			if (CLAN(ch) && (CLAN(ch) == *clan || (*clan)->CheckPolitics(CLAN(ch)->GetRent()) == POLITICS_ALLIANCE))
				return 0;
			else
				return 1;
		}
	return 0;
}

Clan::Clan()
		: guard(0), builtOn(time(0)), bankBuffer(0), entranceMode(0), bank(2000), exp(0), clan_exp(0),
		exp_buf(0), clan_level(0), rent(0), out_rent(0), chest_room(0), storehouse(1), exp_info(1),
		test_clan(0), chest_objcount(0), chest_discount(0), chest_weight(0)
{

}

Clan::~Clan()
{

}

// лоад/релоад индекса и файлов кланов
void Clan::ClanLoad()
{
	init_chest_rnum();
	// на случай релоада
	Clan::ClanList.clear();

	// файл со списком кланов
	std::ifstream file(LIB_HOUSE "index");
	if (!file.is_open())
	{
		log("Error open file: %s! (%s %s %d)", LIB_HOUSE "index", __FILE__, __func__, __LINE__);
		return;
	}
	std::string buffer;
	std::list<std::string> clanIndex;
	while (file >> buffer)
		clanIndex.push_back(buffer);
	file.close();
	// собственно грузим кланы
	for (std::list < std::string >::const_iterator it = clanIndex.begin(); it != clanIndex.end(); ++it)
	{
		ClanPtr tempClan(new Clan);

		std::string filename = LIB_HOUSE + *it + "/" + *it;
		std::ifstream file(filename.c_str());
		if (!file.is_open())
		{
			log("Error open file: %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
			return;
		}
		while (file >> buffer)
		{
			if (buffer == "Abbrev:")
			{
				if (!(file >> tempClan->abbrev))
				{
					log("Error open 'Abbrev:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
					break;
				}
			}
			else if (buffer == "Name:")
			{
				std::getline(file, buffer);
				boost::trim(buffer);
				tempClan->name = buffer;
			}
			else if (buffer == "Title:")
			{
				std::getline(file, buffer);
				boost::trim(buffer);
				tempClan->title = buffer;
			}
			else if (buffer == "Rent:")
			{
				int rent = 0;
				if (!(file >> rent))
				{
					log("Error open 'Rent:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
					break;
				}
				// зоны может и не быть
				if (!real_room(rent))
				{
					log("Room %d is no longer exist (%s).", rent, filename.c_str());
					break;
				}
				tempClan->rent = rent;
			}
			else if (buffer == "OutRent:")
			{
				int out_rent = 0;
				if (!(file >> out_rent))
				{
					log("Error open 'OutRent:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
					break;
				}
				// зоны может и не быть
				if (!real_room(out_rent))
				{
					log("Room %d is no longer exist (%s).", out_rent, filename.c_str());
					break;
				}
				tempClan->out_rent = out_rent;
			}
			else if (buffer == "ChestRoom:")
			{
				int chest_room = 0;
				if (!(file >> chest_room))
				{
					log("Error open 'OutRent:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
					break;
				}
				// зоны может и не быть
				if (!real_room(chest_room))
				{
					log("Room %d is no longer exist (%s).", chest_room, filename.c_str());
					break;
				}
				tempClan->chest_room = chest_room;
			}
			else if (buffer == "Guard:")
			{
				int guard = 0;
				if (!(file >> guard))
				{
					log("Error open 'Guard:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
					break;
				}
				// как и охранника
				if (real_mobile(guard) < 0)
				{
					log("Guard %d is no longer exist (%s).", guard, filename.c_str());
					break;
				}
				tempClan->guard = guard;
			}
			else if (buffer == "BuiltOn:")
			{
				if (!(file >> tempClan->builtOn))
				{
					log("Error open 'BuiltOn:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
					break;
				}
			}
			else if (buffer == "Ranks:")
			{
				std::getline(file, buffer, '~');
				std::istringstream stream(buffer);
				unsigned long priv = 0;

				std::string buffer2;
				// для верности воеводе проставим все привилегии, заодно сверим с файлом
				if (!(stream >> buffer >> buffer2 >> priv))
				{
					log("Error open 'Ranks' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
					break;
				}
				tempClan->ranks.push_back(buffer);
				tempClan->ranks_female.push_back(buffer2);
				tempClan->privileges.push_back(std::bitset<CLAN_PRIVILEGES_NUM> (0));
				for (int i = 0; i < CLAN_PRIVILEGES_NUM; ++i)
					tempClan->privileges[0].set(i);

				while (stream >> buffer)
				{
					if (!(stream >> buffer2 >> priv))
					{
						log("Error open 'Ranks' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
						break;
					}
					tempClan->ranks.push_back(buffer);
					tempClan->ranks_female.push_back(buffer2);
					// на случай уменьшения привилегий
					if (priv > tempClan->privileges[0].to_ulong())
						priv = tempClan->privileges[0].to_ulong();
					tempClan->privileges.push_back(std::bitset<CLAN_PRIVILEGES_NUM> (priv));
				}
			}
			else if (buffer == "Politics:")
			{
				std::getline(file, buffer, '~');
				std::istringstream stream(buffer);
				int room = 0;
				int state = 0;

				while (stream >> room >> state)
					tempClan->politics[room] = state;
			}
			else if (buffer == "EntranceMode:")
			{
				if (!(file >> tempClan->entranceMode))
				{
					log("Error open 'EntranceMode:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
					break;
				}
			}
			else if (buffer == "Storehouse:")
			{
				if (!(file >> tempClan->storehouse))
				{
					log("Error open 'Storehouse:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
					break;
				}
			}
			else if (buffer == "ExpInfo:")
			{
				if (!(file >> tempClan->exp_info))
				{
					log("Error open 'ExpInfo:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
					break;
				}
			}
			else if (buffer == "TestClan:")
			{
				if (!(file >> tempClan->test_clan))
				{
					log("Error open 'TestClan:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
					break;
				}
			}
			else if (buffer == "Exp:")
			{
				if (!(file >> tempClan->exp))
				{
					log("Error open 'Exp:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
					break;
				}
			}
			else if (buffer == "ExpBuf:")
			{
				if (!(file >> tempClan->exp_buf))
				{
					log("Error open 'ExpBuf:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
					break;
				}
			}
			else if (buffer == "Bank:")
			{
				file >> tempClan->bank;
				if (tempClan->bank <= 0)
				{
					log("Clan has 0 in bank, remove from list (%s).", filename.c_str());
					break;
				}
			}
			else if (buffer == "StoredExp:")
			{
				if (!(file >> tempClan->clan_exp))
					log("Error open 'StoredExp:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
			}
			else if (buffer == "ClanLevel:")
			{
				if (!(file >> tempClan->clan_level))
					log("Error open 'ClanLevel:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
			}
			else if (buffer == "Owner:")
			{
				long unique = 0;
				long long money = 0;
				long long exp = 0;
				int exp_persent = 0;
				long long clan_exp = 0;

				if (!(file >> unique >> money >> exp >> exp_persent >> clan_exp))
				{
					log("Error open 'Owner:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
					break;
				}
				// воеводы тоже уже может не быть
				ClanMemberPtr tempMember(new ClanMember);
				tempMember->name = GetNameByUnique(unique);
				if (tempMember->name.empty())
				{
					log("Owner %ld is no longer exist (%s).", unique, filename.c_str());
					break;
				}
				tempMember->name[0] = UPPER(tempMember->name[0]);
				tempMember->rank_num = 0;
				tempMember->money = money;
				tempMember->exp = exp;
				tempMember->exp_persent = exp_persent;
				tempMember->clan_exp = clan_exp;
				tempClan->members[unique] = tempMember;
				tempClan->owner = tempMember->name;

			}
			else if (buffer == "Members:")
			{
				// параметры, критичные для мемберов, нужно загрузить до того как (ранги например)
				long unique = 0;
				unsigned rank = 0;
				long long money = 0;
				long long exp = 0;
				int exp_persent = 0;
				long long clan_exp = 0;

				std::getline(file, buffer, '~');
				std::istringstream stream(buffer);
				while (stream >> unique)
				{
					if (!(stream >> rank >> money >> exp >> exp_persent >> clan_exp))
					{
						log("Error read %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
						break;
					}
					// на случай, если рангов стало меньше
					if (!tempClan->ranks.empty() && rank > tempClan->ranks.size() - 1)
						rank = tempClan->ranks.size() - 1;

					// удаленные персонажи просто игнорируются
					ClanMemberPtr tempMember(new ClanMember);
					tempMember->name = GetNameByUnique(unique);
					if (tempMember->name.empty())
					{
						log("Member %ld is no longer exist (%s).", unique, filename.c_str());
						continue;
					}
					tempMember->name[0] = UPPER(tempMember->name[0]);
					tempMember->rank_num = rank;
					tempMember->money = money;
					tempMember->exp = exp;
					tempMember->exp_persent = exp_persent;
					tempMember->clan_exp = clan_exp;
					tempClan->members[unique] = tempMember;
				}
			}
		}
		file.close();

		// тут нужно проверить наличие критичных для клана полей
		// т.к. загрузка без привязки к положению в файле - что-то может не проинициализироваться
		if (tempClan->abbrev.empty() || tempClan->name.empty()
				|| tempClan->title.empty() || tempClan->owner.empty()
				|| tempClan->rent == 0 || tempClan->guard == 0 || tempClan->out_rent == 0
				|| tempClan->ranks.empty() || tempClan->privileges.empty())
			continue;
		// сундук по дефолту на ренте
		if (!tempClan->chest_room)
			tempClan->chest_room = tempClan->rent;

		// чтобы не получилось потерь/прибавок экспы
		if (tempClan->exp_buf)
		{
			tempClan->exp += tempClan->exp_buf;
			if (tempClan->exp < 0)
				tempClan->exp = 0;
			tempClan->exp_buf = 0;
		}

		// подгружаем пкл/дрл
		std::ifstream pkFile((filename + ".pkl").c_str());
		if (pkFile.is_open())
		{
			int author = 0;
			while (pkFile >> author)
			{
				int victim = 0;
				time_t tempTime = time(0);
				bool flag = 0;

				if (!(pkFile >> victim >> tempTime >> flag))
				{
					log("Error read %s! (%s %s %d)", (filename + ".pkl").c_str(), __FILE__, __func__, __LINE__);
					break;
				}
				std::getline(pkFile, buffer, '~');
				boost::trim(buffer);
				std::string authorName = GetNameByUnique(author);
				std::string victimName = GetNameByUnique(victim, 1);
				name_convert(authorName);
				name_convert(victimName);
				// если автора уже нет - сбросим УИД для верности
				if (authorName.empty())
					author = 0;
				// если жертвы уже нет, или жертва выбилась в БОГИ - не грузим
				if (!victimName.empty())
				{
					ClanPkPtr tempRecord(new ClanPk);
					tempRecord->author = author;
					tempRecord->authorName = authorName.empty() ? "Того уж с нами нет" : authorName;
					tempRecord->victimName = victimName;
					tempRecord->time = tempTime;
					tempRecord->text = buffer;
					if (!flag)
						tempClan->pkList[victim] = tempRecord;
					else
						tempClan->frList[victim] = tempRecord;
				}
			}
			pkFile.close();
		}
		//подгружаем кланстафф
		std::ifstream stuffFile((filename + ".stuff").c_str());
		if (stuffFile.is_open())
		{
			std::string line;
			int i;
			while (stuffFile >> i)
			{
				ClanStuffName temp;
				temp.num = i;

				std::getline(stuffFile, temp.name, '~');
				boost::trim(temp.name);

				for (int j = 0;j < 6; j++)
				{
					std::getline(stuffFile, buffer, '~');
					boost::trim(buffer);
					temp.PNames.push_back(buffer);
				}

				std::getline(stuffFile, temp.desc, '~');
				boost::trim(temp.desc);
				std::getline(stuffFile, temp.longdesc, '~');
				boost::trim(temp.longdesc);

				tempClan->clanstuff.push_back(temp);
			}
		}
		// клан-экспа
		tempClan->last_exp.load(tempClan->get_abbrev());

		Clan::ClanList.push_back(tempClan);
	}

	Clan::ChestLoad();
	Clan::ChestUpdate();
	Clan::ChestSave();
	Clan::ClanSave();
	Board::ClanInit();

	// на случай релоада кланов для выставления изменений игрокам онлайн
	// лдшникам воткнется в другом месте, можно и тут чар-лист прогнать, варианты одинаково корявые
	DESCRIPTOR_DATA *d;
	for (d = descriptor_list; d; d = d->next)
		if (d->character)
			Clan::SetClanData(d->character);
}

// вывод имму информации о кланах
void Clan::HconShow(CHAR_DATA * ch)
{
	std::ostringstream buffer;
	buffer << "Abbrev|  Rent|OutRent| Chest|  Guard|CreateDate|      StoredExp|      Bank|Items|DayTax|Lvl|Test\r\n";
	boost::format show("%6d|%6d|%7d|%6d|%7d|%10s|%15d|%10d|%5d|%6d|%3s|%4s\r\n");
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
	{
		char timeBuf[17];
		strftime(timeBuf, sizeof(timeBuf), "%d-%m-%Y", localtime(&(*clan)->builtOn));

		int cost = (*clan)->ChestTax();
		cost += CLAN_TAX + ((*clan)->storehouse * CLAN_STOREHOUSE_TAX);

		buffer << show % (*clan)->abbrev % (*clan)->rent % (*clan)->out_rent % (*clan)->chest_room % (*clan)->guard % timeBuf
		% (*clan)->clan_exp % (*clan)->bank % (*clan)->chest_objcount % cost % (*clan)->clan_level % ((*clan)->test_clan ? "y" : "n");
	}
	send_to_char(ch, buffer.str().c_str());
}

// сохранение кланов в файлы
void Clan::ClanSave()
{
	std::ofstream index(LIB_HOUSE "index");
	if (!index.is_open())
	{
		log("Error open file: %s! (%s %s %d)", LIB_HOUSE "index", __FILE__, __func__, __LINE__);
		return;
	}

	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
	{
		// именем файла для клана служит его аббревиатура (английский и нижний регистр)
		std::string buffer = (*clan)->abbrev;
		CreateFileName(buffer);
		index << buffer << "\n";

		std::string filepath = LIB_HOUSE + buffer + "/" + buffer;
		std::string path = LIB_HOUSE + buffer;
		struct stat result;
		if (stat(filepath.c_str(), &result))
		{
			if (mkdir(path.c_str(), 0700))
			{
				log("Can't create dir: %s! (%s %s %d)", path.c_str(), __FILE__, __func__, __LINE__);
				return;
			}
		}
		std::ofstream file(filepath.c_str());
		if (!file.is_open())
		{
			log("Error open file: %s! (%s %s %d)", filepath.c_str(), __FILE__, __func__, __LINE__);
			return;
		}

		file << "Abbrev: " << (*clan)->abbrev << "\n"
		<< "Name: " << (*clan)->name << "\n"
		<< "Title: " << (*clan)->title << "\n"
		<< "Rent: " << (*clan)->rent << "\n"
		<< "OutRent: " << (*clan)->out_rent << "\n"
		<< "ChestRoom: " << (*clan)->chest_room << "\n"
		<< "Guard: " << (*clan)->guard << "\n"
		<< "BuiltOn: " << (*clan)->builtOn << "\n"
		<< "EntranceMode: " << (*clan)->entranceMode << "\n"
		<< "Storehouse: " << (*clan)->storehouse << "\n"
		<< "ExpInfo: " << (*clan)->exp_info << "\n"
		<< "TestClan: " << (*clan)->test_clan << "\n"
		<< "Exp: " << (*clan)->exp << "\n"
		<< "ExpBuf: " << (*clan)->exp_buf << "\n"
		<< "StoredExp: " << (*clan)->clan_exp << "\n"
		<< "ClanLevel: " << (*clan)->clan_level << "\n"
		<< "Bank: " << (*clan)->bank << "\n" << "Politics:\n";
		for (ClanPolitics::const_iterator it = (*clan)->politics.begin(); it != (*clan)->politics.end(); ++it)
			file << " " << it->first << " " << it->second << "\n";
		file << "~\n" << "Ranks:\n";
		int i = 0;

		for (std::vector < std::string >::const_iterator it = (*clan)->ranks.begin(); it != (*clan)->ranks.end(); ++it, ++i)
			file << " " << (*it) << " " << (*clan)->ranks_female[i] << " " << (*clan)->privileges[i].to_ulong() << "\n";
		file << "~\n" << "Owner: ";
		for (ClanMemberList::const_iterator it = (*clan)->members.begin(); it != (*clan)->members.end(); ++it)
			if (it->second->rank_num == 0)
			{
				file << it->first << " " << it->second->money << " " << it->second->exp << " " << it->second->exp_persent << " " << it->second->clan_exp << "\n";
				break;
			}
		file << "Members:\n";
		for (ClanMemberList::const_iterator it = (*clan)->members.begin(); it != (*clan)->members.end(); ++it)
			if (it->second->rank_num != 0)
				file << " " << it->first << " " << it->second->rank_num << " " << it->second->money  << " " << it->second->exp << " " << it->second->exp_persent << " " << it->second->clan_exp << "\n";
		file << "~\n";
		file.close();

		std::ofstream pkFile((filepath + ".pkl").c_str());
		if (!pkFile.is_open())
		{
			log("Error open file: %s! (%s %s %d)", (filepath + ".pkl").c_str(), __FILE__, __func__, __LINE__);
			return;
		}
		for (ClanPkList::const_iterator it = (*clan)->pkList.begin(); it != (*clan)->pkList.end(); ++it)
			pkFile << it->second->author << " " << it->first << " " << (*it).
			second->time << " " << "0\n" << it->second->text << "\n~\n";
		for (ClanPkList::const_iterator it = (*clan)->frList.begin(); it != (*clan)->frList.end(); ++it)
			pkFile << it->second->author << " " << it->first << " " << (*it).
			second->time << " " << "1\n" << it->second->text << "\n~\n";
		pkFile.close();
	}
	index.close();
}

// проставляем персонажу нужные поля если он клановый
// отсюда и далее для проверки на клановость пользуем CLAN()
void Clan::SetClanData(CHAR_DATA * ch)
{
	CLAN(ch).reset();
	CLAN_MEMBER(ch).reset();
	// если правит свою дружину в олц, то радостно его выкидываем
	if (STATE(ch->desc) == CON_CLANEDIT)
	{
		ch->desc->clan_olc.reset();
		STATE(ch->desc) = CON_PLAYING;
		send_to_char("Редактирование отменено из-за обновления Ваших данных в дружине.\r\n", ch);
	}

	// если куда-то приписан, то дергаем сразу итераторы на клан и список членов
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
	{
		ClanMemberList::const_iterator member = (*clan)->members.find(GET_UNIQUE(ch));
		if (member != (*clan)->members.end())
		{
			CLAN(ch) = *clan;
			CLAN_MEMBER(ch) = member->second;
			break;
		}
	}
	// никуда не приписан
	if (!CLAN(ch))
	{
		free(GET_CLAN_STATUS(ch));
		GET_CLAN_STATUS(ch) = 0;
		return;
	}
	// куда-то таки приписан
	std::string buffer;
	if (IS_MALE(ch))
		buffer = CLAN(ch)->ranks[CLAN_MEMBER(ch)->rank_num] + " " + CLAN(ch)->title;
	else
		buffer = CLAN(ch)->ranks_female[CLAN_MEMBER(ch)->rank_num] + " " + CLAN(ch)->title;
	GET_CLAN_STATUS(ch) = str_dup(buffer.c_str());

	// чтобы при выходе не смог приписаться опять за один ребут мада
	ch->desc->clan_invite.reset();
}

// проверка комнаты на принадлежность какому-либо замку
ClanListType::const_iterator Clan::IsClanRoom(room_rnum room)
{
	ClanListType::const_iterator clan;
	for (clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
		if (world[room]->zone == world[real_room((*clan)->rent)]->zone)
			return clan;
	return clan;
}

// может ли персонаж зайти в замок
bool Clan::MayEnter(CHAR_DATA * ch, room_rnum room, bool mode)
{
	ClanListType::const_iterator clan = IsClanRoom(room);
	if (clan == Clan::ClanList.end()
			|| IS_GRGOD(ch)
			|| !ROOM_FLAGGED(room, ROOM_HOUSE)
			|| (*clan)->entranceMode
			|| Privilege::check_flag(ch, Privilege::KRODER))
	{
		return 1;
	}
	if (!CLAN(ch))
		return 0;

	bool isMember = 0;

	if (CLAN(ch) == *clan || (*clan)->CheckPolitics(CLAN(ch)->GetRent()) == POLITICS_ALLIANCE)
		isMember = 1;

	switch (mode)
	{
		// вход через дверь - контролирует охранник
	case HCE_ATRIUM:
		CHAR_DATA * mobs;
		for (mobs = world[IN_ROOM(ch)]->people; mobs; mobs = mobs->next_in_room)
			if ((*clan)->guard == GET_MOB_VNUM(mobs))
				break;
		// охранника нет - свободный доступ
		if (!mobs)
			return 1;
		// break не надо

		// телепортация
	case HCE_PORTAL:
		if (!isMember)
			return 0;
		// с временным флагом тоже курят
		if (RENTABLE(ch))
		{
			if (mode == HCE_ATRIUM)
				send_to_char("Пускай сначала кровь с тебя стечет, а потом входи сколько угодно.\r\n", ch);
			return 0;
		}
		return 1;
	}
	return 0;
}

// в зависимости от доступности команды будут видны только нужные строки
const char *HOUSE_FORMAT[] =
{
	"  клан информация\r\n",
	"  клан принять имя звание\r\n",
	"  клан изгнать имя\r\n",
	"  клан привилегии\r\n",
	"  гдругам (текст)\r\n",
	"  политика <имя дружины> <нейтралитет|война|альянс>\r\n",
	"  дрновости <писать|очистить>\r\n",
	"  пклист|дрлист <добавить|удалить>\r\n",
	"  класть вещи в хранилище\r\n",
	"  брать вещи из хранилища\r\n",
	"  казна (снимать)\r\n",
	"  клан покинуть (выход из дружины)\r\n",
};

// обработка клановых привилегий (команда house)
ACMD(DoHouse)
{
	if (IS_NPC(ch))
		return;

	std::string buffer = argument, buffer2;
	GetOneParam(buffer, buffer2);

	// если игрок неклановый, то есть вариант с приглашением
	if (!CLAN(ch))
	{
		if (CompareParam(buffer2, "согласен") && ch->desc->clan_invite)
			ch->desc->clan_invite->clan->ClanAddMember(ch, ch->desc->clan_invite->rank);
		else if (CompareParam(buffer2, "отказать") && ch->desc->clan_invite)
		{
			ch->desc->clan_invite.reset();
			send_to_char("Приглашение дружины отклонено.\r\n", ch);
			return;
		}
		else
			send_to_char("Данная команда доступна только высоко привилегированным членам дружин,\r\n"
						 "а если вас пригласили вступить в оную, то так и пишите 'клан согласен' или 'клан отказать'.\r\n"
						 "Никто не сможет послать вам новое приглашение до тех пор, пока вы не разберетесь с этим.\r\n", ch);
		return;
	}

	if (CompareParam(buffer2, "информация") && CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_INFO])
		CLAN(ch)->HouseInfo(ch);
	else if (CompareParam(buffer2, "принять") && CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_ADD])
		CLAN(ch)->HouseAdd(ch, buffer);
	else if (CompareParam(buffer2, "изгнать") && CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_REMOVE])
		CLAN(ch)->HouseRemove(ch, buffer);
	else if (CompareParam(buffer2, "привилегии") && CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_PRIVILEGES])
	{
		boost::shared_ptr<struct ClanOLC> temp_clan_olc(new ClanOLC);
		temp_clan_olc->clan = CLAN(ch);
		temp_clan_olc->privileges = CLAN(ch)->privileges;
		ch->desc->clan_olc = temp_clan_olc;
		STATE(ch->desc) = CON_CLANEDIT;
		CLAN(ch)->MainMenu(ch->desc);
	}
	else if (CompareParam(buffer2, "воевода") && !CLAN_MEMBER(ch)->rank_num)
		CLAN(ch)->HouseOwner(ch, buffer);
	else if (CompareParam(buffer2, "статистика"))
		CLAN(ch)->HouseStat(ch, buffer);
	else if (CompareParam(buffer2, "покинуть", 1) && CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_EXIT])
		CLAN(ch)->HouseLeave(ch);
	else if (CompareParam(buffer2, "налог"))
		CLAN(ch)->TaxManage(ch, buffer);
	else
	{
		// обработка списка доступных команд по званию персонажа
		buffer = "Доступные Вам привилегии дружины:\r\n";
		for (int i = 0; i < CLAN_PRIVILEGES_NUM; ++i)
			if (CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][i])
				buffer += HOUSE_FORMAT[i];
		// воевода до кучи может сам сменить у дружины воеводу
		if (!CLAN_MEMBER(ch)->rank_num)
			buffer += "  клан воевода (имя)\r\n";
		buffer += "  клан налог (процент отдаваемого опыта)\r\n";
		if (CLAN(ch)->storehouse)
			buffer += "  хранилище <фильтры>\r\n";
		buffer += "  клан статистика <!опыт/!заработанным/!налог/!последнему/!имя>";
		if (!CLAN_MEMBER(ch)->rank_num)
			buffer += " <имя|все|очистить|очистить деньги>\r\n";
		else
			buffer += " <имя|все>\r\n";
		if (!CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_POLITICS])
			buffer += "  политика (только просмотр)\r\n";
		send_to_char(buffer, ch);
	}
}

// house информация
void Clan::HouseInfo(CHAR_DATA * ch)
{
	// думаю, вываливать список сортированный по уиду некрасиво, поэтому перебираем по рангам
	std::vector<ClanMemberPtr> temp_list;
	for (ClanMemberList::const_iterator it = members.begin(); it != members.end(); ++it)
		temp_list.push_back(it->second);

	std::sort(temp_list.begin(), temp_list.end(),
			  boost::bind(std::less<long long>(),
						  boost::bind(&ClanMember::rank_num, _1),
						  boost::bind(&ClanMember::rank_num, _2)));

	std::ostringstream buffer;
	buffer << "К замку приписаны: ";
	for (std::vector<ClanMemberPtr>::const_iterator it = temp_list.begin(); it != temp_list.end(); ++it)
		buffer << (*it)->name << " ";
	buffer << "\r\nПривилегии:\r\n";
	int num = 0;

	for (std::vector<std::string>::const_iterator it = ranks.begin(); it != ranks.end(); ++it, ++num)
	{
		buffer << "  " << (*it) << ":";
		for (int i = 0; i < CLAN_PRIVILEGES_NUM; ++i)
			if (this->privileges[num][i])
				switch (i)
				{
				case MAY_CLAN_INFO:
					buffer << " инфо";
					break;
				case MAY_CLAN_ADD:
					buffer << " принять";
					break;
				case MAY_CLAN_REMOVE:
					buffer << " изгнать";
					break;
				case MAY_CLAN_PRIVILEGES:
					buffer << " привилегии";
					break;
				case MAY_CLAN_CHANNEL:
					buffer << " гд";
					break;
				case MAY_CLAN_POLITICS:
					buffer << " политика";
					break;
				case MAY_CLAN_NEWS:
					buffer << " дрн";
					break;
				case MAY_CLAN_PKLIST:
					buffer << " пкл";
					break;
				case MAY_CLAN_CHEST_PUT:
					buffer << " хран.полож";
					break;
				case MAY_CLAN_CHEST_TAKE:
					buffer << " хран.взять";
					break;
				case MAY_CLAN_BANK:
					buffer << " казна";
					break;
				case MAY_CLAN_EXIT:
					buffer << " выход";
					break;
				}
		buffer << "\r\n";
	}
	//инфа о экспе замка левеле замка, рейтинге замка и плюшках
	buffer << "Ваш замок набрал " << this->clan_exp << " очков опыта и имеет уровень " << this->clan_level << "\r\n";
	buffer << "Рейтинг вашего замка: " << this->exp << " Это очень круто :), но ничего Вам не дает.\r\n";
	buffer << "В хранилище замка может хранится до " << this->ChestMaxObjects() << " " << desc_count(this->ChestMaxObjects(), WHAT_OBJECT) << " с общим весом не более чем " << this->ChestMaxWeight() << ".\r\n";

	// инфа о банке и хранилище
	int cost = ChestTax();
	buffer << "В хранилище Вашей дружины " << this->chest_objcount << " " << desc_count(this->chest_objcount, WHAT_OBJECT)  << " общим весом в " << this->chest_weight << " (" << cost << " " << desc_count(cost, WHAT_MONEYa) << " в день).\r\n"
	<< "Состояние казны: " << this->bank << " " << desc_count(this->bank, WHAT_MONEYa) << ".\r\n"
	<< "Налог составляет " << CLAN_TAX + (this->storehouse * CLAN_STOREHOUSE_TAX) << " " << desc_count(CLAN_TAX + (this->storehouse * CLAN_STOREHOUSE_TAX), WHAT_MONEYa) << " в день.\r\n";
	cost += CLAN_TAX + (this->storehouse * CLAN_STOREHOUSE_TAX);
	if (!cost)
		buffer << "Ваших денег хватит на нереальное количество дней.\r\n";
	else
		buffer << "Ваших денег хватит примерно на " << this->bank / cost << " " << desc_count(this->bank / cost, WHAT_DAY) << ".\r\n";
	send_to_char(buffer.str(), ch);
}

// клан принять, повлиять можно только на соклановцев ниже рангом
void Clan::HouseAdd(CHAR_DATA * ch, std::string & buffer)
{
	std::string buffer2;
	GetOneParam(buffer, buffer2);
	if (buffer2.empty())
	{
		send_to_char("Укажите имя персонажа.\r\n", ch);
		return;
	}

	long unique = GetUniqueByName(buffer2);
	if (!unique)
	{
		send_to_char("Неизвестный персонаж.\r\n", ch);
		return;
	}
	std::string name = buffer2;
	name[0] = UPPER(name[0]);
	if (unique == GET_UNIQUE(ch))
	{
		send_to_char("Сам себя повысил, самому себе вынес благодарность?\r\n", ch);
		return;
	}

	// изменение звания у членов дружины
	// даже если они находятся оффлайн
	ClanMemberList::iterator it_member = this->members.find(unique);
	if (it_member != this->members.end())
	{
		if (it_member->second->rank_num <= CLAN_MEMBER(ch)->rank_num)
		{
			send_to_char("Вы можете менять звания только у нижестоящих членов дружины.\r\n", ch);
			return;
		}

		int rank = CLAN_MEMBER(ch)->rank_num;
		if (!rank) ++rank;

		GetOneParam(buffer, buffer2);
		if (buffer2.empty())
		{
			buffer = "Укажите звание персонажа.\r\nДоступные положения: ";
			for (std::vector<std::string>::const_iterator it = this->ranks.begin() + rank; it != this->ranks.end(); ++it)
				buffer += "'" + *it + "' ";
			buffer += "\r\n";
			send_to_char(buffer, ch);
			return;
		}

		int temp_rank = rank;
		for (std::vector<std::string>::const_iterator it = this->ranks.begin() + rank; it != this->ranks.end(); ++it, ++temp_rank)
			if (CompareParam(buffer2, *it))
			{
				CHAR_DATA *editedChar = NULL;
				DESCRIPTOR_DATA *d = DescByUID(unique);
				this->members[unique]->rank_num = temp_rank;
				if (d)
				{
					editedChar = d->character;
					Clan::SetClanData(d->character);
					send_to_char(d->character, "%sВаше звание изменили, теперь вы %s.%s\r\n",
								 CCWHT(d->character, C_NRM),
								 (*it).c_str(),
								 CCNRM(d->character, C_NRM));
				}

				// оповещение соклановцев о изменении звания
				for (DESCRIPTOR_DATA *d = descriptor_list; d; d = d->next)
				{
					if (d->character && CLAN(d->character) && CLAN(d->character)->GetRent() == this->GetRent() && editedChar != d->character)
						send_to_char(d->character, "%s%s теперь %s.%s\r\n",
									 CCWHT(d->character, C_NRM),
									 it_member->second->name.c_str(), (*it).c_str(),
									 CCNRM(d->character, C_NRM));
				}
				return;
			}

		buffer = "Неверное звание, доступные положения:\r\n";
		for (std::vector<std::string>::const_iterator it = this->ranks.begin() + rank; it != this->ranks.end(); ++it)
			buffer += "'" + *it + "' ";
		buffer += "\r\n";
		send_to_char(buffer, ch);

		return;
	}

	DESCRIPTOR_DATA *d = DescByUID(unique);
	if (!d || !CAN_SEE(ch, d->character))
	{
		send_to_char("Этого персонажа нет в игре!\r\n", ch);
		return;
	}
	if (PRF_FLAGGED(d->character, PRF_CODERINFO))
	{
		send_to_char("Вы не можете приписать этого игрока.\r\n", ch);
		return;
	}
	if (CLAN(d->character) && CLAN(ch) != CLAN(d->character))
	{
		send_to_char("Вы не можете приписать члена другой дружины.\r\n", ch);
		return;
	}
	if (d->clan_invite)
	{
		if (d->clan_invite->clan == CLAN(ch))
		{
			send_to_char("Вы уже пригласили этого игрока, ждите реакции.\r\n", ch);
			return;
		}
		else
		{
			send_to_char("Он уже приглашен в другую дружину, дождитесь его ответа и пригласите снова.\r\n", ch);
			return;
		}
	}
	if (GET_KIN(d->character) != GET_KIN(ch))
	{
		send_to_char("Вы не можете сражаться в одном строю с иноплеменником.\r\n", ch);
		return;
	}
	GetOneParam(buffer, buffer2);

	// чтобы учесть воеводу с 0 рангом во время приписки и не дать ему приписать еще 10 воевод
	int rank = CLAN_MEMBER(ch)->rank_num;
	if (!rank)
		++rank;

	if (buffer2.empty())
	{
		buffer = "Укажите звание персонажа.\r\nДоступные положения: ";
		for (std::vector<std::string>::const_iterator it = this->ranks.begin() + rank; it != this->ranks.end(); ++it)
			buffer += "'" + *it + "' ";
		buffer += "\r\n";
		send_to_char(buffer, ch);
		return;
	}
	int temp_rank = rank; // дальше rank тоже мб использован в случае неверного выбора
	for (std::vector<std::string>::const_iterator it = this->ranks.begin() + rank; it != this->ranks.end(); ++it, ++temp_rank)
		if (CompareParam(buffer2, *it))
		{
			// не приписан - втыкаем ему приглашение и курим, пока не согласится
			boost::shared_ptr<struct ClanInvite> temp_invite(new ClanInvite);
			temp_invite->clan = CLAN(ch);
			temp_invite->rank = temp_rank;
			d->clan_invite = temp_invite;
			buffer = CCWHT(d->character, C_NRM);
			buffer += "$N приглашен$G в Вашу дружину, статус - " + *it + ".";
			buffer += CCNRM(d->character, C_NRM);
			// оповещаем счастливца
			act(buffer.c_str(), FALSE, ch, 0, d->character, TO_CHAR);
			buffer = CCWHT(d->character, C_NRM);
			buffer += "Вы получили приглашение в дружину '" + this->name + "', статус - " + *it + ".\r\n"
					  + "Чтобы принять приглашение наберите 'клан согласен', для отказа 'клан отказать'.\r\n"
					  + "Никто не сможет послать вам новое приглашение до тех пор, пока вы не разберетесь с этим.\r\n";

			buffer += CCNRM(d->character, C_NRM);
			send_to_char(buffer, d->character);
			return;
		}
	buffer = "Неверное звание, доступные положения:\r\n";
	for (std::vector<std::string>::const_iterator it = this->ranks.begin() + rank; it != this->ranks.end(); ++it)
		buffer += "'" + *it + "' ";
	buffer += "\r\n";
	send_to_char(buffer, ch);
}

/**
* Отписывание персонажа от клана с оповещением всех заинтересованных сторон,
* выдворением за пределы замка и изменением ренты при необходимости.
*/
void Clan::remove_member(ClanMemberList::iterator &it)
{
	std::string name = it->second->name;
	long unique = it->first;
	this->members.erase(it);

	DESCRIPTOR_DATA *k = DescByUID(unique);
	if (k && k->character)
	{
		Clan::SetClanData(k->character);
		send_to_char(k->character, "Вас исключили из дружины '%s'!\r\n", this->name.c_str());

		ClanListType::const_iterator clan = Clan::IsClanRoom(IN_ROOM(k->character));
		if (clan != Clan::ClanList.end())
		{
			char_from_room(k->character);
			act("$n был$g выдворен$a за пределы замка!", TRUE, k->character, 0, 0, TO_ROOM);
			send_to_char("Вы были выдворены за пределы замка!\r\n", k->character);
			char_to_room(k->character, real_room((*clan)->out_rent));
			look_at_room(k->character, real_room((*clan)->out_rent));
			act("$n свалил$u с небес, выкрикивая какие-то ругательства!", TRUE, k->character, 0, 0, TO_ROOM);
		}
	}
	for (DESCRIPTOR_DATA *d = descriptor_list; d; d = d->next)
	{
		if (d->character && CLAN(d->character) && CLAN(d->character)->GetRent() == this->GetRent())
			send_to_char(d->character, "%s более не является членом Вашей дружины.\r\n", name.c_str());
	}
}

// house изгнать (только званием ниже своего)
void Clan::HouseRemove(CHAR_DATA * ch, std::string & buffer)
{
	std::string buffer2;
	GetOneParam(buffer, buffer2);
	long unique = GetUniqueByName(buffer2);
	ClanMemberList::iterator it = this->members.find(unique);

	if (buffer2.empty())
		send_to_char("Укажите имя персонажа.\r\n", ch);
	else if (!unique)
		send_to_char("Неизвестный персонаж.\r\n", ch);
	else if (unique == GET_UNIQUE(ch))
		send_to_char("Выглядит довольно странно...\r\n", ch);
	if (it == this->members.end())
		send_to_char("Он и так не приписан к Вашей дружине.\r\n", ch);
	else if (it->second->rank_num <= CLAN_MEMBER(ch)->rank_num)
		send_to_char("Вы можете исключить из дружины только персонажа со званием ниже Вашего.\r\n", ch);
	else
		remove_member(it);
}

void Clan::HouseLeave(CHAR_DATA * ch)
{
	if (!CLAN_MEMBER(ch)->rank_num)
	{
		send_to_char("Если Вы хотите распустить свою дружину, то обращайтесь к Богам.\r\n"
					 "А если Вам просто нечем заняться, то передайте воеводство и идите куда хотите...\r\n", ch);
		return;
	}

	ClanMemberList::iterator it = this->members.find(GET_UNIQUE(ch));
	if (it != this->members.end())
		remove_member(it);
}

/**
* hcontrol outcast имя - отписывание любого персонажа от дружины, кроме воеводы.
*/
void Clan::hcon_outcast(CHAR_DATA *ch, std::string buffer)
{
	std::string name;
	GetOneParam(buffer, name);
	long unique = GetUniqueByName(name);

	if (!unique)
	{
		send_to_char("Неизвестный персонаж.\r\n", ch);
		return;
	}

	for (ClanListType::iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
	{
		ClanMemberList::iterator it = (*clan)->members.find(unique);
		if (it != (*clan)->members.end())
		{
			if (!it->second->rank_num)
			{
				send_to_char(ch, "Вы не можете исключить воеводу, для удаления дружины существует hcontrol destroy.\r\n");
				return;
			}
			(*clan)->remove_member(it);
			send_to_char(ch, "%s исключен из дружины '%s'.\r\n", name.c_str(), (*clan)->name.c_str());
			return;
		}
	}

	send_to_char("Он и так не состоит ни в какой дружине.\r\n", ch);
}

// бог, текст, клан/альянс
void Clan::GodToChannel(CHAR_DATA *ch, std::string text, int subcmd)
{
	boost::trim(text);
	// на счет скобок я хз, по-моему так нагляднее все же в ифах, где условий штук 5, мож опять индентом пройтись? Ж)
	if (text.empty())
	{
		send_to_char("Что Вы хотите им сообщить?\r\n", ch);
		return;
	}
	switch (subcmd)
	{
		// большой БОГ говорит какой-то дружине
	case SCMD_CHANNEL:
		for (DESCRIPTOR_DATA *d = descriptor_list; d; d = d->next)
		{
			if (d->character && CLAN(d->character)
					&& ch != d->character
					&& STATE(d) == CON_PLAYING
					&& CLAN(d->character).get() == this
					&& !AFF_FLAGGED(d->character, AFF_DEAFNESS))
			{
				send_to_char(d->character, "%s ВАШЕЙ ДРУЖИНЕ: %s'%s'%s\r\n",
							 GET_NAME(ch), CCIRED(d->character, C_NRM), text.c_str(), CCNRM(d->character, C_NRM));
			}
		}
		send_to_char(ch, "Вы дружине %s: %s'%s'.%s\r\n", this->abbrev.c_str(), CCIRED(ch, C_NRM), text.c_str(), CCNRM(ch, C_NRM));
		break;
		// он же в канал союзников этой дружины, если они вообще есть
	case SCMD_ACHANNEL:
		for (DESCRIPTOR_DATA *d = descriptor_list; d; d = d->next)
		{
			if (d->character && CLAN(d->character)
					&& STATE(d) == CON_PLAYING
					&& !AFF_FLAGGED(d->character, AFF_DEAFNESS)
					&& d->character != ch)
			{
				if (this->CheckPolitics(CLAN(d->character)->GetRent()) == POLITICS_ALLIANCE
						|| CLAN(d->character).get() == this)
				{
					// проверка на альянс с обеих сторон, иначе это не альянс
					if (CLAN(d->character).get() != this)
					{
						if (CLAN(d->character)->CheckPolitics(this->rent) == POLITICS_ALLIANCE)
							send_to_char(d->character, "%s ВАШИМ СОЮЗНИКАМ: %s'%s'%s\r\n",
										 GET_NAME(ch), CCIGRN(d->character, C_NRM), text.c_str(), CCNRM(d->character, C_NRM));
					}
					// первоначальному клану выдается всегда
					else
						send_to_char(d->character, "%s ВАШИМ СОЮЗНИКАМ: %s'%s'%s\r\n",
									 GET_NAME(ch), CCIGRN(d->character, C_NRM), text.c_str(), CCNRM(d->character, C_NRM));
				}
			}
		}
		send_to_char(ch, "Вы союзникам %s: %s'%s'.%s\r\n",
					 this->abbrev.c_str(), CCIGRN(ch, C_NRM), text.c_str(), CCNRM(ch, C_NRM));
		break;
	}
}

// чар (или клановый бог), текст, клан/альянс
void Clan::CharToChannel(CHAR_DATA *ch, std::string text, int subcmd)
{
	boost::trim(text);
	if (text.empty())
	{
		send_to_char("Что Вы хотите сообщить?\r\n", ch);
		return;
	}

	switch (subcmd)
	{
	// своей дружине
	case SCMD_CHANNEL:
	{
		// вспомнить
		snprintf(buf, MAX_STRING_LENGTH, "%s дружине: &R'%s'.&n\r\n", GET_NAME(ch), text.c_str());
		CLAN(ch)->add_remember(buf, Remember::CLAN);

		for (DESCRIPTOR_DATA *d = descriptor_list; d; d = d->next)
		{
			if (d->character && d->character != ch
					&& STATE(d) == CON_PLAYING
					&& CLAN(d->character) == CLAN(ch)
					&& !AFF_FLAGGED(d->character, AFF_DEAFNESS)
					&& !ignores(d->character, ch, IGNORE_CLAN))
			{
				snprintf(buf, MAX_STRING_LENGTH, "%s дружине: %s'%s'.%s\r\n",
						GET_NAME(ch), CCIRED(d->character, C_NRM), text.c_str(), CCNRM(d->character, C_NRM));
				d->character->remember_add(buf, Remember::ALL);
				send_to_char(buf, d->character);
			}
		}
		snprintf(buf, MAX_STRING_LENGTH, "Вы дружине: %s'%s'.%s\r\n", CCIRED(ch, C_NRM), text.c_str(), CCNRM(ch, C_NRM));
		ch->remember_add(buf, Remember::ALL);
		send_to_char(buf, ch);
		break;
	}
	// союзникам
	case SCMD_ACHANNEL:
	{
		// вспомнить
		snprintf(buf, MAX_STRING_LENGTH, "%s союзникам: &G'%s'.&n\r\n", GET_NAME(ch), text.c_str());
		for (ClanListType::iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
		{
			if ((CLAN(ch)->CheckPolitics((*clan)->GetRent())== POLITICS_ALLIANCE
					&& (*clan)->CheckPolitics(CLAN(ch)->GetRent()) == POLITICS_ALLIANCE)
				|| CLAN(ch) == (*clan))
			{
				(*clan)->add_remember(buf, Remember::ALLY);
			}
		}

		for (DESCRIPTOR_DATA *d = descriptor_list; d; d = d->next)
		{
			if (d->character && CLAN(d->character)
					&& STATE(d) == CON_PLAYING
					&& d->character != ch
					&& !AFF_FLAGGED(d->character, AFF_DEAFNESS)
					&& !ignores(d->character, ch, IGNORE_ALLIANCE))
			{
				if (CLAN(ch)->CheckPolitics(CLAN(d->character)->GetRent()) == POLITICS_ALLIANCE
						|| CLAN(ch) == CLAN(d->character))
				{
					// проверка на альянс с обеих сторон, шоб не спамили друг другу на зло
					if ((CLAN(d->character)->CheckPolitics(CLAN(ch)->GetRent()) == POLITICS_ALLIANCE)
						|| CLAN(ch) == CLAN(d->character))
					{
							snprintf(buf, MAX_STRING_LENGTH, "%s союзникам: %s'%s'.%s\r\n",
										 GET_NAME(ch), CCIGRN(d->character, C_NRM), text.c_str(), CCNRM(d->character, C_NRM));
							d->character->remember_add(buf, Remember::ALL);
							send_to_char(buf, d->character);
					}
				}
			}
		}
		snprintf(buf, MAX_STRING_LENGTH, "Вы союзникам: %s'%s'.%s\r\n", CCIGRN(ch, C_NRM), text.c_str(), CCNRM(ch, C_NRM));
		ch->remember_add(buf, Remember::ALL);
		send_to_char(buf, ch);
		break;
	}
	} // switch
}


// обработка клан-канала и канала союзников, как игрока, так и имма
// клановые БОГи ниже 34 не могут говорить другим дружинам, и им и остальным спокойнее
// для канала союзников нужен обоюдный альянс дружин
ACMD(DoClanChannel)
{
	if (IS_NPC(ch))
		return;

	std::string buffer = argument;

	// большой неклановый или 34 клановый БОГ говорит какой-то дружине
	if (IS_IMPL(ch) || (IS_GRGOD(ch) && !CLAN(ch)))
	{
		std::string buffer2;
		GetOneParam(buffer, buffer2);

		ClanListType::const_iterator clan;
		for (clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
			if (CompareParam((*clan)->abbrev, buffer2, 1))
				break;
		if (clan == Clan::ClanList.end())
		{
			if (!CLAN(ch)) // неклановый 34 ошибся аббревиатурой
				send_to_char("Дружина с такой аббревиатурой не найдена.\r\n", ch);
			else   // клановый 34 ошибиться не может, идет в его клан-канал
			{
				buffer = argument; // финт ушами
				CLAN(ch)->CharToChannel(ch, buffer, subcmd);
			}
			return;
		}
		(*clan)->GodToChannel(ch, buffer, subcmd);
		// остальные говорят только в свою дружину
	}
	else
	{
		if (!CLAN(ch))
		{
			send_to_char("Вы не принадлежите ни к одной дружине.\r\n", ch);
			return;
		}
		// ограничения на клан-канал не канают на любое звание, если это БОГ
		if (!IS_IMMORTAL(ch) && (!(CLAN(ch))->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_CHANNEL] || PLR_FLAGGED(ch, PLR_DUMB)))
		{
			send_to_char("Вы не можете пользоваться каналом дружины.\r\n", ch);
			return;
		}
		CLAN(ch)->CharToChannel(ch, buffer, subcmd);
	}
}

// список зарегестрированных дружин с их онлайновым составом (опционально)
ACMD(DoClanList)
{
	if (IS_NPC(ch))
		return;

	std::string buffer = argument;
	boost::trim_if(buffer, boost::is_any_of(" \'"));
	ClanListType::const_iterator clan;

	if (buffer.empty())
	{
		// сортировка кланов по экспе
		std::multimap<long long, ClanPtr> sort_clan;
		for (clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
		{
			if (!(*clan)->test_clan)
				sort_clan.insert(std::make_pair((*clan)->last_exp.get_exp(), *clan));
			else
				sort_clan.insert(std::make_pair(0, *clan));
		}

		std::ostringstream out;
		boost::format clanTopFormat(" %5d  %6s   %-30s %14s%14s %9d\r\n");
		out << "В игре зарегистрированы следующие дружины:\r\n"
		<< "     #           Название                          Всего опыта    За 30 дней   Человек\r\n\r\n";
		int count = 1;
		for (std::multimap<long long, ClanPtr>::reverse_iterator it = sort_clan.rbegin(); it != sort_clan.rend(); ++it, ++count)
			out << clanTopFormat % count % it->second->abbrev % it->second->name % ExpFormat(it->second->exp) % ExpFormat(it->second->last_exp.get_exp()) % it->second->members.size();
		send_to_char(out.str(), ch);
		return;
	}

	bool all = 0;
	std::vector<CHAR_DATA *> temp_list;

	for (clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
		if (CompareParam(buffer, (*clan)->abbrev))
			break;
	if (clan == Clan::ClanList.end())
	{
		if (CompareParam(buffer, "все"))
			all = 1;
		else
		{
			send_to_char("Такая дружина не зарегистрирована\r\n", ch);
			return;
		}
	}
	// строится список членов дружины или всех дружин (по флагу all)
	DESCRIPTOR_DATA *d;
	for (d = descriptor_list; d; d = d->next)
	{
		if (d->character
				&& STATE(d) == CON_PLAYING
				&& CLAN(d->character)
				&& CAN_SEE_CHAR(ch, d->character)
				&& !IS_IMMORTAL(d->character)
				&& !PRF_FLAGGED(d->character, PRF_CODERINFO)
				&& (all || CLAN(d->character) == *clan))
		{
			temp_list.push_back(d->character);
		}
	}

	// до кучи сортировка по рангам
	std::sort(temp_list.begin(), temp_list.end(), Clan::SortRank());

	std::ostringstream buffer2;
	buffer2 << "В игре зарегистрированы следующие дружины:\r\n" << "     #                  Глава Название\r\n\r\n";
	boost::format clanFormat(" %5d  %6s %15s %s\r\n");
	boost::format memberFormat(" %-10s%s%s%s %s%s%s\r\n");
	// если искали конкретную дружину - выводим ее
	if (!all)
	{
		buffer2 << clanFormat % 1 % (*clan)->abbrev % (*clan)->owner % (*clan)->name;
		for (std::vector < CHAR_DATA * >::const_iterator it = temp_list.begin(); it != temp_list.end(); ++it)
			buffer2 << memberFormat % (*clan)->ranks[CLAN_MEMBER(*it)->rank_num]
			% CCPK(ch, C_NRM, *it) % noclan_title(*it)
			% CCNRM(ch, C_NRM) % CCIRED(ch, C_NRM)
			% (PLR_FLAGGED(*it, PLR_KILLER) ? "(ДУШЕГУБ)" : "")
			% CCNRM(ch, C_NRM);
	}
	// просто выводим все дружины и всех членов (без параметра 'все' в списке будут только дружины)
	else
	{
		int count = 1;
		for (clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan, ++count)
		{
			buffer2 << clanFormat % count % (*clan)->abbrev % (*clan)->owner % (*clan)->name;
			for (std::vector < CHAR_DATA * >::const_iterator it = temp_list.begin(); it != temp_list.end(); ++it)
				if (CLAN(*it) == *clan)
					buffer2 << memberFormat % (*clan)->ranks[CLAN_MEMBER(*it)->rank_num]
					% CCPK(ch, C_NRM, *it) % noclan_title(*it)
					% CCNRM(ch, C_NRM) % CCIRED(ch, C_NRM)
					% (PLR_FLAGGED(*it, PLR_KILLER) ? "(ДУШЕГУБ)" : "")
					% CCNRM(ch, C_NRM);
		}
	}
	buffer2 << "\r\nВсего игроков - " << temp_list.size() << "\r\n";
	send_to_char(buffer2.str(), ch);
}

// возвращает состояние политики clan по отношению к victim
// при отсутствии подразумевается нейтралитет
int Clan::CheckPolitics(int victim)
{
	ClanPolitics::const_iterator it = politics.find(victim);
	if (it != politics.end())
		return it->second;
	return POLITICS_NEUTRAL;
}

// выставляем клану политику(state) по отношению к victim
// нейтралитет означает просто удаление записи, если она была
void Clan::SetPolitics(int victim, int state)
{
	ClanPolitics::iterator it = politics.find(victim);
	if (it == politics.end() && state == POLITICS_NEUTRAL)
		return;
	if (state == POLITICS_NEUTRAL)
		politics.erase(it);
	else
		politics[victim] = state;
}

const char *politicsnames[] = { "Нейтралитет", "Война", "Альянс" };

// выводим информацию об отношениях дружин между собой
ACMD(DoShowPolitics)
{
	if (IS_NPC(ch) || !CLAN(ch))
	{
		send_to_char("Чаво ?\r\n", ch);
		return;
	}

	std::string buffer = argument;
	boost::trim_if(buffer, boost::is_any_of(" \'"));
	if (!buffer.empty() && CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_POLITICS])
	{
		CLAN(ch)->ManagePolitics(ch, buffer);
		return;
	}
	boost::format strFormat("  %-3s             %s%-11s%s                 %s%-11s%s\r\n");
	int p1 = 0, p2 = 0;

	std::ostringstream buffer2;
	buffer2 << "Отношения Вашей дружины с другими дружинами:\r\n" <<
	"Название     Отношение Вашей дружины     Отношение к Вашей дружине\r\n";

	ClanListType::const_iterator clanVictim;
	for (clanVictim = Clan::ClanList.begin(); clanVictim != Clan::ClanList.end(); ++clanVictim)
	{
		if (*clanVictim == CLAN(ch))
			continue;
		p1 = CLAN(ch)->CheckPolitics((*clanVictim)->rent);
		p2 = (*clanVictim)->CheckPolitics(CLAN(ch)->rent);
		buffer2 << strFormat % (*clanVictim)->abbrev
		% (p1 == POLITICS_WAR ? CCIRED(ch, C_NRM) : (p1 == POLITICS_ALLIANCE ? CCGRN(ch, C_NRM) : CCNRM(ch, C_NRM)))
		% politicsnames[p1] % CCNRM(ch, C_NRM)
		% (p2 == POLITICS_WAR ? CCIRED(ch, C_NRM) : (p2 == POLITICS_ALLIANCE ? CCGRN(ch, C_NRM) : CCNRM(ch, C_NRM)))
		% politicsnames[p2] % CCNRM(ch, C_NRM);
	}
	send_to_char(buffer2.str(), ch);
}

// выставляем политику с уведомлением другой дружины о войне или альянсе
void Clan::ManagePolitics(CHAR_DATA * ch, std::string & buffer)
{
	std::string buffer2;
	GetOneParam(buffer, buffer2);
	DESCRIPTOR_DATA *d;

	ClanListType::const_iterator vict;

	for (vict = Clan::ClanList.begin(); vict != Clan::ClanList.end(); ++vict)
		if (CompareParam(buffer2, (*vict)->abbrev))
			break;
	if (vict == Clan::ClanList.end())
	{
		send_to_char("Нет такой дружины.\r\n", ch);
		return;
	}
	if (*vict == CLAN(ch))
	{
		send_to_char("Менять политику по отношению к своей дружине? Что за бред...\r\n", ch);
		return;
	}
	GetOneParam(buffer, buffer2);
	if (buffer2.empty())
		send_to_char("Укажите действие: нейтралитет|война|альянс.\r\n", ch);
	else if (CompareParam(buffer2, "нейтралитет"))
	{
		if (CheckPolitics((*vict)->rent) == POLITICS_NEUTRAL)
		{
			send_to_char(ch, "Ваша дружина уже находится в состоянии нейтралитета с дружиной %s.\r\n", (*vict)->abbrev.c_str());
			return;
		}
		SetPolitics((*vict)->rent, POLITICS_NEUTRAL);
		set_wait(ch, 1, FALSE);
		// уведомляем обе дружины
		for (d = descriptor_list; d; d = d->next)
		{
			if (d->character && STATE(d) == CON_PLAYING && PRF_FLAGGED(d->character, PRF_POLIT_MODE))
			{
				if (CLAN(d->character) == *vict)
					send_to_char(d->character, "%sДружина %s заключила с Вашей дружиной нейтралитет!%s\r\n", CCWHT(d->character, C_NRM), this->abbrev.c_str(), CCNRM(d->character, C_NRM));
				else if (CLAN(d->character) == CLAN(ch))
					send_to_char(d->character, "%sВаша дружина заключила с дружиной %s нейтралитет!%s\r\n", CCWHT(d->character, C_NRM), (*vict)->abbrev.c_str(), CCNRM(d->character, C_NRM));
			}
		}
		if (!PRF_FLAGGED(ch, PRF_POLIT_MODE)) // а то сам может не увидеть нафик
			send_to_char(ch, "%sВаша дружина заключила с дружиной %s нейтралитет!%s\r\n", CCWHT(ch, C_NRM), (*vict)->abbrev.c_str(), CCNRM(ch, C_NRM));

	}
	else if (CompareParam(buffer2, "война"))
	{
		if (CheckPolitics((*vict)->rent) == POLITICS_WAR)
		{
			send_to_char(ch, "Ваша дружина уже воюет с дружиной %s.\r\n", (*vict)->abbrev.c_str());
			return;
		}
		SetPolitics((*vict)->rent, POLITICS_WAR);
		set_wait(ch, 1, FALSE);
		// тож самое
		for (d = descriptor_list; d; d = d->next)
		{
			if (d->character && STATE(d) == CON_PLAYING && PRF_FLAGGED(d->character, PRF_POLIT_MODE))
			{
				if (CLAN(d->character) == *vict)
					send_to_char(d->character, "%sДружина %s объявила вашей дружине войну!%s\r\n", CCIRED(d->character, C_NRM), this->abbrev.c_str(), CCNRM(d->character, C_NRM));
				else if (CLAN(d->character) == CLAN(ch))
					send_to_char(d->character, "%sВаша дружина объявила дружине %s войну!%s\r\n", CCIRED(d->character, C_NRM), (*vict)->abbrev.c_str(), CCNRM(d->character, C_NRM));
			}
		}
		if (!PRF_FLAGGED(ch, PRF_POLIT_MODE))
			send_to_char(ch, "%sВаша дружина объявила дружине %s войну!%s\r\n", CCIRED(ch, C_NRM), (*vict)->abbrev.c_str(), CCNRM(ch, C_NRM));

	}
	else if (CompareParam(buffer2, "альянс"))
	{
		if (CheckPolitics((*vict)->rent) == POLITICS_ALLIANCE)
		{
			send_to_char(ch, "Ваша дружина уже в альянсе с дружиной %s.\r\n", (*vict)->abbrev.c_str());
			return;
		}
		set_wait(ch, 1, FALSE);
		SetPolitics((*vict)->rent, POLITICS_ALLIANCE);
		// тож самое
		for (d = descriptor_list; d; d = d->next)
		{
			if (d->character && STATE(d) == CON_PLAYING && PRF_FLAGGED(d->character, PRF_POLIT_MODE))
			{
				if (CLAN(d->character) == *vict)
					send_to_char(d->character, "%sДружина %s заключила с вашей дружиной альянс!%s\r\n", CCGRN(d->character, C_NRM), this->abbrev.c_str(), CCNRM(d->character, C_NRM));
				else if (CLAN(d->character) == CLAN(ch))
					send_to_char(d->character, "%sВаша дружина заключила альянс с дружиной %s!%s\r\n", CCGRN(d->character, C_NRM), (*vict)->abbrev.c_str(), CCNRM(d->character, C_NRM));
			}
		}
		if (!PRF_FLAGGED(ch, PRF_POLIT_MODE))
			send_to_char(ch, "%sВаша дружина заключила альянс с дружиной %s!%s\r\n", CCGRN(ch, C_NRM), (*vict)->abbrev.c_str(), CCNRM(ch, C_NRM));
	}
}

const char *HCONTROL_FORMAT =
	"Формат: hcontrol build <rent vnum> <outrent vnum> <guard vnum> <leader name> <abbreviation> <clan title> <clan name>\r\n"
	"        hcontrol show\r\n"
	"        hcontrol destroy <house vnum>\r\n"
	"        hcontrol outcast <name>\r\n"
	"        hcontrol save\r\n";

// божественный hcontrol
ACMD(DoHcontrol)
{
	if (IS_NPC(ch))
		return;

	std::string buffer = argument, buffer2;
	GetOneParam(buffer, buffer2);

	if (CompareParam(buffer2, "build") && !buffer.empty())
		Clan::HcontrolBuild(ch, buffer);
	else if (CompareParam(buffer2, "destroy") && !buffer.empty())
		Clan::HcontrolDestroy(ch, buffer);
	else if (CompareParam(buffer2, "outcast") && !buffer.empty())
		Clan::hcon_outcast(ch, buffer);
	else if (CompareParam(buffer2, "show"))
		Clan::HconShow(ch);
	else if (CompareParam(buffer2, "save"))
	{
		Clan::ClanSave();
		Clan::ChestUpdate();
		Clan::ChestSave();
	}
	else
		send_to_char(HCONTROL_FORMAT, ch);
}

// создание дружины (hcontrol build)
void Clan::HcontrolBuild(CHAR_DATA * ch, std::string & buffer)
{
	// парсим все параметры, чтобы сразу проверить на ввод всех полей
	std::string buffer2;
	// рента
	GetOneParam(buffer, buffer2);
	int rent = atoi(buffer2.c_str());
	// рента вне замка
	GetOneParam(buffer, buffer2);
	int out_rent = atoi(buffer2.c_str());
	// охранник
	GetOneParam(buffer, buffer2);
	mob_vnum guard = atoi(buffer2.c_str());
	// воевода
	std::string owner;
	GetOneParam(buffer, owner);
	// аббревиатура
	std::string abbrev;
	GetOneParam(buffer, abbrev);
	// приписка в титул
	std::string title;
	GetOneParam(buffer, title);
	// название клана
	boost::trim_if(buffer, boost::is_any_of(" \'"));
	std::string name = buffer;

	// тут проверяем наличие все этого дела
	if (name.empty())
	{
		send_to_char(HCONTROL_FORMAT, ch);
		return;
	}
	if (!real_room(rent))
	{
		send_to_char(ch, "Комнаты %d не существует.\r\n", rent);
		return;
	}
	if (!real_room(out_rent))
	{
		send_to_char(ch, "Комнаты %d не существует.\r\n", out_rent);
		return;
	}
	if (real_mobile(guard) < 0)
	{
		send_to_char(ch, "Моба %d не существует.\r\n", guard);
		return;
	}
	long unique = 0;
	if (!(unique = GetUniqueByName(owner)))
	{
		send_to_char(ch, "Персонажа %s не существует.\r\n", owner.c_str());
		return;
	}
	// а тут - не занят ли параметр другим кланом
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
	{
		if ((*clan)->rent == rent)
		{
			send_to_char(ch, "Комната %d уже занята другой дружиной.\r\n", rent);
			return;
		}
		if ((*clan)->guard == guard)
		{
			send_to_char(ch, "Охранник %d уже занят другой дружиной.\r\n", rent);
			return;
		}
		ClanMemberList::const_iterator it = (*clan)->members.find(unique);
		if (it != (*clan)->members.end())
		{
			send_to_char(ch, "%s уже приписан к дружине %s.\r\n", owner.c_str(), (*clan)->abbrev.c_str());
			return;
		}
		if (CompareParam((*clan)->abbrev, abbrev, 1))
		{
			send_to_char(ch, "Аббревиатура '%s' уже занята другой дружиной.\r\n", abbrev.c_str());
			return;
		}
		if (CompareParam((*clan)->name, name, 1))
		{
			send_to_char(ch, "Имя '%s' уже занято другой дружиной.\r\n", name.c_str());
			return;
		}
		if (CompareParam((*clan)->title, title, 1))
		{
			send_to_char(ch, "Приписка в титул '%s' уже занята другой дружиной.\r\n", title.c_str());
			return;
		}
	}

	// собственно клан
	ClanPtr tempClan(new Clan);
	tempClan->rent = rent;
	tempClan->out_rent = out_rent;
	tempClan->chest_room = rent;
	tempClan->guard = guard;
	// пишем воеводу
	owner[0] = UPPER(owner[0]);
	tempClan->owner = owner;
	ClanMemberPtr tempMember(new ClanMember);
	tempMember->name = owner;
	tempClan->members[unique] = tempMember;
	// названия
	tempClan->abbrev = abbrev;
	tempClan->name = name;
	tempClan->title = title;
	// ранги
	const char *ranks[] = { "воевода", "боярин", "десятник", "храбр", "кметь", "гридень", "муж", "вой", "отрок", "гость" };
	// женский род пока тоже самое, а то воплей будет...
	const char *ranks_female[] = { "воевода", "боярин", "десятник", "храбр", "кметь", "гридень", "муж", "вой", "отрок", "гость" };
	std::vector<std::string> temp_ranks(ranks, ranks + 10);
	std::vector<std::string> temp_ranks_female(ranks_female, ranks_female + 10);
	tempClan->ranks = temp_ranks;
	tempClan->ranks_female = temp_ranks_female;
	// привилегии
	for (std::vector<std::string>::const_iterator it =
				tempClan->ranks.begin(); it != tempClan->ranks.end(); ++it)
		tempClan->privileges.push_back(std::bitset<CLAN_PRIVILEGES_NUM> (0));
	// воеводе проставим все привилегии
	for (int i = 0; i < CLAN_PRIVILEGES_NUM; ++i)
		tempClan->privileges[0].set(i);
	// залоадим сразу хранилище
	OBJ_DATA * chest = read_object(CLAN_CHEST_VNUM, VIRTUAL);
	if (chest)
		obj_to_room(chest, real_room(tempClan->chest_room));

	Clan::ClanList.push_back(tempClan);
	Clan::ClanSave();
	Board::ClanInit();

	// уведомляем счастливых воеводу и имма
	DESCRIPTOR_DATA *d;
	if ((d = DescByUID(unique)))
	{
		Clan::SetClanData(d->character);
		send_to_char(d->character, "Вы стали хозяином нового замка. Добро пожаловать!\r\n");
	}
	send_to_char(ch, "Дружина '%s' создана!\r\n", abbrev.c_str());
}

// удаление дружины (hcontrol destroy)
void Clan::HcontrolDestroy(CHAR_DATA * ch, std::string & buffer)
{
	boost::trim_if(buffer, boost::is_any_of(" \'"));
	int rent = atoi(buffer.c_str());

	ClanListType::iterator clan;
	for (clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
		if ((*clan)->rent == rent)
			break;
	if (clan == Clan::ClanList.end())
	{
		send_to_char(ch, "Дружины с номером %d не существует.\r\n", rent);
		return;
	}

	// если воевода онлайн - обрадуем его лично
	DESCRIPTOR_DATA *d;
	ClanMemberList members = (*clan)->members;
	Clan::ClanList.erase(clan);
	Clan::ClanSave();
	Board::ClanInit();
	// TODO: по идее можно сундук и его содержимое пуржить, но не факт, что это хорошо
	// уведомляем и чистим инфу игрокам
	for (ClanMemberList::const_iterator it = members.begin(); it != members.end(); ++it)
	{
		if ((d = DescByUID(it->first)))
		{
			Clan::SetClanData(d->character);
			send_to_char(d->character, "Ваша дружина распущена. Желаем удачи!\r\n");
		}
	}
	send_to_char("Дружина распущена.\r\n", ch);
}

// ктодружина (список соклановцев, находящихся онлайн)
ACMD(DoWhoClan)
{
	if (IS_NPC(ch) || !CLAN(ch))
	{
		send_to_char("Чаво ?\r\n", ch);
		return;
	}

	std::ostringstream buffer;
	buffer << boost::format(" Ваша дружина: %s%s%s.\r\n") % CCIRED(ch, C_NRM) % CLAN(ch)->abbrev % CCNRM(ch, C_NRM);
	buffer << boost::format(" %sСейчас в игре Ваши соратники:%s\r\n\r\n") % CCWHT(ch, C_NRM) % CCNRM(ch, C_NRM);
	DESCRIPTOR_DATA *d;
	int num = 0;

	for (d = descriptor_list, num = 0; d; d = d->next)
		if (d->character && STATE(d) == CON_PLAYING && CLAN(d->character) == CLAN(ch))
		{
			buffer << "    " << race_or_title(d->character) << "\r\n";
			++num;
		}
	buffer << "\r\n Всего: " << num << ".\r\n";
	send_to_char(buffer.str(), ch);
}

const char *CLAN_PKLIST_FORMAT[] =
{
	"Формат: пклист|дрлист (все)\r\n"
	"        пклист|дрлист имя (все)\r\n",
	"        пклист|дрлист добавить имя причина\r\n"
	"        пклист|дрлист удалить имя|все\r\n"
};

/**
* Для клановых пкл/дрл - не показываются чары в состоянии дисконета, кроме находящихся в бд,
* т.е. они стоят где-то в мире полюбому, все остальные состояния считаются как онлайн.
*/
bool check_online_state(long uid)
{
	for (CHAR_DATA *tch = character_list; tch; tch = tch->next)
	{
		if (IS_NPC(tch) || GET_UNIQUE(tch) != uid || (!tch->desc && !RENTABLE(tch))) continue;

		return true;
	}
	return false;
}

/**
* Распечатка пкл/дрл с учетом режима 'пкфортмат'.
*/
void print_pkl(CHAR_DATA *ch, std::ostringstream &stream, ClanPkList::const_iterator &it)
{
	static char timeBuf[11];
	static boost::format frmt("%s [%s] :: %s\r\n%s\r\n\r\n");

	if (PRF_FLAGGED(ch, PRF_PKFORMAT_MODE))
		stream << it->second->victimName << " : " << it->second->text << "\n";
	else
	{
		strftime(timeBuf, sizeof(timeBuf), "%d/%m/%Y", localtime(&(it->second->time)));
		stream << frmt % timeBuf % it->second->authorName % it->second->victimName % it->second->text;
	}
}

// пкл/дрл
ACMD(DoClanPkList)
{
	if (IS_NPC(ch) || !CLAN(ch))
	{
		send_to_char("Чаво ?\r\n", ch);
		return;
	}

	std::string buffer = argument, buffer2;
	GetOneParam(buffer, buffer2);
	std::ostringstream info;

	boost::format frmt("%s [%s] :: %s\r\n%s\r\n\r\n");
	if (buffer2.empty())
	{
		// выводим список тех, кто онлайн
		send_to_char(ch, "%sОтображаются только находящиеся в игре персонажи:%s\r\n\r\n", CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
		ClanPkList::const_iterator it;
		// вобщем чтобы словить чаров с бд, находящихся в лд - придется гонять по чарактер-листу
		for (CHAR_DATA *tch = character_list; tch; tch = tch->next)
		{
			if (IS_NPC(tch) || (!tch->desc && !RENTABLE(tch)))
				continue;
			// пкл
			if (!subcmd)
			{
				it = CLAN(ch)->pkList.find(GET_UNIQUE(tch));
				if (it != CLAN(ch)->pkList.end())
					print_pkl(ch, info, it);
			}
			// дрл
			else
			{
				it = CLAN(ch)->frList.find(GET_UNIQUE(tch));
				if (it != CLAN(ch)->frList.end())
					print_pkl(ch, info, it);
			}
		}
		if (info.str().empty())
			info << "Записи в выбранном списке отсутствуют.\r\n";
		page_string(ch->desc, info.str(), 1);

	}
	else if (CompareParam(buffer2, "все") || CompareParam(buffer2, "all"))
	{
		// выводим весь список
		send_to_char(ch, "%sСписок отображатеся полностью:%s\r\n\r\n", CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
		// пкл
		if (!subcmd)
			for (ClanPkList::const_iterator it = CLAN(ch)->pkList.begin(); it != CLAN(ch)->pkList.end(); ++it)
				print_pkl(ch, info, it);
		// дрл
		else
			for (ClanPkList::const_iterator it = CLAN(ch)->frList.begin(); it != CLAN(ch)->frList.end(); ++it)
				print_pkl(ch, info, it);

		if (info.str().empty())
			info << "Записи в выбранном списке отсутствуют.\r\n";

		page_string(ch->desc, info.str(), 1);

	}
	else if ((CompareParam(buffer2, "добавить") || CompareParam(buffer2, "add")) && CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_PKLIST])
	{
		// добавляем нового
		GetOneParam(buffer, buffer2);
		if (buffer2.empty())
		{
			send_to_char("Кого добавить то?\r\n", ch);
			return;
		}
		long unique = GetUniqueByName(buffer2, 1);

		if (!unique)
		{
			send_to_char("Интересующий Вас персонаж не найден.\r\n", ch);
			return;
		}
		if (unique < 0)
		{
			send_to_char("Не дело это, Богов добалять куда не надо....\r\n", ch);
			return;
		}
		ClanMemberList::const_iterator it = CLAN(ch)->members.find(unique);
		if (it != CLAN(ch)->members.end())
		{
			send_to_char("Давайте не будем засорять список всяким бредом?\r\n", ch);
			return;
		}
		boost::trim_if(buffer, boost::is_any_of(" \'"));
		if (buffer.empty())
		{
			send_to_char("Потрудитесь прокомментировать, за что Вы его так.\r\n", ch);
			return;
		}

		// тож надо проверять
		ClanPkList::iterator it2;
		if (!subcmd)
			it2 = CLAN(ch)->pkList.find(unique);
		else
			it2 = CLAN(ch)->frList.find(unique);
		if ((!subcmd && it2 != CLAN(ch)->pkList.end()) || (subcmd && it2 != CLAN(ch)->frList.end()))
		{
			// уид тут саавсем не обязательно, что валидный
			ClanMemberList::const_iterator rank_it = CLAN(ch)->members.find(it2->second->author);
			if (rank_it != CLAN(ch)->members.end() && rank_it->second->rank_num < CLAN_MEMBER(ch)->rank_num)
			{
				if (!subcmd)
					send_to_char("Ваша жертва уже добавлена в список врагов старшим по званию.\r\n", ch);
				else
					send_to_char("Персонаж уже добавлен в список друзей старшим по званию.\r\n", ch);
				return;
			}
		}

		// собственно пишем новую жертву/друга
		ClanPkPtr tempRecord(new ClanPk);
		tempRecord->author = GET_UNIQUE(ch);
		tempRecord->authorName = GET_NAME(ch);
		tempRecord->victimName = buffer2;
		name_convert(tempRecord->victimName);
		tempRecord->time = time(0);
		tempRecord->text = buffer;
		if (!subcmd)
			CLAN(ch)->pkList[unique] = tempRecord;
		else
			CLAN(ch)->frList[unique] = tempRecord;
		DESCRIPTOR_DATA *d;
		if ((d = DescByUID(unique)) && PRF_FLAGGED(d->character, PRF_PKL_MODE))
		{
			if (!subcmd)
				send_to_char(d->character, "%sДружина '%s' добавила Вас в список своих врагов!%s\r\n", CCIRED(d->character, C_NRM), CLAN(ch)->name.c_str(), CCNRM(d->character, C_NRM));
			else
				send_to_char(d->character, "%sДружина '%s' добавила Вас в список своих друзей!%s\r\n", CCGRN(d->character, C_NRM), CLAN(ch)->name.c_str(), CCNRM(d->character, C_NRM));
			set_wait(ch, 1, FALSE);
		}
		send_to_char("Ладушки, добавили.\r\n", ch);

	}
	else if ((CompareParam(buffer2, "удалить") || CompareParam(buffer2, "delete")) && CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_PKLIST])
	{
		// удаление записи
		GetOneParam(buffer, buffer2);
		if (CompareParam(buffer2, "все", 1) || CompareParam(buffer2, "all", 1))
		{
			if (CLAN_MEMBER(ch)->rank_num)
			{
				send_to_char("Полная очистка списка доступна только воеводе.\r\n", ch);
				return;
			}
			// пкл
			if (!subcmd)
				CLAN(ch)->pkList.erase(CLAN(ch)->pkList.begin(), CLAN(ch)->pkList.end());
			// дрл
			else
				CLAN(ch)->frList.erase(CLAN(ch)->frList.begin(), CLAN(ch)->frList.end());
			send_to_char("Список очищен.\r\n", ch);
			return;
		}
		long unique = GetUniqueByName(buffer2, 1);

		if (unique <= 0)
		{
			send_to_char("Интересующий Вас персонаж не найден.\r\n", ch);
			return;
		}
		ClanPkList::iterator it;
		// пкл, раздельно они мне больше нравятся, чем по пять раз subcmd проверять
		if (!subcmd)
		{
			it = CLAN(ch)->pkList.find(unique);
			if (it != CLAN(ch)->pkList.end())
			{
				ClanMemberList::const_iterator pk_rank_it = CLAN(ch)->members.find(it->second->author);
				if (pk_rank_it != CLAN(ch)->members.end() && pk_rank_it->second->rank_num < CLAN_MEMBER(ch)->rank_num)
				{
					send_to_char("Ваша жертва была добавлена старшим по званию.\r\n", ch);
					return;
				}
				CLAN(ch)->pkList.erase(it);
			}
			// дрл
		}
		else
		{
			it = CLAN(ch)->frList.find(unique);
			if (it != CLAN(ch)->frList.end())
			{
				ClanMemberList::const_iterator fr_rank_it = CLAN(ch)->members.find(it->second->author);
				if (fr_rank_it != CLAN(ch)->members.end() && fr_rank_it->second->rank_num < CLAN_MEMBER(ch)->rank_num)
				{
					send_to_char("Персонаж был добавлен старшим по званию.\r\n", ch);
					return;
				}
				CLAN(ch)->frList.erase(it);
			}
		}

		if (it != CLAN(ch)->pkList.end() && it != CLAN(ch)->frList.end())
		{
			send_to_char("Запись удалена.\r\n", ch);
			DESCRIPTOR_DATA *d;
			if ((d = DescByUID(unique)) && PRF_FLAGGED(d->character, PRF_PKL_MODE))
			{
				if (!subcmd)
					send_to_char(d->character, "%sДружина '%s' удалила Вас из списка своих врагов!%s\r\n", CCGRN(d->character, C_NRM), CLAN(ch)->name.c_str(), CCNRM(d->character, C_NRM));
				else
					send_to_char(d->character, "%sДружина '%s' удалила Вас из списка своих друзей!%s\r\n", CCIRED(d->character, C_NRM), CLAN(ch)->name.c_str(), CCNRM(d->character, C_NRM));
				set_wait(ch, 1, FALSE);
			}
		}
		else
			send_to_char("Запись не найдена.\r\n", ch);

	}
	else
	{
		boost::trim(buffer);
		bool online = 1;

		if (CompareParam(buffer, "all") || CompareParam(buffer, "все"))
			online = 0;

		if (online)
			send_to_char(ch, "%sОтображаются только находящиеся в игре персонажи:%s\r\n\r\n", CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
		else
			send_to_char(ch, "%sСписок отображатеся полностью:%s\r\n\r\n", CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));

		std::ostringstream out;

		if (!subcmd)
		{
			for (ClanPkList::const_iterator it = CLAN(ch)->pkList.begin(); it != CLAN(ch)->pkList.end(); ++it)
			{
				if (CompareParam(buffer2, it->second->victimName, 1))
				{
					if (!online || check_online_state(it->first))
						print_pkl(ch, out, it);
				}
			}
		}
		else
		{
			for (ClanPkList::const_iterator it = CLAN(ch)->frList.begin(); it != CLAN(ch)->frList.end(); ++it)
			{
				if (CompareParam(buffer2, it->second->victimName, 1))
				{
					if (!online || check_online_state(it->first))
						print_pkl(ch, out, it);
				}
			}
		}
		if (!out.str().empty())
			page_string(ch->desc, out.str(), 1);
		else
		{
			send_to_char("По Вашему запросу никого не найдено.\r\n", ch);
			if (CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_PKLIST])
			{
				buffer = CLAN_PKLIST_FORMAT[0];
				buffer += CLAN_PKLIST_FORMAT[1];
				send_to_char(buffer, ch);
			}
			else
				send_to_char(CLAN_PKLIST_FORMAT[0], ch);
		}
	}
}

// кладем в сундук (при наличии привилегии)
// если предмет - деньги, то автоматом идут в клан-казну, контейнеры только пустые
bool Clan::PutChest(CHAR_DATA * ch, OBJ_DATA * obj, OBJ_DATA * chest)
{
	if (IS_NPC(ch) || !CLAN(ch)
			|| real_room(CLAN(ch)->chest_room) != IN_ROOM(ch)
			|| !CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_CHEST_PUT])
	{
		send_to_char("Не имеете таких правов!\r\n", ch);
		return 0;
	}

	if (GET_OBJ_TYPE(obj) == ITEM_MONEY)
	{
		long gold = GET_OBJ_VAL(obj, 0);
		// здесь и далее: в случае переполнения  - кладем сколько можем, остальное возвращаем чару
		// на данный момент для персонажей и кланов максимум 2147483647 кун
		// не знаю как сейчас в маде с деньгами и есть ли смысл поднимать планку
		if ((CLAN(ch)->bank + gold) < 0)
		{
			long over = std::numeric_limits<long int>::max() - CLAN(ch)->bank;
			CLAN(ch)->bank += over;
			CLAN(ch)->members[GET_UNIQUE(ch)]->money += over;
			gold -= over;
			add_gold(ch, gold);
			obj_from_char(obj);
			extract_obj(obj);
			send_to_char(ch, "Вы удалось вложить в казну дружины только %ld %s.\r\n", over, desc_count(over, WHAT_MONEYu));
			return 1;
		}
		CLAN(ch)->bank += gold;
		CLAN(ch)->members[GET_UNIQUE(ch)]->money += gold;
		obj_from_char(obj);
		extract_obj(obj);
		send_to_char(ch, "Вы вложили в казну дружины %ld %s.\r\n", gold, desc_count(gold, WHAT_MONEYu));

	}
	else if (IS_OBJ_STAT(obj, ITEM_NODROP) || OBJ_FLAGGED(obj, ITEM_ZONEDECAY) || GET_OBJ_TYPE(obj) == ITEM_KEY || IS_OBJ_STAT(obj, ITEM_NORENT) || GET_OBJ_RENT(obj) < 0 || GET_OBJ_RNUM(obj) <= NOTHING)
		act("Неведомая сила помешала положить $o3 в $O3.", FALSE, ch, obj, chest, TO_CHAR);
	else if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && obj->contains)
		act("В $o5 что-то лежит.", FALSE, ch, obj, 0, TO_CHAR);
	else
	{
		if ((GET_OBJ_WEIGHT(chest) + GET_OBJ_WEIGHT(obj)) > CLAN(ch)->ChestMaxWeight() ||
				CLAN(ch)->chest_objcount == CLAN(ch)->ChestMaxObjects())
		{
			act("Вы попытались запихнуть $o3 в $O3, но не смогли - там просто нет места.", FALSE, ch, obj, chest, TO_CHAR);
			return 0;
		}
		obj_from_char(obj);
		obj_to_obj(obj, chest);

		// канал хранилища
		for (DESCRIPTOR_DATA *d = descriptor_list; d; d = d->next)
			if (d->character && STATE(d) == CON_PLAYING && !AFF_FLAGGED(d->character, AFF_DEAFNESS) && CLAN(d->character) && CLAN(d->character) == CLAN(ch) && PRF_FLAGGED(d->character, PRF_TAKE_MODE))
				send_to_char(d->character, "[Хранилище]: %s'%s сдал%s %s.'%s\r\n", CCIRED(d->character, C_NRM), GET_NAME(ch), GET_CH_SUF_1(ch), obj->PNames[3], CCNRM(d->character, C_NRM));

		if (!PRF_FLAGGED(ch, PRF_DECAY_MODE))
			act("Вы положили $o3 в $O3.", FALSE, ch, obj, chest, TO_CHAR);
		CLAN(ch)->chest_objcount++;
	}
	return 1;
}

// берем из клан-сундука (при наличии привилегии)
bool Clan::TakeChest(CHAR_DATA * ch, OBJ_DATA * obj, OBJ_DATA * chest)
{
	if (IS_NPC(ch) || !CLAN(ch)
			|| real_room(CLAN(ch)->chest_room) != IN_ROOM(ch)
			|| !CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_CHEST_TAKE])
	{
		send_to_char("Не имеете таких правов!\r\n", ch);
		return 0;
	}

	obj_from_obj(obj);
	obj_to_char(obj, ch);
	if (obj->carried_by == ch)
	{
		// канал хранилища
		for (DESCRIPTOR_DATA *d = descriptor_list; d; d = d->next)
			if (d->character && STATE(d) == CON_PLAYING && !AFF_FLAGGED(d->character, AFF_DEAFNESS) && CLAN(d->character) && CLAN(d->character) == CLAN(ch) && PRF_FLAGGED(d->character, PRF_TAKE_MODE))
				send_to_char(d->character, "[Хранилище]: %s'%s забрал%s %s.'%s\r\n", CCIRED(d->character, C_NRM), GET_NAME(ch), GET_CH_SUF_1(ch), obj->PNames[3], CCNRM(d->character, C_NRM));

		if (!PRF_FLAGGED(ch, PRF_TAKE_MODE))
			act("Вы взяли $o3 из $O1.", FALSE, ch, obj, chest, TO_CHAR);
		CLAN(ch)->chest_objcount--;
	}
	return 1;
}

// сохраняем все сундуки в файлы
// пользует write_one_object (мне кажется это разуменее, чем плодить свои форматы везде и потом
// заниматься с ними сексом при изменении параметров на шмотках)
void Clan::ChestSave()
{
	OBJ_DATA *chest, *temp;

	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
	{
		std::string buffer = (*clan)->abbrev;
		for (unsigned i = 0; i != buffer.length(); ++i)
			buffer[i] = LOWER(AtoL(buffer[i]));
		std::string filename = LIB_HOUSE + buffer + "/" + buffer + ".obj";
		for (chest = world[real_room((*clan)->chest_room)]->contents; chest; chest = chest->next_content)
			if (Clan::is_clan_chest(chest))
			{
				std::string buffer2;
				for (temp = chest->contains; temp; temp = temp->next_content)
				{
					char databuf[MAX_STRING_LENGTH];
					char *data = databuf;

					write_one_object(&data, temp, 0);
					buffer2 += databuf;
				}

				std::ofstream file(filename.c_str());
				if (!file.is_open())
				{
					log("Error open file: %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
					return;
				}
				file << "* Items file\n" << buffer2 << "\n$\n$\n";
				file.close();
				break;
			}
	}
}

// чтение файлов клановых сундуков
// пользует read_one_object_new для чтения шмоток плееров в ренте
void Clan::ChestLoad()
{
	OBJ_DATA *obj, *chest, *temp, *obj_next;

	// TODO: при сильном желании тут можно пробегать все зоны замков или вообще все зоны/предметы и пуржить все chest
	// предметы и их содержимое, на случай релоада кланов и изменения комнаты с хранилищем (чтобы в маде не пуржить руками)

	// на случай релоада - чистим перед этим все что было в сундуках
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
	{
		for (chest = world[real_room((*clan)->chest_room)]->contents; chest; chest = chest->next_content)
		{
			if (Clan::is_clan_chest(chest))
			{
				for (temp = chest->contains; temp; temp = obj_next)
				{
					obj_next = temp->next_content;
					obj_from_obj(temp);
					extract_obj(temp);
				}
				extract_obj(chest);
			}
		}
	}

	int error = 0, fsize = 0;
	FILE *fl;
	char *data, *databuf;
	std::string buffer;

	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
	{
		buffer = (*clan)->abbrev;
		for (unsigned i = 0; i != buffer.length(); ++i)
			buffer[i] = LOWER(AtoL(buffer[i]));
		std::string filename = LIB_HOUSE + buffer + "/" + buffer + ".obj";

		//лоадим сундук. в зонах его лоадить не нужно.
		chest = read_object(CLAN_CHEST_VNUM, VIRTUAL);
		if (chest)
			obj_to_room(chest, real_room((*clan)->chest_room));

		if (!chest)
		{
			log("<Clan> Chest load error '%s'! (%s %s %d)", (*clan)->abbrev.c_str(), __FILE__, __func__, __LINE__);
			return;
		}
		if (!(fl = fopen(filename.c_str(), "r+b")))
			continue;
		fseek(fl, 0L, SEEK_END);
		fsize = ftell(fl);
		if (!fsize)
		{
			fclose(fl);
			log("<Clan> Empty obj file '%s'. (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
			continue;
		}
		databuf = new char [fsize + 1];

		fseek(fl, 0L, SEEK_SET);
		if (!fread(databuf, fsize, 1, fl) || ferror(fl) || !databuf)
		{
			fclose(fl);
			log("<Clan> Error reading obj file '%s'. (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
			continue;
		}
		fclose(fl);

		data = databuf;
		*(data + fsize) = '\0';

		for (fsize = 0; *data && *data != '$'; fsize++)
		{
			if (!(obj = read_one_object_new(&data, &error)))
			{
				if (error)
					log("<Clan> Items reading fail for %s error %d.", filename.c_str(), error);
				continue;
			}
			obj_to_obj(obj, chest);
		}
		delete [] databuf;
	}
}

// смотрим чего в сундуках и берем из казны за ренту
void Clan::ChestUpdate()
{
	double i, cost;

	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
	{
		cost = (*clan)->ChestTax();
		// расчет и снимание за ренту (целой части по возможности) сразу снимем налог
		cost += CLAN_TAX + ((*clan)->storehouse * CLAN_STOREHOUSE_TAX);
		cost = (cost * CHEST_UPDATE_PERIOD) / (60 * 24);

		(*clan)->bankBuffer += cost;
		modf((*clan)->bankBuffer, &i);
		if (i)
		{
			(*clan)->bank -= (long) i;
			(*clan)->bankBuffer -= i;
		}
		// при нулевом счете все шмотки в сундуке пуржим
		// TODO: а тут придется опять искать, ибо выше сундук уже похерен, надо фиксить
		if ((*clan)->bank < 0)
		{
			(*clan)->bank = 0;
			OBJ_DATA *temp, *chest, *obj_next;
			for (chest = world[real_room((*clan)->chest_room)]->contents; chest; chest = chest->next_content)
			{
				if (Clan::is_clan_chest(chest))
				{
					for (temp = chest->contains; temp; temp = obj_next)
					{
						obj_next = temp->next_content;
						obj_from_obj(temp);
						extract_obj(temp);
					}
					break;
				}
			}
		}
	}
}

//изменение налога на свой опыт
void Clan::TaxManage(CHAR_DATA * ch, std::string & arg)
{
	if (IS_NPC(ch) || !CLAN(ch))
		return;
	if (GET_LEVEL(ch) >= LVL_IMMORT)
	{
		send_to_char("Нефик читить.\r\n", ch);
		return;
	}

	std::string buffer = arg, buffer2;
	GetOneParam(buffer, buffer2);
	long value = atol(buffer2.c_str());

	if (value < 0 || value > 100)
	{
		send_to_char("Какой налог вы хотите установить? Пределы от 0 до 100.\r\n", ch);
		return;
	}
	else
	{
		if (value == 0)
			send_to_char("Дружине - ни капли! Ну и жадина же вы.\r\n", ch);
		else if (value < 100)
			send_to_char(ch, "Теперь %d%% вашего опыта идут на благоустройство замка.\r\n"
						 "Напоминаем, что данный опыт не учитывается в топе дружин и пока вообще ничего не дает. Ж)\r\n", value);
		else
			send_to_char(ch, "Вы самоотверженно отдаете весь получаемый опыт своей дружине.\r\n"
						 "Напоминаем, что данный опыт не учитывается в топе дружин и пока вообще ничего не дает. Ж)\r\n");

		CLAN_MEMBER(ch)->exp_persent = value;
	}
}

// казна дружины... команды теже самые с приставкой 'казна' в начале
// смотреть/вкладывать могут все, снимать по привилегии, висит на стандартных банкирах
bool Clan::BankManage(CHAR_DATA * ch, char *arg)
{
	if (IS_NPC(ch) || !CLAN(ch) || GET_LEVEL(ch) >= LVL_IMMORT)
		return 0;

	std::string buffer = arg, buffer2;
	GetOneParam(buffer, buffer2);

	if (CompareParam(buffer2, "баланс") || CompareParam(buffer2, "balance"))
	{
		send_to_char(ch, "На счету Вашей дружины ровно %ld %s.\r\n", CLAN(ch)->bank, desc_count(CLAN(ch)->bank, WHAT_MONEYu));
		return 1;

	}
	else if (CompareParam(buffer2, "вложить") || CompareParam(buffer2, "deposit"))
	{
		GetOneParam(buffer, buffer2);
		long gold = atol(buffer2.c_str());

		if (gold <= 0)
		{
			send_to_char("Сколько Вы хотите вложить?\r\n", ch);
			return 1;
		}
		if (get_gold(ch) < gold)
		{
			send_to_char("О такой сумме Вы можете только мечтать!\r\n", ch);
			return 1;
		}

		// на случай переполнения казны
		if ((CLAN(ch)->bank + gold) < 0)
		{
			long over = std::numeric_limits<long int>::max() - CLAN(ch)->bank;
			CLAN(ch)->bank += over;
			CLAN(ch)->members[GET_UNIQUE(ch)]->money += over;
			add_gold(ch, -over);
			send_to_char(ch, "Вы удалось вложить в казну дружины только %ld %s.\r\n", over, desc_count(over, WHAT_MONEYu));
			act("$n произвел$g финансовую операцию.", TRUE, ch, 0, FALSE, TO_ROOM);
			return 1;
		}

		add_gold(ch, -gold);
		CLAN(ch)->bank += gold;
		CLAN(ch)->members[GET_UNIQUE(ch)]->money += gold;
		send_to_char(ch, "Вы вложили %ld %s.\r\n", gold, desc_count(gold, WHAT_MONEYu));
		act("$n произвел$g финансовую операцию.", TRUE, ch, 0, FALSE, TO_ROOM);
		return 1;

	}
	else if (CompareParam(buffer2, "получить") || CompareParam(buffer2, "withdraw"))
	{
		if (!CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_BANK])
		{
			send_to_char("К сожалению, у Вас нет возможности транжирить средства дружины.\r\n", ch);
			return 1;
		}
		GetOneParam(buffer, buffer2);
		long gold = atol(buffer2.c_str());

		if (gold <= 0)
		{
			send_to_char("Уточните количество денег, которые Вы хотите получить?\r\n", ch);
			return 1;
		}
		if (CLAN(ch)->bank < gold)
		{
			send_to_char("К сожалению, Ваша дружина не так богата.\r\n", ch);
			return 1;
		}

		// на случай переполнения персонажа
		if ((get_gold(ch) + gold) < 0)
		{
			long over = std::numeric_limits<long int>::max() - get_gold(ch);
			add_gold(ch, over);
			CLAN(ch)->bank -= over;
			CLAN(ch)->members[GET_UNIQUE(ch)]->money -= over;
			send_to_char(ch, "Вы удалось снять только %ld %s.\r\n", over, desc_count(over, WHAT_MONEYu));
			act("$n произвел$g финансовую операцию.", TRUE, ch, 0, FALSE, TO_ROOM);
			return 1;
		}

		CLAN(ch)->bank -= gold;
		CLAN(ch)->members[GET_UNIQUE(ch)]->money -= gold;
		add_gold(ch, gold);
		send_to_char(ch, "Вы сняли %ld %s.\r\n", gold, desc_count(gold, WHAT_MONEYu));
		act("$n произвел$g финансовую операцию.", TRUE, ch, 0, FALSE, TO_ROOM);
		return 1;

	}
	else
		send_to_char(ch, "Формат команды: казна вложить|получить|баланс сумма.\r\n");
	return 1;
}

void Clan::Manage(DESCRIPTOR_DATA * d, const char *arg)
{
	unsigned num = atoi(arg);

	switch (d->clan_olc->mode)
	{
	case CLAN_MAIN_MENU:
		switch (*arg)
		{
		case 'в':
		case 'В':
		case 'q':
		case 'Q':
			// есть вариант, что за время в олц в клане изменят кол-во званий
			if (d->clan_olc->clan->privileges.size() != d->clan_olc->privileges.size())
			{
				send_to_char("Во время редактирования привилегий в Вашей дружине было изменено количество званий.\r\n"
							 "Во избежание несоответствий зайдите в меню еще раз.\r\n"
							 "Редактирование отменено.\r\n", d->character);
				d->clan_olc.reset();
				STATE(d) = CON_PLAYING;
				return;
			}
			send_to_char("Вы желаете сохранить изменения? Y(Д)/N(Н) : ", d->character);
			d->clan_olc->mode = CLAN_SAVE_MENU;
			return;
		default:
			if (!*arg || !num)
			{
				send_to_char("Неверный выбор!\r\n", d->character);
				d->clan_olc->clan->MainMenu(d);
				return;
			}
			// тут парсим циферки уже (меню начинается с 1 + ранг ниже себя)
			unsigned choise = num + CLAN_MEMBER(d->character)->rank_num;
			if (choise >= d->clan_olc->clan->ranks.size())
			{
				unsigned i = choise - d->clan_olc->clan->ranks.size();
				if (i == 0)
					d->clan_olc->clan->AllMenu(d, i);
				else if (i == 1)
					d->clan_olc->clan->AllMenu(d, i);
				else if (i == 2 && !CLAN_MEMBER(d->character)->rank_num)
				{
					if (d->clan_olc->clan->storehouse)
						d->clan_olc->clan->storehouse = 0;
					else
						d->clan_olc->clan->storehouse = 1;
					d->clan_olc->clan->MainMenu(d);
					return;
				}
				else if (i == 3 && !CLAN_MEMBER(d->character)->rank_num)
				{
					if (d->clan_olc->clan->exp_info)
						d->clan_olc->clan->exp_info = 0;
					else
						d->clan_olc->clan->exp_info = 1;
					d->clan_olc->clan->MainMenu(d);
					return;
				}
				else
				{
					send_to_char("Неверный выбор!\r\n", d->character);
					d->clan_olc->clan->MainMenu(d);
					return;
				}
				return;
			}

			if (choise >= d->clan_olc->clan->ranks.size())
			{
				send_to_char("Неверный выбор!\r\n", d->character);
				d->clan_olc->clan->MainMenu(d);
				return;
			}
			d->clan_olc->rank = choise;
			d->clan_olc->clan->PrivilegeMenu(d, choise);
			return;
		}
		break;
	case CLAN_PRIVILEGE_MENU:
		switch (*arg)
		{
		case 'в':
		case 'В':
		case 'q':
		case 'Q':
			// выход в общее меню
			d->clan_olc->rank = 0;
			d->clan_olc->mode = CLAN_MAIN_MENU;
			d->clan_olc->clan->MainMenu(d);
			return;
		default:
			if (!*arg)
			{
				send_to_char("Неверный выбор!\r\n", d->character);
				d->clan_olc->clan->PrivilegeMenu(d, d->clan_olc->rank);
				return;
			}
			if (num > CLAN_PRIVILEGES_NUM)
			{
				send_to_char("Неверный выбор!\r\n", d->character);
				d->clan_olc->clan->PrivilegeMenu(d, d->clan_olc->rank);
				return;
			}
			// мы выдаем только доступные привилегии в меню и нормально парсим их тут
			int parse_num;
			for (parse_num = 0; parse_num < CLAN_PRIVILEGES_NUM; ++parse_num)
			{
				if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][parse_num])
				{
					if (!--num)
						break;
				}
			}
			if (d->clan_olc->privileges[d->clan_olc->rank][parse_num])
				d->clan_olc->privileges[d->clan_olc->rank][parse_num] = 0;
			else
				d->clan_olc->privileges[d->clan_olc->rank][parse_num] = 1;
			d->clan_olc->clan->PrivilegeMenu(d, d->clan_olc->rank);
			return;
		}
		break;
	case CLAN_SAVE_MENU:
		switch (*arg)
		{
		case 'y':
		case 'Y':
		case 'д':
		case 'Д':
			d->clan_olc->clan->privileges.clear();
			d->clan_olc->clan->privileges = d->clan_olc->privileges;
			d->clan_olc.reset();
			Clan::ClanSave();
			STATE(d) = CON_PLAYING;
			send_to_char("Изменения сохранены.\r\n", d->character);
			return;
		case 'n':
		case 'N':
		case 'н':
		case 'Н':
			d->clan_olc.reset();
			STATE(d) = CON_PLAYING;
			send_to_char("Редактирование отменено.\r\n", d->character);
			return;
		default:
			send_to_char("Неверный выбор!\r\nВы желаете сохранить изменения? Y(Д)/N(Н) : ", d->character);
			d->clan_olc->mode = CLAN_SAVE_MENU;
			return;
		}
		break;
	case CLAN_ADDALL_MENU:
		switch (*arg)
		{
		case 'в':
		case 'В':
		case 'q':
		case 'Q':
			// выход в общее меню с изменением всех званий
			for (int i = 0; i < CLAN_PRIVILEGES_NUM; ++i)
			{
				if (d->clan_olc->all_ranks[i])
				{
					unsigned j = CLAN_MEMBER(d->character)->rank_num + 1;
					for (; j <= d->clan_olc->clan->ranks.size(); ++j)
					{
						d->clan_olc->privileges[j][i] = 1;
						d->clan_olc->all_ranks[i] = 0;
					}
				}

			}
			d->clan_olc->mode = CLAN_MAIN_MENU;
			d->clan_olc->clan->MainMenu(d);
			return;
		default:
			if (!*arg)
			{
				send_to_char("Неверный выбор!\r\n", d->character);
				d->clan_olc->clan->AllMenu(d, 0);
				return;
			}
			if (num > CLAN_PRIVILEGES_NUM)
			{
				send_to_char("Неверный выбор!\r\n", d->character);
				d->clan_olc->clan->AllMenu(d, 0);
				return;
			}
			int parse_num;
			for (parse_num = 0; parse_num < CLAN_PRIVILEGES_NUM; ++parse_num)
			{
				if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][parse_num])
				{
					if (!--num)
						break;
				}
			}
			if (d->clan_olc->all_ranks[parse_num])
				d->clan_olc->all_ranks[parse_num] = 0;
			else
				d->clan_olc->all_ranks[parse_num] = 1;
			d->clan_olc->clan->AllMenu(d, 0);
			return;
		}
		break;
	case CLAN_DELALL_MENU:
		switch (*arg)
		{
		case 'в':
		case 'В':
		case 'q':
		case 'Q':
			// выход в общее меню с изменением всех званий
			for (int i = 0; i < CLAN_PRIVILEGES_NUM; ++i)
			{
				if (d->clan_olc->all_ranks[i])
				{
					unsigned j = CLAN_MEMBER(d->character)->rank_num + 1;
					for (; j <= d->clan_olc->clan->ranks.size(); ++j)
					{
						d->clan_olc->privileges[j][i] = 0;
						d->clan_olc->all_ranks[i] = 0;
					}
				}

			}
			d->clan_olc->mode = CLAN_MAIN_MENU;
			d->clan_olc->clan->MainMenu(d);
			return;
		default:
			if (!*arg)
			{
				send_to_char("Неверный выбор!\r\n", d->character);
				d->clan_olc->clan->AllMenu(d, 1);
				return;
			}
			if (num > CLAN_PRIVILEGES_NUM)
			{
				send_to_char("Неверный выбор!\r\n", d->character);
				d->clan_olc->clan->AllMenu(d, 1);
				return;
			}
			int parse_num;
			for (parse_num = 0; parse_num < CLAN_PRIVILEGES_NUM; ++parse_num)
			{
				if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][parse_num])
				{
					if (!--num)
						break;
				}
			}
			if (d->clan_olc->all_ranks[parse_num])
				d->clan_olc->all_ranks[parse_num] = 0;
			else
				d->clan_olc->all_ranks[parse_num] = 1;
			d->clan_olc->clan->AllMenu(d, 1);
			return;
		}
		break;
	default:
		log("No case, wtf!? mode: %d (%s %s %d)", d->clan_olc->mode, __FILE__, __func__, __LINE__);
	}
}

void Clan::MainMenu(DESCRIPTOR_DATA * d)
{
	std::ostringstream buffer;
	buffer << "Раздел добавления и удаления привилегий.\r\n"
	<< "Выберите редактируемое звание или действие:\r\n";
	// звания ниже своего
	int rank = CLAN_MEMBER(d->character)->rank_num + 1;
	int num = 0;
	for (std::vector<std::string>::const_iterator it = d->clan_olc->clan->ranks.begin() + rank; it != d->clan_olc->clan->ranks.end(); ++it)
		buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++num << CCNRM(d->character, C_NRM) << ") " << (*it) << "\r\n";
	buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++num << CCNRM(d->character, C_NRM)
	<< ") " << "добавить всем\r\n";
	buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++num << CCNRM(d->character, C_NRM)
	<< ") " << "убрать у всех\r\n";
	if (!CLAN_MEMBER(d->character)->rank_num)
	{
		buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++num << CCNRM(d->character, C_NRM)
		<< ") " << "Выборка из хранилища по параметрам предметов\r\n"
		<< "    + просмотр хранилища вне замка в качестве бонуса (1000 кун в день) ";
		if (this->storehouse)
			buffer << "(отключить)\r\n";
		else
			buffer << "(включить)\r\n";
		buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++num << CCNRM(d->character, C_NRM)
		<< ") " << "Моментально обновлять очки дружины в топе (при отключении обновление раз в 6 часов) ";
		if (this->exp_info)
			buffer << "(отключить)\r\n";
		else
			buffer << "(включить)\r\n";
	}
	buffer << CCGRN(d->character, C_NRM) << " В(Q)" << CCNRM(d->character, C_NRM)
	<< ") Выход\r\n" << "Ваш выбор:";

	send_to_char(buffer.str(), d->character);
	d->clan_olc->mode = CLAN_MAIN_MENU;
}

void Clan::PrivilegeMenu(DESCRIPTOR_DATA * d, unsigned num)
{
	if (num > d->clan_olc->clan->ranks.size())
	{
		log("Different size clan->ranks and clan->privileges! (%s %s %d)", __FILE__, __func__, __LINE__);
		if (d->character)
		{
			d->clan_olc.reset();
			STATE(d) = CON_PLAYING;
			send_to_char(d->character, "Случилось что-то страшное, сообщите Богам!");
		}
		return;
	}
	std::ostringstream buffer;
	buffer << "Список привилегий для звания '" << CCIRED(d->character, C_NRM)
	<< d->clan_olc->clan->ranks[num] << CCNRM(d->character, C_NRM) << "':\r\n";

	int count = 0;
	for (int i = 0; i < CLAN_PRIVILEGES_NUM; ++i)
	{
		switch (i)
		{
		case MAY_CLAN_INFO:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_INFO])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_INFO])
					buffer << "[x] информация о дружине (клан информация)\r\n";
				else
					buffer << "[ ] информация о дружине (клан информация)\r\n";
			}
			break;
		case MAY_CLAN_ADD:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_ADD])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_ADD])
					buffer << "[x] принятие в дружину (клан принять)\r\n";
				else
					buffer << "[ ] принятие в дружину (клан принять)\r\n";
			}
			break;
		case MAY_CLAN_REMOVE:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_REMOVE])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_REMOVE])
					buffer << "[x] изгнание из дружины (клан изгнать)\r\n";
				else
					buffer << "[ ] изгнание из дружины (клан изгнать)\r\n";
			}
			break;
		case MAY_CLAN_PRIVILEGES:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_PRIVILEGES])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_PRIVILEGES])
					buffer << "[x] редактирование привилегий (клан привилегии)\r\n";
				else
					buffer << "[ ] редактирование привилегий (клан привилегии)\r\n";
			}
			break;
		case MAY_CLAN_CHANNEL:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_CHANNEL])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_CHANNEL])
					buffer << "[x] клан-канал (гдругам)\r\n";
				else
					buffer << "[ ] клан-канал (гдругам)\r\n";
			}
			break;
		case MAY_CLAN_POLITICS:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_POLITICS])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_POLITICS])
					buffer << "[x] изменение политики дружины (политика)\r\n";
				else
					buffer << "[ ] изменение политики дружины (политика)\r\n";
			}
			break;
		case MAY_CLAN_NEWS:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_NEWS])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_NEWS])
					buffer << "[x] добавление новостей дружины (дрновости)\r\n";
				else
					buffer << "[ ] добавление новостей дружины (дрновости)\r\n";
			}
			break;
		case MAY_CLAN_PKLIST:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_PKLIST])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_PKLIST])
					buffer << "[x] добавление в пк-лист (пклист)\r\n";
				else
					buffer << "[ ] добавление в пк-лист (пклист)\r\n";
			}
			break;
		case MAY_CLAN_CHEST_PUT:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_CHEST_PUT])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_CHEST_PUT])
					buffer << "[x] класть вещи в хранилище\r\n";
				else
					buffer << "[ ] класть вещи в хранилище\r\n";
			}
			break;
		case MAY_CLAN_CHEST_TAKE:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_CHEST_TAKE])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_CHEST_TAKE])
					buffer << "[x] брать вещи из хранилища\r\n";
				else
					buffer << "[ ] брать вещи из хранилища\r\n";
			}
			break;
		case MAY_CLAN_BANK:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_BANK])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_BANK])
					buffer << "[x] брать из казны дружины\r\n";
				else
					buffer << "[ ] брать из казны дружины\r\n";
			}
			break;
		case MAY_CLAN_EXIT:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_EXIT])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_EXIT])
					buffer << "[x] свободный выход из дружины\r\n";
				else
					buffer << "[ ] свободный выход из дружины\r\n";
			}
			break;
		}
	}
	buffer << CCGRN(d->character, C_NRM) << " В(Q)" << CCNRM(d->character, C_NRM)
	<< ") Выход\r\n" << "Ваш выбор:";
	send_to_char(buffer.str(), d->character);
	d->clan_olc->mode = CLAN_PRIVILEGE_MENU;
}

// меню добавления/удаления привилегий у всех званий, решил не нагружать все в одну функцию
// flag 0 - добавление, 1 - удаление
void Clan::AllMenu(DESCRIPTOR_DATA * d, unsigned flag)
{
	std::ostringstream buffer;
	if (flag == 0)
		buffer << "Выберите привилегии, который Вы хотите " << CCIRED(d->character, C_NRM)
		<< "добавить всем" << CCNRM(d->character, C_NRM) << " званиям:\r\n";
	else
		buffer << "Выберите привилегии, который Вы хотите " << CCIRED(d->character, C_NRM)
		<< "убрать у всех" << CCNRM(d->character, C_NRM) << " званий:\r\n";

	int count = 0;
	for (int i = 0; i < CLAN_PRIVILEGES_NUM; ++i)
	{
		switch (i)
		{
		case MAY_CLAN_INFO:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_INFO])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_INFO])
					buffer << "[x] информация о дружине (клан информация)\r\n";
				else
					buffer << "[ ] информация о дружине (клан информация)\r\n";
			}
			break;
		case MAY_CLAN_ADD:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_ADD])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_ADD])
					buffer << "[x] принятие в дружину (клан принять)\r\n";
				else
					buffer << "[ ] принятие в дружину (клан принять)\r\n";
			}
			break;
		case MAY_CLAN_REMOVE:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_REMOVE])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_REMOVE])
					buffer << "[x] изгнание из дружины (клан изгнать)\r\n";
				else
					buffer << "[ ] изгнание из дружины (клан изгнать)\r\n";
			}
			break;
		case MAY_CLAN_PRIVILEGES:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_PRIVILEGES])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_PRIVILEGES])
					buffer << "[x] редактирование привилегий (клан привилегии)\r\n";
				else
					buffer << "[ ] редактирование привилегий (клан привилегии)\r\n";
			}
			break;
		case MAY_CLAN_CHANNEL:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_CHANNEL])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_CHANNEL])
					buffer << "[x] клан-канал (гдругам)\r\n";
				else
					buffer << "[ ] клан-канал (гдругам)\r\n";
			}
			break;
		case MAY_CLAN_POLITICS:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_POLITICS])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_POLITICS])
					buffer << "[x] изменение политики дружины (политика)\r\n";
				else
					buffer << "[ ] изменение политики дружины (политика)\r\n";
			}
			break;
		case MAY_CLAN_NEWS:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_NEWS])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_NEWS])
					buffer << "[x] добавление новостей дружины (дрновости)\r\n";
				else
					buffer << "[ ] добавление новостей дружины (дрновости)\r\n";
			}
			break;
		case MAY_CLAN_PKLIST:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_PKLIST])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_PKLIST])
					buffer << "[x] добавление в пк-лист (пклист)\r\n";
				else
					buffer << "[ ] добавление в пк-лист (пклист)\r\n";
			}
			break;
		case MAY_CLAN_CHEST_PUT:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_CHEST_PUT])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_CHEST_PUT])
					buffer << "[x] класть вещи в хранилище\r\n";
				else
					buffer << "[ ] класть вещи в хранилище\r\n";
			}
			break;
		case MAY_CLAN_CHEST_TAKE:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_CHEST_TAKE])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_CHEST_TAKE])
					buffer << "[x] брать вещи из хранилища\r\n";
				else
					buffer << "[ ] брать вещи из хранилища\r\n";
			}
			break;
		case MAY_CLAN_BANK:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_BANK])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_BANK])
					buffer << "[x] брать из казны дружины\r\n";
				else
					buffer << "[ ] брать из казны дружины\r\n";
			}
			break;
		case MAY_CLAN_EXIT:
			if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_EXIT])
			{
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_EXIT])
					buffer << "[x] свободный выход из дружины\r\n";
				else
					buffer << "[ ] свободный выход из дружины\r\n";
			}
			break;
		}
	}
	buffer << CCGRN(d->character, C_NRM) << " В(Q)" << CCNRM(d->character, C_NRM)
	<< ") Применить\r\n" << "Ваш выбор:";
	send_to_char(buffer.str(), d->character);
	if (flag == 0)
		d->clan_olc->mode = CLAN_ADDALL_MENU;
	else
		d->clan_olc->mode = CLAN_DELALL_MENU;
}

// игрок ранг
void Clan::ClanAddMember(CHAR_DATA * ch, int rank)
{
	ClanMemberPtr tempMember(new ClanMember);
	tempMember->name = GET_NAME(ch);
	tempMember->rank_num = rank;

	this->members[GET_UNIQUE(ch)] = tempMember;
	Clan::SetClanData(ch);
	DESCRIPTOR_DATA *d;
	std::string buffer;
	for (d = descriptor_list; d; d = d->next)
	{
		if (d->character && CLAN(d->character) && STATE(d) == CON_PLAYING && !AFF_FLAGGED(d->character, AFF_DEAFNESS)
				&& this->GetRent() == CLAN(d->character)->GetRent() && ch != d->character)
		{
			send_to_char(d->character, "%s%s приписан%s к Вашей дружине, статус - '%s'.%s\r\n",
						 CCWHT(d->character, C_NRM), GET_NAME(ch), GET_CH_SUF_6(ch), (this->ranks[rank]).c_str(), CCNRM(d->character, C_NRM));
		}
	}
	send_to_char(ch, "%sВас приписали к дружине '%s', статус - '%s'.%s\r\n", CCWHT(ch, C_NRM), this->name.c_str(), (this->ranks[rank]).c_str(), CCNRM(ch, C_NRM));
	Clan::ClanSave();
	return;
}

// передача воеводства
void Clan::HouseOwner(CHAR_DATA * ch, std::string & buffer)
{
	std::string buffer2;
	GetOneParam(buffer, buffer2);
	long unique = GetUniqueByName(buffer2);
	DESCRIPTOR_DATA *d = DescByUID(unique);

	if (buffer2.empty())
		send_to_char("Укажите имя персонажа.\r\n", ch);
	else if (!unique)
		send_to_char("Неизвестный персонаж.\r\n", ch);
	else if (unique == GET_UNIQUE(ch))
		send_to_char("Сменить себя на самого себя? Вы бредите.\r\n", ch);
	else if (!d || !CAN_SEE(ch, d->character))
		send_to_char("Этого персонажа нет в игре!\r\n", ch);
	else if (CLAN(d->character) && CLAN(ch) != CLAN(d->character))
		send_to_char("Вы не можете передать свои права члену другой дружины.\r\n", ch);
	else
	{
		buffer2[0] = UPPER(buffer2[0]);
		// воевода идет рангом ниже
		this->members[GET_UNIQUE(ch)]->rank_num = 1;
		Clan::SetClanData(ch);
		// ставим нового воеводу (если он был в клане - меняем только ранг)
		if (CLAN(d->character))
		{
			this->members[GET_UNIQUE(d->character)]->rank_num = 0;
			Clan::SetClanData(d->character);
		}
		else
			this->ClanAddMember(d->character, 0);
		this->owner = buffer2;
		send_to_char(ch, "Поздравляем, Вы передали свои полномочия %s!\r\n", GET_PAD(d->character, 2));
	}
}

void Clan::CheckPkList(CHAR_DATA * ch)
{
	if (IS_NPC(ch))
		return;
	ClanPkList::iterator it;
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
	{
		// пкл
		if ((it = (*clan)->pkList.find(GET_UNIQUE(ch))) != (*clan)->pkList.end())
			send_to_char(ch, "Находитесь в списке врагов дружины '%s', добавивший: %s.\r\n", (*clan)->name.c_str(), it->second->authorName.c_str());
		// дрл
		if ((it = (*clan)->frList.find(GET_UNIQUE(ch))) != (*clan)->frList.end())
			send_to_char(ch, "Находитесь в списке друзей дружины '%s', добавивший: %s.\r\n", (*clan)->name.c_str(), it->second->authorName.c_str());
	}
}

// заколебали эти флаги... сравниваем num и все поля в flags
bool CompareBits(FLAG_DATA flags, const char *names[], int affect)
{
	int i;
	for (i = 0; i < 4; i++)
	{
		int nr = 0, fail = 0;
		bitvector_t bitvector = flags.flags[i] | (i << 30);
		if ((unsigned long) bitvector < (unsigned long) INT_ONE);
		else if ((unsigned long) bitvector < (unsigned long) INT_TWO)
			fail = 1;
		else if ((unsigned long) bitvector < (unsigned long) INT_THREE)
			fail = 2;
		else
			fail = 3;
		bitvector &= 0x3FFFFFFF;
		while (fail)
		{
			if (*names[nr] == '\n')
				fail--;
			nr++;
		}

		for (; bitvector; bitvector >>= 1)
		{
			if (IS_SET(bitvector, 1))
				if (*names[nr] != '\n')
					if (nr == affect)
						return 1;
			if (*names[nr] != '\n')
				nr++;
		}
	}
	return 0;
}

// для выборки содержимого хранилища по фильтрам
struct ChestFilter
{
	std::string name;      // имя
	int type;              // тип
	int state;             // состояние
	int wear;              // куда одевается
	int wear_message;      // для названия куда одеть
	int weap_class;        // класс оружие
	int weap_message;      // для названия оружия
	vector<int> affect;    // аффекты weap
	vector<int> affect2;   // аффекты apply
};

// вобщем это копи-паст из биржи + флаги
ACMD(DoStoreHouse)
{
	if (IS_NPC(ch) || !CLAN(ch))
	{
		send_to_char("Чаво ?\r\n", ch);
		return;
	}
	if (!CLAN(ch)->storehouse)
	{
		send_to_char("Ваш воевода зажал денег и отключил эту возможность! :(\r\n", ch);
		return;
	}

	OBJ_DATA *temp_obj, *chest;

	skip_spaces(&argument);
	if (!*argument)
	{
		for (chest = world[real_room(CLAN(ch)->chest_room)]->contents; chest; chest = chest->next_content)
		{
			if (Clan::is_clan_chest(chest))
			{
				Clan::ChestShow(chest, ch);
				return;
			}
		}
	}

	char *stufina = one_argument(argument, arg);
	skip_spaces(&stufina);


	if (is_abbrev(arg, "характеристики") || is_abbrev(arg, "identify") || is_abbrev(arg, "опознать"))
	{
		if ((get_bank_gold(ch) < CHEST_IDENT_PAY) && (GET_LEVEL(ch) < LVL_IMPL))
		{
			send_to_char("У вас недостаточно денег в банке для такого исследования.\r\n", ch);
			return;
		}

		for (chest = world[real_room(CLAN(ch)->chest_room)]->contents; chest; chest = chest->next_content)
		{
			if (Clan::is_clan_chest(chest))
			{
				for (temp_obj = chest->contains; temp_obj; temp_obj = temp_obj->next_content)
				{
					if (isname(stufina, temp_obj->name))
					{
						sprintf(buf1, "Характеристики предмета: %s\r\n", stufina);
						send_to_char(buf1, ch);
						GET_LEVEL(ch) < LVL_IMPL ? mort_show_obj_values(temp_obj, ch, 200) : imm_show_obj_values(temp_obj, ch);
						add_bank_gold(ch, -(CHEST_IDENT_PAY));
						sprintf(buf1, "%sЗа информацию о предмете с вашего банковского счета сняли %d %s%s\r\n",  CCIGRN(ch, C_NRM), CHEST_IDENT_PAY, desc_count(CHEST_IDENT_PAY, WHAT_MONEYu), CCNRM(ch, C_NRM));
						send_to_char(buf1, ch);
						return;
					}
				}
			}
		}

		sprintf(buf1, "Ничего похожего на %s в хранилище ненайдено! Будьте внимательнее.\r\n", stufina);
		send_to_char(buf1, ch);
		return;

	}

	// мож сэйвить надумается или еще что, вобщем объединим все в структурку
	ChestFilter filter;
	filter.type = -1;
	filter.state = -1;
	filter.wear = -1;
	filter.wear_message = -1;
	filter.weap_class = -1;
	filter.weap_message = -1;

	int num = 0;
	bool find = 0;
	char tmpbuf[MAX_INPUT_LENGTH];
	while (*argument)
	{
		switch (*argument)
		{
		case 'И':
			argument = one_argument(++argument, tmpbuf);
			filter.name = tmpbuf;
			break;
		case 'Т':
			argument = one_argument(++argument, tmpbuf);
			if (is_abbrev(tmpbuf, "свет") || is_abbrev(tmpbuf, "light"))
				filter.type = ITEM_LIGHT;
			else if (is_abbrev(tmpbuf, "свиток") || is_abbrev(tmpbuf, "scroll"))
				filter.type = ITEM_SCROLL;
			else if (is_abbrev(tmpbuf, "палочка") || is_abbrev(tmpbuf, "wand"))
				filter.type = ITEM_WAND;
			else if (is_abbrev(tmpbuf, "посох") || is_abbrev(tmpbuf, "staff"))
				filter.type = ITEM_STAFF;
			else if (is_abbrev(tmpbuf, "оружие") || is_abbrev(tmpbuf, "weapon"))
				filter.type = ITEM_WEAPON;
			else if (is_abbrev(tmpbuf, "броня") || is_abbrev(tmpbuf, "armor"))
				filter.type = ITEM_ARMOR;
			else if (is_abbrev(tmpbuf, "напиток") || is_abbrev(tmpbuf, "potion"))
				filter.type = ITEM_POTION;
			else if (is_abbrev(tmpbuf, "прочее") || is_abbrev(tmpbuf, "другое") || is_abbrev(tmpbuf, "other"))
				filter.type = ITEM_OTHER;
			else if (is_abbrev(tmpbuf, "контейнер") || is_abbrev(tmpbuf, "container"))
				filter.type = ITEM_CONTAINER;
			else if (is_abbrev(tmpbuf, "книга") || is_abbrev(tmpbuf, "book"))
				filter.type = ITEM_BOOK;
			else if (is_abbrev(tmpbuf, "руна") || is_abbrev(tmpbuf, "rune"))
				filter.type = ITEM_INGRADIENT;
			else if (is_abbrev(tmpbuf, "ингредиент") || is_abbrev(tmpbuf, "ingradient"))
				filter.type = ITEM_MING;
			else
			{
				send_to_char("Неверный тип предмета.\r\n", ch);
				return;
			}
			break;
		case 'С':
			argument = one_argument(++argument, tmpbuf);
			if (is_abbrev(tmpbuf, "ужасно"))
				filter.state = 0;
			else if (is_abbrev(tmpbuf, "скоро испортится"))
				filter.state = 20;
			else if (is_abbrev(tmpbuf, "плоховато"))
				filter.state = 40;
			else if (is_abbrev(tmpbuf, "средне"))
				filter.state = 60;
			else if (is_abbrev(tmpbuf, "идеально"))
				filter.state = 80;
			else
			{
				send_to_char("Неверное состояние предмета.\r\n", ch);
				return;
			}

			break;
		case 'О':
			argument = one_argument(++argument, tmpbuf);
			if (is_abbrev(tmpbuf, "палец"))
			{
				filter.wear = ITEM_WEAR_FINGER;
				filter.wear_message = 1;
			}
			else if (is_abbrev(tmpbuf, "шея") || is_abbrev(tmpbuf, "грудь"))
			{
				filter.wear = ITEM_WEAR_NECK;
				filter.wear_message = 2;
			}
			else if (is_abbrev(tmpbuf, "тело"))
			{
				filter.wear = ITEM_WEAR_BODY;
				filter.wear_message = 3;
			}
			else if (is_abbrev(tmpbuf, "голова"))
			{
				filter.wear = ITEM_WEAR_HEAD;
				filter.wear_message = 4;
			}
			else if (is_abbrev(tmpbuf, "ноги"))
			{
				filter.wear = ITEM_WEAR_LEGS;
				filter.wear_message = 5;
			}
			else if (is_abbrev(tmpbuf, "ступни"))
			{
				filter.wear = ITEM_WEAR_FEET;
				filter.wear_message = 6;
			}
			else if (is_abbrev(tmpbuf, "кисти"))
			{
				filter.wear = ITEM_WEAR_HANDS;
				filter.wear_message = 7;
			}
			else if (is_abbrev(tmpbuf, "руки"))
			{
				filter.wear = ITEM_WEAR_ARMS;
				filter.wear_message = 8;
			}
			else if (is_abbrev(tmpbuf, "щит"))
			{
				filter.wear = ITEM_WEAR_SHIELD;
				filter.wear_message = 9;
			}
			else if (is_abbrev(tmpbuf, "плечи"))
			{
				filter.wear = ITEM_WEAR_ABOUT;
				filter.wear_message = 10;
			}
			else if (is_abbrev(tmpbuf, "пояс"))
			{
				filter.wear = ITEM_WEAR_WAIST;
				filter.wear_message = 11;
			}
			else if (is_abbrev(tmpbuf, "запястья"))
			{
				filter.wear = ITEM_WEAR_WRIST;
				filter.wear_message = 12;
			}
			else if (is_abbrev(tmpbuf, "правая"))
			{
				filter.wear = ITEM_WEAR_WIELD;
				filter.wear_message = 13;
			}
			else if (is_abbrev(tmpbuf, "левая"))
			{
				filter.wear = ITEM_WEAR_HOLD;
				filter.wear_message = 14;
			}
			else if (is_abbrev(tmpbuf, "обе"))
			{
				filter.wear = ITEM_WEAR_BOTHS;
				filter.wear_message = 15;
			}
			else
			{
				send_to_char("Неверное место одевания предмета.\r\n", ch);
				return;
			}
			break;
		case 'К':
			argument = one_argument(++argument, tmpbuf);
			if (is_abbrev(tmpbuf, "луки"))
			{
				filter.weap_class = SKILL_BOWS;
				filter.weap_message = 0;
			}
			else if (is_abbrev(tmpbuf, "короткие"))
			{
				filter.weap_class = SKILL_SHORTS;
				filter.weap_message = 1;
			}
			else if (is_abbrev(tmpbuf, "длинные"))
			{
				filter.weap_class = SKILL_LONGS;
				filter.weap_message = 2;
			}
			else if (is_abbrev(tmpbuf, "секиры"))
			{
				filter.weap_class = SKILL_AXES;
				filter.weap_message = 3;
			}
			else if (is_abbrev(tmpbuf, "палицы"))
			{
				filter.weap_class = SKILL_CLUBS;
				filter.weap_message = 4;
			}
			else if (is_abbrev(tmpbuf, "иное"))
			{
				filter.weap_class = SKILL_NONSTANDART;
				filter.weap_message = 5;
			}
			else if (is_abbrev(tmpbuf, "двуручники"))
			{
				filter.weap_class = SKILL_BOTHHANDS;
				filter.weap_message = 6;
			}
			else if (is_abbrev(tmpbuf, "проникающее"))
			{
				filter.weap_class = SKILL_PICK;
				filter.weap_message = 7;
			}
			else if (is_abbrev(tmpbuf, "копья"))
			{
				filter.weap_class = SKILL_SPADES;
				filter.weap_message = 8;
			}
			else
			{
				send_to_char("Неверный класс оружия.\r\n", ch);
				return;
			}
			break;
		case 'А':
			argument = one_argument(++argument, tmpbuf);
			if (filter.affect.size() + filter.affect2.size() >= 2)
				break;
			find = 0;
			num = 0;
			for (int flag = 0; flag < 4; ++flag)
			{
				for (/* тут ничего не надо */; *weapon_affects[num] != '\n'; ++num)
				{
					if (is_abbrev(tmpbuf, weapon_affects[num]))
					{
						filter.affect.push_back(num);
						find = 1;
						break;
					}
				}
				if (find)
					break;
				++num;
			}
			if (!find)
			{
				for (num = 0; *apply_types[num] != '\n'; ++num)
				{
					if (is_abbrev(tmpbuf, apply_types[num]))
					{
						filter.affect2.push_back(num);
						find = 1;
						break;
					}
				}
			}
			if (!find)
			{
				send_to_char("Неверный аффект предмета.\r\n", ch);
				return;
			}
			break;
		default:
			++argument;
		}
	}
	std::string buffer = "Выборка по следующим параметрам: ";
	if (!filter.name.empty())
		buffer += filter.name + " ";
	if (filter.type >= 0)
	{
		buffer += item_types[filter.type];
		buffer += " ";
	}
	if (filter.state >= 0)
	{
		if (!filter.state)
			buffer += "ужасно ";
		else if (filter.state == 20)
			buffer += "скоро испортится ";
		else if (filter.state == 40)
			buffer += "плоховато ";
		else if (filter.state == 60)
			buffer += "средне ";
		else if (filter.state == 80)
			buffer += "идеально ";
	}
	if (filter.wear >= 0)
	{
		buffer += wear_bits[filter.wear_message];
		buffer += " ";
	}
	if (filter.weap_class >= 0)
	{
		buffer += weapon_class[filter.weap_message];
		buffer += " ";
	}
	if (!filter.affect.empty())
	{
		for (vector<int>::const_iterator it = filter.affect.begin(); it != filter.affect.end(); ++it)
		{
			buffer += weapon_affects[*it];
			buffer += " ";
		}
	}
	if (!filter.affect2.empty())
	{
		for (vector<int>::const_iterator it = filter.affect2.begin(); it != filter.affect2.end(); ++it)
		{
			buffer += apply_types[*it];
			buffer += " ";
		}
	}
	buffer += "\r\n";
	send_to_char(buffer, ch);
	set_wait(ch, 1, FALSE);

	std::ostringstream out;
	for (chest = world[real_room(CLAN(ch)->chest_room)]->contents; chest; chest = chest->next_content)
	{
		if (Clan::is_clan_chest(chest))
		{
			for (temp_obj = chest->contains; temp_obj; temp_obj = temp_obj->next_content)
			{
				std::ostringstream modif;
				// сверяем имя
				if (!filter.name.empty() && !CompareParam(filter.name, temp_obj->name))
					continue;
				// тип
				if (filter.type >= 0 && filter.type != GET_OBJ_TYPE(temp_obj))
					continue;
				// таймер
				if (filter.state >= 0)
				{
					if (!GET_OBJ_TIMER(obj_proto[GET_OBJ_RNUM(temp_obj)]))
					{
						send_to_char("Нулевой таймер прототипа, сообщите Богам!", ch);
						return;
					}
					int tm = (GET_OBJ_TIMER(temp_obj) * 100 / GET_OBJ_TIMER(obj_proto[GET_OBJ_RNUM(temp_obj)]));
					if ((tm + 1) < filter.state || (tm + 1) > (filter.state + 20))
						continue;
				}
				// куда можно одеть
				if (filter.wear >= 0 && !CAN_WEAR(temp_obj, filter.wear))
					continue;
				// класс оружия
				if (filter.weap_class >= 0 && filter.weap_class != GET_OBJ_SKILL(temp_obj))
					continue;
				// аффекты
				find = 1;
				if (!filter.affect.empty())
				{
					for (vector<int>::const_iterator it = filter.affect.begin(); it != filter.affect.end(); ++it)
					{
						if (!CompareBits(temp_obj->obj_flags.affects, weapon_affects, *it))
						{
							find = 0;
							break;
						}
					}
					// первая часть не найдена, продолжать смысла нет
					if (!find)
						continue;
				}
				if (!filter.affect2.empty())
				{
					for (vector<int>::const_iterator it = filter.affect2.begin(); it != filter.affect2.end() && find; ++it)
					{
						find = 0;
						for (int i = 0; i < MAX_OBJ_AFFECT; ++i)
						{
							if (temp_obj->affected[i].location == *it)
							{
								if (temp_obj->affected[i].modifier > 0)
									modif << "+";
								else
									modif << "-";
								find = 1;
								break;
							}
						}
					}
				}
				if (find)
				{
					if (!modif.str().empty())
						out << modif.str() << " ";
					out << show_obj_to_char(temp_obj, ch, 1, 3, 1);
				}
			}
			break;
		}
	}
	if (!out.str().empty())
		page_string(ch->desc, out.str(), TRUE);
	else
		send_to_char("Ничего не найдено.\r\n", ch);
}

void Clan::HouseStat(CHAR_DATA * ch, std::string & buffer)
{
	std::string buffer2;
	GetOneParam(buffer, buffer2);
	bool all = 0, name = 0;
	std::ostringstream out;
	// параметр сортировки
	enum ESortParam
	{
		SORT_STAT_BY_EXP,
		SORT_STAT_BY_CLANEXP,
		SORT_STAT_BY_MONEY,
		SORT_STAT_BY_EXPPERCENT,
		SORT_STAT_BY_LOGON,
		SORT_STAT_BY_NAME
	};
	ESortParam sortParameter;
	long long lSortParam;

	// т.к. в кои8-р русские буквы не попорядку
	const char *pSortAlph = "яюэьыъщшчцхфутсрпонмлкизжёедгвба";
	// первая буква имени
	char pcFirstChar[2];

	// для избежания путаницы с именами фильтр начинается со знака "!"
	// формат команды:
	// клан стат [!опыт/!заработанным/!налог/!последнему] [имя/все]
	sortParameter = SORT_STAT_BY_EXP;
	if (buffer2.length() > 1)
	{
		if ((buffer2[0] == '!') && (is_abbrev(buffer2.c_str() + 1, "опыту"))) // опыту дружине
			sortParameter = SORT_STAT_BY_CLANEXP;
		else if ((buffer2[0] == '!') && (is_abbrev(buffer2.c_str() + 1, "заработанным")))
			sortParameter = SORT_STAT_BY_MONEY;
		else if ((buffer2[0] == '!') && (is_abbrev(buffer2.c_str() + 1, "налог")))
			sortParameter = SORT_STAT_BY_EXPPERCENT;
		else if ((buffer2[0] == '!') && (is_abbrev(buffer2.c_str() + 1, "последнему")))
			sortParameter = SORT_STAT_BY_LOGON;
		else if ((buffer2[0] == '!') && (is_abbrev(buffer2.c_str() + 1, "имя")))
			sortParameter = SORT_STAT_BY_NAME;

		// берем следующий параметр
		if (buffer2[0] == '!') GetOneParam(buffer, buffer2);
	}

	out << CCWHT(ch, C_NRM);
	if (CompareParam(buffer2, "очистить") || CompareParam(buffer2, "удалить"))
	{
		if (CLAN_MEMBER(ch)->rank_num)
		{
			send_to_char("У Вас нет прав удалять статистику.\r\n", ch);
			return;
		}
		// можно почистить тока деньги для удобства сбора с мемберов налога
		GetOneParam(buffer, buffer2);
		bool money = 0;
		if (CompareParam(buffer2, "деньги") || CompareParam(buffer2, "money"))
			money = 1;

		for (ClanMemberList::const_iterator it = this->members.begin(); it != this->members.end(); ++it)
		{
			it->second->money = 0;
			if (money)
				continue;
			it->second->exp = 0;
			it->second->clan_exp = 0;
		}

		if (money)
			send_to_char("Статистика доходов Вашей дружины очищена.\r\n", ch);
		else
			send_to_char("Статистика Вашей дружины полностью очищена.\r\n", ch);
		return;
	}
	else if (CompareParam(buffer2, "все"))
	{
		all = 1;
		out << "Статистика Вашей дружины ";
	}
	else if (!buffer2.empty())
	{
		name = 1;
		out << "Статистика Вашей дружины (поиск по имени '" << buffer2 << "') ";
	}
	else
		out << "Статистика Вашей дружины (находящиеся онлайн) ";

	// вывод режима сортировки
	out << "(сортировка по: ";
	switch (sortParameter)
	{
	case SORT_STAT_BY_EXP:
		out << "рейтинговым очкам";
		break;
	case SORT_STAT_BY_CLANEXP:
		out << "опыту дружине";
		break;
	case SORT_STAT_BY_MONEY:
		out << "заработанным кунам";
		break;
	case SORT_STAT_BY_EXPPERCENT:
		out << "отдаваемому опыту дружине";
		break;
	case SORT_STAT_BY_LOGON:
		out << "последнему заходу в игру";
		break;
	case SORT_STAT_BY_NAME:
		out << "первой букве имени";
		break;

		// этого быть не должно
	default:
		out << "чему БОГ пошлет";
	}
	out << "):\r\n";

	out << CCNRM(ch, C_NRM) << "Имя             Рейтинговых очков  Опыта дружины  Заработано кун  Налог Последний вход\r\n";

	// multimap ибо могут быть совпадения
	std::multimap<long long, std::pair<std::string, ClanMemberPtr> > temp_list;

	for (ClanMemberList::const_iterator it = this->members.begin(); it != this->members.end(); ++it)
	{
		if (!all && !name)
		{
			DESCRIPTOR_DATA *d;
			if (!(d = DescByUID(it->first)))
				continue;
		}
		else if (name)
		{
			if (!CompareParam(buffer2, it->second->name))
				continue;
		}
		char timeBuf[17];
		time_t tmp_time = get_lastlogon_by_uniquie(it->first);
		if (tmp_time <= 0) tmp_time = time(0);
		strftime(timeBuf, sizeof(timeBuf), "%d-%m-%Y", localtime(&tmp_time));

		// сортировка по...
		switch (sortParameter)
		{
		case SORT_STAT_BY_EXP:
			lSortParam = it->second->exp;
			break;
		case SORT_STAT_BY_CLANEXP:
			lSortParam = it->second->clan_exp;
			break;
		case SORT_STAT_BY_MONEY:
			lSortParam = it->second->money;
			break;
		case SORT_STAT_BY_EXPPERCENT:
			lSortParam = it->second->exp_persent;
			break;
		case SORT_STAT_BY_LOGON:
			lSortParam = get_lastlogon_by_uniquie(it->first);
			break;
		case SORT_STAT_BY_NAME:
		{
			pcFirstChar[0] = LOWER(it->second->name[0]);
			pcFirstChar[1] = '\0';
			char const *pTmp = strpbrk(pSortAlph, pcFirstChar);
			if (pTmp) lSortParam = pTmp - pSortAlph; // индекс первой буквы в массиве
			else lSortParam = pcFirstChar[0]; // или не русская буква или я хз
			break;
		}

		// на всякий случай
		default:
			lSortParam = it->second->exp;
		}

		temp_list.insert(std::make_pair(lSortParam,
										std::make_pair(std::string(timeBuf), it->second)));
	}

	for (std::multimap<long long, std::pair<std::string, ClanMemberPtr> >::reverse_iterator it = temp_list.rbegin(); it != temp_list.rend(); ++it)
	{
		out.setf(std::ios_base::left, std::ios_base::adjustfield);
		out << std::setw(15) << it->second.second->name << " "
		<< std::setw(18) << it->second.second->exp << " " // аналог it->first
		<< std::setw(14) << it->second.second->clan_exp << " "
		<< std::setw(15) << it->second.second->money << " "
		<< std::setw(5)  << it->second.second->exp_persent << " "
		<< std::setw(14) << it->second.first << "\r\n";
	}
	page_string(ch->desc, out.str(), 1);
}

// напоминалка про кончину денег в казне дружины
void Clan::ChestInvoice()
{
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
	{
		int cost = (*clan)->ChestTax();
		cost += CLAN_TAX + ((*clan)->storehouse * CLAN_STOREHOUSE_TAX);
		if (!cost)
			continue; // чем черт не шутит
		else
		{
			// опаньки
			if ((*clan)->bank < cost)
				for (DESCRIPTOR_DATA *d = descriptor_list; d; d = d->next)
					if (d->character && STATE(d) == CON_PLAYING && !AFF_FLAGGED(d->character, AFF_DEAFNESS) && CLAN(d->character) && CLAN(d->character) == *clan)
						send_to_char(d->character, "[Хранилище]: %s'Напоминаю, что средств в казне дружины хватит менее, чем на сутки!'%s\r\n", CCIRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
		}
	}
}

int Clan::ChestTax()
{
	OBJ_DATA *temp, *chest;
	int cost = 0;
	int count = 0;
	for (chest = world[real_room(this->chest_room)]->contents; chest; chest = chest->next_content)
	{
		if (Clan::is_clan_chest(chest))
		{
			// перебираем шмот
			for (temp = chest->contains; temp; temp = temp->next_content)
			{
				cost += GET_OBJ_RENTEQ(temp);
				++count;
			}
			this->chest_weight = GET_OBJ_WEIGHT(chest);
			break;
		}
	}
	this->chest_objcount = count;
	this->chest_discount = MAX(25, 50 - 5 * (this->clan_level));
	return cost * this->chest_discount / 100;
}

/**
* Вместо спешиала теперь просто перехватываем осмотр контейнеров на случай клан-сундука.
* Смотреть могут ес-сно только соклановцы.
* \todo Вынести из класса. Да и вообще там чистить давно пора.
* \param obj - контейнер
* \param ch - смотрящий
* \return 0 - это не клан-сундук, 1 - это был он
*/
bool Clan::ChestShow(OBJ_DATA * obj, CHAR_DATA * ch)
{
	if (!ch->desc || !Clan::is_clan_chest(obj))
		return 0;

	if (CLAN(ch) && real_room(CLAN(ch)->chest_room) == IN_ROOM(obj))
	{
		send_to_char("Хранилище Вашей дружины:\r\n", ch);
		int cost = CLAN(ch)->ChestTax();
		send_to_char(ch, "Всего вещей: %d   Рента в день: %d %s\r\n\r\n", CLAN(ch)->chest_objcount, cost, desc_count(cost, WHAT_MONEYa));
		list_obj_to_char(obj->contains, ch, 1, 3);
	}
	else
		send_to_char("Не на что тут глазеть, пусто, вот те крест.\r\n", ch); // засланым казачкам показываем хер, а не хранилище
	return 1;
}

// +/- клан-экспы
int Clan::SetClanExp(CHAR_DATA *ch, int add)
{
	// шоб не читили
	if (GET_LEVEL(ch) >= LVL_IMMORT)
		return add;

	int toclan, tochar;
	if (add > 0)
	{
		toclan = add * CLAN_MEMBER(ch)->exp_persent / 100;
		tochar = add - toclan;
	}
	else
	{
		toclan = add / 5;
		tochar = 0;
	}

	CLAN_MEMBER(ch)->clan_exp += toclan; // обнулять не надо минуса

	this->clan_exp += toclan;
	if (this->clan_exp < 0)
		this->clan_exp = 0;

	if (this->clan_exp > clan_level_exp[this->clan_level+1] && this->clan_level < MAX_CLANLEVEL)
	{
		this->clan_level++;
		for (DESCRIPTOR_DATA *d = descriptor_list; d; d = d->next)
			if (d->character && STATE(d) == CON_PLAYING && !AFF_FLAGGED(d->character, AFF_DEAFNESS) && CLAN(d->character) && CLAN(d->character)->GetRent() == this->rent)
				send_to_char(d->character, "%sВаш замок достиг нового, %d уровня! Поздравляем!%s\r\n", CCIRED(d->character, C_NRM), this->clan_level, CCNRM(d->character, C_NRM));
	}
	else if (this->clan_exp < clan_level_exp[this->clan_level] && this->clan_level > 0)
	{
		this->clan_level--;
		for (DESCRIPTOR_DATA *d = descriptor_list; d; d = d->next)
			if (d->character && STATE(d) == CON_PLAYING && !AFF_FLAGGED(d->character, AFF_DEAFNESS) && CLAN(d->character) && CLAN(d->character)->GetRent() == this->rent)
				send_to_char(d->character, "%sВаш замок потерял уровень! Теперь он %d уровня! Поздравляем!%s\r\n", CCIRED(d->character, C_NRM), this->clan_level, CCNRM(d->character, C_NRM));
	}
	return tochar;
}

// добавление экспы для топа кланов и мемберу в зачетку
void Clan::AddTopExp(CHAR_DATA * ch, int add_exp)
{
	// шоб не читили
	if (GET_LEVEL(ch) >= LVL_IMMORT)
		return;

	CLAN_MEMBER(ch)->exp += add_exp;
	if (CLAN_MEMBER(ch)->exp < 0)
		CLAN_MEMBER(ch)->exp = 0;

	// в буффер или сразу в топ
	if (this->exp_info)
	{
		this->exp += add_exp;
		if (this->exp < 0)
			this->exp = 0;
	}
	else
		this->exp_buf += add_exp; // тут обнулять не надо
}

// синхронизация клановой экспы для топ с буффером, если есть режим запрета показа в ран-тайме
void Clan::SyncTopExp()
{
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
		if (!(*clan)->exp_info)
		{
			(*clan)->exp += (*clan)->exp_buf;
			if ((*clan)->exp < 0)
				(*clan)->exp = 0;
			(*clan)->exp_buf = 0;
		}
}

// установка режима оповещения об изменениях в хранилище
void SetChestMode(CHAR_DATA *ch, std::string &buffer)
{
	if (IS_NPC(ch))
		return;
	if (!CLAN(ch))
	{
		send_to_char("Для начала обзаведитесь дружиной.\r\n", ch);
		return;
	}

	boost::trim_if(buffer, boost::is_any_of(" \'"));
	if (CompareParam(buffer, "нет"))
	{
		REMOVE_BIT(PRF_FLAGS(ch, PRF_DECAY_MODE), PRF_DECAY_MODE);
		REMOVE_BIT(PRF_FLAGS(ch, PRF_TAKE_MODE), PRF_TAKE_MODE);
		send_to_char("Ладушки.\r\n", ch);
	}
	else if (CompareParam(buffer, "рассыпание"))
	{
		SET_BIT(PRF_FLAGS(ch, PRF_DECAY_MODE), PRF_DECAY_MODE);
		REMOVE_BIT(PRF_FLAGS(ch, PRF_TAKE_MODE), PRF_TAKE_MODE);
		send_to_char("Ладушки.\r\n", ch);
	}
	else if (CompareParam(buffer, "изменение"))
	{
		REMOVE_BIT(PRF_FLAGS(ch, PRF_DECAY_MODE), PRF_DECAY_MODE);
		SET_BIT(PRF_FLAGS(ch, PRF_TAKE_MODE), PRF_TAKE_MODE);
		send_to_char("Ладушки.\r\n", ch);
	}
	else if (CompareParam(buffer, "полный"))
	{
		SET_BIT(PRF_FLAGS(ch, PRF_DECAY_MODE), PRF_DECAY_MODE);
		SET_BIT(PRF_FLAGS(ch, PRF_TAKE_MODE), PRF_TAKE_MODE);
		send_to_char("Ладушки.\r\n", ch);
	}
	else
	{
		send_to_char("Задается режим оповещения об изменениях в хранилище дружины.\r\n"
					 "Формат команды: режим хранилище <нет|рассыпание|изменение|полный>\r\n"
					 " нет - выключение канала хранилища\r\n"
					 " рассыпание - получать только сообщения о рассыпании вещей\r\n"
					 " изменение - получать только сообщения о взятии/добавлении вещей\r\n"
					 " полный - получать оба вида сообщений\r\n", ch);
		return;
	}
}

// шоб не засорять в режиме, а выдать строку сразу
std::string GetChestMode(CHAR_DATA *ch)
{
	if (PRF_FLAGGED(ch, PRF_DECAY_MODE))
	{
		if (PRF_FLAGGED(ch, PRF_TAKE_MODE))
			return "полный";
		else
			return "рассыпание";
	}
	else if (PRF_FLAGGED(ch, PRF_TAKE_MODE))
		return "изменение";
	else
		return "выкл";
}

ACMD(do_clanstuff)
{
	OBJ_DATA *obj;
	int vnum, rnum;
	int cnt = 0, gold_total = 0;

	if (!CLAN(ch))
	{
		send_to_char("Сначала вступите в какой-нибудь клан.\r\n", ch);
		return;
	}

	if (GET_ROOM_VNUM(IN_ROOM(ch)) != CLAN(ch)->GetRent())
	{
		send_to_char("Получить клановую экипировку вы можете только в центре вашего замка.\r\n", ch);
		return;
	}

	std::string title = CLAN(ch)->GetClanTitle();

	half_chop(argument, arg, buf);

	ClanStuffList::iterator it = CLAN(ch)->clanstuff.begin();
	for (;it != CLAN(ch)->clanstuff.end();it++)
	{
		vnum = CLAN_STUFF_ZONE * 100 + it->num;
		obj = read_object(vnum, VIRTUAL);
		if (!obj)
			continue;
		rnum = GET_OBJ_RNUM(obj);

		if (rnum == NOTHING)
			continue;
		if (*arg && !strstr(obj_proto[rnum]->short_description, arg))
			continue;

		sprintf(buf, "%s %s clan%d!", it->name.c_str(), title.c_str(), CLAN(ch)->GetRent());
		obj->name              = str_dup(buf);
		obj->short_description = str_dup((it->PNames[0] + " " + title).c_str());

		for (int i = 0;i < 6; i++)
			obj->PNames[i] = str_dup((it->PNames[i] + " " + title).c_str());

		obj->description = str_dup((it->desc).c_str());

		if (it->longdesc.length() > 0)
		{
			EXTRA_DESCR_DATA * new_descr;
			CREATE(new_descr, EXTRA_DESCR_DATA, 1);
			new_descr->keyword = str_dup(obj->short_description);
			new_descr->description = str_dup(it->longdesc.c_str());
			new_descr->next = NULL;
			obj->ex_description = new_descr;
		}

		if (!cnt)
		{
			act("$n открыл$g крышку сундука со стандартной экипировкой\r\n", FALSE, ch, 0, 0, TO_ROOM);
			act("Вы открыли крышку сундука со стандартной экипировкой\r\n", FALSE, ch, 0, 0, TO_CHAR);
		}

		int gold = GET_OBJ_COST(obj);

		if (get_gold(ch) >= gold)
		{
			add_gold(ch, -gold);
			gold_total += gold;
		}
		else
		{
			send_to_char(ch, "Кончились денюжки!\r\n");
			break;
		}

		obj_to_char(obj, ch);
		cnt++;

		sprintf(buf, "$n взял$g %s из сундука", obj->PNames[0]);
		sprintf(buf2, "Вы взяли %s из сундука", obj->PNames[0]);
		act(buf, FALSE, ch, 0, 0, TO_ROOM);
		act(buf2, FALSE, ch, 0, 0, TO_CHAR);
	}

	if (cnt)
	{
		sprintf(buf2, "\r\nЭкипировка обошлась вам в %d %s.", gold_total, desc_count(gold_total, WHAT_MONEYu));
		act("\r\n$n закрыл$g крышку сундука", FALSE, ch, 0, 0, TO_ROOM);
		act(buf2, FALSE, ch, 0, 0, TO_CHAR);
	}
	else
	{
		act("\r\n$n порыл$u в сундуке со стандартной экипировкой, но ничего не наш$y", FALSE, ch, 0, 0, TO_ROOM);
		act("\r\nВы порылись в сундуке со стандартной экипировкой, но не нашли ничего подходящего", FALSE, ch, 0, 0, TO_CHAR);
	}
	set_wait(ch, 1, FALSE);
}

// неужто реально глючит?
int Clan::GetRent()
{
	return this->rent;
}

int Clan::GetOutRent()
{
	return this->out_rent;
}

/**
* Удаление чара из клана, клан берется не через поля чара, а ищем по всем кланам
*/
void Clan::remove_from_clan(long unique)
{
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
	{
		ClanMemberList::iterator it = (*clan)->members.find(unique);
		if (it != (*clan)->members.end())
		{
			(*clan)->members.erase(it);
			return;
		}
	}
}

int Clan::GetMemberExpPersent(CHAR_DATA *ch)
{
	return CLAN(ch) ? CLAN_MEMBER(ch)->exp_persent : 0;
}

/**
*
*/
void Clan::init_chest_rnum()
{
	CLAN_CHEST_RNUM = real_object(CLAN_CHEST_VNUM);
}

bool Clan::is_clan_chest(OBJ_DATA *obj)
{
	if (CLAN_CHEST_RNUM < 0 || obj->item_number != CLAN_CHEST_RNUM)
		return false;
	return true;
}

/**
* Оповещение соклановцев о входе/выходе друг друга.
* \param enter 1 - вход чара, 0 - выход.
*/
void Clan::clan_invoice(CHAR_DATA *ch, bool enter)
{
	if (IS_NPC(ch) || !CLAN(ch))
		return;

	for (DESCRIPTOR_DATA *d = descriptor_list; d; d = d->next)
	{
		if (d->character && d->character != ch
				&& STATE(d) == CON_PLAYING
				&& CLAN(d->character) == CLAN(ch)
				&& PRF_FLAGGED(d->character, PRF_WORKMATE_MODE))
		{
			if (enter)
			{
				send_to_char(d->character, "%sДружинни%s %s вош%s в мир.%s\r\n",
							 CCINRM(d->character, C_NRM), IS_MALE(ch) ? "к" : "ца", GET_NAME(ch),
							 GET_CH_SUF_5(ch), CCNRM(d->character, C_NRM));
			}
			else
			{
				send_to_char(d->character, "%sДружинни%s %s покинул%s мир.%s\r\n",
							 CCINRM(d->character, C_NRM), IS_MALE(ch) ? "к" : "ца", GET_NAME(ch),
							 GET_CH_SUF_1(ch), CCNRM(d->character, C_NRM));
			}
		}
	}
}

int Clan::print_spell_locate_object(CHAR_DATA *ch, int count, std::string name)
{
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
	{
		OBJ_DATA *temp, *chest;
		for (chest = world[real_room((*clan)->chest_room)]->contents; chest; chest = chest->next_content)
		{
			if (Clan::is_clan_chest(chest))
			{
				for (temp = chest->contains; temp; temp = temp->next_content)
				{
					if (number(1, 100) > (40 + MAX((GET_REAL_INT(ch) - 25) * 2, 0)))
						continue;
					if (!isname(name.c_str(), temp->name))
						continue;

					snprintf(buf, MAX_STRING_LENGTH, "%s наход%sся в хранилище дружины '%s'.\r\n",
							temp->short_description, GET_OBJ_POLY_1(ch, temp), (*clan)->GetAbbrev());
					CAP(buf);
					send_to_char(buf, ch);

					if (--count <= 0)
						return count;
				}
				break;
			}
		}
	}
	return count;
}

int Clan::GetClanWars(CHAR_DATA *ch)
{
	int result=0, p1=0;
	if (IS_NPC(ch) || !CLAN(ch))
		return 0;

	ClanListType::const_iterator clanVictim;
	for (clanVictim = Clan::ClanList.begin(); clanVictim != Clan::ClanList.end(); ++clanVictim)
	{
		if (*clanVictim == CLAN(ch))
			continue;
		p1 = CLAN(ch)->CheckPolitics((*clanVictim)->rent);
		if (p1 == POLITICS_WAR) result++;
	}

	return result;
}

void Clan::add_remember(std::string text, int flag)
{
	std::string buffer = Remember::time_format();
	buffer += text;

	switch (flag)
	{
	case Remember::CLAN:
		remember_.push_back(buffer);
		if (remember_.size() > Remember::MAX_REMEMBER_NUM)
		{
			remember_.erase(remember_.begin());
		}
		break;
	case Remember::ALLY:
		remember_ally_.push_back(buffer);
		if (remember_ally_.size() > Remember::MAX_REMEMBER_NUM)
		{
			remember_ally_.erase(remember_ally_.begin());
		}
		break;
	default:
		log("SYSERROR: мы не должны были сюда попасть, flag: %d, func: %s",
				flag, __func__);
		break;
	}
}

std::string Clan::get_remember(unsigned int num, int flag) const
{
	std::string buffer;

	switch (flag)
	{
	case Remember::CLAN:
	{
		Remember::RememberListType::const_iterator it = remember_.begin();
		if (remember_.size() > num)
		{
			std::advance(it, remember_.size() - num);
		}
		for (; it != remember_.end(); ++it)
		{
			buffer += *it;
		}
		break;
	}
	case Remember::ALLY:
	{
		Remember::RememberListType::const_iterator it = remember_ally_.begin();
		if (remember_ally_.size() > num)
		{
			std::advance(it, remember_ally_.size() - num);
		}
		for (; it != remember_ally_.end(); ++it)
		{
			buffer += *it;
		}
		break;
	}
	default:
		log("SYSERROR: мы не должны были сюда попасть, flag: %d, func: %s",
				flag, __func__);
		break;
	}
	if (buffer.empty())
	{
		buffer = "Вам нечего вспомнить.\r\n";
	}
	return buffer;
}
