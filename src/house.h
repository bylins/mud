/* ****************************************************************************
* File: house.h                                                Part of Bylins *
* Usage: Handling of clan system                                              *
* (c) 2005 Krodo                                                              *
******************************************************************************/

#ifndef _HOUSE_H_
#define _HOUSE_H_

#include <vector>
#include <map>
#include <bitset>
#include <string>
#include <boost/shared_ptr.hpp>
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "interpreter.h"
#include "house_exp.hpp"
#include "remember.hpp"

namespace ClanSystem
{

enum Privileges: unsigned
{
	MAY_CLAN_INFO = 0,
	MAY_CLAN_ADD,
	MAY_CLAN_REMOVE,
	MAY_CLAN_PRIVILEGES,
	MAY_CLAN_CHANNEL,
	MAY_CLAN_POLITICS,
	MAY_CLAN_NEWS,
	MAY_CLAN_PKLIST,
	MAY_CLAN_CHEST_PUT,
	MAY_CLAN_CHEST_TAKE,
	MAY_CLAN_BANK,
	MAY_CLAN_EXIT,
	MAY_CLAN_MOD,
	MAY_CLAN_TAX,
	/// всего привилегий
	CLAN_PRIVILEGES_NUM
};

const int MAX_GOLD_TAX_PCT = 25;
const int MIN_GOLD_TAX_AMOUNT = 100;

bool is_ingr_chest(OBJ_DATA *obj);
void save_ingr_chests();
bool show_ingr_chest(OBJ_DATA *obj, CHAR_DATA *ch);
void save_chest_log();
// управление клан налогом
void tax_manage(CHAR_DATA *ch, std::string &buffer);
// первичная генерация справки сайтов дружин
void init_xhelp();
/// высчитывает и снимает клан-налог с gold кун
/// \return сумму получившегося налога, которую надо снять с чара
long do_gold_tax(CHAR_DATA *ch, long gold);

} // namespace ClanSystem

#define POLITICS_NEUTRAL  0
#define POLITICS_WAR      1
#define POLITICS_ALLIANCE 2

#define HCE_ATRIUM 0
#define	HCE_PORTAL 1

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

void fix_ingr_chest_rnum(const int room_rnum);//Нужно чтоб позиция короба не съехала

class ClanMember
{
public:
	ClanMember() :
		rank_num(0),
		money(0),
		exp(0),
		clan_exp(0),
		level(0),
		remort(false)
	{};

	std::string name;   // имя игрока
	int rank_num;       // номер ранга
	long long money;    // баланс персонажа по отношению к клановой казне
	long long exp;      // набраная топ-экспа
	long long clan_exp; // набранная клан-экспа

/// не сохраняются в клан-файле
	// уровень для клан стата (те, кто онлайн)
	int level;
	// краткое название класса для того же
	std::string class_abbr;
	// на праве или нет
	bool remort;
};

struct ClanPk
{
	long author;            // уид автора
	std::string victimName;	// имя жертвы
	std::string authorName;	// имя автора
	time_t time;            // время записи
	std::string text;       // комментарий
};

struct ClanStuffName
{
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
typedef std::vector<std::bitset<ClanSystem::CLAN_PRIVILEGES_NUM> > ClanPrivileges;
typedef std::map<int, int> ClanPolitics;
typedef std::vector<ClanStuffName> ClanStuffList;

struct ClanOLC
{
	int mode;                  // для контроля состояния олц
	ClanPtr clan;              // клан, который правим
	ClanPrivileges privileges; // свой список привилегий на случай не сохранения при выходе
	int rank;                  // редактируемый в данный момент ранг
	std::bitset<ClanSystem::CLAN_PRIVILEGES_NUM> all_ranks; // буфер для удаления/добавления всем рангам
};

struct ClanInvite
{
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
	static void SaveChestAll();
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
	static int print_spell_locate_object(CHAR_DATA *ch, int count, std::string name);
	static int GetClanWars(CHAR_DATA * ch);
	static void init_chest_rnum();
	static bool is_clan_chest(OBJ_DATA *obj);
	bool is_clan_member(int unique);//Возвращает true если чар с данным unique в клане
	bool is_alli_member(int unique);//Возвращает true если чар с данным unique в альянсе
	static void clan_invoice(CHAR_DATA *ch, bool enter);
	static void save_pk_log();

	static bool put_ingr_chest(CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *chest);
	static bool take_ingr_chest(CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *chest);

	void Manage(DESCRIPTOR_DATA * d, const char * arg);
	void AddTopExp(CHAR_DATA * ch, int add_exp);

	const char * GetAbbrev()
	{
		return this->abbrev.c_str();
	};
	int GetRent();
	int GetOutRent();
	void SetClanExp(CHAR_DATA *ch, int add);
	int GetClanLevel()
	{
		return this->clan_level;
	};
	std::string GetClanTitle()
	{
		return this->title;
	};
	std::string get_abbrev() const
	{
		return abbrev;
	};
	std::string get_file_abbrev() const;
	bool CheckPrivilege(int rank, int privilege)
	{
		return this->privileges[rank][privilege];
	};
	int CheckPolitics(int victim);

	void add_remember(std::string text, int flag);
	std::string get_remember(unsigned int num, int flag) const;

	void write_mod(const std::string &arg);
	void print_mod(CHAR_DATA *ch) const;
	void load_mod();

	void init_ingr_chest();
	int get_ingr_chest_room_rnum() const { return ingr_chest_room_rnum_; };
	void set_ingr_chest_room_rnum(const int new_rnum) { ingr_chest_room_rnum_ = new_rnum; };
	int ingr_chest_tax();
	void purge_ingr_chest();
	int get_ingr_chest_objcount() const { return ingr_chest_objcount_; };
	bool ingr_chest_active() const;
	void set_ingr_chest(CHAR_DATA *ch);
	void disable_ingr_chest(CHAR_DATA *ch);

	int calculate_clan_tax() const;
	void add_offline_member(const std::string &name, int uid, int rank);
	int ingr_chest_max_objects();

	void save_chest();

	std::string get_web_url() const { return web_url_; };

	void set_bank(unsigned num);
	unsigned get_bank() const;

	void set_gold_tax_pct(unsigned num);
	unsigned get_gold_tax_pct() const;

	friend ACMD(DoHouse);
	friend ACMD(DoClanChannel);
	friend ACMD(DoClanList);
	friend ACMD(DoShowPolitics);
	friend ACMD(DoHcontrol);
	friend ACMD(DoWhoClan);
	friend ACMD(DoClanPkList);
	friend ACMD(DoStoreHouse);
	friend ACMD(do_clanstuff);
	friend ACMD(DoShowWars);

	// клан пк
	ClanPkLog pk_log;
	// набранная за последний месяц экспа
	ClanExp last_exp;
	// помесячная история экспы без учета минусов
	ClanExpHistory exp_history;
	// лог клан-храна
	ClanChestLog chest_log;

private:
	std::string abbrev; // аббревиатура клана, ОДНО слово
	std::string name;   // длинное имя клана
	std::string title;  // что будет видно в титуле членов клана (лучше род.падеж, если это не аббревиатура)
	std::string title_female; // title для персонажей женского рода
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
	std::string mod_text; // сообщение дружины
	// рнум комнаты, где стоит хранилище под ингры (-1 если опция выключена)
	int ingr_chest_room_rnum_;
	// адрес сайта дружины для 'справка сайтыдружин'
	std::string web_url_;
	// пока общий на всех налог на лут кун
	unsigned gold_tax_pct_;

	//no save
	int chest_objcount;
	int chest_discount;
	int chest_weight;
	Remember::RememberListType remember_; // вспомнить клан
	Remember::RememberListType remember_ally_; // вспомнить союз
	int ingr_chest_objcount_;

	void ClanUpgrade();
	void SetPolitics(int victim, int state);
	void ManagePolitics(CHAR_DATA* ch, std::string& buffer);
	void HouseInfo(CHAR_DATA* ch);
	void HouseAdd(CHAR_DATA* ch, std::string& buffer);
	void HouseRemove(CHAR_DATA* ch, std::string& buffer);
	void ClanAddMember(CHAR_DATA* ch, int rank);
	void HouseOwner(CHAR_DATA* ch, std::string& buffer);
	void HouseLeave(CHAR_DATA* ch);
	int GetClanScore();
	void HouseStat(CHAR_DATA* ch, std::string& buffer);
	void remove_member(ClanMemberList::iterator& it);
	void save_clan_file(const std::string& filename) const;
	void house_web_url(CHAR_DATA* ch, const std::string& buffer);

	// house аля олц
	void MainMenu(DESCRIPTOR_DATA * d);
	void PrivilegeMenu(DESCRIPTOR_DATA * d, unsigned num);
	void AllMenu(DESCRIPTOR_DATA * d, unsigned flag);
	void GodToChannel(CHAR_DATA *ch, std::string text, int subcmd);
	void CharToChannel(CHAR_DATA *ch, std::string text, int subcmd);

	static void HcontrolBuild(CHAR_DATA * ch, std::string & buffer);
	static void HcontrolDestroy(CHAR_DATA * ch, std::string & buffer);
	static void hcontrol_title(CHAR_DATA *ch, std::string &text);
	static void hcontrol_rank(CHAR_DATA *ch, std::string &text);
	static void hcontrol_exphistory(CHAR_DATA *ch, std::string &text);
	static void hcontrol_set_ingr_chest(CHAR_DATA *ch, std::string &text);
	static void hcon_outcast(CHAR_DATA *ch, std::string &buffer);
	static void hcon_owner(CHAR_DATA *ch, std::string &text);

	static void ChestLoad();
	int ChestTax();
	int ChestMaxObjects()
	{
		return (this->clan_level + 1)*500 + 100;
	};
	int ChestMaxWeight()
	{
		return (this->clan_level + 1)*5000 + 500;
	};

	// для сортировки вывода членов клана по рангам, когда оно через поля чара дергается
	class SortRank
	{
	public:
		bool operator()(const CHAR_DATA * ch1, const CHAR_DATA * ch2);
	};
};

void SetChestMode(CHAR_DATA *ch, std::string &buffer);
std::string GetChestMode(CHAR_DATA *ch);
std::string clan_get_custom_label(OBJ_DATA *obj, ClanPtr clan);

#endif
