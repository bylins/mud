/* ****************************************************************************
* File: house.h                                                Part of Bylins *
* Usage: Handling of clan system                                              *
* (c) 2005 Krodo                                                              *
******************************************************************************/

#ifndef _HOUSE_H_
#define _HOUSE_H_

#include "interpreter.h"
#include "house_exp.h"
#include "communication/remember.h"
#include "db.h"
#include "conf.h"
#include "structs/structs.h"
#include "sysdep.h"

#include <vector>
#include <map>
#include <unordered_map>
#include <bitset>
#include <string>

namespace ClanSystem {

enum Privileges : unsigned {
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
	MAY_CLAN_BOARD,
	/// всего привилегий
	CLAN_PRIVILEGES_NUM
};
const unsigned MAX_GOLD_TAX_PCT = 50;
const int MIN_GOLD_TAX_AMOUNT = 100;
bool is_alliance(CharacterData *ch, char *clan_abbr);
void check_player_in_house();
bool is_ingr_chest(ObjectData *obj);
void save_ingr_chests();
bool show_ingr_chest(ObjectData *obj, CharacterData *ch);
void save_chest_log();
// управление клан налогом
void tax_manage(CharacterData *ch, std::string &buffer);
// первичная генерация справки сайтов дружин
void init_xhelp();
/// высчитывает и снимает клан-налог с gold кун
/// \return сумму получившегося налога, которую надо снять с чара
long do_gold_tax(CharacterData *ch, long gold);

} // namespace ClanSystem

const int POLITICS_NEUTRAL = 0;
const int POLITICS_WAR = 1;
const int POLITICS_ALLIANCE = 2;

const int HCE_ATRIUM = 0;
const int HCE_PORTAL = 1;

// период снятия за ренту (минут)
const int kChestUpdatePeriod = 10;
// период оповещения о скорой кончине денег (минут)
const int kChestInvoicePeriod = 60;
// период обновление статов экспы в топе кланов в режиме запрета обновления на лету (минут)
const int kClanTopRefreshPeriod = 360;
// клановый налог в день
const int kClanTax = 1000;
// налог на выборку по параметрам из хранилища в день
const int kClanStorehouseTax = 1000;
// процент стоимости ренты шмотки (надетой) для хранилища
const int kClanStorehouseCoeff = 50;

const int MAX_CLANLEVEL = 5;
// номер зоны с прототипами клан-стафа
const int CLAN_STUFF_ZONE = 18;
const int CHEST_IDENT_PAY = 110;

void fix_ingr_chest_rnum(const int room_rnum);//Нужно чтоб позиция короба не съехала

class ClanMember {
 public:
	using shared_ptr = std::shared_ptr<ClanMember>;

	ClanMember() :
		rank_num(0),
		money(0),
		exp(0),
		clan_exp(0),
		level(0),
		remort(false) {};

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

struct ClanPk {
	long author;            // уид автора
	std::string victimName;    // имя жертвы
	std::string authorName;    // имя автора
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

typedef std::shared_ptr<ClanPk> ClanPkPtr;
typedef std::map<long, ClanPkPtr> ClanPkList;
typedef std::vector<std::bitset<ClanSystem::CLAN_PRIVILEGES_NUM> > ClanPrivileges;
typedef std::map<int, int> ClanPolitics;
typedef std::vector<ClanStuffName> ClanStuffList;

class ClanMembersList : private std::unordered_map<long, ClanMember::shared_ptr> {
 public:
	using base_t = std::unordered_map<long, ClanMember::shared_ptr>;

	using base_t::key_type;
	using base_t::iterator;

	using base_t::begin;
	using base_t::end;
	using base_t::find;
	using base_t::size;
	using base_t::clear;

	void set(const key_type &key, const mapped_type &value) { (*this)[key] = value; }
	void set_rank(const key_type &key, const int rank) const { (*this).at(key)->rank_num = rank; }
	void add_money(const key_type &key, const long long money) { (*this).at(key)->money += money; }
	void sub_money(const key_type &key, const long long money) { (*this).at(key)->money -= money; }
	void erase(const const_iterator &i) { base_t::erase(i); }
};

class Clan {
 public:
	using shared_ptr = std::shared_ptr<Clan>;
	using ClanListType = std::vector<Clan::shared_ptr>;

	Clan();
	~Clan();

	static ClanListType ClanList; // список кланов

	static void ClanLoad();
	static void ClanLoadSingle(const std::string& index);
	static void ClanReload(const std::string& index);
	static void ClanSave();
	static void SaveChestAll();
	static void HconShow(CharacterData *ch);
	static void SetClanData(CharacterData *ch);
	static void ChestUpdate();
	static bool MayEnter(CharacterData *ch, RoomRnum room, bool mode);
	static bool InEnemyZone(CharacterData *ch);
	static bool PutChest(CharacterData *ch, ObjectData *obj, ObjectData *chest);
	static bool TakeChest(CharacterData *ch, ObjectData *obj, ObjectData *chest);
	static void ChestInvoice();
	static bool BankManage(CharacterData *ch, char *arg);
	static RoomRnum CloseRent(RoomRnum to_room);
	static shared_ptr GetClanByRoom(RoomRnum room);
	static void CheckPkList(CharacterData *ch);
	static void SyncTopExp();
	static bool ChestShow(ObjectData *list, CharacterData *ch);
	static void remove_from_clan(long unique);
	static int print_spell_locate_object(CharacterData *ch, int count, std::string name);
	static int print_imm_where_obj(CharacterData *ch, const char *name, int count);
	static int GetClanWars(CharacterData *ch);
	static void init_chest_rnum();
	static bool is_clan_chest(ObjectData *obj);
	static bool is_ingr_chest(ObjectData *obj);
	static void clan_invoice(CharacterData *ch, bool enter);
	static int delete_obj(int vnum);
	static void save_pk_log();
	static bool put_ingr_chest(CharacterData *ch, ObjectData *obj, ObjectData *chest);
	static bool take_ingr_chest(CharacterData *ch, ObjectData *obj, ObjectData *chest);

	bool is_clan_member(int unique);//Возвращает true если чар с данным unique в клане
	bool is_alli_member(int unique);//Возвращает true если чар с данным unique в альянсе

	void Manage(DescriptorData *d, const char *arg);
	void AddTopExp(CharacterData *ch, int add_exp);

	const char *GetAbbrev() { return this->abbrev.c_str(); };
	int get_chest_room();
	int GetRent();
	int GetOutRent();
	void SetClanExp(CharacterData *ch, int add);
	int GetClanLevel() { return this->clan_level; }
	std::string GetClanTitle() { return this->title; }
	std::string get_abbrev() const { return abbrev; }
	std::string get_file_abbrev() const;
	bool CheckPrivilege(int rank, int privilege) { return this->privileges[rank][privilege]; }
	int CheckPolitics(int victim);

	void add_remember(std::string text, int flag);
	std::string get_remember(unsigned int num, int flag) const;

	void write_mod(const std::string &arg);
	bool print_mod(CharacterData *ch) const;
	void load_mod();
	int get_rep();
	void set_rep(int rep);
	void init_ingr_chest();
	int get_ingr_chest_room_rnum() const { return ingr_chest_room_rnum_; };
	void set_ingr_chest_room_rnum(const int new_rnum) { ingr_chest_room_rnum_ = new_rnum; };
	int ingr_chest_tax();
	void purge_ingr_chest();
	int get_ingr_chest_objcount() const { return ingr_chest_objcount_; };
	bool ingr_chest_active() const;
	void set_ingr_chest(CharacterData *ch);
	void disable_ingr_chest(CharacterData *ch);
	int calculate_clan_tax() const;
	void add_offline_member(const std::string &name, int uid, int rank);
	int ingr_chest_max_objects();

	void save_chest();

	std::string get_web_url() const { return web_url_; };

	void set_bank(unsigned num);
	unsigned get_bank() const;

	void set_gold_tax_pct(unsigned num);
	unsigned get_gold_tax_pct() const;

	friend void DoHouse(CharacterData *ch, char *argument, int cmd, int subcmd);
	friend void DoClanChannel(CharacterData *ch, char *argument, int cmd, int subcmd);
	friend void DoClanList(CharacterData *ch, char *argument, int cmd, int subcmd);
	friend void DoShowPolitics(CharacterData *ch, char *argument, int cmd, int subcmd);
	friend void DoHcontrol(CharacterData *ch, char *argument, int cmd, int subcmd);
	friend void DoWhoClan(CharacterData *ch, char *argument, int cmd, int subcmd);
	friend void DoClanPkList(CharacterData *ch, char *argument, int cmd, int subcmd);
	friend void DoStoreHouse(CharacterData *ch, char *argument, int cmd, int subcmd);
	friend void do_clanstuff(CharacterData *ch, char *argument, int cmd, int subcmd);
	friend void DoShowWars(CharacterData *ch, char *argument, int cmd, int subcmd);
	friend void do_show_alliance(CharacterData *ch, char *argument, int cmd, int subcmd);
	bool check_write_board(CharacterData *ch);
	int out_rent;   // номер румы для отписанных, чтобы не тусовались в замке дальше

	// клан пк
	ClanPkLog pk_log;
	// набранная за последний месяц экспа
	ClanExp last_exp;
	// помесячная история экспы без учета минусов
	ClanExpHistory exp_history;
	// лог клан-храна
	ClanChestLog chest_log;

	static void SetPk(CharacterData *ch, std::string buffer);
	bool is_pk();
	void change_pk_status();

 private:
	std::string abbrev; // аббревиатура клана, ОДНО слово
	std::string name;   // длинное имя клана
	std::string title;  // что будет видно в титуле членов клана (лучше род.падеж, если это не аббревиатура)
	std::string title_female; // title для персонажей женского рода
	std::string owner;  // имя воеводы
	MobVnum guard;     // охранник замка
	time_t builtOn;     // дата создания
	double bankBuffer;  // буффер для более точного снятия за хранилище
	bool entranceMode;  // вход в замок для всех/только свои и альянс
	std::vector<std::string> ranks; // список названий рангов
	std::vector<std::string> ranks_female; // список названий рангов для женского рода
	ClanPolitics politics;     // состояние политики
	ClanPkList pkList;  // пклист
	ClanPkList frList;  // дрлист
	long bank;          // состояние счета банка
	long long exp; // суммарная топ-экспа
	long long clan_exp; //суммарная клан-экспа
	long
		exp_buf;  // буффер для суммарной топ-экспы в режиме запрета подсчета в ран-тайме (exp_info), синхронизация раз в 6 часов
	int clan_level; // текущий уровень клана
	int rent;       // номер центральной комнаты в замке, заодно УИД клана
	bool pk;        // является ли клан пк или нет
	int chest_room; // комната с сундуком, по дефолту равняется ренте. чтобы не искать постояно руму в циклах
	ClanPrivileges privileges; // список привилегий для рангов
	ClanMembersList m_members;    // список членов дружины (уид, имя, номер ранга)
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
	// очки репутации
	int reputation;
	//no save
	int chest_objcount;
	int chest_discount;
	int chest_weight;
	Remember::RememberListType remember_; // вспомнить клан
	Remember::RememberListType remember_ally_; // вспомнить союз
	int ingr_chest_objcount_;

	void SetPolitics(int victim, int state);
	void ManagePolitics(CharacterData *ch, std::string &buffer);
	void HouseInfo(CharacterData *ch);
	void HouseAdd(CharacterData *ch, std::string &buffer);
	void HouseRemove(CharacterData *ch, std::string &buffer);
	void ClanAddMember(CharacterData *ch, int rank);
	void HouseOwner(CharacterData *ch, std::string &buffer);
	void HouseLeave(CharacterData *ch);
	void HouseStat(CharacterData *ch, std::string &buffer);
	void remove_member(const ClanMembersList::key_type &it);
	void save_clan_file(const std::string &filename) const;
	void house_web_url(CharacterData *ch, const std::string &buffer);

	// house аля олц
	void MainMenu(DescriptorData *d);
	void PrivilegeMenu(DescriptorData *d, unsigned num);
	void AllMenu(DescriptorData *d, unsigned flag);
	void GodToChannel(CharacterData *ch, std::string text, int subcmd);
	void CharToChannel(CharacterData *ch, std::string text, int subcmd);

	static void HcontrolBuild(CharacterData *ch, std::string &buffer);
	static void HcontrolDestroy(CharacterData *ch, std::string &buffer);
	static void DestroyClan(Clan::shared_ptr clan);
	static void fix_clan_members_load_room(Clan::shared_ptr clan);
	static void hcontrol_title(CharacterData *ch, std::string &text);
	static void hcontrol_rank(CharacterData *ch, std::string &text);
	static void hcontrol_exphistory(CharacterData *ch, std::string &text);
	static void hcontrol_set_ingr_chest(CharacterData *ch, std::string &text);
	static void hcon_outcast(CharacterData *ch, std::string &buffer);
	static void hcon_owner(CharacterData *ch, std::string &text);

	static void ChestLoad();
	int ChestTax();
	int ChestMaxObjects() {
		return (this->clan_level + 1) * 500 + 100;
	};
	int ChestMaxWeight() {
		return (this->clan_level + 1) * 5000 + 500;
	};
};

struct ClanOLC {
	int mode;                  // для контроля состояния олц
	Clan::shared_ptr clan;              // клан, который правим
	ClanPrivileges privileges; // свой список привилегий на случай не сохранения при выходе
	int rank;                  // редактируемый в данный момент ранг
	std::bitset<ClanSystem::CLAN_PRIVILEGES_NUM> all_ranks; // буфер для удаления/добавления всем рангам
};

struct ClanInvite {
	Clan::shared_ptr clan; // приглашающий клан
	int rank;     // номер приписываемого ранга
};

void SetChestMode(CharacterData *ch, std::string &buffer);
std::string GetChestMode(CharacterData *ch);
std::string clan_get_custom_label(ObjectData *obj, Clan::shared_ptr clan);

bool CHECK_CUSTOM_LABEL_CORE(const ObjectData *obj, const CharacterData *ch);

// проверяет arg на совпадение с персональными или клановыми метками
// чармис автора меток их тоже может использовать
bool CHECK_CUSTOM_LABEL(const char *arg, const ObjectData *obj, const CharacterData *ch);

inline bool CHECK_CUSTOM_LABEL(const std::string &arg, const ObjectData *obj, const CharacterData *ch) {
	return CHECK_CUSTOM_LABEL(arg.c_str(), obj, ch);
}

// видит ли ch метки obj
bool AUTH_CUSTOM_LABEL(const ObjectData *obj, const CharacterData *ch);

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
