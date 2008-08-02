/* ****************************************************************************
* File: house.h                                                Part of Bylins *
* Usage: Handling of clan system                                              *
* (c) 2005 Krodo                                                              *
******************************************************************************/

#ifndef _HOUSE_H_
#define _HOUSE_H_

#include "conf.h"
#include <vector>
#include <map>
#include <bitset>
#include <string>
#include <boost/shared_ptr.hpp>

#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "interpreter.h"

#define CLAN_PRIVILEGES_NUM 12
#define MAY_CLAN_INFO       0
#define MAY_CLAN_ADD        1
#define MAY_CLAN_REMOVE     2
#define MAY_CLAN_PRIVILEGES 3
#define MAY_CLAN_CHANNEL    4
#define MAY_CLAN_POLITICS   5
#define MAY_CLAN_NEWS       6
#define MAY_CLAN_PKLIST     7
#define MAY_CLAN_CHEST_PUT  8
#define MAY_CLAN_CHEST_TAKE 9
#define MAY_CLAN_BANK       10
#define MAY_CLAN_EXIT       11
// не забываем про CLAN_PRIVILEGES_NUM

#define POLITICS_NEUTRAL  0
#define POLITICS_WAR      1
#define POLITICS_ALLIANCE 2

#define HCE_ATRIUM 0
#define	HCE_PORTAL 1

#define CLAN_MAIN_MENU      0
#define CLAN_PRIVILEGE_MENU 1
#define CLAN_SAVE_MENU      2
#define CLAN_ADDALL_MENU    3
#define CLAN_DELALL_MENU    4

// vnum кланового сундука
#define CLAN_CHEST 330
// период снятия за ренту (минут)
#define CHEST_UPDATE_PERIOD 10
// период оповещения о скорой кончине денег (минут)
#define CHEST_INVOICE_PERIOD 60
// период обновление статов экспы в топе кланов в режиме запрета обновления на лету (минут)
#define CLAN_TOP_REFRESH_PERIOD 360
// клановый налог в день
#define CLAN_TAX 1000
// налог на выборку по параметрам из хранилища в день
#define CLAN_STOREHOUSE_TAX 1000
// процент стоимости ренты шмотки (одетой) для хранилища
#define CLAN_STOREHOUSE_COEFF 50

#define MAX_CLANLEVEL 5
// номер зоны с прототипами клан-стафа
#define CLAN_STUFF_ZONE 18

#define CHEST_IDENT_PAY 110

class ClanMember {
	public:
	ClanMember() : rank_num(0), money(0), exp(0), exp_persent(0), clan_exp(0) {};
	std::string name;   // имя игрока
	int rank_num;       // номер ранга
	long long money;    // баланс персонажа по отношению к клановой казне
	long long exp;      // набранная топ-экспа
	int exp_persent;    // процент икспы отчисляемый в клан
	long long clan_exp; // набранная клан-экспа
};

struct ClanPk {
	long author;            // уид автора
	std::string victimName;	// имя жертвы
	std::string authorName;	// имя автора
	time_t time;            // время записи
	std::string text;       // комментарий
};

struct ClanStuffName {
	int num;
	std::string name;
	std::string desc;
	std::string longdesc;
	std::vector<std::string> PNames;
};

class Clan;

typedef boost::shared_ptr<Clan> ClanPtr;
typedef std::vector<ClanPtr> ClanListType;
typedef boost::shared_ptr<ClanMember> ClanMemberPtr;
typedef std::map<long, ClanMemberPtr> ClanMemberList;
typedef boost::shared_ptr<ClanPk> ClanPkPtr;
typedef std::map<long, ClanPkPtr> ClanPkList;
typedef std::vector<std::bitset<CLAN_PRIVILEGES_NUM> > ClanPrivileges;
typedef std::map<int, int> ClanPolitics;
typedef std::vector<ClanStuffName> ClanStuffList;

struct ClanOLC {
	int mode;                  // для контроля состояния олц
	ClanPtr clan;              // клан, который правим
	ClanPrivileges privileges; // свой список привилегий на случай не сохранения при выходе
	int rank;                  // редактируемый в данный момент ранг
	std::bitset<CLAN_PRIVILEGES_NUM> all_ranks; // буфер для удаления/добавления всем рангам
};

struct ClanInvite {
	ClanPtr clan; // приглашающий клан
	int rank;     // номер приписываемого ранга
};


class Clan
{
	public:
	Clan();
	~Clan();

	static ClanListType ClanList; // список кланов

	static void ClanLoad();
	static void ClanSave();
	static void ChestSave();
	static void HconShow(CHAR_DATA * ch);
	static void SetClanData(CHAR_DATA * ch);
	static void ChestUpdate();
	static bool MayEnter(CHAR_DATA * ch, room_rnum room, bool mode);
	static bool InEnemyZone(CHAR_DATA * ch);
	static bool PutChest(CHAR_DATA * ch, OBJ_DATA * obj, OBJ_DATA * chest);
	static bool TakeChest(CHAR_DATA * ch, OBJ_DATA * obj, OBJ_DATA * chest);
	static void ChestInvoice();
	static bool BankManage(CHAR_DATA * ch, char *arg);
	static room_rnum CloseRent(room_rnum to_room);
	static ClanListType::const_iterator IsClanRoom(room_rnum room);
	static void CheckPkList(CHAR_DATA * ch);
	static void SyncTopExp();
	static int GetTotalCharScore(CHAR_DATA * ch);
	static int GetRankByUID(long);
	static bool ChestShow(OBJ_DATA * list, CHAR_DATA * ch);
	static void remove_from_clan(long unique);

	void Manage(DESCRIPTOR_DATA * d, const char * arg);
	void AddTopExp(CHAR_DATA * ch, int add_exp);

	const char * GetAbbrev() { return this->abbrev.c_str(); };
	int GetRent();
	int SetClanExp(CHAR_DATA *ch, int add);  //На входе - икспа с моба - на выходе икспа собсно игроку. за вычетом той что идет в клан
	int GetMemberExpPersent(CHAR_DATA *ch);
	int GetClanLevel() { return this->clan_level; };
	std::string GetClanTitle() { return this->title; };
	bool CheckPrivilege(int rank, int privilege) { return this->privileges[rank][privilege]; };

	friend ACMD(DoHouse);
	friend ACMD(DoClanChannel);
	friend ACMD(DoClanList);
	friend ACMD(DoShowPolitics);
	friend ACMD(DoHcontrol);
	friend ACMD(DoWhoClan);
	friend ACMD(DoClanPkList);
	friend ACMD(DoStoreHouse);
	friend ACMD(do_clanstuff);

	private:
	std::string abbrev; // аббревиатура клана, ОДНО слово
	std::string name;   // длинное имя клана
	std::string title;  // что будет видно в титуле членов клана (лучше род.падеж, если это не аббревиатура)
	std::string owner;  // имя воеводы
	mob_vnum guard;     // охранник замка
	time_t builtOn;     // дата создания
	double bankBuffer;  // буффер для более точного снятия за хранилище
	bool entranceMode;  // вход в замок для всех/только свои и альянс
	std::vector <std::string> ranks; // список названий рангов
	std::vector <std::string> ranks_female; // список названий рангов для женского рода
	ClanPolitics politics;     // состояние политики
	ClanPkList pkList;  // пклист
	ClanPkList frList;  // дрлист
	long bank;          // состояние счета банка
	long long exp; // суммарная топ-экспа
	long long clan_exp; //суммарная клан-экспа
	long exp_buf;  // буффер для суммарной топ-экспы в режиме запрета подсчета в ран-тайме (exp_info), синхронизация раз в 6 часов
	int clan_level; // текущий уровень клана
	int rent;       // номер центральной комнаты в замке, заодно УИД клана
	int out_rent;   // номер румы для отписанных, чтобы не тусовались в замке дальше
	int chest_room; // комната с сундуком, по дефолту равняется ренте. чтобы не искать постояно руму в циклах
	ClanPrivileges privileges; // список привилегий для рангов
	ClanMemberList members;    // список членов дружины (уид, имя, номер ранга)
	ClanStuffList clanstuff;   // клан-стаф
	bool storehouse;    // опция выборки из хранилища по параметрам шмота
	bool exp_info;      // показывать или нет набранную экспу
	bool test_clan;     // тестовый клан (привет рсп)
	//no save
	int chest_objcount;
	int chest_discount;
	int chest_weight;
	// вообще, если появится еще пара-тройка опций, то надо будет это в битсет засунуть

	void ClanUpgrade();
	int CheckPolitics(int victim);
	void SetPolitics(int victim, int state);
	void ManagePolitics(CHAR_DATA * ch, std::string & buffer);
	void HouseInfo(CHAR_DATA * ch);
	void HouseAdd(CHAR_DATA * ch, std::string & buffer);
	void HouseRemove(CHAR_DATA * ch, std::string & buffer);
	void TaxManage(CHAR_DATA * ch, std::string & arg);
	void ClanAddMember(CHAR_DATA * ch, int rank);
	void HouseOwner(CHAR_DATA * ch, std::string & buffer);
	void HouseLeave(CHAR_DATA * ch);
	int GetClanScore();
	void HouseStat(CHAR_DATA * ch, std::string & buffer);
	void remove_member(ClanMemberList::iterator &it);

	// house аля олц
	void MainMenu(DESCRIPTOR_DATA * d);
	void PrivilegeMenu(DESCRIPTOR_DATA * d, unsigned num);
	void AllMenu(DESCRIPTOR_DATA * d, unsigned flag);
	void GodToChannel(CHAR_DATA *ch, std::string text, int subcmd);
	void CharToChannel(CHAR_DATA *ch, std::string text, int subcmd);

	static void HcontrolBuild(CHAR_DATA * ch, std::string & buffer);
	static void HcontrolDestroy(CHAR_DATA * ch, std::string & buffer);
	static void hcon_outcast(CHAR_DATA *ch, std::string buffer);
	static void ChestLoad();
	int ChestTax();
	int ChestMaxObjects() {return (this->clan_level+1)*500+100;};
	int ChestMaxWeight() {return (this->clan_level+1)*5000+500;};

	// для сортировки вывода членов клана по рангам, когда оно через поля чара дергается
	class SortRank
	{
		public:
		bool operator() (const CHAR_DATA * ch1, const CHAR_DATA * ch2);
	};
};

void SetChestMode(CHAR_DATA *ch, std::string &buffer);
std::string GetChestMode(CHAR_DATA *ch);

#endif
