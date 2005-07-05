	/* ****************************************************************************
* File: house.cpp                                              Part of Bylins *
* Usage: Handling of clan system                                              *
* (c) 2005 Krodo                                                              *
******************************************************************************/

#include <fstream>
#include <sstream>
#include <boost/format.hpp>
#include "sys/stat.h"
#include <cmath>
#include <iomanip>

#include "house.h"
#include "comm.h"
#include "handler.h"
#include "pk.h"
#include "screen.h"
#include "boards.h"
#include "skills.h"

extern void list_obj_to_char(OBJ_DATA * list, CHAR_DATA * ch, int mode, int show);
extern void write_one_object(char **data, OBJ_DATA * object, int location);
extern OBJ_DATA *read_one_object_new(char **data, int *error);
extern int file_to_string_alloc(const char *name, char **buf);
extern struct player_index_element *player_table;
extern int top_of_p_table;
extern const char *apply_types[];
extern const char *item_types[];
extern const char *wear_bits[];
extern const char *weapon_class[];
extern const char *weapon_affects[];
extern const char *show_obj_to_char(OBJ_DATA * object, CHAR_DATA * ch, int mode, int show_state, int how);
extern int ext_search_block(char *arg, const char **list, int exact);


inline bool Clan::SortRank::operator() (const CHAR_DATA * ch1, const CHAR_DATA * ch2) {
    return GET_CLAN_RANKNUM(ch1) < GET_CLAN_RANKNUM(ch2);
}

ClanListType Clan::ClanList;

// аббревиатура дружины по игроку
const char *Clan::GetAbbrev(CHAR_DATA * ch)
{
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
		if (GET_CLAN_RENT(ch) == (*clan)->rent)
			return (*clan)->abbrev.c_str();
	return 0;
}

// поиск to_room в зонах клан-замков, выставляет ренту если найдено
room_rnum Clan::CloseRent(room_rnum to_room)
{
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
		if (world[to_room]->zone == world[real_room((*clan)->rent)]->zone)
			return (to_room = real_room((*clan)->out_rent));
	return to_room;
}

// проверяет находится ли чар в зоне чужого клана
bool Clan::InEnemyZone(CHAR_DATA * ch)
{
	int zone = world[IN_ROOM(ch)]->zone;

	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
		if (zone == world[real_room((*clan)->rent)]->zone) {
			if (GET_CLAN_RENT(ch) == (*clan)->rent
			    || (*clan)->CheckPolitics(GET_CLAN_RENT(ch)) == POLITICS_ALLIANCE)
				return 0;
			else
				return 1;
		}
	return 0;
}

Clan::Clan()
	: rent(0), bank(0), exp(0), guard(0), builtOn(time(0)), bankBuffer(0), entranceMode(0), out_rent(0), storehouse(0)
{

}

Clan::~Clan()
{

}

// лоад/релоад индекса и файлов кланов
void Clan::ClanLoad()
{
	// на случай релоада
	Clan::ClanList.clear();

	// файл со списком кланов
	std::ifstream file(LIB_HOUSE "index");
	if (!file.is_open()) {
		log("Error open file: %s! (%s %s %d)", LIB_HOUSE "index", __FILE__, __func__, __LINE__);
		return;
	}
	std::string buffer;
	std::list<std::string> clanIndex;
	while (file >> buffer)
		clanIndex.push_back(buffer);
	file.close();

	// собственно грузим кланы
	for (std::list < std::string >::const_iterator it = clanIndex.begin(); it != clanIndex.end(); ++it) {
		ClanPtr tempClan(new Clan);

		std::ifstream file((LIB_HOUSE + *it).c_str());
		if (!file.is_open()) {
			log("Error open file: %s! (%s %s %d)", (LIB_HOUSE + *it).c_str(), __FILE__, __func__, __LINE__);
			return;
		}
		while (file >> buffer) {
			if (buffer == "Abbrev:")
				file >> tempClan->abbrev;
			else if (buffer == "Name:") {
				std::getline(file, buffer);
				SkipSpaces(buffer);
				tempClan->name = buffer;
			} else if (buffer == "Title:") {
				std::getline(file, buffer);
				SkipSpaces(buffer);
				tempClan->title = buffer;
			} else if (buffer == "Rent:") {
				int rent = 0;
				file >> rent;
				// зоны может и не быть
				if (!real_room(rent)) {
					log("Room %d is no longer exist (%s).", rent, (LIB_HOUSE + *it).c_str());
					break;
				}
				tempClan->rent = rent;
			} else if (buffer == "OutRent:") {
				int out_rent = 0;
				file >> out_rent;
				// зоны может и не быть
				if (!real_room(out_rent)) {
					log("Room %d is no longer exist (%s).", out_rent, (LIB_HOUSE + *it).c_str());
					break;
				}
				tempClan->out_rent = out_rent;
			} else if (buffer == "Guard:") {
				int guard = 0;

				file >> guard;
				// как и охранника
				if (real_mobile(guard) < 0) {
					log("Guard %d is no longer exist (%s).", guard, (LIB_HOUSE + *it).c_str());
					break;
				}
				tempClan->guard = guard;
			} else if (buffer == "BuiltOn:")
				file >> tempClan->builtOn;
			else if (buffer == "Ranks:") {
				std::getline(file, buffer, '~');
				std::istringstream stream(buffer);
				unsigned long priv = 0;

				while (stream >> buffer >> priv) {
					tempClan->ranks.push_back(buffer);
					tempClan->privileges.push_back(std::bitset<CLAN_PRIVILEGES_NUM> (priv));
				}
				// для верности воеводе проставим все привилегии
				for (int i = 0; i < CLAN_PRIVILEGES_NUM; ++i)
					tempClan->privileges[0].set(i);
			} else if (buffer == "Politics:") {
				std::getline(file, buffer, '~');
				std::istringstream stream(buffer);
				room_vnum room = 0;
				int state = 0;

				while (stream >> room >> state)
					tempClan->politics[room] = state;
			} else if (buffer == "EntranceMode:")
				file >> tempClan->entranceMode;
			else if (buffer == "Storehouse:")
				file >> tempClan->storehouse;
			else if (buffer == "Exp:")
				file >> tempClan->exp;
			else if (buffer == "Bank:") {
				file >> tempClan->bank;
				if (tempClan->bank == 0) {
					log("Clan has 0 in bank, remove from list (%s).", (LIB_HOUSE + *it).c_str());
					break;
				}
			}
			else if (buffer == "Owner:") {
				long unique = 0;
				long long money = 0;
				unsigned long long exp = 0;
				int tax = 0;

				file >> unique >> money >> exp >> tax;
				std::string name = GetNameByUnique(unique);

				// воеводы тоже уже может не быть
				if (!name.empty()) {
					ClanMemberPtr tempMember(new ClanMember);
					name[0] = UPPER(name[0]);
					tempMember->name = name;
					tempMember->rankNum = 0;
					tempMember->money = money;
					tempMember->exp = exp;
					tempMember->tax = tax;
					tempClan->members[unique] = tempMember;
					tempClan->owner = name;
				} else {
					log("Owner %ld is no longer exist (%s).", unique, (LIB_HOUSE + *it).c_str());
					break;
				}
			} else if (buffer == "Members:") {
				// параметры, критичные для мемберов, нужно загрузить до того как (ранги например)
				long unique = 0;
				unsigned rank = 0;
				long long money = 0;
				unsigned long long exp = 0;
				int tax = 0;

				std::getline(file, buffer, '~');
				std::istringstream stream(buffer);
				while (stream >> unique >> rank >> money >> exp >> tax) {
					// на случай, если рангов стало меньше
					if (!tempClan->ranks.empty() && rank > tempClan->ranks.size() - 1)
						rank = tempClan->ranks.size() - 1;
					std::string name = GetNameByUnique(unique);

					// удаленные персонажи просто игнорируются
					if (!name.empty()) {
						ClanMemberPtr tempMember(new ClanMember);
						name[0] = UPPER(name[0]);
    					tempMember->name = name;
						tempMember->rankNum = rank;
						tempMember->money = money;
						tempMember->exp = exp;
						tempMember->tax = tax;
						tempClan->members[unique] = tempMember;
					}
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

		// подгружаем пкл/дрл
		std::ifstream pkFile((LIB_HOUSE + *it + ".pkl").c_str());
		if (pkFile.is_open()) {
			while (pkFile) {
				int victim = 0, author = 0;
				time_t tempTime = time(0);
				bool flag = 0;

				pkFile >> victim >> author >> tempTime >> flag;
				std::getline(pkFile, buffer, '~');
				SkipSpaces(buffer);
				const char *authorName = GetNameByUnique(author);
				const char *victimName = GetNameByUnique(victim, 1);

				// если автора уже нет - сбросим УИД для верности
				if (!authorName)
					author = 0;
				// если жертвы уже нет, или жертва выбилась в БОГИ - не грузим
				if (victimName) {
					ClanPkPtr tempRecord(new ClanPk);

					tempRecord->author = author;
					tempRecord->authorName = authorName ? authorName : "Кто-то";
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
		Clan::ClanList.push_back(tempClan);
	}
	Clan::ChestLoad();
	Clan::ChestUpdate();
	Clan::ChestSave();
	Clan::ClanSave();
	Board::ClanInit();
	// на случай релоада кланов для выставления изменений игрокам онлайн
	DESCRIPTOR_DATA *d;
	for (d = descriptor_list; d; d = d->next) {
		if (d->character)
			Clan::SetClanData(d->character);
	}
}

// вывод имму информации о кланах
void Clan::ClanInfo(CHAR_DATA * ch)
{
	std::ostringstream buffer;
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan) {
		buffer << "Abbrev: " << (*clan)->abbrev << "\r\n"
		    << "Name: " << (*clan)->name << "\r\n"
		    << "Title: " << (*clan)->title << "\r\n"
		    << "Owner: " << (*clan)->owner << "\r\n"
		    << "Rent: " << (*clan)->rent << "\r\n"
			<< "OutRent: " << (*clan)->out_rent << "\r\n"
			<< "Guard: " << (*clan)->guard << "\r\n";

		char timeBuf[17];
		strftime(timeBuf, sizeof(timeBuf), "%H:%M %d-%m-%Y", localtime(&(*clan)->builtOn));

		buffer << "BuiltOn: " << timeBuf << "\r\n"
		    << "EntranceMode: " << (*clan)->entranceMode << "\r\n"
			<< "Storehouse: " << (*clan)->storehouse << "\r\n"
		    << "Bank: " << (*clan)->bank << "\r\n" << "Ranks: ";
		for (std::vector < std::string >::const_iterator it =
		     (*clan)->ranks.begin(); it != (*clan)->ranks.end(); ++it)
			buffer << (*it) << " ";
		buffer << "\r\n" << "Members:" << "\r\n";
		for (ClanMemberList::const_iterator it = (*clan)->members.begin(); it != (*clan)->members.end(); ++it) {
			buffer << "Uid: " << (*it).first << " "
			    << "Name: " << (*it).second->name << " "
			    << "RankNum: " << (*it).second->rankNum << " "
			    << "Money: " << (*it).second->money << " "
			    << "Exp: " << (*it).second->exp << " "
			    << "Tax: " << (*it).second->tax << " "
			    << "RankName: " << (*clan)->ranks[(*it).second->rankNum] << "\r\n";
		}
		buffer << "Politics:\r\n";
		for (ClanPolitics::const_iterator it = (*clan)->politics.begin(); it != (*clan)->politics.end(); ++it)
			buffer << (*it).first << ": " << (*it).second << "\r\n";
	}
	char *text = str_dup(buffer.str());
	page_string(ch->desc, text, TRUE);
	free(text);
//	send_to_char(buffer.str(), ch);
}

// сохранение кланов в файлы
void Clan::ClanSave()
{
	std::ofstream index(LIB_HOUSE "index");
	if (!index.is_open()) {
		log("Error open file: %s! (%s %s %d)", LIB_HOUSE "index", __FILE__, __func__, __LINE__);
		return;
	}

	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan) {
		// именем файла для клана служит его аббревиатура (английский и нижний регистр)
		std::string buffer = (*clan)->abbrev;
		CreateFileName(buffer);
		index << buffer << "\n";

		std::ofstream file((LIB_HOUSE + buffer).c_str());
		if (!file.is_open()) {
			log("Error open file: %s! (%s %s %d)", (LIB_HOUSE + buffer).c_str(),
			    __FILE__, __func__, __LINE__);
			return;
		}

		file << "Abbrev: " << (*clan)->abbrev << "\n"
		    << "Name: " << (*clan)->name << "\n"
		    << "Title: " << (*clan)->title << "\n"
		    << "Rent: " << (*clan)->rent << "\n"
		    << "OutRent: " << (*clan)->out_rent << "\n"
		    << "Guard: " << (*clan)->guard << "\n"
		    << "BuiltOn: " << (*clan)->builtOn << "\n"
		    << "EntranceMode: " << (*clan)->entranceMode << "\n"
			<< "Storehouse: " << (*clan)->storehouse << "\n"
		    << "Exp: " << (*clan)->exp << "\n"
		    << "Bank: " << (*clan)->bank << "\n" << "Politics:\n";
		for (ClanPolitics::const_iterator it = (*clan)->politics.begin(); it != (*clan)->politics.end(); ++it)
			file << " " << (*it).first << " " << (*it).second << "\n";
		file << "~\n" << "Ranks:\n";
		int i = 0;

		for (std::vector < std::string >::const_iterator it =
		     (*clan)->ranks.begin(); it != (*clan)->ranks.end(); ++it, ++i)
			file << " " << (*it) << " " << (*clan)->privileges[i].to_ulong() << "\n";
		file << "~\n" << "Owner: ";
		for (ClanMemberList::const_iterator it = (*clan)->members.begin(); it != (*clan)->members.end(); ++it)
			if ((*it).second->rankNum == 0) {
				file << (*it).first << " " << (*it).second->money << " " << (*it).second->exp << " " << (*it).second->tax << "\n";
				break;
			}
		file << "Members:\n";
		for (ClanMemberList::const_iterator it = (*clan)->members.begin(); it != (*clan)->members.end(); ++it)
			if ((*it).second->rankNum != 0)
				file << " " << (*it).first << " " << (*it).second->rankNum << " " << (*it).second->money << " " << (*it).second->exp << " " << (*it).second->tax << "\n";
		file << "~\n";
		file.close();

		std::ofstream pkFile((LIB_HOUSE + buffer + ".pkl").c_str());
		if (!pkFile.is_open()) {
			log("Error open file: %s! (%s %s %d)",
			    (LIB_HOUSE + buffer + ".pkl").c_str(), __FILE__, __func__, __LINE__);
			return;
		}
		for (ClanPkList::const_iterator it = (*clan)->pkList.begin(); it != (*clan)->pkList.end(); ++it)
			pkFile << (*it).first << " " << (*it).second->author << " " << (*it).
			    second->time << " " << "0\n" << (*it).second->text << "\n~\n";
		for (ClanPkList::const_iterator it = (*clan)->frList.begin(); it != (*clan)->frList.end(); ++it)
			pkFile << (*it).first << " " << (*it).second->author << " " << (*it).
			    second->time << " " << "1\n" << (*it).second->text << "\n~\n";
		pkFile.close();
	}
	index.close();
}

// проставляем персонажу нужные поля если он клановый
// отсюда и далее для проверки на клановость пользуем GET_CLAN_RENT()
void Clan::SetClanData(CHAR_DATA * ch)
{
	GET_CLAN_RENT(ch) = 0;
	ClanListType::const_iterator clan;
	ClanMemberList::const_iterator it;
	// если куда-то приписан, то дергаем сразу итераторы на клан и список членов
	for (clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan) {
		it = (*clan)->members.find(GET_UNIQUE(ch));
		if (it != (*clan)->members.end()) {
			GET_CLAN_RENT(ch) = (*clan)->rent;
			break;
		}
	}
	// никуда не приписан
	if (!GET_CLAN_RENT(ch)) {
		if (GET_CLAN_STATUS(ch))
			free(GET_CLAN_STATUS(ch));
		GET_CLAN_STATUS(ch) = 0;
		GET_CLAN_RANKNUM(ch) = 0;
		return;
	}
	// куда-то таки приписан
	std::string buffer = (*clan)->ranks[(*it).second->rankNum] + " " + (*clan)->title;
	GET_CLAN_STATUS(ch) = str_dup(buffer.c_str());
	GET_CLAN_RANKNUM(ch) = (*it).second->rankNum;
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
	if (clan == Clan::ClanList.end() || IS_GRGOD(ch)
	    || !ROOM_FLAGGED(room, ROOM_HOUSE) || (*clan)->entranceMode)
		return 1;
	if (!GET_CLAN_RENT(ch))
		return 0;

	bool isMember = 0;

	if (GET_CLAN_RENT(ch) == (*clan)->rent || (*clan)->CheckPolitics(GET_CLAN_RENT(ch)) == POLITICS_ALLIANCE)
		isMember = 1;

	switch (mode) {
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
		if (RENTABLE(ch)) {
			if (mode == HCE_ATRIUM)
				send_to_char
				    ("Пускай сначала кровь с тебя стечет, а потом входи сколько угодно.\r\n", ch);
			return 0;
		}
		return 1;
	}
	return 0;
}

// возвращает итератор на клан, если персонаж куда-нить приписан
ClanListType::const_iterator Clan::GetClan(CHAR_DATA * ch)
{
	ClanListType::const_iterator clan;
	for (clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
		if (GET_CLAN_RENT(ch) == (*clan)->rent)
			return clan;
	return clan;
}

// в зависимости от доступности команды будут видны только нужные строки
const char *HOUSE_FORMAT[] = {
	"  замок информация\r\n",
	"  замок принять имя звание\r\n",
	"  замок изгнать имя\r\n",
	"  замок привилегии\r\n",
	"  гдругам (текст)\r\n",
	"  политика <имя дружины> <нейтралитет|война|альянс>\r\n",
	"  дрновости <писать|очистить>\r\n",
	"  пклист|дрлист <добавить|удалить>\r\n",
	"  класть вещи в хранилище\r\n",
	"  брать вещи из хранилища\r\n",
	"  казна (снимать)\r\n",
	"  замок покинуть (выход из дружины)\r\n"
};

// обработка клановых привилегий (команда house)
ACMD(DoHouse)
{
	if (IS_NPC(ch))
		return;

	std::string buffer = argument, buffer2;
	GetOneParam(buffer, buffer2);

	// если игрок неклановый, то есть вариант с приглашением
	if (!GET_CLAN_RENT(ch)) {
		if (CompareParam(buffer2, "согласен") && ch->desc->clan_invite)
			ch->desc->clan_invite->clan->ClanAddMember(ch, ch->desc->clan_invite->rank, 0, 0, 0);
		else
			send_to_char("Чаво ?\r\n", ch);
		return;
	}

	ClanListType::const_iterator clan = Clan::GetClan(ch);
	if (clan == Clan::ClanList.end()) {
		// здесь и далее - если мы сюда выпадаем, то мы уже где-то очень сильно накосячили ранее
		log("Player have GET_CLAN_RENT and dont stay in Clan::ClanList (%s %s %d)", __FILE__, __func__, __LINE__);
		return;
	}

	if (CompareParam(buffer2, "информация")
		&& (*clan)->privileges[GET_CLAN_RANKNUM(ch)][MAY_CLAN_INFO])
		(*clan)->HouseInfo(ch);
	else if (CompareParam(buffer2, "принять")
		&& (*clan)->privileges[GET_CLAN_RANKNUM(ch)][MAY_CLAN_ADD])
		(*clan)->HouseAdd(ch, buffer);
	else if (CompareParam(buffer2, "изгнать")
		&& (*clan)->privileges[GET_CLAN_RANKNUM(ch)][MAY_CLAN_REMOVE])
		(*clan)->HouseRemove(ch, buffer);
	else if (CompareParam(buffer2, "привилегии")
		&& (*clan)->privileges[GET_CLAN_RANKNUM(ch)][MAY_CLAN_PRIVILEGES]) {
		boost::shared_ptr<struct ClanOLC> temp_clan_olc(new ClanOLC);
		temp_clan_olc->clan = (*clan);
		temp_clan_olc->privileges = (*clan)->privileges;
		ch->desc->clan_olc = temp_clan_olc;
		STATE(ch->desc) = CON_CLANEDIT;
		(*clan)->MainMenu(ch->desc);
	} else if (CompareParam(buffer2, "воевода") && !GET_CLAN_RANKNUM(ch))
		(*clan)->HouseOwner(ch, buffer);
	else if (CompareParam(buffer2, "статистика"))
		(*clan)->HouseStat(ch, buffer);
	else if (CompareParam(buffer2, "покинуть", 1)
		&& (*clan)->privileges[GET_CLAN_RANKNUM(ch)][MAY_CLAN_LEAVE])
		(*clan)->HouseLeave(ch);
	else {
		// обработка списка доступных команд по званию персонажа
		buffer = "Доступные Вам привилегии дружины:\r\n";
		for (int i = 0; i <= CLAN_PRIVILEGES_NUM; ++i)
			if ((*clan)->privileges[GET_CLAN_RANKNUM(ch)][i])
				buffer += HOUSE_FORMAT[i];
		// воевода до кучи может сам сменить у дружины воеводу
		if (!GET_CLAN_RANKNUM(ch))
			buffer += "  замок воевода\r\n";
		if ((*clan)->storehouse)
			buffer += "  хранилище <фильтры>\r\n";
		buffer += "  замок статистика (все|очистить)\r\n";
		send_to_char(buffer, ch);
	}
}

// house информация
void Clan::HouseInfo(CHAR_DATA * ch)
{
	// думаю, вываливать список сортированный по уиду некрасиво, поэтому перебираем по рангам
	std::vector<ClanMemberPtr> tempList;
	for (ClanMemberList::const_iterator it = members.begin(); it != members.end(); ++it)
		tempList.push_back((*it).second);
	std::sort(tempList.begin(), tempList.end(), Clan::SortRank());
	std::ostringstream buffer;
	buffer << "К замку приписаны: ";
	for (std::vector<ClanMemberPtr>::const_iterator it = tempList.begin(); it != tempList.end(); ++it)
		buffer << (*it)->name << " ";
	buffer << "\r\nПривилегии:\r\n";
	int num = 0;

	for (std::vector<std::string>::const_iterator it = ranks.begin(); it != ranks.end(); ++it, ++num) {
		buffer << "  " << (*it) << ":";
		for (int i = 0; i < CLAN_PRIVILEGES_NUM; ++i)
			if (this->privileges[num][i])
				switch (i) {
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
				case MAY_CLAN_LEAVE:
					buffer << " покинуть";
					break;
				}
		buffer << "\r\n";
	}
	// инфа о банке и хранилище
	OBJ_DATA *temp, *chest;
	int chest_num = real_object(CLAN_CHEST), count = 0, cost = 0;
	if (chest_num >= 0) {
		for (chest = world[real_room(this->rent)]->contents; chest; chest = chest->next_content) {
			if (chest->item_number == chest_num) {
				// перебираем шмот
				for (temp = chest->contains; temp; temp = temp->next_content) {
					cost += GET_OBJ_RENTEQ(temp);
					++count;
				}
				break;
			}
		}
	}
	cost = cost * CLAN_STOREHOUSE_COEFF / 100;
	buffer << "В хранилище Вашей дружины " << count << " предметов (" << cost << " " << desc_count(cost, WHAT_MONEYa) << " в день).\r\n"
		<< "Состояние казны: " << this->bank << " " << desc_count(this->bank, WHAT_MONEYa) << ".\r\n"
		<< "Налог составляет " << CLAN_TAX + (this->storehouse * CLAN_STOREHOUSE_TAX) << " " << desc_count(CLAN_TAX + (this->storehouse * CLAN_STOREHOUSE_TAX), WHAT_MONEYa) << " в день.\r\n";
	cost += CLAN_TAX + (this->storehouse * CLAN_STOREHOUSE_TAX);
	if (!cost)
		buffer << "Ваших денег хватит на нереальное количество дней.\r\n";
	else
		buffer << "Ваших денег хватит примерно на " << this->bank / cost << " " << desc_count(this->bank/cost, WHAT_DAY) << ".\r\n";
	send_to_char(buffer.str(), ch);
}

// house принять
// повлиять можно только на соклановцев ниже рангов
void Clan::HouseAdd(CHAR_DATA * ch, std::string & buffer)
{
	std::string buffer2;
	GetOneParam(buffer, buffer2);
	if (buffer2.empty()) {
		send_to_char("Укажите имя персонажа.\r\n", ch);
		return;
	}
	long unique = 0;

	if (!(unique = GetUniqueByName(buffer2))) {
		send_to_char("Неизвестный персонаж.\r\n", ch);
		return;
	}
	std::string name = buffer2;
	name[0] = UPPER(name[0]);
	if (unique == GET_UNIQUE(ch)) {
		send_to_char("Сам себя повысил, самому себе вынес благодарность?\r\n", ch);
		return;
	}
	DESCRIPTOR_DATA *d;

	if (!(d = DescByUID(unique))) {
		send_to_char("Этого персонажа нет в игре!\r\n", ch);
		return;
	}
	if (GET_CLAN_RENT(d->character) && GET_CLAN_RENT(ch) != GET_CLAN_RENT(d->character)) {
		send_to_char("Вы не можете приписать члена другой дружины.\r\n", ch);
		return;
	}
	if (GET_CLAN_RENT(ch) == GET_CLAN_RENT(d->character)
	    && GET_CLAN_RANKNUM(ch) >= GET_CLAN_RANKNUM(d->character)) {
		send_to_char("Вы можете менять звания только у нижестоящих членов дружины.\r\n", ch);
		return;
	}
	GetOneParam(buffer, buffer2);

	// чтобы учесть воеводу с 0 рангом во время приписки и не дать ему приписать еще 10 воевод
	int rank = GET_CLAN_RANKNUM(ch);
	if(!rank)
		++rank;

	if (buffer2.empty()) {
		buffer = "Укажите звание персонажа.\r\nДоступные положения: ";
		for (std::vector<std::string>::const_iterator it = this->ranks.begin() + rank; it != this->ranks.end(); ++it)
			buffer += "'" + *it + "' ";
		buffer += "\r\n";
		send_to_char(buffer, ch);
		return;
	}
	int temp_rank = rank; // дальше rank тоже мб использован в случае неверного выбора
	for (std::vector<std::string>::const_iterator it = this->ranks.begin() + rank; it != this->ranks.end(); ++it, ++temp_rank)
		if (CompareParam(buffer2, *it)) {
			if (GET_CLAN_RENT(d->character) == GET_CLAN_RENT(ch)) {
				// игрок уже приписан к этой дружине, меняем только звание
				ClanAddMember(d->character, temp_rank, this->members[GET_UNIQUE(ch)]->money, this->members[GET_UNIQUE(ch)]->exp, this->members[GET_UNIQUE(ch)]->tax);
				return;
			} else {
				// не приписан - втыкаем ему приглашение и курим, пока не согласится
				if(ignores(d->character, ch, IGNORE_CLAN)) {
					send_to_char(ch, "%s игнорирует Ваши приглашения.\r\n", GET_NAME(d->character));
					return;
				}
				boost::shared_ptr<struct ClanInvite> temp_invite (new ClanInvite);
				ClanListType::const_iterator clan = Clan::GetClan(ch);
				if (clan == Clan::ClanList.end()) {
					log("Player have GET_CLAN_RENT and dont stay in Clan::ClanList (%s %s %d)", __FILE__, __func__, __LINE__);
					return;
				}
				temp_invite->clan = (*clan);
				temp_invite->rank = temp_rank;
				d->clan_invite = temp_invite;
				buffer = CCWHT(d->character, C_NRM);
				buffer += "$N приглашен$G в Вашу дружину, статус - " + *it + ".";
				buffer += CCNRM(d->character, C_NRM);
				// оповещаем счастливца
				act(buffer.c_str(), FALSE, ch, 0, d->character, TO_CHAR);
				buffer = CCWHT(d->character, C_NRM);
				buffer += "Вы получили приглашение в дружину '" + this->name + "', статус - " + *it + ".\r\n"
					+ "Чтобы принять приглашение наберите 'замок согласен'.\r\n";
				buffer += CCNRM(d->character, C_NRM);
				send_to_char(buffer, d->character);
				return;
			}
		}
	buffer = "Неверное звание, доступные положения:\r\n";
	for (std::vector<std::string>::const_iterator it = this->ranks.begin() + rank; it != this->ranks.end(); ++it)
		buffer += "'" + *it + "' ";
	buffer += "\r\n";
	send_to_char(buffer, ch);
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
	else if ((*it).second->rankNum <= GET_CLAN_RANKNUM(ch))
		send_to_char("Вы можете исключить из дружины только персонажа со званием ниже Вашего.\r\n", ch);
	else {
		this->members.erase(it);
		DESCRIPTOR_DATA *d, *k;
		if ((k = DescByUID(unique))) {
			Clan::SetClanData(k->character);
			send_to_char(k->character, "Вас исключили из дружины '%s'.\r\n", this->name.c_str());
		}
		for (d = descriptor_list; d; d = d->next) {
			if (d->character && GET_CLAN_RENT(d->character) == this->rent)
				send_to_char(d->character, "%s изгнан%s из числа Ваших соратников.\r\n", GET_NAME(k->character), GET_CH_SUF_1(k->character));
		}
	}
}

// обработка клан-канала и канала союзников, как игрока, так и имма
// клановые БОГи не могут говорить другим дружинам, и им и остальным спокойнее
// для канала союзников нужен обоюдный альянс дружин
ACMD(DoClanChannel)
{
	if (IS_NPC(ch))
		return;

	std::string buffer = argument;

	if (IS_GRGOD(ch) && !GET_CLAN_RENT(ch)) {
		// большой неклановый БОГ говорит какой-то дружине
		std::string buffer2;
		GetOneParam(buffer, buffer2);
		SkipSpaces(buffer);
		ClanListType::const_iterator clan = Clan::GetClan(ch);
		for (clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
			if (CompareParam((*clan)->abbrev, buffer2))
				break;
		if (clan == Clan::ClanList.end()) {
			send_to_char("Дружина с такой аббревиатурой не найдена.\r\n", ch);
			return;
		}
		if (buffer.empty()) {
			send_to_char("Что Вы хотите им сообщить?\r\n", ch);
			return;
		}
		switch (subcmd) {
			DESCRIPTOR_DATA *d;

			// большой БОГ говорит какой-то дружине
		case SCMD_CHANNEL:
			for (d = descriptor_list; d; d = d->next)
				if (d->character && STATE(d) == CON_PLAYING && !AFF_FLAGGED(d->character, AFF_DEAFNESS)
				    && (*clan)->rent == GET_CLAN_RENT(d->character) && ch != d->character)
					send_to_char(d->character, "%s ВАШЕЙ ДРУЖИНЕ: %s'%s'%s\r\n",
						     GET_NAME(ch), CCIRED(d->character, C_NRM),
						     buffer.c_str(), CCNRM(d->character, C_NRM));
			send_to_char(ch, "Вы дружине %s: %s'%s'.%s\r\n",
				     (*clan)->abbrev.c_str(), CCIRED(ch, C_NRM), buffer.c_str(), CCNRM(ch, C_NRM));
			break;
			// он же в канал союзников этой дружины, если они вообще есть
		case SCMD_ACHANNEL:
			for (d = descriptor_list; d; d = d->next)
				if (d->character && d->character != ch && STATE(d) == CON_PLAYING
				    && !AFF_FLAGGED(d->character, AFF_DEAFNESS) && GET_CLAN_RENT(d->character))
					if ((*clan)->CheckPolitics(GET_CLAN_RENT(d->character)) ==
					    POLITICS_ALLIANCE || GET_CLAN_RENT(d->character) == (*clan)->rent) {
						// проверка на альянс с обеих сторон, иначе это не альянс
						if (GET_CLAN_RENT(d->character) != (*clan)->rent) {
							ClanListType::const_iterator vict = Clan::GetClan(d->character);
							if (vict == Clan::ClanList.end()) {
								log("Player have GET_CLAN_RENT and dont stay in Clan::ClanList (%s %s %d)", __FILE__, __func__, __LINE__);
								return;
							}
							if ((*vict)->CheckPolitics((*clan)->rent) == POLITICS_ALLIANCE)
								send_to_char(d->character,
									     "%s ВАШИМ СОЮЗНИКАМ: %s'%s'%s\r\n",
									     GET_NAME(ch), CCIGRN(d->character, C_NRM),
									     buffer.c_str(), CCNRM(d->character,
												   C_NRM));
						}
						// первоначальному клану выдается всегда
						else
							send_to_char(d->character, "%s ВАШИМ СОЮЗНИКАМ: %s'%s'%s\r\n",
								     GET_NAME(ch), CCIGRN(d->character, C_NRM),
								     buffer.c_str(), CCNRM(d->character, C_NRM));
					}
			send_to_char(ch, "Вы союзникам %s: %s'%s'.%s\r\n",
				     (*clan)->abbrev.c_str(), CCIGRN(ch, C_NRM), buffer.c_str(), CCNRM(ch, C_NRM));
			break;
		}
	} else {
		if (!GET_CLAN_RENT(ch)) {
			send_to_char("Вы не принадлежите ни к одной дружине.\r\n", ch);
			return;
		}
		ClanListType::const_iterator clan = Clan::GetClan(ch);
		if (clan == Clan::ClanList.end()) {
			log("Player have GET_CLAN_RENT and dont stay in Clan::ClanList (%s %s %d)", __FILE__, __func__,
			    __LINE__);
			return;
		}
		// ограничения на клан-канал не канают на любое звание, если это БОГ
		if (!IS_IMMORTAL(ch)
		    && !(*clan)->privileges[GET_CLAN_RANKNUM(ch)][MAY_CLAN_CHANNEL]
		    || PLR_FLAGGED(ch, PLR_DUMB)) {
			send_to_char("Вы не можете пользоваться каналом дружины.\r\n", ch);
			return;
		}
		SkipSpaces(buffer);
		if (buffer.empty()) {
			send_to_char("Что Вы хотите сообщить?\r\n", ch);
			return;
		}
		DESCRIPTOR_DATA *d;

		switch (subcmd) {
			// своей дружине
		case SCMD_CHANNEL:
			for (d = descriptor_list; d; d = d->next)
				if (d->character && d->character != ch && STATE(d) == CON_PLAYING
				    && !AFF_FLAGGED(d->character, AFF_DEAFNESS)
				    && GET_CLAN_RENT(d->character) == GET_CLAN_RENT(ch) && !ignores(d->character, ch, IGNORE_CLAN))
					send_to_char(d->character, "%s дружине: %s'%s'.%s\r\n",
						     GET_NAME(ch), CCIRED(d->character, C_NRM),
						     buffer.c_str(), CCNRM(d->character, C_NRM));
			send_to_char(ch, "Вы дружине: %s'%s'.%s\r\n", CCIRED(ch, C_NRM),
				     buffer.c_str(), CCNRM(ch, C_NRM));
			break;
			// союзникам
		case SCMD_ACHANNEL:
			for (d = descriptor_list; d; d = d->next)
				if (d->character && d->character != ch && STATE(d) == CON_PLAYING
				    && !AFF_FLAGGED(d->character, AFF_DEAFNESS) && !PRF_FLAGGED (d->character, PRF_ALLIANCE)
				    && GET_CLAN_RENT(d->character) && !ignores(d->character, ch, IGNORE_ALLIANCE))
					if ((*clan)->CheckPolitics(GET_CLAN_RENT(d->character)) ==
					    POLITICS_ALLIANCE || GET_CLAN_RENT(ch) == GET_CLAN_RENT(d->character)) {
						// проверка на альянс с обеих сторон, шоб не спамили друг другу на зло
						if (GET_CLAN_RENT(ch) != GET_CLAN_RENT(d->character)) {
							ClanListType::const_iterator vict = Clan::GetClan(d->character);
							if (vict == Clan::ClanList.end()) {
								log("Player have GET_CLAN_RENT and dont stay in Clan::ClanList (%s %s %d)", __FILE__, __func__, __LINE__);
								return;
							}
							if ((*vict)->CheckPolitics(GET_CLAN_RENT(ch)) == POLITICS_ALLIANCE)
								send_to_char(d->character, "%s союзникам: %s'%s'.%s\r\n",
									     GET_NAME(ch), CCIGRN(d->character, C_NRM),
									     buffer.c_str(), CCNRM(d->character, C_NRM));
						}
						// своему клану выдается всегда
						else
							send_to_char(d->character, "%s союзникам: %s'%s'.%s\r\n",
								     GET_NAME(ch), CCIGRN(d->character, C_NRM),
								     buffer.c_str(), CCNRM(d->character, C_NRM));
					}
			send_to_char(ch, "Вы союзникам: %s'%s'.%s\r\n", CCIGRN(ch, C_NRM),
				     buffer.c_str(), CCNRM(ch, C_NRM));
			break;
		}
	}
}

// список зарегестрированных дружин с их онлайновым составом (опционально)
ACMD(DoClanList)
{
	if (IS_NPC(ch))
		return;

	std::string buffer = argument;
	SkipSpaces(buffer);
	ClanListType::const_iterator clan;

	if (buffer.empty()) {
		// сортировка кланов по экспе
		std::multimap<unsigned long long, ClanPtr> sort_clan;
		for (clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
			sort_clan.insert(std::make_pair((*clan)->members.empty() ? (*clan)->exp : (*clan)->exp/(*clan)->members.size(), (*clan)));

		std::ostringstream out;
		out << "В игре зарегистрированы следующие дружины:\r\n"
				<< "     #                   Глава Название                       Набрано очков\r\n\r\n";
		int count = 1;
		// опять же, может и руки, но не получается у меня попользовать реверсные итераторы
		for (std::multimap<unsigned long long, ClanPtr>::reverse_iterator it = sort_clan.rbegin(); it != sort_clan.rend(); ++it, ++count) {
			out << std::setw(6) << count << "  " << std::setw(6) << (*it).second->abbrev << " " << std::setw(15)
				<< (*it).second->owner << " ";
			out.setf(std::ios_base::left, std::ios_base::adjustfield);
			out << std::setw(30) << (*it).second->name << " " << (*it).second->exp << "\r\n";
			out.setf(std::ios_base::right, std::ios_base::adjustfield);
		}
		send_to_char(out.str(), ch);
		return;
	}

	bool all = 1;
	std::vector<CHAR_DATA *> tempList;

	if (!CompareParam(buffer, "все")) {
		for (clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
			if (CompareParam(buffer, (*clan)->abbrev))
				break;
		if (clan == Clan::ClanList.end()) {
			send_to_char("Такая дружина не зарегистрирована\r\n", ch);
			return;
		}
		all = 0;
	}
	// строится список членов дружины или всех дружин (по флагу all)
	DESCRIPTOR_DATA *d;
	for (d = descriptor_list; d; d = d->next)
		if (d->character && STATE(d) == CON_PLAYING && GET_CLAN_RENT(d->character)
		    && CAN_SEE_CHAR(ch, d->character) && !IS_IMMORTAL(d->character))
			if (all || GET_CLAN_RENT(d->character) == (*clan)->rent)
				tempList.push_back(d->character);
	// до кучи сортировка по рангам
	std::sort(tempList.begin(), tempList.end(), Clan::SortRank());

	std::ostringstream buffer2;
	buffer2 << "В игре зарегистрированы следующие дружины:\r\n" << "     #                  Глава Название\r\n\r\n";
	boost::format clanFormat(" %5d  %6s %15s %s\r\n");
	boost::format memberFormat(" %-10s%s%s%s %s%s%s\r\n");
	// если искали конкретную дружину - выводим ее
	if (!all) {
		buffer2 << clanFormat % 0 % (*clan)->abbrev % (*clan)->owner % (*clan)->name;
		for (std::vector < CHAR_DATA * >::const_iterator it = tempList.begin(); it != tempList.end(); ++it)
			buffer2 << memberFormat % (*clan)->ranks[GET_CLAN_RANKNUM(*it)] %
			    CCPK(*it, C_NRM, *it) % noclan_title(*it) % CCNRM(*it, C_NRM) % CCIRED(*it, C_NRM)
			    % (PLR_FLAGGED(*it, PLR_KILLER) ? "(ДУШЕГУБ)" : "") % CCNRM(*it, C_NRM);
	}
	// просто выводим все дружины и всех членов (без параметра 'все' в списке будут только дружины)
	else {
		int count = 1;
		for (clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan, ++count) {
			buffer2 << clanFormat % count % (*clan)->abbrev % (*clan)->owner % (*clan)->name;
			for (std::vector < CHAR_DATA * >::const_iterator it = tempList.begin(); it != tempList.end(); ++it)
				if (GET_CLAN_RENT(*it) == (*clan)->rent)
					buffer2 << memberFormat % (*clan)->ranks[GET_CLAN_RANKNUM(*it)] %
					    CCPK(*it, C_NRM, *it) % noclan_title(*it) % CCNRM(*it, C_NRM) %
					    CCIRED(*it, C_NRM) % (PLR_FLAGGED(*it, PLR_KILLER) ? "(ДУШЕГУБ)" : "") % CCNRM(*it, C_NRM);
		}
	}
	send_to_char(buffer2.str(), ch);
}

// возвращает состояние политики clan по отношению к victim
// при отсутствии подразумевается нейтралитет
int Clan::CheckPolitics(room_vnum victim)
{
	ClanPolitics::const_iterator it = politics.find(victim);
	if (it != politics.end())
		return (*it).second;
	return POLITICS_NEUTRAL;
}

// выставляем клану политику(state) по отношению к victim
// нейтралитет означает просто удаление записи, если она была
void Clan::SetPolitics(room_vnum victim, int state)
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
	if (IS_NPC(ch) || !GET_CLAN_RENT(ch)) {
		send_to_char("Чаво ?\r\n", ch);
		return;
	}
	ClanListType::const_iterator clan = Clan::GetClan(ch);
	if (clan == Clan::ClanList.end()) {
		log("Player have GET_CLAN_RENT and dont stay in Clan::ClanList (%s %s %d)",
		    __FILE__, __func__, __LINE__);
		return;
	}
	std::string buffer = argument;
	SkipSpaces(buffer);
	if (!buffer.empty()
	    && (*clan)->privileges[GET_CLAN_RANKNUM(ch)][MAY_CLAN_POLITICS]) {
		(*clan)->ManagePolitics(ch, buffer);
		return;
	}
	boost::format strFormat("  %-3s             %s%-11s%s                 %s%-11s%s\r\n");
	int p1 = 0, p2 = 0;

	std::ostringstream buffer2;
	buffer2 << "Отношения Вашей дружины с другими дружинами:\r\n" <<
	    "Название     Отношение Вашей дружины     Отношение к Вашей дружине\r\n";

	ClanListType::const_iterator clanVictim;
	for (clanVictim = Clan::ClanList.begin(); clanVictim != Clan::ClanList.end(); ++clanVictim) {
		if (clanVictim == clan)
			continue;
		p1 = (*clan)->CheckPolitics((*clanVictim)->rent);
		p2 = (*clanVictim)->CheckPolitics((*clan)->rent);
		buffer2 << strFormat % (*clanVictim)->abbrev % (p1 == POLITICS_WAR ? CCIRED(ch, C_NRM)
								 : (p1 ==
								    POLITICS_ALLIANCE ?
								    CCGRN(ch, C_NRM) : CCNRM(ch, C_NRM)))
		    % politicsnames[p1] % CCNRM(ch, C_NRM) % (p2 == POLITICS_WAR ? CCIRED(ch, C_NRM)
							      : (p2 == POLITICS_ALLIANCE ? CCGRN(ch, C_NRM)
								 : CCNRM(ch,
									 C_NRM))) %
		    politicsnames[p2] % CCNRM(ch, C_NRM);
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
	if (vict == Clan::ClanList.end()) {
		send_to_char("Нет такой дружины.\r\n", ch);
		return;
	}
	if ((*vict)->rent == GET_CLAN_RENT(ch)) {
		send_to_char("Менять политику по отношению к своей дружине? Что за бред...\r\n", ch);
		return;
	}
	GetOneParam(buffer, buffer2);
	if (buffer2.empty())
		send_to_char("Укажите действие: нейтралитет|война|альянс.\r\n", ch);
	else if (CompareParam(buffer2, "нейтралитет")) {
		if (CheckPolitics((*vict)->rent) == POLITICS_NEUTRAL) {
			send_to_char(ch,
				     "Ваша дружина уже находится в состоянии нейтралитета с дружиной %s.\r\n",
				     (*vict)->abbrev.c_str());
			return;
		}
		SetPolitics((*vict)->rent, POLITICS_NEUTRAL);
		send_to_char(ch, "Вы заключили нейтралитет с дружиной %s.\r\n", (*vict)->abbrev.c_str());
	} else if (CompareParam(buffer2, "война")) {
		if (CheckPolitics((*vict)->rent) == POLITICS_WAR) {
			send_to_char(ch, "Ваша дружина уже воюет с дружиной %s.\r\n", (*vict)->abbrev.c_str());
			return;
		}
		SetPolitics((*vict)->rent, POLITICS_WAR);
		// уведомим другую дружину
		for (d = descriptor_list; d; d = d->next)
			if (d->character && d->character != ch && STATE(d) == CON_PLAYING
			    && GET_CLAN_RENT(d->character) == (*vict)->rent)
				send_to_char(d->character,
					     "%sДружина %s объявила вашей дружине войну!!!%s\r\n",
					     CCIRED(d->character, C_NRM), this->abbrev.c_str(), CCNRM(d->character, C_NRM));
		send_to_char(ch, "Вы объявили войну дружине %s.\r\n", (*vict)->abbrev.c_str());
	} else if (CompareParam(buffer2, "альянс")) {
		if (CheckPolitics((*vict)->rent) == POLITICS_ALLIANCE) {
			send_to_char(ch, "Ваша дружина уже в альянсе с дружиной %s.\r\n", (*vict)->abbrev.c_str());
			return;
		}
		SetPolitics((*vict)->rent, POLITICS_ALLIANCE);
		// тож самое
		for (d = descriptor_list; d; d = d->next)
			if (d->character && d->character != ch && STATE(d) == CON_PLAYING
			    && GET_CLAN_RENT(d->character) == (*vict)->rent)
				send_to_char(d->character,
					     "%sДружина %s заключила с вашей дружиной альянс!!!%s\r\n",
					     CCIRED(d->character, C_NRM), this->abbrev.c_str(), CCNRM(d->character, C_NRM));
		send_to_char(ch, "Вы заключили альянс с дружиной %s.\r\n", (*vict)->abbrev.c_str());
	}
}

const char *HCONTROL_FORMAT =
    "Формат: hcontrol build <rent vnum> <outrent vnum> <guard vnum> <leader name> <abbreviation> <clan title> <clan name>\r\n"
	"        hcontrol show\r\n"
	"        hcontrol save\r\n"
    "        hcontrol destroy <house vnum>\r\n";

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
	else if (CompareParam(buffer2, "show"))
		Clan::ClanInfo(ch);
	else if (CompareParam(buffer2, "save"))
		Clan::ClanSave();
	else
		send_to_char(HCONTROL_FORMAT, ch);
}

// создрание дружины (hcontrol build)
void Clan::HcontrolBuild(CHAR_DATA * ch, std::string & buffer)
{
	// парсим все параметры, чтобы сразу проверить на ввод всех полей
	std::string buffer2;
	// рента
	GetOneParam(buffer, buffer2);
	room_vnum rent = atoi(buffer2.c_str());
	// рента вне замка
	GetOneParam(buffer, buffer2);
	room_vnum out_rent = atoi(buffer2.c_str());
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
	SkipSpaces(buffer);
	std::string name = buffer;

	// тут проверяем наличие все этого дела
	if (name.empty()) {
		send_to_char(HCONTROL_FORMAT, ch);
		return;
	}
	if (!real_room(rent)) {
		send_to_char(ch, "Комнаты %d не существует.\r\n", rent);
		return;
	}
	if (!real_room(out_rent)) {
		send_to_char(ch, "Комнаты %d не существует.\r\n", out_rent);
		return;
	}
	if (real_mobile(guard) < 0) {
		send_to_char(ch, "Моба %d не существует.\r\n", guard);
		return;
	}
	long unique = 0;
	if (!(unique = GetUniqueByName(owner))) {
		send_to_char(ch, "Персонажа %s не существует.\r\n", owner.c_str());
		return;
	}
	// а тут - не занят ли параметр другим кланом
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan) {
		if ((*clan)->rent == rent) {
			send_to_char(ch, "Комната %d уже занята другой дружиной.\r\n", rent);
			return;
		}
		if ((*clan)->guard == guard) {
			send_to_char(ch, "Охранник %d уже занят другой дружиной.\r\n", rent);
			return;
		}
		ClanMemberList::const_iterator it = (*clan)->members.find(unique);
		if (it != (*clan)->members.end()) {
			send_to_char(ch, "%s уже приписан к дружине %s.\r\n", owner.c_str(), (*clan)->abbrev.c_str());
			return;
		}
		if (CompareParam((*clan)->abbrev, abbrev, 1)) {
			send_to_char(ch, "Аббревиатура '%s' уже занята другой дружиной.\r\n", abbrev.c_str());
			return;
		}
		if (CompareParam((*clan)->name, name, 1)) {
			send_to_char(ch, "Имя '%s' уже занято другой дружиной.\r\n", name.c_str());
			return;
		}
		if (CompareParam((*clan)->title, title, 1)) {
			send_to_char(ch, "Приписка в титул '%s' уже занята другой дружиной.\r\n", title.c_str());
			return;
		}
	}

	// собственно клан
	ClanPtr tempClan(new Clan);
	tempClan->rent = rent;
	tempClan->out_rent = out_rent;
	tempClan->guard = guard;
	// пишем воеводу
	owner[0] = UPPER(owner[0]);
	tempClan->owner = owner;
	ClanMemberPtr tempMember(new ClanMember);
	tempMember->name = owner;
	tempMember->rankNum = 0;
	tempMember->money = 0;
	tempMember->exp = 0;
	tempMember->tax = 0;
	tempClan->members[unique] = tempMember;
	// названия
	tempClan->abbrev = abbrev;
	tempClan->name = name;
	tempClan->title = title;
	// ранги
	const char *ranks[] = { "воевода", "боярин", "десятник", "храбр", "кметь", "гридень", "муж", "вой", "отрок", "гость" };
	std::vector<std::string> tempRanks(ranks, ranks + 10);
	tempClan->ranks = tempRanks;
	// привилегии
	for (std::vector<std::string>::const_iterator it =
	     tempClan->ranks.begin(); it != tempClan->ranks.end(); ++it)
		tempClan->privileges.push_back(std::bitset<CLAN_PRIVILEGES_NUM> (0));
	// воеводе проставим все привилегии
	for (int i = 0; i < CLAN_PRIVILEGES_NUM; ++i)
		tempClan->privileges[0].set(i);

	Clan::ClanList.push_back(tempClan);
	Clan::ClanSave();
	Board::ClanInit();

	// уведомляем счастливых воеводу и имма
	DESCRIPTOR_DATA *d;
	if ((d = DescByUID(unique))) {
		Clan::SetClanData(d->character);
		send_to_char(d->character, "Вы стали хозяином нового замка. Добро пожаловать!\r\n");
	}
	send_to_char(ch, "Дружина '%s' создана!\r\n", abbrev.c_str());
}

// удаление дружины (hcontrol destroy)
void Clan::HcontrolDestroy(CHAR_DATA * ch, std::string & buffer)
{
	SkipSpaces(buffer);
	room_vnum rent = atoi(buffer.c_str());

	ClanListType::iterator clan;
	for (clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
		if ((*clan)->rent == rent)
			break;
	if (clan == Clan::ClanList.end()) {
		send_to_char(ch, "Дружины с номером %d не существует.\r\n", rent);
		return;
	}

	// если воевода онлайн - обрадуем его лично
	DESCRIPTOR_DATA *d;
	ClanMemberList members = (*clan)->members;
	Clan::ClanList.erase(clan);
	Clan::ClanSave();
	Board::ClanInit();
	// уведомляем и чистим инфу игрокам
	for (ClanMemberList::const_iterator it = members.begin(); it != members.end(); ++it) {
		if ((d = DescByUID((*it).first))) {
			Clan::SetClanData(d->character);
			send_to_char(d->character, "Ваша дружина распущена. Желаем удачи!\r\n");
		}
	}
	send_to_char("Дружина распущена.\r\n", ch);
}

// ктодружина (список соклановцев, находящихся онлайн)
ACMD(DoWhoClan)
{
	if (IS_NPC(ch) || !GET_CLAN_RENT(ch)) {
		send_to_char("Чаво ?\r\n", ch);
		return;
	}

	ClanListType::const_iterator clan = Clan::GetClan(ch);
	if (clan == Clan::ClanList.end()) {
		log("Player have GET_CLAN_RENT and dont stay in Clan::ClanList (%s %s %d)",
		    __FILE__, __func__, __LINE__);
		return;
	}

	std::ostringstream buffer;
	buffer << boost::format(" Ваша дружина: %s%s%s.\r\n") % CCIRED(ch, C_NRM) % (*clan)->abbrev % CCNRM(ch, C_NRM);
	buffer << boost::format(" %sСейчас в игре Ваши соратники:%s\r\n\r\n") % CCWHT(ch, C_NRM) % CCNRM(ch, C_NRM);
	DESCRIPTOR_DATA *d;
	int num = 0;

	for (d = descriptor_list, num = 0; d; d = d->next)
		if (d->character && STATE(d) == CON_PLAYING && GET_CLAN_RENT(d->character) == GET_CLAN_RENT(ch)) {
			buffer << "    " << race_or_title(d->character) << "\r\n";
			++num;
		}
	buffer << "\r\n Всего: " << num << ".\r\n";
	send_to_char(buffer.str(), ch);
}

const char *CLAN_PKLIST_FORMAT[] = {
	"Формат: пклист|дрлист\r\n"
	"        пклист|дрлист все\r\n",
	"        пклист|дрлист добавить имя причина\r\n"
	"        пклист|дрлист удалить имя|все\r\n"
};

// пкл/дрл
ACMD(DoClanPkList)
{
	if (IS_NPC(ch) || !GET_CLAN_RENT(ch)) {
		send_to_char("Чаво ?\r\n", ch);
		return;
	}

	ClanListType::const_iterator clan = Clan::GetClan(ch);
	if (clan == Clan::ClanList.end()) {
		log("Player have GET_CLAN_RENT and dont stay in Clan::ClanList (%s %s %d)",
		    __FILE__, __func__, __LINE__);
		return;
	}

	std::string buffer = argument, buffer2;
	GetOneParam(buffer, buffer2);
	std::ostringstream info;
	char timeBuf[11];
	DESCRIPTOR_DATA *d;

	boost::format frmt("%s [%s] :: %s\r\n%s\r\n\r\n");
	// выводим список тех, кто онлайн
	if (buffer2.empty()) {
		ClanPkList::const_iterator it;
		info << CCWHT(ch,
			      C_NRM) << "Отображаются только находящиеся в игре персонажи:\r\n\r\n" << CCNRM(ch, C_NRM);
		// вобщем, наверное, лучше искать по списку онлайновых, чем по списку пк/др листа
		for (d = descriptor_list; d; d = d->next)
			if (d->character && STATE(d) == CON_PLAYING && GET_CLAN_RENT(d->character) != GET_CLAN_RENT(ch)) {
				// пкл
				if (!subcmd)
					it = (*clan)->pkList.find(GET_UNIQUE(d->character));
				// дрл
				else
					it = (*clan)->frList.find(GET_UNIQUE(d->character));
				if (it != (*clan)->pkList.end() && it != (*clan)->frList.end()) {
					strftime(timeBuf, sizeof(timeBuf), "%d/%m/%Y",
						 localtime(&((*it).second->time)));
					info << frmt % timeBuf %
					    ((*it).second->author ? (*it).second->
					     authorName : "Того уж с нами нет") % (*it).second->victimName %
					    (*it).second->text;
				}
			}
		// тут и далее - по крайней мере все в одном месте и забыть очистить сложно
		char *text = str_dup(info.str());
		page_string(ch->desc, text, TRUE);
		free(text);

	} else if (CompareParam(buffer2, "все") || CompareParam(buffer2, "all")) {
		// выводим весь список
		info << CCWHT(ch, C_NRM) << "Список отображается полностью:\r\n\r\n" << CCNRM(ch, C_NRM);
		// пкл
		if (!subcmd)
			for (ClanPkList::const_iterator it = (*clan)->pkList.begin(); it != (*clan)->pkList.end(); ++it) {
				strftime(timeBuf, sizeof(timeBuf), "%d/%m/%Y", localtime(&((*it).second->time)));
				info << frmt % timeBuf % ((*it).second->author ? (*it).second->authorName : "Того уж с нами нет")
					% (*it).second->victimName % (*it).second->text;
			}
		// дрл
		else
			for (ClanPkList::const_iterator it = (*clan)->frList.begin(); it != (*clan)->frList.end(); ++it) {
				strftime(timeBuf, sizeof(timeBuf), "%d/%m/%Y", localtime(&((*it).second->time)));
				info << frmt % timeBuf % ((*it).second->author ? (*it).second->authorName : "Того уж с нами нет")
					% (*it).second->victimName % (*it).second->text;
			}

		char *text = str_dup(info.str());
		page_string(ch->desc, text, TRUE);
		free(text);

	} else if ((CompareParam(buffer2, "добавить") || CompareParam(buffer2, "add"))
		   && (*clan)->privileges[GET_CLAN_RANKNUM(ch)][MAY_CLAN_PKLIST]) {
		// добавляем нового
		GetOneParam(buffer, buffer2);
		if (buffer2.empty()) {
			send_to_char("Кого добавить то?\r\n", ch);
			return;
		}
		long unique = GetUniqueByName(buffer2, 1);

		if (!unique) {
			send_to_char("Интересующий Вас персонаж не найден.\r\n", ch);
			return;
		}
		if (unique < 0) {
			send_to_char("Не дело это, Богов добалять куда не надо....\r\n", ch);
			return;
		}
		ClanMemberList::const_iterator it = (*clan)->members.find(unique);
		if (it != (*clan)->members.end()) {
			send_to_char("Давайте не будем засорять список всяким бредом?\r\n", ch);
			return;
		}
		SkipSpaces(buffer);
		if (buffer.empty()) {
			send_to_char("Потрудитесь прокомментировать, за что Вы его так.\r\n", ch);
			return;
		}

		// собственно пишем новую жертву/друга
		ClanPkPtr tempRecord(new ClanPk);
		tempRecord->author = GET_UNIQUE(ch);
		tempRecord->authorName = GET_NAME(ch);
		buffer2[0] = UPPER(buffer2[0]);
		tempRecord->victimName = buffer2;
		tempRecord->time = time(0);
		tempRecord->text = buffer;
		if (!subcmd)
			(*clan)->pkList[unique] = tempRecord;
		else
			(*clan)->frList[unique] = tempRecord;
		DESCRIPTOR_DATA *d;
		if ((d = DescByUID(unique))) {
			if (!subcmd)
				send_to_char(d->character, "%sДружина '%s' добавила Вас в список своих врагов!%s\r\n", CCIRED(d->character, C_NRM), (*clan)->name.c_str(), CCNRM(d->character, C_NRM));
			else
				send_to_char(d->character, "%sДружина '%s' добавила Вас в список своих друзей!%s\r\n", CCGRN(d->character, C_NRM), (*clan)->name.c_str(), CCNRM(d->character, C_NRM));
		}
		send_to_char("Ладушки, добавили.\r\n", ch);

	} else if ((CompareParam(buffer2, "удалить") || CompareParam(buffer2, "remove"))
		   && (*clan)->privileges[GET_CLAN_RANKNUM(ch)][MAY_CLAN_PKLIST]) {
		// удаление записи
		GetOneParam(buffer, buffer2);
		if (CompareParam(buffer2, "все", 1)) {
			// пкл
			if (!subcmd)
				(*clan)->pkList.erase((*clan)->pkList.begin(), (*clan)->pkList.end());
			// дрл
			else
				(*clan)->frList.erase((*clan)->frList.begin(), (*clan)->frList.end());
			send_to_char("Список очищен.\r\n", ch);
			return;
		}
		long unique = GetUniqueByName(buffer2, 1);

		if (unique <= 0) {
			send_to_char("Интересующий Вас персонаж не найден.\r\n", ch);
			return;
		}
		ClanPkList::iterator it;
		// пкл
		if (!subcmd)
		{
			it = (*clan)->pkList.find(unique);
			if (it != (*clan)->pkList.end())
				(*clan)->pkList.erase(it);
		// дрл
		} else
		{
			it = (*clan)->frList.find(unique);
			if (it != (*clan)->frList.end())
				(*clan)->frList.erase(it);
		}

		if (it != (*clan)->pkList.end() && it != (*clan)->frList.end()) {
			send_to_char("Запись удалена.\r\n", ch);
			DESCRIPTOR_DATA *d;
			if ((d = DescByUID(unique))) {
				if (!subcmd)
					send_to_char(d->character, "%sДружина '%s' удалила Вас из списка своих врагов!%s\r\n", CCGRN(d->character, C_NRM), (*clan)->name.c_str(), CCNRM(d->character, C_NRM));
				else
					send_to_char(d->character, "%sДружина '%s' удалила Вас из списка своих друзей!%s\r\n", CCIRED(d->character, C_NRM), (*clan)->name.c_str(), CCNRM(d->character, C_NRM));
			}
    	} else
			send_to_char("Запись не найдена.\r\n", ch);

	} else {
		if ((*clan)->privileges[GET_CLAN_RANKNUM(ch)][MAY_CLAN_PKLIST]) {
			buffer = CLAN_PKLIST_FORMAT[0];
			buffer += CLAN_PKLIST_FORMAT[1];
			send_to_char(buffer, ch);
		} else
			send_to_char(CLAN_PKLIST_FORMAT[0], ch);
	}
}

// смотрим в сундук (могут все соклановцы)
SPECIAL(Clan::ClanChest)
{
	OBJ_DATA *chest = (OBJ_DATA *) me;

	if (!ch->desc)
		return 0;
	if (CMD_IS("смотреть") || CMD_IS("осмотреть") || CMD_IS("look") || CMD_IS("examine")) {
		std::string buffer = argument, buffer2;
		GetOneParam(buffer, buffer2);
		SkipSpaces(buffer);
		// эта мутная запись для всяких 'см в сундук' и подобных написаний в два слова
		if (buffer2.empty()
		    || (buffer.empty() && !isname(buffer2.c_str(), chest->name))
		    || (!buffer.empty() && !isname(buffer.c_str(), chest->name)))
			return 0;
		// засланым казачкам показываем хер, а не хранилище
		if (real_room(GET_CLAN_RENT(ch)) != IN_ROOM(ch)) {
			send_to_char("Не на что тут глазеть, пусто, вот те крест.\r\n", ch);
			return 1;
		}
		send_to_char("Хранилище Вашей дружины:\r\n", ch);
		list_obj_to_char(chest->contains, ch, 1, 3);
		return 1;
	}
	return 0;
}

// кладем в сундук (при наличии привилегии)
// если предмет - деньги, то автоматом идут в клан-казну, контейнеры только пустые
bool Clan::PutChest(CHAR_DATA * ch, OBJ_DATA * obj, OBJ_DATA * chest)
{
	if (IS_NPC(ch) || !GET_CLAN_RENT(ch) || real_room(GET_CLAN_RENT(ch)) != IN_ROOM(ch)) {
		send_to_char("Не имеете таких правов!\r\n", ch);
		return 0;
	}
	ClanListType::const_iterator clan = Clan::GetClan(ch);
	if (clan == Clan::ClanList.end()) {
		log("Player have GET_CLAN_RENT and dont stay in Clan::ClanList (%s %s %d)", __FILE__, __func__, __LINE__);
		return 0;
	}
	if (!(*clan)->privileges[GET_CLAN_RANKNUM(ch)][MAY_CLAN_CHEST_PUT]) {
		send_to_char("Не имеете таких правов!\r\n", ch);
		return 0;
	}

	if (IS_OBJ_STAT(obj, ITEM_NODROP) || OBJ_FLAGGED(obj, ITEM_ZONEDECAY) || GET_OBJ_TYPE(obj) == ITEM_KEY)
		act("Неведомая сила помешала положить $o3 в $O3.", FALSE, ch, obj, chest, TO_CHAR);
	else if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && obj->contains)
		act("В $o5 что-то лежит.", FALSE, ch, obj, 0, TO_CHAR);

	else if (GET_OBJ_TYPE(obj) == ITEM_MONEY) {
		long gold = GET_OBJ_VAL(obj, 0);
		// здесь и далее: в случае переполнения  - кладем сколько можем, остальное возвращаем чару
		// на данный момент для персонажей и кланов максимум 2147483647 кун
		// не знаю как сейчас в маде с деньгами и есть ли смысл поднимать планку
		if (((*clan)->bank + gold) < 0) {
			long over = LONG_MAX - (*clan)->bank;
			(*clan)->bank += over;
			(*clan)->members[GET_UNIQUE(ch)]->money += over;
			gold -= over;
			GET_GOLD(ch) += gold;
			obj_from_char(obj);
			extract_obj(obj);
			send_to_char(ch, "Вы удалось вложить в казну дружины только %ld %s.\r\n", over, desc_count(over, WHAT_MONEYu));
			return 1;
		}
		(*clan)->bank += gold;
		(*clan)->members[GET_UNIQUE(ch)]->money += gold;
		obj_from_char(obj);
		extract_obj(obj);
		send_to_char(ch, "Вы вложили в казну дружины %ld %s.\r\n", gold, desc_count(gold, WHAT_MONEYu));

	} else {
		obj_from_char(obj);
		obj_to_obj(obj, chest);
		act("$n положил$g $o3 в $O3.", TRUE, ch, obj, chest, TO_ROOM);
		act("Вы положили $o3 в $O3.", FALSE, ch, obj, chest, TO_CHAR);
	}
	return 1;
}

// берем из клан-сундука (при наличии привилегии)
bool Clan::TakeChest(CHAR_DATA * ch, OBJ_DATA * obj, OBJ_DATA * chest)
{
	if (IS_NPC(ch) || !GET_CLAN_RENT(ch) || real_room(GET_CLAN_RENT(ch)) != IN_ROOM(ch)) {
		send_to_char("Не имеете таких правов!\r\n", ch);
		return 0;
	}
	ClanListType::const_iterator clan = Clan::GetClan(ch);
	if (clan == Clan::ClanList.end()) {
		log("Player have GET_CLAN_RENT and dont stay in Clan::ClanList (%s %s %d)",
		    __FILE__, __func__, __LINE__);
		return 0;
	}
	if (!(*clan)->privileges[GET_CLAN_RANKNUM(ch)][MAY_CLAN_CHEST_TAKE]) {
		send_to_char("Не имеете таких правов!\r\n", ch);
		return 0;
	}

	obj_from_obj(obj);
	obj_to_char(obj, ch);
	if (obj->carried_by == ch) {
		act("Вы взяли $o3 из $O1.", FALSE, ch, obj, chest, TO_CHAR);
		act("$n взял$g $o3 из $O1.", TRUE, ch, obj, chest, TO_ROOM);
	}
	return 1;
}

// сохраняем все сундуки в файлы
// пользует write_one_object (мне кажется это разуменее, чем плодить свои форматы везде и потом
// заниматься с ними сексом при изменении параметров на шмотках)
void Clan::ChestSave()
{
	OBJ_DATA *chest, *temp;
	int chest_num = real_object(CLAN_CHEST);
	if (chest_num < 0)
		return;

	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan) {
		std::string buffer = (*clan)->abbrev;
		for (unsigned i = 0; i != buffer.length(); ++i)
			buffer[i] = LOWER(AtoL(buffer[i]));
		buffer += ".obj";
		for (chest = world[real_room((*clan)->rent)]->contents; chest; chest = chest->next_content)
			if (chest->item_number == chest_num) {
				std::string buffer2;
				for (temp = chest->contains; temp; temp = temp->next_content) {
					char databuf[MAX_STRING_LENGTH];
					char *data = databuf;

					write_one_object(&data, temp, 0);
					buffer2 += databuf;
				}

				std::ofstream file((LIB_HOUSE + buffer).c_str());
				if (!file.is_open()) {
					log("Error open file: %s! (%s %s %d)", (LIB_HOUSE + buffer).c_str(),
					    __FILE__, __func__, __LINE__);
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
	int chest_num = real_object(CLAN_CHEST);
	if (chest_num < 0)
		return;

	// на случай релоада - чистим перед этим все что было в сундуках
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan) {
		for (chest = world[real_room((*clan)->rent)]->contents; chest; chest = chest->next_content) {
			if (chest->item_number == chest_num) {
				for (temp = chest->contains; temp; temp = obj_next) {
					obj_next = temp->next_content;
					obj_from_obj(temp);
					extract_obj(temp);
				}
			}
		}
	}

	int error = 0, fsize = 0;
	FILE *fl;
	char *data, *databuf;
	std::string buffer;

	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan) {
		buffer = (*clan)->abbrev;
		for (unsigned i = 0; i != buffer.length(); ++i)
			buffer[i] = LOWER(AtoL(buffer[i]));
		buffer += ".obj";

		for (chest = world[real_room((*clan)->rent)]->contents; chest; chest = chest->next_content)
			if (chest->item_number == chest_num)
				break;
		if (!chest) {
			log("<Clan> Where chest for '%s' ?! (%s %s %d)", (*clan)->abbrev.c_str(),
			    __FILE__, __func__, __LINE__);
			return;
		}
		if (!(fl = fopen((LIB_HOUSE + buffer).c_str(), "r+b")))
			continue;
		fseek(fl, 0L, SEEK_END);
		fsize = ftell(fl);
		if (!fsize) {
			fclose(fl);
			log("<Clan> Empty obj file '%s'. (%s %s %d)",
			    (LIB_HOUSE + buffer).c_str(), __FILE__, __func__, __LINE__);
			continue;
		}
		CREATE(databuf, char, fsize + 1);

		fseek(fl, 0L, SEEK_SET);
		if (!fread(databuf, fsize, 1, fl) || ferror(fl) || !databuf) {
			fclose(fl);
			log("<Clan> Error reading obj file '%s'. (%s %s %d)",
			    (LIB_HOUSE + buffer).c_str(), __FILE__, __func__, __LINE__);
			continue;
		}
		fclose(fl);

		data = databuf;
		*(data + fsize) = '\0';

		for (fsize = 0; *data && *data != '$'; fsize++) {
			if (!(obj = read_one_object_new(&data, &error))) {
				if (error)
					log("<Clan> Objects reading fail for %s error %d.",
					    (LIB_HOUSE + buffer).c_str(), error);
				continue;
			}
			obj_to_obj(obj, chest);
		}
		free(databuf);
	}
}

// смотрим чего в сундуках и берем из казны за ренту
void Clan::ChestUpdate()
{
	OBJ_DATA *temp, *chest, *obj_next;
	double cost, i;
	int chest_num = real_object(CLAN_CHEST);
	if (chest_num < 0)
		return;
	bool find = 0;

	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan) {
		cost = 0;
		// ищем сундук
		for (chest = world[real_room((*clan)->rent)]->contents; chest; chest = chest->next_content)
			if (chest->item_number == chest_num) {
				// перебираем в нем шмот
				for (temp = chest->contains; temp; temp = temp->next_content)
					cost += GET_OBJ_RENTEQ(temp);
				find = 1;
				break;
			}
		// расчет и снимание за ренту (целой части по возможности) понижающий коэф-т 1/3
		cost = (((cost / (60  * 24)) * CLAN_STOREHOUSE_COEFF) / 100) * CHEST_UPDATE_PERIOD;
		// сразу снимем налог
		cost += ((double)(CLAN_TAX + ((*clan)->storehouse * CLAN_STOREHOUSE_COEFF)) / (60 * 24)) * CHEST_UPDATE_PERIOD;
		(*clan)->bankBuffer += cost;
		modf((*clan)->bankBuffer, &i);
		if (i) {
			(*clan)->bank -= (long) i;
			(*clan)->bankBuffer -= i;
		}
		// при нулевом счете все шмотки в сундуке пуржим
		if ((*clan)->bank < 0 && find) {
			(*clan)->bank = 0;
			for (temp = chest->contains; temp; temp = obj_next) {
				obj_next = temp->next_content;
				obj_from_obj(temp);
				extract_obj(temp);
			}
		}
	}
}

// казна дружины... команды теже самые с приставкой 'казна' в начале
// смотреть/вкладывать могут все, снимать по привилегии, висит на стандартных банкирах
bool Clan::BankManage(CHAR_DATA * ch, char *arg)
{
	if (IS_NPC(ch) || !GET_CLAN_RENT(ch))
		return 0;

	ClanListType::const_iterator clan = Clan::GetClan(ch);
	if (clan == Clan::ClanList.end()) {
		log("Player have GET_CLAN_RENT and dont stay in Clan::ClanList (%s %s %d)", __FILE__, __func__, __LINE__);
		return 0;
	}
	std::string buffer = arg, buffer2;
	GetOneParam(buffer, buffer2);

	if (CompareParam(buffer2, "баланс") || CompareParam(buffer2, "balance")) {
		send_to_char(ch, "На счету Вашей дружины ровно %ld.\r\n", (*clan)->bank, desc_count((*clan)->bank, WHAT_MONEYu));
		return 1;

	} else if (CompareParam(buffer2, "вложить") || CompareParam(buffer2, "deposit")) {
		GetOneParam(buffer, buffer2);
		long gold = atol(buffer2.c_str());

		if (gold <= 0) {
			send_to_char("Сколько Вы хотите вложить?\r\n", ch);
			return 1;
		}
		if (GET_GOLD(ch) < gold) {
			send_to_char("О такой сумме Вы можете только мечтать!\r\n", ch);
			return 1;
		}

		// на случай переполнения казны
		if (((*clan)->bank + gold) < 0) {
			long over = LONG_MAX - (*clan)->bank;
			(*clan)->bank += over;
			(*clan)->members[GET_UNIQUE(ch)]->money += over;
			GET_GOLD(ch) -= over;
			send_to_char(ch, "Вы удалось вложить в казну дружины только %ld %s.\r\n", over, desc_count(over, WHAT_MONEYu));
			act("$n произвел$g финансовую операцию.", TRUE, ch, 0, FALSE, TO_ROOM);
			return 1;
		}

		GET_GOLD(ch) -= gold;
		(*clan)->bank += gold;
		(*clan)->members[GET_UNIQUE(ch)]->money += gold;
		send_to_char(ch, "Вы вложили %ld %s.\r\n", gold, desc_count(gold, WHAT_MONEYu));
		act("$n произвел$g финансовую операцию.", TRUE, ch, 0, FALSE, TO_ROOM);
		return 1;

	} else if (CompareParam(buffer2, "получить") || CompareParam(buffer2, "withdraw")) {
		if (!(*clan)->privileges[GET_CLAN_RANKNUM(ch)][MAY_CLAN_BANK]) {
			send_to_char("К сожалению, у Вас нет возможности транжирить средства дружины.\r\n", ch);
			return 1;
		}
		GetOneParam(buffer, buffer2);
		long gold = atol(buffer2.c_str());

		if (gold <= 0) {
			send_to_char("Уточните количество денег, которые Вы хотите получить?\r\n", ch);
			return 1;
		}
		if ((*clan)->bank < gold) {
			send_to_char("К сожалению, Ваша дружина не так богата.\r\n", ch);
			return 1;
		}

		// на случай переполнения персонажа
		if ((GET_GOLD(ch) + gold) < 0) {
			long over = LONG_MAX - GET_GOLD(ch);
			GET_GOLD(ch) += over;
			(*clan)->bank -= over;
			(*clan)->members[GET_UNIQUE(ch)]->money -= over;
			send_to_char(ch, "Вы удалось снять только %ld %s.\r\n", over, desc_count(over, WHAT_MONEYu));
			act("$n произвел$g финансовую операцию.", TRUE, ch, 0, FALSE, TO_ROOM);
			return 1;
		}

		(*clan)->bank -= gold;
		(*clan)->members[GET_UNIQUE(ch)]->money -= gold;
		GET_GOLD(ch) += gold;
		send_to_char(ch, "Вы сняли %ld %s.\r\n", gold, desc_count(gold, WHAT_MONEYu));
		act("$n произвел$g финансовую операцию.", TRUE, ch, 0, FALSE, TO_ROOM);
		return 1;

	} else
		send_to_char(ch, "Формат команды: казна вложить|получить|баланс сумма.\r\n");
	return 1;
}

void Clan::Manage(DESCRIPTOR_DATA * d, const char *arg)
{
	unsigned num = atoi(arg);

	switch (d->clan_olc->mode) {
	case CLAN_MAIN_MENU:
		switch (*arg) {
		case 'q':
		case 'Q':
			// есть вариант, что за время в олц в клане изменят кол-во званий
			if (d->clan_olc->clan->privileges.size() != d->clan_olc->privileges.size()) {
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
			if (!*arg || !num) {
				send_to_char("Неверный выбор!\r\n", d->character);
				d->clan_olc->clan->MainMenu(d);
				return;
			}
			// тут парсим циферки уже (меню начинается с 1 + ранг ниже себя)
			unsigned min_rank = GET_CLAN_RANKNUM(d->character);
			if ((num + min_rank) >= d->clan_olc->clan->ranks.size()) {
				unsigned i = (num + min_rank) - d->clan_olc->clan->ranks.size();
				if (i == 0)
					d->clan_olc->clan->AllMenu(d, i);
				else if (i == 1)
					d->clan_olc->clan->AllMenu(d, i);
				else if (i == 2 && !GET_CLAN_RANKNUM(d->character)) {
					if (d->clan_olc->clan->storehouse)
						d->clan_olc->clan->storehouse = 0;
					else
						d->clan_olc->clan->storehouse = 1;
					d->clan_olc->clan->MainMenu(d);
					return;
				}
				else {
					send_to_char("Неверный выбор!\r\n", d->character);
					d->clan_olc->clan->MainMenu(d);
					return;
				}
				return;
			}
			if (num < 0 || (num + min_rank) >= d->clan_olc->clan->ranks.size()) {
				send_to_char("Неверный выбор!\r\n", d->character);
				d->clan_olc->clan->MainMenu(d);
				return;
			}
			d->clan_olc->rank = num + min_rank;
			d->clan_olc->clan->PrivilegeMenu(d, num + min_rank);
			return;
		}
		break;
	case CLAN_PRIVILEGE_MENU:
		switch (*arg) {
		case 'q':
		case 'Q':
			// выход в общее меню
			d->clan_olc->rank = 0;
			d->clan_olc->mode = CLAN_MAIN_MENU;
			d->clan_olc->clan->MainMenu(d);
			return;
		default:
			if (!*arg) {
				send_to_char("Неверный выбор!\r\n", d->character);
				d->clan_olc->clan->PrivilegeMenu(d, d->clan_olc->rank);
				return;
			}
			if (num < 0 || num > CLAN_PRIVILEGES_NUM) {
				send_to_char("Неверный выбор!\r\n", d->character);
				d->clan_olc->clan->PrivilegeMenu(d, d->clan_olc->rank);
				return;
			}
			// мы выдаем только доступные привилегии в меню и нормально парсим их тут
			int parse_num;
			for (parse_num = 0; parse_num < CLAN_PRIVILEGES_NUM; ++parse_num) {
				if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][parse_num]) {
					if(!--num)
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
		switch (*arg) {
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
		switch (*arg) {
		case 'q':
		case 'Q':
			// выход в общее меню с изменением всех званий
			for (int i = 0; i <= CLAN_PRIVILEGES_NUM; ++i) {
				if (d->clan_olc->all_ranks[i]) {
					unsigned j = GET_CLAN_RANKNUM(d->character) + 1;
					for (; j <= d->clan_olc->clan->ranks.size(); ++j) {
						d->clan_olc->privileges[j][i] = 1;
						d->clan_olc->all_ranks[i] = 0;
					}
				}

			}
			d->clan_olc->mode = CLAN_MAIN_MENU;
			d->clan_olc->clan->MainMenu(d);
			return;
		default:
			if (!*arg) {
				send_to_char("Неверный выбор!\r\n", d->character);
				d->clan_olc->clan->AllMenu(d, 0);
				return;
			}
			if (num < 0 || num > CLAN_PRIVILEGES_NUM) {
				send_to_char("Неверный выбор!\r\n", d->character);
				d->clan_olc->clan->AllMenu(d, 0);
				return;
			}
			int parse_num;
			for (parse_num = 0; parse_num < CLAN_PRIVILEGES_NUM; ++parse_num) {
				if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][parse_num]) {
					if(!--num)
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
		switch (*arg) {
		case 'q':
		case 'Q':
			// выход в общее меню с изменением всех званий
			for (int i = 0; i <= CLAN_PRIVILEGES_NUM; ++i) {
				if (d->clan_olc->all_ranks[i]) {
					unsigned j = GET_CLAN_RANKNUM(d->character) + 1;
					for (; j <= d->clan_olc->clan->ranks.size(); ++j) {
						d->clan_olc->privileges[j][i] = 0;
						d->clan_olc->all_ranks[i] = 0;
					}
				}

			}
			d->clan_olc->mode = CLAN_MAIN_MENU;
			d->clan_olc->clan->MainMenu(d);
			return;
		default:
			if (!*arg) {
				send_to_char("Неверный выбор!\r\n", d->character);
				d->clan_olc->clan->AllMenu(d, 1);
				return;
			}
			if (num < 0 || num > CLAN_PRIVILEGES_NUM) {
				send_to_char("Неверный выбор!\r\n", d->character);
				d->clan_olc->clan->AllMenu(d, 1);
				return;
			}
			int parse_num;
			for (parse_num = 0; parse_num < CLAN_PRIVILEGES_NUM; ++parse_num) {
				if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][parse_num]) {
					if(!--num)
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
	int rank = GET_CLAN_RANKNUM(d->character) + 1;
	int num = 0;
	for (std::vector<std::string>::const_iterator it = d->clan_olc->clan->ranks.begin() + rank; it != d->clan_olc->clan->ranks.end(); ++it)
		buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++num << CCNRM(d->character, C_NRM) << ") " << (*it) << "\r\n";
	buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++num << CCNRM(d->character, C_NRM)
		<< ") " << "добавить всем\r\n"
		<< CCGRN(d->character, C_NRM) << std::setw(2) << ++num << CCNRM(d->character, C_NRM)
		<< ") " << "убрать у всех\r\n";
	if (!GET_CLAN_RANKNUM(d->character)) {
		buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++num << CCNRM(d->character, C_NRM)
			<< ") " << "Выборка из хранилища по параметрам предметов\r\n"
			<< "    + просмотр хранилища вне замка в качестве бонуса (1000 кун в день) ";
		if (this->storehouse)
			buffer << "(отключить)\r\n";
		else
			buffer << "(включить)\r\n";
	}
	buffer << CCGRN(d->character, C_NRM) << " Q" << CCNRM(d->character, C_NRM)
		<< ") Выход\r\n" << "Ваш выбор:";

	send_to_char(buffer.str(), d->character);
	d->clan_olc->mode = CLAN_MAIN_MENU;
}

void Clan::PrivilegeMenu(DESCRIPTOR_DATA * d, unsigned num)
{
	if (num > d->clan_olc->clan->ranks.size() || num < 0) {
		log("Different size clan->ranks and clan->privileges! (%s %s %d)", __FILE__, __func__, __LINE__);
		if (d->character) {
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
	for (int i = 0; i < CLAN_PRIVILEGES_NUM; ++i) {
		switch (i) {
		case MAY_CLAN_INFO:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_INFO]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_INFO])
					buffer << "[x] информация о дружине (замок информация)\r\n";
				else
					buffer << "[ ] информация о дружине (замок информация)\r\n";
			}
			break;
		case MAY_CLAN_ADD:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_ADD]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_ADD])
					buffer << "[x] принятие в дружину (замок принять)\r\n";
				else
					buffer << "[ ] принятие в дружину (замок принять)\r\n";
			}
			break;
		case MAY_CLAN_REMOVE:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_REMOVE]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_REMOVE])
					buffer << "[x] изгнание из дружины (замок изгнать)\r\n";
				else
					buffer << "[ ] изгнание из дружины (замок изгнать)\r\n";
			}
			break;
		case MAY_CLAN_PRIVILEGES:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_PRIVILEGES]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_PRIVILEGES])
					buffer << "[x] редактирование привилегий (замок привилегии)\r\n";
				else
					buffer << "[ ] редактирование привилегий (замок привилегии)\r\n";
			}
			break;
		case MAY_CLAN_CHANNEL:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_CHANNEL]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_CHANNEL])
					buffer << "[x] клан-канал (гдругам)\r\n";
				else
					buffer << "[ ] клан-канал (гдругам)\r\n";
			}
			break;
		case MAY_CLAN_POLITICS:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_POLITICS]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_POLITICS])
					buffer << "[x] изменение политики дружины (политика)\r\n";
				else
					buffer << "[ ] изменение политики дружины (политика)\r\n";
			}
			break;
		case MAY_CLAN_NEWS:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_NEWS]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_NEWS])
					buffer << "[x] добавление новостей дружины (дрновости)\r\n";
				else
					buffer << "[ ] добавление новостей дружины (дрновости)\r\n";
			}
			break;
		case MAY_CLAN_PKLIST:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_PKLIST]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_PKLIST])
					buffer << "[x] добавление в пк-лист (пклист)\r\n";
				else
					buffer << "[ ] добавление в пк-лист (пклист)\r\n";
			}
			break;
		case MAY_CLAN_CHEST_PUT:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_CHEST_PUT]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_CHEST_PUT])
					buffer << "[x] класть вещи в хранилище\r\n";
				else
					buffer << "[ ] класть вещи в хранилище\r\n";
			}
			break;
		case MAY_CLAN_CHEST_TAKE:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_CHEST_TAKE]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_CHEST_TAKE])
					buffer << "[x] брать вещи из хранилища\r\n";
				else
					buffer << "[ ] брать вещи из хранилища\r\n";
			}
			break;
		case MAY_CLAN_BANK:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_BANK]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_BANK])
					buffer << "[x] брать из казны дружины\r\n";
				else
					buffer << "[ ] брать из казны дружины\r\n";
			}
			break;
		case MAY_CLAN_LEAVE:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_LEAVE]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->privileges[num][MAY_CLAN_LEAVE])
					buffer << "[x] свободный выход из дружины\r\n";
				else
					buffer << "[ ] свободный выход из дружины\r\n";
			}
			break;
		}
	}
	buffer << CCGRN(d->character, C_NRM) << " Q" << CCNRM(d->character, C_NRM)
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
	for (int i = 0; i < CLAN_PRIVILEGES_NUM; ++i) {
		switch (i) {
		case MAY_CLAN_INFO:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_INFO]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_INFO])
					buffer << "[x] информация о дружине (замок информация)\r\n";
				else
					buffer << "[ ] информация о дружине (замок информация)\r\n";
			}
			break;
		case MAY_CLAN_ADD:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_ADD]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_ADD])
					buffer << "[x] принятие в дружину (замок принять)\r\n";
				else
					buffer << "[ ] принятие в дружину (замок принять)\r\n";
			}
			break;
		case MAY_CLAN_REMOVE:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_REMOVE]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_REMOVE])
					buffer << "[x] изгнание из дружины (замок изгнать)\r\n";
				else
					buffer << "[ ] изгнание из дружины (замок изгнать)\r\n";
			}
			break;
		case MAY_CLAN_PRIVILEGES:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_PRIVILEGES]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_PRIVILEGES])
					buffer << "[x] редактирование привилегий (замок привилегии)\r\n";
				else
					buffer << "[ ] редактирование привилегий (замок привилегии)\r\n";
			}
			break;
		case MAY_CLAN_CHANNEL:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_CHANNEL]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_CHANNEL])
					buffer << "[x] клан-канал (гдругам)\r\n";
				else
					buffer << "[ ] клан-канал (гдругам)\r\n";
			}
			break;
		case MAY_CLAN_POLITICS:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_POLITICS]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_POLITICS])
					buffer << "[x] изменение политики дружины (политика)\r\n";
				else
					buffer << "[ ] изменение политики дружины (политика)\r\n";
			}
			break;
		case MAY_CLAN_NEWS:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_NEWS]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_NEWS])
					buffer << "[x] добавление новостей дружины (дрновости)\r\n";
				else
					buffer << "[ ] добавление новостей дружины (дрновости)\r\n";
			}
			break;
		case MAY_CLAN_PKLIST:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_PKLIST]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_PKLIST])
					buffer << "[x] добавление в пк-лист (пклист)\r\n";
				else
					buffer << "[ ] добавление в пк-лист (пклист)\r\n";
			}
			break;
		case MAY_CLAN_CHEST_PUT:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_CHEST_PUT]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_CHEST_PUT])
					buffer << "[x] класть вещи в хранилище\r\n";
				else
					buffer << "[ ] класть вещи в хранилище\r\n";
			}
			break;
		case MAY_CLAN_CHEST_TAKE:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_CHEST_TAKE]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_CHEST_TAKE])
					buffer << "[x] брать вещи из хранилища\r\n";
				else
					buffer << "[ ] брать вещи из хранилища\r\n";
			}
			break;
		case MAY_CLAN_BANK:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_BANK]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_BANK])
					buffer << "[x] брать из казны дружины\r\n";
				else
					buffer << "[ ] брать из казны дружины\r\n";
			}
			break;
		case MAY_CLAN_LEAVE:
			if(d->clan_olc->privileges[GET_CLAN_RANKNUM(d->character)][MAY_CLAN_LEAVE]) {
				buffer << CCGRN(d->character, C_NRM) << std::setw(2) << ++count << CCNRM(d->character, C_NRM) << ") ";
				if (d->clan_olc->all_ranks[MAY_CLAN_LEAVE])
					buffer << "[x] свободный выход из дружины\r\n";
				else
					buffer << "[ ] свободный выход из дружины\r\n";
			}
			break;
		}
	}
	buffer << CCGRN(d->character, C_NRM) << " Q" << CCNRM(d->character, C_NRM)
		<< ") Применить\r\n" << "Ваш выбор:";
	send_to_char(buffer.str(), d->character);
	if (flag == 0)
		d->clan_olc->mode = CLAN_ADDALL_MENU;
	else
		d->clan_olc->mode = CLAN_DELALL_MENU;
}

// игрок ранг (деньги экспа) если есть что писать
void Clan::ClanAddMember(CHAR_DATA * ch, int rank, long long money, unsigned long long exp, int tax)
{
	ClanMemberPtr tempMember(new ClanMember);
	tempMember->name = GET_NAME(ch);
	tempMember->rankNum = rank;
	tempMember->money = money;
	tempMember->exp = exp;
	tempMember->tax = tax;
	this->members[GET_UNIQUE(ch)] = tempMember;
	Clan::SetClanData(ch);
	DESCRIPTOR_DATA *d;
	std::string buffer;
	for (d = descriptor_list; d; d = d->next) {
		if (d->character && STATE(d) == CON_PLAYING && !AFF_FLAGGED(d->character, AFF_DEAFNESS)
		    && this->rent == GET_CLAN_RENT(d->character) && ch != d->character) {
				send_to_char(d->character, "%s%s приписан%s к Вашей дружине, статус - '%s'.%s\r\n",
					CCWHT(d->character, C_NRM), GET_NAME(ch), GET_CH_SUF_6(ch), (this->ranks[rank]).c_str(), CCNRM(d->character, C_NRM));
		}
	}
	send_to_char(ch, "%sВас приписали к дружине '%s', статус - '%s'.%s\r\n", CCWHT(ch, C_NRM), this->name.c_str(), (this->ranks[rank]).c_str(), CCNRM(ch, C_NRM));
	Clan::ClanSave();
	return;
}

void Clan::HouseOwner(CHAR_DATA * ch, std::string & buffer)
{
	std::string buffer2;
	GetOneParam(buffer, buffer2);
	DESCRIPTOR_DATA *d;
	long unique = 0;

	if (buffer2.empty())
		send_to_char("Укажите имя персонажа.\r\n", ch);
	else if (!(unique = GetUniqueByName(buffer2)))
		send_to_char("Неизвестный персонаж.\r\n", ch);
	else if (unique == GET_UNIQUE(ch))
		send_to_char("Сменить себя на самого себя? Вы бредите.\r\n", ch);
	else if (!(d = DescByUID(unique)))
		send_to_char("Этого персонажа нет в игре!\r\n", ch);
	else if (GET_CLAN_RENT(d->character) && GET_CLAN_RENT(ch) != GET_CLAN_RENT(d->character))
		send_to_char("Вы не можете передать свои права члену другой дружины.\r\n", ch);
	else {
		buffer2[0] = UPPER(buffer2[0]);
		// воеводу нафик
		ClanMemberList::iterator it = this->members.find(GET_UNIQUE(ch));
		this->members.erase(it);
		Clan::SetClanData(ch);
		// ставим нового воеводу (если он был в клане - сохраняем эксп)
		if (GET_CLAN_RENT(d->character)) {
			this->ClanAddMember(d->character, 0, this->members[GET_UNIQUE(d->character)]->money, this->members[GET_UNIQUE(d->character)]->exp, this->members[GET_UNIQUE(d->character)]->tax);
		} else
			this->ClanAddMember(d->character, 0, 0, 0, 0);
		this->owner = buffer2;
		send_to_char(ch, "Поздравляем, Вы передали свои полномочия %s!\r\n", GET_PAD(d->character, 2));
		Clan::ClanSave();
	}
}

void Clan::HouseLeave(CHAR_DATA * ch)
{
	if(!GET_CLAN_RANKNUM(ch)) {
		send_to_char("Если Вы хотите распустить свою дружину, то обращайтесь к Богам.\r\n"
					"А если Вам просто нечем заняться, то передайте воеводство и идите куда хотите...\r\n", ch);
		return;
	}

	ClanMemberList::iterator it = this->members.find(GET_UNIQUE(ch));
	if (it != this->members.end()) {
		this->members.erase(it);
		Clan::SetClanData(ch);
		send_to_char(ch, "Вы покинули дружину '%s'.\r\n", this->name.c_str());
		DESCRIPTOR_DATA *d;
		for (d = descriptor_list; d; d = d->next) {
			if (d->character && GET_CLAN_RENT(d->character) == this->rent)
				send_to_char(d->character, "%s покинул%s Вашу дружину.", GET_NAME(ch), GET_CH_SUF_1(ch));
		}
	}
}

void Clan::CheckPkList(CHAR_DATA * ch)
{
	if (IS_NPC(ch))
		return;
	ClanPkList::iterator it;
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)	{
		// пкл
		for (ClanPkList::const_iterator pklist = (*clan)->pkList.begin(); pklist != (*clan)->pkList.end(); ++pklist) {
			if ((it = (*clan)->pkList.find(GET_UNIQUE(ch))) != (*clan)->pkList.end())
				send_to_char(ch, "Находитесь в списке врагов дружины '%s'.\r\n", (*clan)->name.c_str());
		}
		// дрл
		for (ClanPkList::const_iterator frlist = (*clan)->frList.begin(); frlist != (*clan)->frList.end(); ++frlist) {
			if ((it = (*clan)->frList.find(GET_UNIQUE(ch))) != (*clan)->frList.end())
				send_to_char(ch, "Находитесь в списке друзей дружины '%s'.\r\n", (*clan)->name.c_str());
		}
	}
}

// заколебали эти флаги... сравниваем num и все поля в flags
bool CompareBits(FLAG_DATA flags, const char *names[], int affect)
{
	int i;
	for (i = 0; i < 4; i++) {
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
		while (fail) {
			if (*names[nr] == '\n')
				fail--;
			nr++;
		}

		for (; bitvector; bitvector >>= 1) {
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

// вобщем это копи-паст из биржи + флаги
ACMD(DoStorehouse)
{
	if (IS_NPC(ch) || !GET_CLAN_RENT(ch)) {
		send_to_char("Чаво ?\r\n", ch);
		return;
	}
	ClanListType::const_iterator clan = Clan::GetClan(ch);
	if (clan == Clan::ClanList.end()) {
		log("Player have GET_CLAN_RENT and dont stay in Clan::ClanList (%s %s %d)", __FILE__, __func__, __LINE__);
		return;
	}
	if (!(*clan)->storehouse) {
		send_to_char("Чаво ?\r\n", ch);
		return;
	}

	skip_spaces(&argument);
	if (!*argument) {
		OBJ_DATA *chest;
		int chest_num = real_object(CLAN_CHEST);
		if (chest_num < 0)
			return;
		for (chest = world[real_room((*clan)->rent)]->contents; chest; chest = chest->next_content) {
			if (chest->item_number == chest_num) {
				send_to_char("Хранилище Вашей дружины:\r\n", ch);
				list_obj_to_char(chest->contains, ch, 1, 3);
				return;
			}
		}
	}

	// мож сэйвить надумается или еще что, вобщем объединим все в структурку
	boost::shared_ptr<ChestFilter> filter(new ChestFilter);
	filter->type = -1;
	filter->state= -1;
	filter->wear = -1;
	filter->weap_class = -1;
	filter->affect = -1;

	int num = 0, find = 0;
	char tmpbuf[MAX_INPUT_LENGTH];
	while (*argument) {
		switch (*argument) {
		case 'И':
			argument = one_argument(++argument, tmpbuf);
			filter->name = tmpbuf;
			break;
		case 'Т':
			argument = one_argument(++argument, tmpbuf);
			if (is_abbrev(tmpbuf, "свет") || is_abbrev(tmpbuf, "light"))
				filter->type = ITEM_LIGHT;
			else if (is_abbrev(tmpbuf, "свиток") || is_abbrev(tmpbuf, "scroll"))
				filter->type = ITEM_SCROLL;
			else if (is_abbrev(tmpbuf, "палочка") || is_abbrev(tmpbuf, "wand"))
				filter->type = ITEM_WAND;
			else if (is_abbrev(tmpbuf, "посох") || is_abbrev(tmpbuf, "staff"))
				filter->type = ITEM_STAFF;
			else if (is_abbrev(tmpbuf, "оружие") || is_abbrev(tmpbuf, "weapon"))
				filter->type = ITEM_WEAPON;
			else if (is_abbrev(tmpbuf, "броня") || is_abbrev(tmpbuf, "armor"))
				filter->type = ITEM_ARMOR;
			else if (is_abbrev(tmpbuf, "напиток") || is_abbrev(tmpbuf, "potion"))
				filter->type = ITEM_POTION;
			else if (is_abbrev(tmpbuf, "прочее") || is_abbrev(tmpbuf, "другое") || is_abbrev(tmpbuf, "other"))
				filter->type = ITEM_OTHER;
			else if (is_abbrev(tmpbuf, "контейнер") || is_abbrev(tmpbuf, "container"))
				filter->type = ITEM_CONTAINER;
			else if (is_abbrev(tmpbuf, "книга") || is_abbrev(tmpbuf, "book"))
				filter->type = ITEM_BOOK;
			else if (is_abbrev(tmpbuf, "руна") || is_abbrev(tmpbuf, "rune"))
				filter->type = ITEM_INGRADIENT;
			else if (is_abbrev(tmpbuf, "ингредиент") || is_abbrev(tmpbuf, "ingradient"))
				filter->type = ITEM_MING;
			else {
				send_to_char("Неверный тип предмета.\r\n", ch);
				return;
			}
			break;
		case 'С':
			argument = one_argument(++argument, tmpbuf);
			if (is_abbrev(tmpbuf, "ужасно"))
				filter->state = 1;
			else if (is_abbrev(tmpbuf, "скоро_испортится"))
				filter->state = 21;
			else if (is_abbrev(tmpbuf, "плоховато"))
				filter->state = 41;
			else if (is_abbrev(tmpbuf, "средне"))
				filter->state = 61;
			else if (is_abbrev(tmpbuf, "идеально"))
				filter->state = 81;
			else {
				send_to_char("Неверное состояние предмета.\r\n", ch);
				return;
			}

			break;
		case 'О':
			argument = one_argument(++argument, tmpbuf);
			if (is_abbrev(tmpbuf, "палец"))
				filter->wear = 1;
			else if (is_abbrev(tmpbuf, "шея") || is_abbrev(tmpbuf, "грудь"))
				filter->wear = 2;
			else if (is_abbrev(tmpbuf, "тело"))
				filter->wear = 3;
			else if (is_abbrev(tmpbuf, "голова"))
				filter->wear = 4;
			else if (is_abbrev(tmpbuf, "ноги"))
				filter->wear = 5;
			else if (is_abbrev(tmpbuf, "ступни"))
				filter->wear = 6;
			else if (is_abbrev(tmpbuf, "кисти"))
				filter->wear = 7;
			else if (is_abbrev(tmpbuf, "руки"))
				filter->wear = 8;
			else if (is_abbrev(tmpbuf, "щит"))
				filter->wear = 9;
			else if (is_abbrev(tmpbuf, "плечи"))
				filter->wear = 10;
			else if (is_abbrev(tmpbuf, "пояс"))
				filter->wear = 11;
			else if (is_abbrev(tmpbuf, "запястья"))
				filter->wear = 12;
			else if (is_abbrev(tmpbuf, "левая"))
				filter->wear = 13;
			else if (is_abbrev(tmpbuf, "правая"))
				filter->wear = 14;
			else if (is_abbrev(tmpbuf, "обе"))
				filter->wear = 15;
			else {
				send_to_char("Неверное место одевания предмета.\r\n", ch);
				return;
			}
			break;
		case 'К':
			argument = one_argument(++argument, tmpbuf);
			if (is_abbrev(tmpbuf, "луки"))
				filter->weap_class = 0;
			else if (is_abbrev(tmpbuf, "короткие"))
				filter->weap_class = 1;
			else if (is_abbrev(tmpbuf, "длинные"))
				filter->weap_class = 2;
			else if (is_abbrev(tmpbuf, "секиры"))
				filter->weap_class = 3;
			else if (is_abbrev(tmpbuf, "палицы"))
				filter->weap_class = 4;
			else if (is_abbrev(tmpbuf, "иное"))
				filter->weap_class = 5;
			else if (is_abbrev(tmpbuf, "двуручники"))
				filter->weap_class = 6;
			else if (is_abbrev(tmpbuf, "проникающее"))
				filter->weap_class = 7;
			else if (is_abbrev(tmpbuf, "копья"))
				filter->weap_class = 8;
			else {
				send_to_char("Неверный класс оружия.\r\n", ch);
				return;
			}
			break;
		case 'А':
			num = 0;
			argument = one_argument(++argument, tmpbuf);
			if (strlen(tmpbuf) < 2) {
				send_to_char("Укажите хотя бы два первых символа аффекта.\r\n", ch);
				return;
			}
			for (int flag = 0; flag < 4; ++flag) {
				for (; *weapon_affects[num] != '\n'; ++num) {
					if (is_abbrev(tmpbuf, weapon_affects[num])) {
						filter->affect = num;
						find = 1;
						break;
					}
				}
				++num;
			}
			if (!find) {
				num = 0;
				for (; *apply_types[num] != '\n'; ++num) {
					if (is_abbrev(tmpbuf, apply_types[num])) {
						filter->affect = num;
						find = 2;
						break;
					}
				}
			}
			if (filter->affect < 0) {
				send_to_char("Неверный аффект предмета.\r\n", ch);
				return;
			}
			break;
		default:
			++argument;
		}
	}
	std::string buffer = "Выборка по следующим параметрам: ";
	if (!filter->name.empty())
		buffer += filter->name + " ";
	if (filter->type >= 0) {
		buffer += item_types[filter->type];
		buffer += " ";
	}
	if (filter->state >= 0) {
		if (filter->state < 20)
			buffer += "ужасно ";
		else if (filter->state < 40)
			buffer += "скоро испортится ";
		else if (filter->state < 60)
			buffer += "плоховато ";
		else if (filter->state < 80)
			buffer += "средне ";
		else
			buffer += "идеально ";
	}
	if (filter->wear >= 0) {
		buffer += wear_bits[filter->wear];
		buffer += " ";
	}
	if (filter->weap_class >= 0) {
		buffer += weapon_class[filter->weap_class];
		buffer += " ";
	}
	if (filter->affect >= 0) {
		if (find == 1)
			buffer += weapon_affects[filter->affect];
		else if (find == 2)
			buffer += apply_types[filter->affect];
		else // какого?
			return;
		buffer += " ";
	}
	buffer += "\r\n";
	send_to_char(buffer, ch);

	int chest_num = real_object(CLAN_CHEST);
	OBJ_DATA *temp_obj, *chest;
	for (chest = world[real_room((*clan)->rent)]->contents; chest; chest = chest->next_content) {
		if (chest->item_number == chest_num) {
			for (temp_obj = chest->contains; temp_obj; temp_obj = temp_obj->next_content) {
				// сверяем имя
				if (!filter->name.empty() && CompareParam(filter->name, temp_obj->name)) {
					show_obj_to_char(temp_obj, ch, 1, 2, 1);
					continue;
				}
				// тип
				if (filter->type >= 0 && filter->type == GET_OBJ_TYPE(temp_obj)) {
					show_obj_to_char(temp_obj, ch, 1, 2, 1);
					continue;
				}
				// таймер
				if (filter->state >= 0) {
					if (!GET_OBJ_TIMER(obj_proto + GET_OBJ_RNUM(temp_obj))) {
						send_to_char("Нулевой таймер прототипа, сообщите Богам!", ch);
						return;
					}
					int tm = (GET_OBJ_TIMER(temp_obj) * 100 / GET_OBJ_TIMER(obj_proto + GET_OBJ_RNUM(temp_obj)));
					if ((tm + 1) > filter->state) {
						show_obj_to_char(temp_obj, ch, 1, 2, 1);
						continue;
					}
				}
				// куда можно одеть
				if (filter->wear >= 0 && CAN_WEAR(temp_obj, filter->wear)) {
					show_obj_to_char(temp_obj, ch, 1, 2, 1);
					continue;
				}
				// класс оружия
				if (filter->weap_class >= 0 && filter->weap_class == GET_OBJ_SKILL(temp_obj)) {
					show_obj_to_char(temp_obj, ch, 1, 2, 1);
					continue;
				}
				// аффект
				if (filter->affect >= 0) {
					if (find == 1) {
						if (CompareBits(temp_obj->obj_flags.affects, weapon_affects, filter->affect))
							show_obj_to_char(temp_obj, ch, 1, 2, 1);
					} else if (find == 2) {
						for (int i = 0; i < MAX_OBJ_AFFECT; ++i) {
							if(temp_obj->affected[i].location == filter->affect) {
								show_obj_to_char(temp_obj, ch, 1, 2, 1);
								break;
							}
						}
					}
				}
			}
			break;
		}
	}
}

void Clan::HouseStat(CHAR_DATA * ch, std::string & buffer)
{
	std::string buffer2;
	GetOneParam(buffer, buffer2);
	bool all = 0;
	std::ostringstream out;

	if (CompareParam(buffer2, "очистить") || CompareParam(buffer2, "удалить")) {
		if (GET_CLAN_RANKNUM(ch)) {
			send_to_char("У Вас нет прав удалять статистику.\r\n", ch);
			return;
		}
		for (ClanMemberList::const_iterator it = this->members.begin(); it != this->members.end(); ++it) {
			(*it).second->money = 0;
			(*it).second->exp = 0;
		}
		send_to_char("Статистика Вашей дружины очищена.\r\n", ch);
		return;
	} else if (CompareParam(buffer2, "все")) {
		all = 1;
		out << "Статистика Вашей дружины:\r\n";
	} else
		out << "Статистика Вашей дружины (находящиеся онлайн):\r\n";

	out << "Имя             Набрано опыта  Заработано кун  Процент сдачи в казну\r\n";
	for (ClanMemberList::const_iterator it = this->members.begin(); it != this->members.end(); ++it) {
		if (!all) {
			DESCRIPTOR_DATA *d;
			if (!(d = DescByUID((*it).first)))
				continue;
		}
		out.setf(std::ios_base::left, std::ios_base::adjustfield);
		out << std::setw(15) << (*it).second->name << " " << std::setw(14) << (*it).second->exp << " ";
		out << std::setw(15) << (*it).second->money;
		out << " " << (*it).second->tax << "%\r\n";
	}
	send_to_char(out.str(), ch);
}

int Clan::GetClanTax(CHAR_DATA * ch)
{
	if(IS_NPC(ch) || !GET_CLAN_RENT(ch))
		return 0;
	
	ClanListType::const_iterator clan = Clan::GetClan(ch);
	if (clan != Clan::ClanList.end())
		return (*clan)->members[GET_UNIQUE(ch)]->tax;
	return 0;
}

// из money считается и вычитается tax, возвращается собсно величина налога
int Clan::SetTax(CHAR_DATA * ch, int * money)
{
	ClanListType::const_iterator clan = Clan::GetClan(ch);
	if (clan != Clan::ClanList.end()) {
		if(!(*clan)->members[GET_UNIQUE(ch)]->tax)
			return -1;
		int tax = *money * (*clan)->members[GET_UNIQUE(ch)]->tax / 100;
		(*clan)->bank += tax;
		(*clan)->members[GET_UNIQUE(ch)]->money += tax;
		*money -= tax;
		return tax;
	}
	return -1;
}
