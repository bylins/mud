/*****************************************************************************
* File: house.cpp                                              Part of Bylins *
* Usage: Handling of clan system                                              *
* (c) 2005 Krodo                                                              *
******************************************************************************/

#include "house.h"

#include <sys/stat.h>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cmath>
#include <iomanip>
#include <limits>

#include <third_party_libs/fmt/include/fmt/format.h>

#include "administration/karma.h"
#include "engine/db/world_objects.h"
#include "engine/db/world_characters.h"
#include "engine/db/obj_prototypes.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "engine/entities/obj_data.h"
#include "engine/core/comm.h"
#include "engine/core/handler.h"
#include "gameplay/fight/pk.h"
#include "engine/ui/color.h"
#include "gameplay/communication/boards/boards.h"
#include "gameplay/skills/skills.h"
#include "gameplay/magic/spells.h"
#include "administration/privilege.h"
#include "engine/entities/char_data.h"
#include "engine/entities/char_player.h"
#include "engine/ui/modify.h"
#include "engine/entities/room_data.h"
#include "engine/db/obj_save.h"
#include "engine/core/handler.h"
#include "gameplay/mechanics/named_stuff.h"
#include "engine/db/help.h"
#include "engine/core/conf.h"
#include "engine/db/global_objects.h"
#include "engine/ui/objects_filter.h"
#include "engine/ui/table_wrapper.h"
#include "gameplay/mechanics/sight.h"
#include "gameplay/communication/ignores.h"
#include "gameplay/core/constants.h"
#include "utils/utils_time.h"
#include "gameplay/mechanics/dungeons.h"
#include "engine/ui/cmd/do_who.h"

using namespace ClanSystem;

extern int mortal_start_room;
extern void list_obj_to_char(ObjData *list, CharData *ch, int mode, int show);
extern int AllocateBufferForFile(const char *name, char **destination_buf);
// TODO: думать надо с этим, или глобально следить за спамом, или игноров напихать на все случаи жизни, или так и оставить
extern void SetWait(CharData *ch, int waittime, int victim_in_room);
extern const char *show_obj_to_char(ObjData *object, CharData *ch, int mode, int show_state, int how);
extern bool char_to_pk_clan(CharData *ch);

void fix_ingr_chest_rnum(const int room_rnum)//Нужно чтоб позиция короба не съехала
{
	for (const auto &i : Clan::ClanList) {
		if (i->get_ingr_chest_room_rnum() >= room_rnum)
			i->set_ingr_chest_room_rnum(i->get_ingr_chest_room_rnum() + 1);
	}
}
namespace {

long long clan_level_exp[kMaxClanlevel + 1] =
	{
		0LL,
		100000000LL, // 1 level, should be achieved fast, 100M imho is possible//
		1000000000LL, // 1bilion. OMG. //
		5000000000LL, // 5bilions. //
		15000000000LL, // 15billions. //
		1000000000000LL // BIG NUMBER. //
	};

// vnum кланового сундука
const int CLAN_CHEST_VNUM = 330;
int CLAN_CHEST_RNUM = -1;
// vnum кланового сундука
const int INGR_CHEST_VNUM = 333;
int INGR_CHEST_RNUM = -1;
// налог на хранилище ингров (в день)
const int INGR_CHEST_TAX = 1000;
// макс. длина сообщения дружины
const int MAX_MOD_LENGTH = 3 * 80;
// макс. длина названия ранга в дружине
const unsigned MAX_RANK_LENGHT = 10;

enum {
  CLAN_MAIN_MENU = 0, CLAN_PRIVILEGE_MENU, CLAN_SAVE_MENU,
  CLAN_ADDALL_MENU, CLAN_DELALL_MENU
};

void prepare_write_mod(CharData *ch, std::string &param) {
	utils::Trim(param);
	if (!param.empty() && (CompareParam(param, "очистить") || CompareParam(param, "удалить"))) {
		std::string zero_str;
		CLAN(ch)->write_mod(zero_str);
		SendMsgToChar("Сообщение удалено.\r\n", ch);
		return;
	}
	SendMsgToChar("Можете писать сообщение.  (/s записать /h помощь)\r\n", ch);
	ch->desc->state = EConState::kWriteMod;
	utils::AbstractStringWriter::shared_ptr writer(new utils::StdStringWriter());
	string_write(ch->desc, writer, MAX_MOD_LENGTH, 0, nullptr);
}

/**
* Обрезание названий рангов дружины до MAX_RANK_LENGHT символов
* и перевод всего слова в нижний регистр.
*/
void check_rank(std::string &rank) {
	if (rank.size() > MAX_RANK_LENGHT) {
		rank = rank.substr(0, MAX_RANK_LENGHT);
	}
	utils::ConvertToLow(rank);
}

} // namespace

// для сортировки вывода членов клана по рангам, когда оно через поля чара дергается
class SortRank {
 public:
  bool operator()(const CharData::shared_ptr ch1, const CharData::shared_ptr ch2);
};

inline bool SortRank::operator()(const CharData::shared_ptr ch1, const CharData::shared_ptr ch2) {
	return CLAN_MEMBER(ch1)->rank_num < CLAN_MEMBER(ch2)->rank_num;
}

Clan::ClanListType Clan::ClanList;

// поиск to_room в зонах клан-замков, выставляет за замок, если найдено
RoomRnum Clan::CloseRent(RoomRnum to_room) {
	for (auto &clan : Clan::ClanList)
		if (world[to_room]->zone_rn == world[GetRoomRnum(clan->rent)]->zone_rn)
			return GetRoomRnum(clan->out_rent);
	return to_room;
}

int Clan::get_chest_room() {
	return this->chest_room;
}

// проверяет находится ли чар в зоне чужого клана
bool Clan::InEnemyZone(CharData *ch) {
	int zone = world[ch->in_room]->zone_rn;
	for (auto &clan : Clan::ClanList)
		if (zone == world[GetRoomRnum(clan->rent)]->zone_rn) {
			if (CLAN(ch) && (CLAN(ch) == clan || clan->CheckPolitics(CLAN(ch)->GetRent()) == kPoliticsAlliance))
				return false;
			else
				return true;
		}
	return false;
}

Clan::Clan() :
	out_rent(0),
	guard(0), builtOn(time(nullptr)), bankBuffer(0), entranceMode(false), bank(2000),
	exp(0), clan_exp(0), exp_buf(0), clan_level(0), rent(0),
	pk(true),
	chest_room(0), storehouse(true), exp_info(true), test_clan(false),
	ingr_chest_room_rnum_(-1), gold_tax_pct_(0), reputation(10),
	chest_objcount(0), chest_discount(0), chest_weight(0),
	ingr_chest_objcount_(0) {
}

Clan::~Clan() {
}
// релоад одного отдельного клана, абр. указывать на латинице!
void Clan::ClanReload(const std::string &index) {
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
	// ищем наш клан
	for (const auto &it : clanIndex) {
		if (it == index) {
			// надо удалить наш клан из общего списка
			for (auto clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan) {
				std::string name_buffer = (*clan)->abbrev;
				CreateFileName(name_buffer);
				if (name_buffer == index) {
					Clan::ClanList.erase(clan);
					break;
				}
			}
			Clan::ClanLoadSingle(index);
		}
	}

}

// лоад отдельного клана из файла
void Clan::ClanLoadSingle(const std::string &index) {
	std::string buffer;
	const auto tempClan = std::make_shared<Clan>();

	std::string filename = LIB_HOUSE + index + "/" + index;
	std::ifstream file(filename.c_str());
	if (!file.is_open()) {
		log("Error open file: %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
		return;
	}

	while (file >> buffer) {
		if (buffer == "Abbrev:") {
			if (!(file >> tempClan->abbrev)) {
				log("Error open 'Abbrev:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
				break;
			}
		} else if (buffer == "Name:") {
			std::getline(file, buffer);
			utils::Trim(buffer);
			tempClan->name = buffer;
		} else if (buffer == "Title:") {
			std::getline(file, buffer);
			utils::Trim(buffer);
			tempClan->title = buffer;
		} else if (buffer == "TitleFemale:") {
			std::getline(file, buffer);
			utils::Trim(buffer);
			tempClan->title_female = buffer;
		} else if (buffer == "Rent:") {
			int rent = 0;
			if (!(file >> rent)) {
				log("Error open 'Rent:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
				break;
			}
			// зоны может и не быть
			if (!GetRoomRnum(rent)) {
				log("Room %d is no longer exist (%s).", rent, filename.c_str());
				break;
			}
			tempClan->rent = rent;
		} else if (buffer == "OutRent:") {
			int out_rent = 0;
			if (!(file >> out_rent)) {
				log("Error open 'OutRent:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
				break;
			}
			// зоны может и не быть
			if (!GetRoomRnum(out_rent)) {
				log("Room %d is no longer exist (%s).", out_rent, filename.c_str());
				break;
			}
			tempClan->out_rent = out_rent;
		} else if (buffer == "ChestRoom:") {
			int chest_room = 0;
			if (!(file >> chest_room)) {
				log("Error open 'ChestRoom:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
				break;
			}
			// зоны может и не быть
			if (!GetRoomRnum(chest_room)) {
				log("Room %d is no longer exist (%s).", chest_room, filename.c_str());
				break;
			}
			tempClan->chest_room = chest_room;
		} else if (buffer == "IngrChestRoom:") {
			int tmp_vnum = 0;
			if (!(file >> tmp_vnum)) {
				log("Error read 'IngrChestRoom:' in %s! (%s %s %d)",
					filename.c_str(), __FILE__, __func__, __LINE__);
				break;
			}
			if (tmp_vnum != 0) {
				// зоны может и не быть
				int ingr_chest_room_rnum = GetRoomRnum(tmp_vnum);
				if (ingr_chest_room_rnum > 0) {
					tempClan->ingr_chest_room_rnum_ = ingr_chest_room_rnum;
				} else {
					log("Room %d is no longer exist (%s).",
						tmp_vnum, filename.c_str());
				}
			}
		} else if (buffer == "Rep:") {
			int rep = 0;
			if (!(file >> rep)) {
				log("Error open 'Rep:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
				break;
			}
			tempClan->set_rep(rep);
		} else if (buffer == "Guard:") {
			int guard = 0;
			if (!(file >> guard)) {
				log("Error open 'Guard:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
				break;
			}
			// как и охранника
			if (GetMobRnum(guard) < 0) {
				log("Guard %d is no longer exist (%s).", guard, filename.c_str());
				break;
			}
			tempClan->guard = guard;
		} else if (buffer == "BuiltOn:") {
			if (!(file >> tempClan->builtOn)) {
				log("Error open 'BuiltOn:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
				break;
			}
		} else if (buffer == "Ranks:") {
			std::getline(file, buffer, '~');
			std::istringstream stream(buffer);
			unsigned long priv = 0;

			std::string buffer2;
			// для верности воеводе проставим все привилегии, заодно сверим с файлом
			if (!(stream >> buffer >> buffer2 >> priv)) {
				log("Error open 'Ranks' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
				break;
			}
			check_rank(buffer);
			check_rank(buffer2);
			tempClan->ranks.push_back(buffer);
			tempClan->ranks_female.push_back(buffer2);

			tempClan->privileges.push_back(std::bitset<CLAN_PRIVILEGES_NUM>());
			for (unsigned i = 0; i < CLAN_PRIVILEGES_NUM; ++i) {
				tempClan->privileges[0].set(i);
			}

			while (stream >> buffer) {
				if (!(stream >> buffer2 >> priv)) {
					log("Error open 'Ranks' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
					break;
				}
				check_rank(buffer);
				check_rank(buffer2);
				tempClan->ranks.push_back(buffer);
				tempClan->ranks_female.push_back(buffer2);
				// на случай уменьшения привилегий
				if (priv > tempClan->privileges[0].to_ulong())
					priv = tempClan->privileges[0].to_ulong();
				tempClan->privileges.push_back(std::bitset<CLAN_PRIVILEGES_NUM>(priv));
			}
		} else if (buffer == "Politics:") {
			std::getline(file, buffer, '~');
			std::istringstream stream(buffer);
			int room = 0;
			int state = 0;

			while (stream >> room >> state)
				tempClan->politics[room] = state;
		} else if (buffer == "EntranceMode:") {
			if (!(file >> tempClan->entranceMode)) {
				log("Error open 'EntranceMode:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
				break;
			}
		} else if (buffer == "Storehouse:") {
			if (!(file >> tempClan->storehouse)) {
				log("Error open 'Storehouse:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
				break;
			}
		} else if (buffer == "ExpInfo:") {
			if (!(file >> tempClan->exp_info)) {
				log("Error open 'ExpInfo:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
				break;
			}
		} else if (buffer == "TestClan:") {
			if (!(file >> tempClan->test_clan)) {
				log("Error open 'TestClan:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
				break;
			}
		} else if (buffer == "Exp:") {
			if (!(file >> tempClan->exp)) {
				log("Error open 'Exp:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
				break;
			}
		} else if (buffer == "ExpBuf:") {
			if (!(file >> tempClan->exp_buf)) {
				log("Error open 'ExpBuf:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
				break;
			}
		} else if (buffer == "Bank:") {
			file >> tempClan->bank;
			log("Clans in bank, file (%s) банк %ld.", filename.c_str(), tempClan->bank);
			if (tempClan->bank <= 0) {
				log("Clan has 0 in bank, file (%s) возможно будет удален.", filename.c_str());
			}
		} else if (buffer == "Pk:") {
			file >> tempClan->pk;
		} else if (buffer == "GoldTax:") {
			file >> tempClan->gold_tax_pct_;
			if (tempClan->gold_tax_pct_ > MAX_GOLD_TAX_PCT) {
				log("Clan has invalid tax (%ld), remove from list (%s).",
					tempClan->gold_tax_pct_, filename.c_str());
				break;
			}
		} else if (buffer == "Site:") {
			file >> tempClan->web_url_;
		} else if (buffer == "StoredExp:") {
			if (!(file >> tempClan->clan_exp))
				log("Error open 'StoredExp:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
		} else if (buffer == "ClanLevel:") {
			if (!(file >> tempClan->clan_level))
				log("Error open 'ClanLevel:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
		} else if (buffer == "Owner:") {
			long unique = 0;
			long long money = 0;
			long long exp = 0;
			int exp_persent = 0; // заглушка
			long long clan_exp = 0;

			if (!(file >> unique >> money >> exp >> exp_persent >> clan_exp)) {
				log("Error open 'Owner:' in %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
				break;
			}
			// воеводы тоже уже может не быть
			const auto tempMember = std::make_shared<ClanMember>();
			tempMember->name = GetNameByUnique(unique);
			if (tempMember->name.empty()) {
				log("Owner %ld is no longer exist (%s).", unique, filename.c_str());
				break;
			}
			tempMember->name[0] = UPPER(tempMember->name[0]);
			tempMember->rank_num = 0;
			tempMember->money = money;
			tempMember->exp = exp;
			tempMember->clan_exp = clan_exp;
			tempClan->m_members.set(unique, tempMember);
			tempClan->owner = tempMember->name;

		} else if (buffer == "Members:") {
			// параметры, критичные для мемберов, нужно загрузить до того как (ранги например)
			long unique = 0;
			unsigned rank = 0;
			long long money = 0;
			long long exp = 0;
			int exp_persent = 0; // заглушка
			long long clan_exp = 0;

			std::getline(file, buffer, '~');
			std::istringstream stream(buffer);
			while (stream >> unique) {
				if (!(stream >> rank >> money >> exp >> exp_persent >> clan_exp)) {
					log("Error read %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
					break;
				}
				// на случай, если рангов стало меньше
				if (!tempClan->ranks.empty() && rank > tempClan->ranks.size() - 1) {
					rank = static_cast<decltype(rank)>(tempClan->ranks.size()) - 1;
				}

				// удаленные персонажи просто игнорируются
				const auto tempMember = std::make_shared<ClanMember>();
				tempMember->name = GetNameByUnique(unique);
				if (tempMember->name.empty()) {
					log("Member %ld is no longer exist (%s).", unique, filename.c_str());
					continue;
				}
				tempMember->name[0] = UPPER(tempMember->name[0]);
				tempMember->rank_num = rank;
				tempMember->money = money;
				tempMember->exp = exp;
				tempMember->clan_exp = clan_exp;
				tempClan->m_members.set(unique, tempMember);
			}
		}

	}
	file.close();

	// тут нужно проверить наличие критичных для клана полей
	// т.к. загрузка без привязки к положению в файле - что-то может не проинициализироваться
	if (tempClan->abbrev.empty() || tempClan->name.empty()
		|| tempClan->title.empty()
		|| tempClan->rent == 0 || tempClan->guard == 0 || tempClan->out_rent == 0
		|| tempClan->ranks.empty() || tempClan->privileges.empty()) {
		log("Clan read fail: %s", filename.c_str());
		return;
	}
	// удаление неактивных кланов
	tempClan->exp_history.load(tempClan->get_file_abbrev());
	// иним на случай полной неактивности по итогам месяца, чтобы не было пропусков в списке
	tempClan->exp_history.add_exp(0);
	if (tempClan->exp_history.need_destroy() && !tempClan->test_clan) {
		// клан-банк на воеводу
		if (tempClan->bank > 0) {
			Player t_victim;
			Player *victim = &t_victim;
			if (LoadPlayerCharacter(tempClan->owner.c_str(), victim, ELoadCharFlags::kFindId) < 0) {
				log("SYSERROR: error read owner file %s for clan delete (%s:%d)",
					tempClan->owner.c_str(), __FILE__, __LINE__);
				return;
			}
			victim->add_bank(tempClan->bank);
			victim->save_char();
		}
		Boards::Static::clan_delete_message(tempClan->abbrev, tempClan->rent / 100);
		char smallbuf[128];
		sprintf(smallbuf, "неактивность");
		DestroyClan(tempClan, smallbuf);
		log("Clan deleted: %s", filename.c_str());
	}

	// по дефолту жен род для титула берем из основного
	if (tempClan->title_female.empty()) {
		tempClan->title_female = tempClan->title;
	}
	// сундук по дефолту на ренте
	if (!tempClan->chest_room)
		tempClan->chest_room = tempClan->rent;

	// чтобы не получилось потерь/прибавок экспы
	if (tempClan->exp_buf) {
		tempClan->exp += tempClan->exp_buf;
		if (tempClan->exp < 0)
			tempClan->exp = 0;
		tempClan->exp_buf = 0;
	}

	// подгружаем пкл/дрл
	std::ifstream pkFile((filename + ".pkl").c_str());
	if (pkFile.is_open()) {
		int author = 0;
		while (pkFile >> author) {
			int victim = 0;
			time_t tempTime = time(nullptr);
			bool flag = false;

			if (!(pkFile >> victim >> tempTime >> flag)) {
				log("Error read %s! (%s %s %d)", (filename + ".pkl").c_str(), __FILE__, __func__, __LINE__);
				break;
			}
			std::getline(pkFile, buffer, '~');
			utils::Trim(buffer);
			std::string authorName = GetNameByUnique(author);
			std::string victimName = GetNameByUnique(victim, true);
			name_convert(authorName);
			name_convert(victimName);
			// если автора уже нет - сбросим УИД для верности
			if (authorName.empty())
				author = 0;
			// если жертвы уже нет, или жертва выбилась в БОГИ - не грузим
			if (!victimName.empty()) {
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
	if (stuffFile.is_open()) {
		int i;
		while (stuffFile >> i) {
			ClanStuffName temp;
			temp.num = i;

			std::getline(stuffFile, temp.name, '~');
			utils::Trim(temp.name);

			for (int j = 0; j < 6; j++) {
				std::getline(stuffFile, buffer, '~');
				utils::Trim(buffer);
				temp.PNames.push_back(buffer);
			}

			std::getline(stuffFile, temp.desc, '~');
			utils::Trim(temp.desc);
			std::getline(stuffFile, temp.longdesc, '~');
			utils::Trim(temp.longdesc);

			tempClan->clanstuff.push_back(temp);
		}
	}
	// лоад доп. параметров клана
	tempClan->load_mod();
	tempClan->pk_log.load(tempClan->get_file_abbrev());
	tempClan->last_exp.load(tempClan->get_file_abbrev());
	tempClan->init_ingr_chest();
	tempClan->chest_log.load(tempClan->get_file_abbrev());
	if ((tempClan->bank <= 0) && (tempClan->m_members.size() > 0) && !tempClan->test_clan) {
		Boards::Static::clan_delete_message(tempClan->abbrev, tempClan->rent / 100);
		char smallbuf[128];
		sprintf(smallbuf, "банк_пуст");
		DestroyClan(tempClan, smallbuf);
		log("Clan deleted bank 0: %s", filename.c_str());
	}
	Clan::ClanList.push_back(tempClan);
}

// лоад/релоад индекса и файлов кланов
void Clan::ClanLoad() {
	const bool reload = Clan::ClanList.empty() ? false : true;

	init_chest_rnum();
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
	for (const auto &it : clanIndex) {
		Clan::ClanLoadSingle(it);
	}

	Clan::ChestLoad();
	Clan::ChestUpdate();
	Clan::SaveChestAll();
	Clan::ClanSave();
	Boards::Static::ClanInit();
	save_ingr_chests();

	if (reload) {
		HelpSystem::reload(HelpSystem::DYNAMIC);
	}

	// на случай релоада кланов для выставления изменений игрокам онлайн
	// лдшникам воткнется в другом месте, можно и тут чар-лист прогнать, варианты одинаково корявые
	for (auto d = descriptor_list; d; d = d->next) {
		if (d->character) {
			Clan::SetClanData(d->character.get());
		}
	}
}

// вывод имму информации о кланах
void Clan::HconShow(CharData *ch) {
	std::ostringstream buffer;
	buffer
		<< "Abbrev|  Rent|OutRent| Chest|iChest|  Guard|CreateDate|      StoredExp|      Bank|Items| Ing |DayTax|Lvl|Test|Распущена\r\n";
	std::string_view
		show("{:<4}|{:<6}|{:<7}|{:<6}|{:<6}|{:<7}|{:<10}|{:<15}|{:<10}|{:<5}|{:<5}|{:<6}|{:<3}|{:<4}|{:<9}\r\n");
	int total_day_tax = 0;

	for (auto &clan : Clan::ClanList) {
		char timeBuf[17];
		strftime(timeBuf, sizeof(timeBuf), "%d-%m-%Y", localtime(&(clan->builtOn)));

		int cost = clan->ChestTax() + clan->ingr_chest_tax();
		cost += clan->calculate_clan_tax();
		total_day_tax += cost;

		buffer << fmt::format(fmt::runtime(show), clan->abbrev, clan->rent, clan->out_rent, clan->chest_room,
							  GET_ROOM_VNUM(clan->get_ingr_chest_room_rnum()), clan->guard, timeBuf,
							  clan->clan_exp, clan->bank, clan->chest_objcount,
							  clan->ingr_chest_objcount_, cost, clan->clan_level,
							  (clan->test_clan ? "y" : "n"), ((clan->m_members.size() > 0) ? "Нет" : "Да"));
	}

	buffer << "Total day tax: " << total_day_tax << "\r\n";
	SendMsgToChar(buffer.str().c_str(), ch);
}

void Clan::save_clan_file(const std::string &filename) const {
	std::ofstream file(filename.c_str());
	if (!file.is_open()) {
		log("Error open file: %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
		return;
	}

	file << "Abbrev: " << abbrev << "\n"
		 << "Name: " << name << "\n"
		 << "Title: " << title << "\n"
		 << "TitleFemale: " << title_female << "\n"
		 << "Rent: " << rent << "\n"
		 << "OutRent: " << out_rent << "\n"
		 << "ChestRoom: " << chest_room << "\n"
		 << "IngrChestRoom: " << GET_ROOM_VNUM(get_ingr_chest_room_rnum()) << "\n"
		 << "Rep: " << reputation << "\n"
		 << "Guard: " << guard << "\n"
		 << "BuiltOn: " << builtOn << "\n"
		 << "EntranceMode: " << entranceMode << "\n"
		 << "Storehouse: " << storehouse << "\n"
		 << "ExpInfo: " << exp_info << "\n"
		 << "TestClan: " << test_clan << "\n"
		 << "Exp: " << exp << "\n"
		 << "ExpBuf: " << exp_buf << "\n"
		 << "StoredExp: " << clan_exp << "\n"
		 << "ClanLevel: " << clan_level << "\n"
		 << "Bank: " << bank << "\n"
		 << "GoldTax: " << gold_tax_pct_ << "\n"
		 << "Pk: " << pk << "\n";

	if (!web_url_.empty()) {
		file << "Site: " << web_url_ << "\n";
	}

	file << "Politics:\n";
	for (auto politic : politics) {
		file << " " << politic.first << " " << politic.second << "\n";
	}

	file << "~\n";

	file << "Ranks:\n";
	int i = 0;
	for (auto it = ranks.begin(); it != ranks.end(); ++it, ++i)
		file << " " << (*it) << " " << ranks_female[i] << " " << privileges[i].to_ulong() << "\n";
	file << "~\n";

	file << "Owner: ";
	for (const auto &it : m_members) {
		if (it.second->rank_num == 0) {
			file << it.first << " " << it.second->money
				 << " " << it.second->exp << " " << 0
				 << " " << it.second->clan_exp << "\n";
			break;
		}
	}

	file << "Members:\n";
	for (const auto &it : m_members) {
		if (it.second->rank_num != 0) {
			file << " " << it.first << " " << it.second->rank_num << " "
				 << it.second->money << " " << it.second->exp << " " << 0
				 << " " << it.second->clan_exp << "\n";
		}
	}
	file << "~\n";
	file.close();
}

// сохранение кланов в файлы
void Clan::ClanSave() {
	std::ofstream index(LIB_HOUSE "index");
	if (!index.is_open()) {
		log("Error open file: %s! (%s %s %d)", LIB_HOUSE "index", __FILE__, __func__, __LINE__);
		return;
	}

	for (auto &clan : Clan::ClanList) {
		// именем файла для клана служит его аббревиатура (английский и нижний регистр)
		std::string buffer = clan->abbrev;
		CreateFileName(buffer);
		index << buffer << "\n";

		std::string filepath = LIB_HOUSE + buffer + "/" + buffer;
		std::string path = LIB_HOUSE + buffer;
		struct stat result;
		if (stat(filepath.c_str(), &result)) {
			if (mkdir(path.c_str(), 0700)) {
				log("Can't create dir: %s! (%s %s %d)", path.c_str(), __FILE__, __func__, __LINE__);
				return;
			}
		}
		// основной файл клана
		clan->save_clan_file(filepath);
		// пкл/дрл
		std::ofstream pkFile((filepath + ".pkl").c_str());
		if (!pkFile.is_open()) {
			log("Error open file: %s! (%s %s %d)", (filepath + ".pkl").c_str(), __FILE__, __func__, __LINE__);
			return;
		}
		for (auto it = clan->pkList.begin(); it != clan->pkList.end(); ++it)
			pkFile << it->second->author << " " << it->first << " " << (*it).
				second->time << " " << "0\n" << it->second->text << "\n~\n";
		for (auto it = clan->frList.begin(); it != clan->frList.end(); ++it)
			pkFile << it->second->author << " " << it->first << " " << (*it).
				second->time << " " << "1\n" << it->second->text << "\n~\n";
		pkFile.close();
	}
	index.close();
}

/**
* проставляем персонажу нужные поля если он клановый
* отсюда и далее для проверки на клановость пользуем CLAN()
* может быть вызвана с пустым дескриптором персонажа (stat file)
*/
void Clan::SetClanData(CharData *ch) {
	CLAN(ch).reset();
	CLAN_MEMBER(ch).reset();
	// если правит свою дружину в олц, то радостно его выкидываем
	if (ch->desc && ch->desc->state == EConState::kClanedit) {
		ch->desc->clan_olc.reset();
		ch->desc->state = EConState::kPlaying;
		SendMsgToChar("Редактирование отменено из-за обновления ваших данных в дружине.\r\n", ch);
	}

	// если куда-то приписан, то дергаем сразу итераторы на клан и список членов
	for (auto &clan : Clan::ClanList) {
		const auto member = clan->m_members.find(ch->get_uid());
		if (member != clan->m_members.end()) {
			CLAN(ch) = clan;
			CLAN_MEMBER(ch) = member->second;
			break;
		}
	}
	// никуда не приписан
	if (!CLAN(ch)) {
		free(GET_CLAN_STATUS(ch));
		GET_CLAN_STATUS(ch) = nullptr;
		return;
	}
	// куда-то таки приписан
	std::string buffer;
	if (IS_MALE(ch))
		buffer = CLAN(ch)->ranks[CLAN_MEMBER(ch)->rank_num] + " " + CLAN(ch)->title;
	else
		buffer = CLAN(ch)->ranks_female[CLAN_MEMBER(ch)->rank_num] + " " + CLAN(ch)->title_female;
	GET_CLAN_STATUS(ch) = str_dup(buffer.c_str());

	// чтобы при выходе не смог приписаться опять за один ребут мада
	if (ch->desc) {
		ch->desc->clan_invite.reset();
	}
}

// проверка комнаты на принадлежность какому-либо замку
Clan::shared_ptr Clan::GetClanByRoom(RoomRnum room) {
	for (const auto &clan : ClanList) {
		if (world[room]->zone_rn == world[GetRoomRnum(clan->rent)]->zone_rn) {
			return clan;
		}
	}

	return nullptr;
}

// может ли персонаж зайти в замок
bool Clan::MayEnter(CharData *ch, RoomRnum room, bool mode) {
	const auto clan = GetClanByRoom(room);
	if (!clan
		|| IS_GRGOD(ch)
		|| !ROOM_FLAGGED(room, ERoomFlag::kHouse)
		|| clan->entranceMode
		|| ch->IsFlagged(EPrf::kCoderinfo)) {
		return true;
	}

	if (!CLAN(ch)) {
		return false;
	}

	bool isMember = false;

	if (CLAN(ch) == clan || clan->CheckPolitics(CLAN(ch)->GetRent()) == kPoliticsAlliance) {
		isMember = true;
	}

	int _mode = mode ? kHousePortal : kHouseAtrium;
	switch (_mode) {
		// вход через дверь - контролирует охранник
		case kHouseAtrium:
			for (const auto mobs : world[ch->in_room]->people) {
				if (clan->guard == GET_MOB_VNUM(mobs)
					&& !isMember) {
					return false;
				}
			}

			// охранника нет - свободный доступ
			return true;

			// телепортация
		case kHousePortal:
			if (!isMember) {
				SendMsgToChar("Частная собственность - посторонним в ней делать нечего!\r\n", ch);
				return false;
			}

			// с временным флагом тоже курят
			if (NORENTABLE(ch)) {
				if (mode == kHouseAtrium) {
					SendMsgToChar("Пускай сначала кровь с тебя стечет, а потом входи сколько угодно.\r\n", ch);
				}
				return false;
			}

			return true;
	}
	return false;
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
		"  клан сообщение (написание сообщения дружины)\r\n",
		"  клан налог <процент отдаваемых кун>\r\n",
		"  дрвече писать\r\n"
	};

// репутация
int Clan::get_rep() {
	return this->reputation;
}

void Clan::set_rep(int rep) {
	this->reputation = rep;
}

// house информация
void Clan::HouseInfo(CharData *ch) {
	// думаю, вываливать список сортированный по уиду некрасиво, поэтому перебираем по рангам
	std::vector<ClanMember::shared_ptr> temp_list;
	for (const auto &it : m_members) {
		temp_list.push_back(it.second);
	}

	std::sort(temp_list.begin(), temp_list.end(),
			  [](const ClanMember::shared_ptr &lrs, const ClanMember::shared_ptr &rhs) {
				return lrs->rank_num < rhs->rank_num;
			  });

	std::ostringstream buffer;
	buffer << "К замку приписаны:\r\n";

	size_t char_num = 0;
	std::string temp;
	for (const auto &it : temp_list) {
		if (temp != ranks[it->rank_num]) {
			std::string rnk = ranks[it->rank_num];
			rnk[0] = UPPER(rnk[0]);

			if (temp == "") {
				buffer << rnk << ": ";
			} else {
				buffer << "\r\n" << rnk << ": ";
			}

			temp = ranks[it->rank_num];
			char_num = 0;
		}

		if (char_num >= 80) {
			buffer << "\r\n";
			char_num = 0;
		}

		buffer << it->name << " ";
		char_num += it->name.size() + 1;
	}

	buffer << "\r\nПривилегии:\r\n";
	int num = 0;

	for (auto it = ranks.begin(); it != ranks.end(); ++it, ++num) {
		buffer << "  " << (*it) << ":";
		for (unsigned i = 0; i < CLAN_PRIVILEGES_NUM; ++i) {
			if (this->privileges[num][i]) {
				switch (i) {
					case MAY_CLAN_INFO: buffer << " инфо";
						break;
					case MAY_CLAN_ADD: buffer << " принять";
						break;
					case MAY_CLAN_REMOVE: buffer << " изгнать";
						break;
					case MAY_CLAN_PRIVILEGES: buffer << " привилегии";
						break;
					case MAY_CLAN_CHANNEL: buffer << " гд";
						break;
					case MAY_CLAN_POLITICS: buffer << " политика";
						break;
					case MAY_CLAN_NEWS: buffer << " дрн";
						break;
					case MAY_CLAN_PKLIST: buffer << " пкл";
						break;
					case MAY_CLAN_CHEST_PUT: buffer << " хран.полож";
						break;
					case MAY_CLAN_CHEST_TAKE: buffer << " хран.взять";
						break;
					case MAY_CLAN_BANK: buffer << " казна";
						break;
					case MAY_CLAN_EXIT: buffer << " выход";
						break;
					case MAY_CLAN_MOD: buffer << " сообщение дружины";
						break;
					case MAY_CLAN_TAX: buffer << " налог";
						break;
					case MAY_CLAN_BOARD: buffer << " дрвече";
						break;
				}
			}
		}
		buffer << "\r\n";
	}
	//инфа о экспе замка левеле замка, рейтинге замка и плюшках
	buffer << "Ваш замок набрал " << this->clan_exp
		   << " очков опыта и имеет уровень " << this->clan_level << "\r\n"
		   << "Рейтинг вашего замка: " << this->exp
		   << " Это очень круто :), но ничего вам не дает.\r\n"
		   << "Ваша дружина имеет " << this->get_rep() << " очков репутации.\r\n"
		   << "В хранилище замка может храниться до " << this->ChestMaxObjects()
		   << " " << GetDeclensionInNumber(this->ChestMaxObjects(), EWhat::kObjU)
		   << " с общим весом не более чем " << this->ChestMaxWeight() << "\r\n"
		   << "В хранилище ингредиентов может храниться до " << this->ingr_chest_max_objects()
		   << " " << GetDeclensionInNumber(this->ingr_chest_max_objects(), EWhat::kObjU)
		   << ".\r\n";

	// инфа о банке и хранилище
	int cost = ChestTax();
	int ingr_cost = ingr_chest_tax();
	int options_tax = calculate_clan_tax();
	int total_tax = cost + ingr_cost + options_tax;

	buffer << "В хранилище вашей дружины " << this->chest_objcount << " "
		   << GetDeclensionInNumber(this->chest_objcount, EWhat::kObject)
		   << " общим весом в " << this->chest_weight
		   << " (" << cost << " " << GetDeclensionInNumber(cost, EWhat::kMoneyA) << " в день).\r\n"
		   << "В хранилище ингредиентов " << ingr_chest_objcount_ << " "
		   << GetDeclensionInNumber(ingr_chest_objcount_, EWhat::kObject)
		   << " (" << ingr_cost << " " << GetDeclensionInNumber(ingr_cost, EWhat::kMoneyA) << " в день).\r\n\r\n"
		   << "Состояние казны: " << this->bank << " "
		   << GetDeclensionInNumber(this->bank, EWhat::kMoneyA) << ".\r\n"
		   << "Расходы на инфраструктуру замка: " << options_tax << " "
		   << GetDeclensionInNumber(options_tax, EWhat::kMoneyA)
		   << " в день, Общие расходы: " << total_tax << " "
		   << GetDeclensionInNumber(total_tax, EWhat::kMoneyA) << " в день.\r\n";

	if (total_tax <= 0) {
		buffer << "Ваших денег хватит на нереальное количество дней.\r\n";
	} else {
		buffer << "Ваших денег хватит примерно на "
			   << bank / total_tax << " "
			   << GetDeclensionInNumber(bank / total_tax, EWhat::kDay) << ".\r\n";
	}
	buffer << "Налог для ратников дружины: " << get_gold_tax_pct() << "%\r\n";

	SendMsgToChar(buffer.str(), ch);
	exp_history.show(ch);
}

// клан принять, повлиять можно только на соклановцев ниже рангом
void Clan::HouseAdd(CharData *ch, std::string &buffer) {
	std::string buffer2;
	GetOneParam(buffer, buffer2);
	if (buffer2.empty()) {
		SendMsgToChar("Укажите имя персонажа.\r\n", ch);
		return;
	}

	long unique = GetUniqueByName(buffer2);
	if (!unique) {
		SendMsgToChar("Неизвестный персонаж.\r\n", ch);
		return;
	}
	std::string name = buffer2;
	name[0] = UPPER(name[0]);
	if (unique == ch->get_uid()) {
		SendMsgToChar("Сам себя повысил, самому себе вынес благодарность?\r\n", ch);
		return;
	}

	// изменение звания у членов дружины
	// даже если они находятся оффлайн
	const auto it_member = this->m_members.find(unique);
	if (it_member != this->m_members.end()) {
		if (it_member->second->rank_num <= CLAN_MEMBER(ch)->rank_num) {
			SendMsgToChar("Вы можете менять звания только у нижестоящих членов дружины.\r\n", ch);
			return;
		}

		int rank = CLAN_MEMBER(ch)->rank_num;
		if (!rank) ++rank;

		GetOneParam(buffer, buffer2);
		if (buffer2.empty()) {
			buffer = "Укажите звание персонажа.\r\nДоступные положения: ";
			for (auto it = this->ranks.begin() + rank; it != this->ranks.end(); ++it)
				buffer += "'" + *it + "' ";
			buffer += "\r\n";
			SendMsgToChar(buffer, ch);
			return;
		}

		int temp_rank = rank;
		for (auto it = this->ranks.begin() + rank; it != this->ranks.end(); ++it, ++temp_rank) {
			if (CompareParam(buffer2, *it)) {
				CharData::shared_ptr editedChar;
				DescriptorData *d = DescriptorByUid(unique);
				m_members.set_rank(unique, temp_rank);
				if (d) {
					editedChar = d->character;
					Clan::SetClanData(d->character.get());
					SendMsgToChar(d->character.get(),
								  "%sВаше звание изменили, теперь вы %s.%s\r\n",
								  kColorWht,
								  (*it).c_str(),
								  kColorNrm);
					sprintf(buf, "Звание в дружине изменено на %s", (*it).c_str());
					AddKarma(d->character.get(), buf, ch->get_name().c_str());
				}

				// оповещение соклановцев о изменении звания
				for (DescriptorData *d = descriptor_list; d; d = d->next) {
					if (d->character
						&& CLAN(d->character)
						&& CLAN(d->character)->GetRent() == this->GetRent()
						&& editedChar != d->character) {
						SendMsgToChar(d->character.get(), "%s%s теперь %s.%s\r\n",
									  kColorWht, it_member->second->name.c_str(), (*it).c_str(), kColorNrm);
					}
				}

				return;
			}
		}

		buffer = "Неверное звание, доступные положения:\r\n";
		for (std::vector<std::string>::const_iterator it = this->ranks.begin() + rank; it != this->ranks.end(); ++it) {
			buffer += "'" + *it + "' ";
		}
		buffer += "\r\n";
		SendMsgToChar(buffer, ch);

		return;
	}

	DescriptorData *d = DescriptorByUid(unique);
	if (!d || !CAN_SEE(ch, d->character)) {
		SendMsgToChar("Этого персонажа нет в игре!\r\n", ch);
		return;
	}

	if (d->character->IsFlagged(EPrf::kCoderinfo) || (GetRealLevel(d->character) >= kLvlGod)) {
		SendMsgToChar("Вы не можете приписать этого игрока.\r\n", ch);
		return;
	}

	if (CLAN(d->character) && CLAN(ch) != CLAN(d->character)) {
		SendMsgToChar("Вы не можете приписать члена другой дружины.\r\n", ch);
		return;
	}

	if (d->clan_invite) {
		if (d->clan_invite->clan == CLAN(ch)) {
			SendMsgToChar("Вы уже пригласили этого игрока, ждите реакции.\r\n", ch);
			return;
		} else {
			SendMsgToChar("Он уже приглашен в другую дружину, дождитесь его ответа и пригласите снова.\r\n", ch);
			return;
		}
	}

	if (GET_KIN(d->character) != GET_KIN(ch)) {
		SendMsgToChar("Вы не можете сражаться в одном строю с иноплеменником.\r\n", ch);
		return;
	}

	GetOneParam(buffer, buffer2);

	// чтобы учесть воеводу с 0 рангом во время приписки и не дать ему приписать еще 10 воевод
	int rank = CLAN_MEMBER(ch)->rank_num;
	if (!rank) {
		++rank;
	}

	if (buffer2.empty()) {
		buffer = "Укажите звание персонажа.\r\nДоступные положения: ";
		for (auto it = this->ranks.begin() + rank; it != this->ranks.end(); ++it)
			buffer += "'" + *it + "' ";
		buffer += "\r\n";
		SendMsgToChar(buffer, ch);
		return;
	}

	int temp_rank = rank; // дальше rank тоже мб использован в случае неверного выбора
	for (auto it = this->ranks.begin() + rank; it != this->ranks.end(); ++it, ++temp_rank) {
		if (CompareParam(buffer2, *it)) {
			// не приписан - втыкаем ему приглашение и курим, пока не согласится
			std::shared_ptr<struct ClanInvite> temp_invite(new ClanInvite);
			temp_invite->clan = CLAN(ch);
			temp_invite->rank = temp_rank;
			temp_invite->invite_name = "Игроком " + ch->get_name();
			d->clan_invite = temp_invite;
			buffer = kColorWht;
			buffer += "$N приглашен$G в вашу дружину, статус - " + *it + ".";
			buffer += kColorNrm;
			// оповещаем счастливца
			act(buffer.c_str(), false, ch, nullptr, d->character.get(), kToChar);
			buffer = kColorWht;
			buffer += "Вы получили приглашение в дружину '" + this->name + "', статус - " + *it + ".\r\n"
				+ "Чтобы принять приглашение наберите 'клан согласен', для отказа 'клан отказать'.\r\n"
				+ "Никто не сможет послать вам новое приглашение до тех пор, пока вы не разберетесь с этим.\r\n"
				+ "Настоятельно рекомендуем ознакомиться с уставом дружины и разделом справки ПРАВИЛАДРУЖИНЫ.\r\n";

			buffer += kColorNrm;
			SendMsgToChar(buffer, d->character.get());
			return;
		}
	}

	buffer = "Неверное звание, доступные положения:\r\n";
	for (auto it = this->ranks.begin() + rank; it != this->ranks.end(); ++it) {
		buffer += "'" + *it + "' ";
	}
	buffer += "\r\n";

	SendMsgToChar(buffer, ch);
}

/**
* Отписывание персонажа от клана с оповещением всех заинтересованных сторон,
* выдворением за пределы замка и изменением ренты при необходимости.
*/
void Clan::remove_member(const ClanMembersList::key_type &key, char *reason) {
	const auto it = m_members.find(key);
	std::string name = it->second->name;
	long unique = it->first;
	m_members.erase(it);

	DescriptorData *k = DescriptorByUid(unique);
	if (k && k->character) {
		Clan::SetClanData(k->character.get());
		SendMsgToChar(k->character.get(), "Вас исключили из дружины '%s'!\r\n", this->name.c_str());
		sprintf(buf, "Исключен(а) из дружины '%s'", this->name.c_str());
		AddKarma(k->character.get(), buf, reason);
		const auto clan = Clan::GetClanByRoom(k->character->in_room);
		if (clan) {
			char_from_room(k->character);
			act("$n был$g выдворен$a за пределы замка!", true, k->character.get(), nullptr, nullptr, kToRoom);
			SendMsgToChar("Вы были выдворены за пределы замка!\r\n", k->character.get());
			PlaceCharToRoom(k->character.get(), GetRoomRnum(clan->out_rent));
			look_at_room(k->character.get(), GetRoomRnum(clan->out_rent));
			act("$n свалил$u с небес, выкрикивая какие-то ругательства!",
				true,
				k->character.get(),
				nullptr,
				nullptr,
				kToRoom);
		}
	} else {
		Player p_vict;
		CharData *vict = &p_vict;
		if (LoadPlayerCharacter(name.c_str(), vict, ELoadCharFlags::kFindId) > -1) {
			sprintf(buf, "Исключен(а) из дружины '%s'", this->name.c_str());
			AddKarma(vict, buf, reason);
			vict->save_char();
		}
	}
	for (DescriptorData *d = descriptor_list; d; d = d->next) {
		if (d->character
			&& CLAN(d->character)
			&& CLAN(d->character)->GetRent() == this->GetRent()) {
			name[0] = UPPER(name[0]);
			SendMsgToChar(d->character.get(), "%s более не является членом вашей дружины.\r\n", name.c_str());
		}
	}
}

// house изгнать (только званием ниже своего)
void Clan::HouseRemove(CharData *ch, std::string &buffer) {
	std::string buffer2;
	GetOneParam(buffer, buffer2);
	long unique = GetUniqueByName(buffer2);
	const auto it = this->m_members.find(unique);

	if (buffer2.empty()) {
		SendMsgToChar("Укажите имя персонажа.\r\n", ch);
	} else if (!unique) {
		SendMsgToChar("Неизвестный персонаж.\r\n", ch);
	} else if (unique == ch->get_uid()) {
		SendMsgToChar("Выглядит довольно странно...\r\n", ch);
	}

	if (it == this->m_members.end()) {
		SendMsgToChar("Он и так не приписан к вашей дружине.\r\n", ch);
	} else if (it->second->rank_num <= CLAN_MEMBER(ch)->rank_num) {
		SendMsgToChar("Вы можете исключить из дружины только персонажа со званием ниже вашего.\r\n", ch);
	} else {
		char tmpstr[kMaxInputLength];
		sprintf(tmpstr, "%s", GET_NAME(ch));
		remove_member(it->first, tmpstr);
	}
}

void Clan::HouseLeave(CharData *ch) {
	if (!CLAN_MEMBER(ch)->rank_num) {
		SendMsgToChar("Если вы хотите распустить свою дружину, то обращайтесь к Богам.\r\n"
					  "А если вам просто нечем заняться, то передайте воеводство и идите куда хотите...\r\n", ch);
		return;
	}

	const auto member_id = ch->get_uid();
	const auto it = this->m_members.find(member_id);
	if (it != this->m_members.end()) {
		char tmpstr[kMaxInputLength];
		sprintf(tmpstr, "самовыход");
		remove_member(member_id, tmpstr);
	}
}

// удаляет объект из хранилищ клана
int Clan::delete_obj(int vnum) {
	int num = 0;
	for (auto &clan : Clan::ClanList) {
		ObjData *temp, *chest;
		for (chest = world[GetRoomRnum(clan->chest_room)]->contents; chest; chest = chest->get_next_content()) {
			if (Clan::is_clan_chest(chest)) {
				for (temp = chest->get_contains(); temp; temp = temp->get_next_content()) {
					if (GET_OBJ_VNUM(temp) == vnum) {
						temp->set_timer(0);
						num++;
					}
				}
			}
		}
	}
	return num;
}

// * hcontrol outcast имя - отписывание любого персонажа от дружины, кроме воеводы.
void Clan::hcon_outcast(CharData *ch, std::string &buffer) {
	std::string name;
	GetOneParam(buffer, name);
	long member_uid = GetUniqueByName(name);
	if (!member_uid) {
		SendMsgToChar("Неизвестный персонаж.\r\n", ch);
		return;
	}
	for (const auto &clan : Clan::ClanList) {
		const auto it = clan->m_members.find(member_uid);
		if (it != clan->m_members.end()) {
			if (!it->second->rank_num) {
				SendMsgToChar(ch,
							  "Вы не можете исключить воеводу, для удаления дружины существует hcontrol destroy.\r\n");
				return;
			}
			char tmpstr[kMaxInputLength];
			sprintf(tmpstr, "Богом %s", GET_NAME(ch));
			clan->remove_member(member_uid, tmpstr);
			name[0] = UPPER(name[0]);
			SendMsgToChar(ch, "%s исключен(a) из дружины '%s'.\r\n", name.c_str(), clan->name.c_str());
			return;
		}
	}
	SendMsgToChar("Он и так не состоит ни в какой дружине.\r\n", ch);
}

// бог, текст, клан/альянс
void Clan::GodToChannel(CharData *ch, std::string text, int subcmd) {
	utils::Trim(text);
	// на счет скобок я хз, по-моему так нагляднее все же в ифах, где условий штук 5, мож опять индентом пройтись? Ж)
	if (text.empty()) {
		SendMsgToChar("Что вы хотите им сообщить?\r\n", ch);
		return;
	}
	switch (subcmd) {
		// большой БОГ говорит какой-то дружине
		case kScmdChannel:
			for (DescriptorData *d = descriptor_list; d; d = d->next) {
				if (d->character
					&& CLAN(d->character)
					&& ch != d->character.get()
					&& d->state == EConState::kPlaying
					&& CLAN(d->character).get() == this
					&& !AFF_FLAGGED(d->character, EAffect::kDeafness)) {
					SendMsgToChar(d->character.get(),
								  "%s ВАШЕЙ дружине: %s'%s'%s\r\n",
								  GET_NAME(ch),
								  kColorBoldRed,
								  text.c_str(),
								  kColorNrm);
				}
			}
			SendMsgToChar(ch, "Вы дружине %s: %s'%s'.%s\r\n", this->abbrev.c_str(), kColorBoldRed, text.c_str(), kColorNrm);
			break;

			// он же в канал союзников этой дружины, если они вообще есть
		case kScmdAchannel:
			for (DescriptorData *d = descriptor_list; d; d = d->next) {
				if (d->character
					&& CLAN(d->character)
					&& d->state == EConState::kPlaying
					&& !AFF_FLAGGED(d->character, EAffect::kDeafness)
					&& d->character.get() != ch) {
					if (CheckPolitics(CLAN(d->character)->GetRent()) == kPoliticsAlliance
						|| CLAN(d->character).get() == this) {
						// проверка на альянс с обеих сторон, иначе это не альянс
						if (CLAN(d->character).get() != this) {
							if (CLAN(d->character)->CheckPolitics(this->rent) == kPoliticsAlliance) {
								SendMsgToChar(d->character.get(),
											  "%s ВАШИМ СОЮЗНИКАМ: %s'%s'%s\r\n",
											  GET_NAME(ch),
											  kColorBoldGrn,
											  text.c_str(),
											  kColorNrm);
							}
						}
							// первоначальному клану выдается всегда
						else {
							SendMsgToChar(d->character.get(),
										  "%s ВАШИМ СОЮЗНИКАМ: %s'%s'%s\r\n", GET_NAME(ch), kColorBoldGrn, text.c_str(), kColorNrm);
						}
					}
				}
			}
			SendMsgToChar(ch, "Вы союзникам %s: %s'%s'.%s\r\n", abbrev.c_str(), kColorBoldGrn, text.c_str(), kColorNrm);
			break;
	}
}

// чар (или клановый бог), текст, клан/альянс
void Clan::CharToChannel(CharData *ch, std::string text, int subcmd) {
	utils::Trim(text);
	if (text.empty()) {
		SendMsgToChar("Что вы хотите сообщить?\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffect::kSilence)) {
		SendMsgToChar(SIELENCE, ch);
		return;
	}

	if (ch->IsFlagged(EPlrFlag::kDumbed)) {
		SendMsgToChar("Вам запрещено обращаться к другим игрокам!\r\n", ch);
		return;
	}

	switch (subcmd) {
		// своей дружине
		case kScmdChannel:
			// вспомнить
			snprintf(buf, kMaxStringLength, "%s дружине: &R'%s'.&n\r\n", GET_NAME(ch), text.c_str());
			CLAN(ch)->add_remember(buf, Remember::CLAN);

			for (auto d = descriptor_list; d; d = d->next) {
				if (d->character
					&& d->character.get() != ch
					&& d->state == EConState::kPlaying
					&& CLAN(d->character) == CLAN(ch)
					&& !AFF_FLAGGED(d->character, EAffect::kDeafness)
					&& !ignores(d->character.get(), ch, EIgnore::kClan)) {
					snprintf(buf,
							 kMaxStringLength,
							 "%s дружине: %s'%s'.%s\r\n",
							 GET_NAME(ch),
							 kColorBoldRed,
							 text.c_str(),
							 kColorNrm);
					d->character->remember_add(buf, Remember::ALL);
					SendMsgToChar(buf, d->character.get());
				}
			}

			snprintf(buf,
					 kMaxStringLength,
					 "Вы дружине: %s'%s'.%s\r\n",
					 kColorBoldRed,
					 text.c_str(),
					 kColorNrm);
			ch->remember_add(buf, Remember::ALL);
			SendMsgToChar(buf, ch);

			break;

			// союзникам
		case kScmdAchannel:
			// вспомнить
			snprintf(buf, kMaxStringLength, "%s союзникам: &G'%s'.&n\r\n", GET_NAME(ch), text.c_str());
			for (auto &clan : Clan::ClanList) {
				if ((CLAN(ch)->CheckPolitics(clan->GetRent()) == kPoliticsAlliance
					&& clan->CheckPolitics(CLAN(ch)->GetRent()) == kPoliticsAlliance)
					|| CLAN(ch) == clan) {
					clan->add_remember(buf, Remember::ALLY);
				}
			}

			for (auto d = descriptor_list; d; d = d->next) {
				if (d->character
					&& CLAN(d->character)
					&& d->state == EConState::kPlaying
					&& d->character.get() != ch
					&& !AFF_FLAGGED(d->character, EAffect::kDeafness)
					&& !ignores(d->character.get(), ch, EIgnore::kAlliance)) {
					if (CLAN(ch)->CheckPolitics(CLAN(d->character)->GetRent()) == kPoliticsAlliance
						|| CLAN(ch) == CLAN(d->character)) {
						// проверка на альянс с обеих сторон, шоб не спамили друг другу на зло
						if ((CLAN(d->character)->CheckPolitics(CLAN(ch)->GetRent()) == kPoliticsAlliance)
							|| CLAN(ch) == CLAN(d->character)) {
							snprintf(buf,
									 kMaxStringLength,
									 "%s союзникам: %s'%s'.%s\r\n",
									 GET_NAME(ch),
									 kColorBoldGrn,
									 text.c_str(),
									 kColorNrm);
							d->character->remember_add(buf, Remember::ALL);
							SendMsgToChar(buf, d->character.get());
						}
					}
				}
			}

			snprintf(buf,
					 kMaxStringLength,
					 "Вы союзникам: %s'%s'.%s\r\n",
					 kColorBoldGrn,
					 text.c_str(),
					 kColorNrm);
			ch->remember_add(buf, Remember::ALL);
			SendMsgToChar(buf, ch);

			break;
	} // switch
}

// возвращает состояние политики clan по отношению к victim
// при отсутствии подразумевается нейтралитет
int Clan::CheckPolitics(int victim) {
	auto it = politics.find(victim);
	if (it != politics.end()) {
		return it->second;
	}
	return kPoliticsNeutral;
}

// выставляем клану политику(state) по отношению к victim
// нейтралитет означает просто удаление записи, если она была
void Clan::SetPolitics(int victim, int state) {
	auto it = politics.find(victim);
	if (it == politics.end() && state == kPoliticsNeutral)
		return;
	if (state == kPoliticsNeutral)
		politics.erase(it);
	else
		politics[victim] = state;
}

const char *politicsnames[] = {"Нейтралитет", "Война", "Альянс"};

// показывает, может ли чар писать в дрв
bool Clan::check_write_board(CharData *ch) {
	if (this->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_BOARD]) {
		return true;
	}
	return false;
}

bool Clan::is_pk() {
	return this->pk ? true : false;
}

void Clan::change_pk_status() {
	this->pk = this->pk ? false : true;
}

void Clan::SetPk(CharData *ch, std::string buffer) {
	const int vnum = atoi(buffer.c_str());

	auto clan = std::find_if(ClanList.begin(), ClanList.end(),
							 [&](const Clan::shared_ptr &clan) {
							   return clan->rent == vnum;
							 });

	if (clan == Clan::ClanList.end()) {
		SendMsgToChar(ch, "Дружины с номером %d не существует.\r\n", vnum);
		return;
	}

	clan->get()->change_pk_status();
	SendMsgToChar(ch, "Статус дружины изменен.\r\n");
}

bool char_to_pk_clan(CharData *ch) {
	if (CLAN(ch) && CLAN(ch)->is_pk())
		return true;
	return false;
}

// выставляем политику с уведомлением другой дружины о войне или альянсе
void Clan::ManagePolitics(CharData *ch, std::string &buffer) {
	std::string buffer2;
	GetOneParam(buffer, buffer2);
	DescriptorData *d;

	ClanListType::const_iterator vict;

	for (vict = Clan::ClanList.begin(); vict != Clan::ClanList.end(); ++vict)
		if (CompareParam(buffer2, (*vict)->abbrev))
			break;
	if ((vict == Clan::ClanList.end()) || ((*vict)->m_members.size() == 0)) {
		SendMsgToChar("Нет такой дружины.\r\n", ch);
		return;
	}
	if (*vict == CLAN(ch)) {
		SendMsgToChar("Менять политику по отношению к своей дружине? Что за бред...\r\n", ch);
		return;
	}
	GetOneParam(buffer, buffer2);
	if (buffer2.empty())
		SendMsgToChar("Укажите действие: нейтралитет|война|альянс.\r\n", ch);
	else if (CompareParam(buffer2, "нейтралитет")) {
		if (CheckPolitics((*vict)->rent) == kPoliticsNeutral) {
			SendMsgToChar(ch,
						  "Ваша дружина уже находится в состоянии нейтралитета с дружиной %s.\r\n",
						  (*vict)->abbrev.c_str());
			return;
		}
		SetPolitics((*vict)->rent, kPoliticsNeutral);
		SetWait(ch, 1, false);
		// уведомляем обе дружины
		for (d = descriptor_list; d; d = d->next) {
			if (d->character
				&& d->state == EConState::kPlaying
				&& d->character->IsFlagged(EPrf::kPolitMode)) {
				if (CLAN(d->character) == *vict) {
					SendMsgToChar(d->character.get(), "%sДружина %s заключила с вашей дружиной нейтралитет!%s\r\n",
								  kColorWht, this->abbrev.c_str(), kColorNrm);
				} else if (CLAN(d->character) == CLAN(ch)) {
					SendMsgToChar(d->character.get(), "%sВаша дружина заключила с дружиной %s нейтралитет!%s\r\n",
								  kColorWht, (*vict)->abbrev.c_str(), kColorNrm);
				}
			}
		}

		if (!ch->IsFlagged(EPrf::kPolitMode)) // а то сам может не увидеть нафик
		{
			SendMsgToChar(ch, "%sВаша дружина заключила с дружиной %s нейтралитет!%s\r\n",
						  kColorWht, (*vict)->abbrev.c_str(), kColorNrm);
		}
	} else if (CompareParam(buffer2, "война")) {
		if (CheckPolitics((*vict)->rent) == kPoliticsWar) {
			SendMsgToChar(ch, "Ваша дружина уже воюет с дружиной %s.\r\n", (*vict)->abbrev.c_str());
			return;
		}

		SetPolitics((*vict)->rent, kPoliticsWar);
		SetWait(ch, 1, false);
		// тож самое

		for (d = descriptor_list; d; d = d->next) {
			if (d->character
				&& d->state == EConState::kPlaying
				&& d->character->IsFlagged(EPrf::kPolitMode)) {
				if (CLAN(d->character) == *vict) {
					SendMsgToChar(d->character.get(),
								  "%sДружина %s объявила вашей дружине войну!%s\r\n",
								  kColorBoldRed,
								  this->abbrev.c_str(),
								  kColorNrm);
				} else if (CLAN(d->character) == CLAN(ch)) {
					SendMsgToChar(d->character.get(),
								  "%sВаша дружина объявила дружине %s войну!%s\r\n",
								  kColorBoldRed,
								  (*vict)->abbrev.c_str(),
								  kColorNrm);
				}
			}
		}

		if (!ch->IsFlagged(EPrf::kPolitMode)) {
			SendMsgToChar(ch,
						  "%sВаша дружина объявила дружине %s войну!%s\r\n", kColorBoldRed, (*vict)->abbrev.c_str(), kColorNrm);
		}
	} else if (CompareParam(buffer2, "альянс")) {
		if (CheckPolitics((*vict)->rent) == kPoliticsAlliance) {
			SendMsgToChar(ch, "Ваша дружина уже в альянсе с дружиной %s.\r\n", (*vict)->abbrev.c_str());
			return;
		}

		SetWait(ch, 1, false);
		SetPolitics((*vict)->rent, kPoliticsAlliance);

		// тож самое
		for (d = descriptor_list; d; d = d->next) {
			if (d->character && d->state == EConState::kPlaying && d->character->IsFlagged(EPrf::kPolitMode)) {
				if (CLAN(d->character) == *vict) {
					SendMsgToChar(d->character.get(),
								  "%sДружина %s заключила с вашей дружиной альянс!%s\r\n",
								  kColorGrn,
								  this->abbrev.c_str(),
								  kColorNrm);
				} else if (CLAN(d->character) == CLAN(ch)) {
					SendMsgToChar(d->character.get(),
								  "%sВаша дружина заключила альянс с дружиной %s!%s\r\n",
								  kColorGrn,
								  (*vict)->abbrev.c_str(),
								  kColorNrm);
				}
			}
		}

		if (!ch->IsFlagged(EPrf::kPolitMode)) {
			SendMsgToChar(ch, "%sВаша дружина заключила альянс с дружиной %s!%s\r\n",
						  kColorGrn, (*vict)->abbrev.c_str(), kColorNrm);
		}
	}
}

const char *HCONTROL_FORMAT =
	"Формат: hcontrol build <rent vnum> <outrent vnum> <guard vnum> <leader name> <abbreviation> <clan name>\r\n"
	"        hcontrol show\r\n"
	"        hcontrol destroy <house vnum> - удалить дружину\r\n"
	"        hcontrol outcast <name> - исключить игрока из дружины\r\n"
	"        hcontrol save\r\n"
	"        hcontrol title <vnum ренты> <аббревиатура для муж рода> <аббревиатура для жен рода>\r\n"
	"        hcontrol rank <vnum ренты> <старое звание муж рода> <звание для муж рода> <звание для жен рода>\r\n"
	"        hcontrol owner <vnum ренты> <имя нового воеводы>\r\n"
	"        hcontrol ingr <vnum ренты> <vnum комнаты для сундука с ингредиентами>\r\n"
	"        hcontrol exphitory <число месяцев>\r\n"
	"        hcontrol pk <vnum ренты>\r\n";

// * hcontrol title - изменение аббревиатуры клана в титуле персонажа.
void Clan::hcontrol_title(CharData *ch, std::string &text) {
	std::string buffer;

	GetOneParam(text, buffer);
	int rent = atoi(buffer.c_str());
	ClanListType::iterator clan = std::find_if(ClanList.begin(), ClanList.end(),
											   [&](const Clan::shared_ptr clan) {
												 return clan->rent == rent;
											   });

	if (clan == Clan::ClanList.end()) {
		SendMsgToChar(ch, "Дружины с номером %d не существует.\r\n", rent);
		return;
	}

	std::string title_male, title_female;
	GetOneParam(text, title_male);
	GetOneParam(text, title_female);
	if (title_male.empty() || title_female.empty()) {
		SendMsgToChar(HCONTROL_FORMAT, ch);
		return;
	}

	(*clan)->title = title_male;
	(*clan)->title_female = title_female;

	Clan::ClanSave();
	for (DescriptorData *d = descriptor_list; d; d = d->next) {
		if (d->character
			&& CLAN(d->character)
			&& CLAN(d->character) == *clan) {
			Clan::SetClanData(d->character.get());
		}
	}

	SendMsgToChar("Сделано.\r\n", ch);
}

// * hcontrol rank - изменение кланового звания персонажа в титуле.
void Clan::hcontrol_rank(CharData *ch, std::string &text) {
	std::string buffer;

	GetOneParam(text, buffer);
	int rent = atoi(buffer.c_str());
	ClanListType::iterator clan = std::find_if(ClanList.begin(), ClanList.end(),
											   [&](const Clan::shared_ptr &clan) {
												 return clan->rent == rent;
											   });

	if (clan == Clan::ClanList.end()) {
		SendMsgToChar(ch, "Дружины с номером %d не существует.\r\n", rent);
		return;
	}

	std::string old_rank, rank_male, rank_female;
	GetOneParam(text, old_rank);
	GetOneParam(text, rank_male);
	GetOneParam(text, rank_female);
	if (old_rank.empty() || rank_male.empty() || rank_female.empty()) {
		SendMsgToChar(HCONTROL_FORMAT, ch);
		return;
	}
	if (rank_male.size() > MAX_RANK_LENGHT || rank_female.size() > MAX_RANK_LENGHT) {
		SendMsgToChar(ch, "Звание не должно быть длиннее %d символов.\r\n", MAX_RANK_LENGHT);
		return;
	}

	utils::ConvertToLow(old_rank);
	utils::ConvertToLow(rank_male);
	utils::ConvertToLow(rank_female);

	try {
		for (unsigned i = 0; i < (*clan)->ranks.size(); ++i) {
			if ((*clan)->ranks[i] == old_rank) {
				(*clan)->ranks[i] = rank_male;
				(*clan)->ranks_female[i] = rank_female;
			}
		}
	}
	catch (...) {
		SendMsgToChar(ch, "Ошибка в званиях дружины.\r\n");
	}

	Clan::ClanSave();
	for (DescriptorData *d = descriptor_list; d; d = d->next) {
		if (d->character
			&& CLAN(d->character)
			&& CLAN(d->character) == *clan) {
			Clan::SetClanData(d->character.get());
		}
	}
	SendMsgToChar("Сделано.\r\n", ch);
}

/**
* Распечатка списка кланов с историей экспы за указанное кол-во месяцев.
* \param text - число последних месяцев, если пустая строка - 0 (только текущий месяц).
*/
void Clan::hcontrol_exphistory(CharData *ch, std::string &text) {
	if (!ch->IsFlagged(EPrf::kCoderinfo)) {
		SendMsgToChar(HCONTROL_FORMAT, ch);
		return;
	}

	int month = 0;
	if (!text.empty()) {
		utils::Trim(text);
		try {
			month = std::stoi(text, nullptr, 10);
		}
		catch (const std::invalid_argument &) {
			SendMsgToChar(ch, "Неверный формат (\"hcontrol exp <кол-во последних месяцев>\").");
			return;
		}
	}

	std::multimap<long long, std::string> tmp_list;
	for (ClanListType::const_iterator i = Clan::ClanList.begin(), iend = Clan::ClanList.end(); i != iend; ++i) {
		tmp_list.insert(std::make_pair((*i)->exp_history.get(month), (*i)->get_abbrev()));
	}
	for (std::multimap<long long, std::string>::const_reverse_iterator i = tmp_list.rbegin(), iend = tmp_list.rend();
		 i != iend; ++i) {
		SendMsgToChar(ch, "%5s - %s\r\n", i->second.c_str(), ExpFormat(i->first).c_str());
	}
}

void Clan::hcontrol_set_ingr_chest(CharData *ch, std::string &text) {
	if (!ch->IsFlagged(EPrf::kCoderinfo) || !IS_IMPL(ch)) {
		SendMsgToChar(HCONTROL_FORMAT, ch);
		return;
	}

	// <клан> <комната> - buffer2, text
	std::string buffer2;
	GetOneParam(text, buffer2);
	utils::Trim(text);

	int clan_vnum = 0, vnumum = 0;
	if (!text.empty()) {
		try {
			clan_vnum = std::stol(buffer2, nullptr, 10);
			vnumum = std::stol(text, nullptr, 10);
		}
		catch (const std::invalid_argument &) {
			SendMsgToChar(ch, "Неверный формат (\"hcontrol ingr <клан-рента> <комната хранилища>\").");
			return;
		}
	}

	int room_rnum = GetRoomRnum(vnumum);
	if (room_rnum <= 0) {
		SendMsgToChar(ch, "Комнаты %d не существует.", vnumum);
		return;
	}

	auto i = Clan::ClanList.begin(), iend = Clan::ClanList.end();
	for (/**/; i != iend; ++i) {
		if ((*i)->GetRent() == clan_vnum) {
			break;
		}
	}
	if (i == iend) {
		SendMsgToChar(ch, "Клана %d не существует.", clan_vnum);
		return;
	}
	if ((*i)->GetRent() / 100 != vnumum / 100) {
		SendMsgToChar(ch, "Комната %d находится вне зоны замка %d.", vnumum, (*i)->GetRent());
		return;
	}

	bool chest_moved = false;
	// хран под ингры уже был
	if ((*i)->ingr_chest_active()) {
		for (ObjData *chest = world[(*i)->get_ingr_chest_room_rnum()]->contents; chest;
			 chest = chest->get_next_content()) {
			if (is_ingr_chest(chest)) {
				RemoveObjFromRoom(chest);
				PlaceObjToRoom(chest, room_rnum);
				chest_moved = true;
				break;
			}
		}
	}

	(*i)->ingr_chest_room_rnum_ = room_rnum;
	Clan::ClanSave();

	if (!chest_moved) {
		const auto chest = world_objects.create_from_prototype_by_vnum(INGR_CHEST_VNUM);
		if (chest) {
			PlaceObjToRoom(chest.get(), (*i)->get_ingr_chest_room_rnum());
		}
		SendMsgToChar("Хранилище установлено.\r\n", ch);
	} else {
		SendMsgToChar("Хранилище перенесено.\r\n", ch);
	}
}

// создание дружины (hcontrol build)
void Clan::HcontrolBuild(CharData *ch, std::string &buffer) {
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
	MobVnum guard = atoi(buffer2.c_str());
	// воевода
	std::string owner;
	GetOneParam(buffer, owner);
	// аббревиатура
	std::string abbrev;
	GetOneParam(buffer, abbrev);
	// название клана
	std::string name = buffer;

	// тут проверяем наличие все этого дела
	if (name.empty()) {
		SendMsgToChar(HCONTROL_FORMAT, ch);
		return;
	}
	if (!GetRoomRnum(rent)) {
		SendMsgToChar(ch, "Комнаты %d не существует.\r\n", rent);
		return;
	}
	if (!GetRoomRnum(out_rent)) {
		SendMsgToChar(ch, "Комнаты %d не существует.\r\n", out_rent);
		return;
	}
	if (GetMobRnum(guard) < 0) {
		SendMsgToChar(ch, "Моба %d не существует.\r\n", guard);
		return;
	}
	long unique = 0;
	if (!(unique = GetUniqueByName(owner))) {
		SendMsgToChar(ch, "Персонажа %s не существует.\r\n", owner.c_str());
		return;
	}
	// а тут - не занят ли параметр другим кланом
	for (const auto &clan : Clan::ClanList) {
		if (clan->rent == rent) {
			SendMsgToChar(ch, "Комната %d уже занята другой дружиной.\r\n", rent);
			return;
		}
		if (clan->guard == guard) {
			SendMsgToChar(ch, "Охранник %d уже занят другой дружиной.\r\n", rent);
			return;
		}
		const auto it = clan->m_members.find(unique);
		if (it != clan->m_members.end()) {
			SendMsgToChar(ch, "%s уже приписан к дружине %s.\r\n", owner.c_str(), clan->abbrev.c_str());
			return;
		}
		if (CompareParam(clan->abbrev, abbrev, 1)) {
			SendMsgToChar(ch, "Аббревиатура '%s' уже занята другой дружиной.\r\n", abbrev.c_str());
			return;
		}
		if (CompareParam(clan->name, name, 1)) {
			SendMsgToChar(ch, "Имя '%s' уже занято другой дружиной.\r\n", name.c_str());
			return;
		}
	}

	// собственно клан
	const auto tempClan = std::make_shared<Clan>();
	tempClan->rent = rent;
	tempClan->out_rent = out_rent;
	tempClan->chest_room = rent;
	tempClan->guard = guard;
	// пишем воеводу
	owner[0] = UPPER(owner[0]);
	tempClan->owner = owner;
	const auto tempMember = std::make_shared<ClanMember>();
	tempMember->name = owner;
	tempClan->m_members.set(unique, tempMember);
	// названия
	tempClan->name = name;
	tempClan->builtOn = time(nullptr);
	tempClan->title_female = tempClan->title = tempClan->abbrev = abbrev;
	// ранги
	const char
		*ranks[] = {"воевода", "боярин", "десятник", "храбр", "кметь", "гридень", "муж", "вой", "отрок", "гость"};
	// женский род пока тоже самое, а то воплей будет...
	const char *ranks_female[] =
		{"воевода", "боярин", "десятник", "храбр", "кметь", "гридень", "муж", "вой", "отрок", "гость"};
	std::vector<std::string> temp_ranks(ranks, ranks + 10);
	std::vector<std::string> temp_ranks_female(ranks_female, ranks_female + 10);
	tempClan->ranks = temp_ranks;
	tempClan->bank = 1000;
	tempClan->ranks_female = temp_ranks_female;

	// привилегии
	for (std::vector<std::string>::const_iterator it = tempClan->ranks.begin(); it != tempClan->ranks.end(); ++it) {
		tempClan->privileges.push_back(std::bitset<CLAN_PRIVILEGES_NUM>());
	}

	// воеводе проставим все привилегии
	for (unsigned i = 0; i < CLAN_PRIVILEGES_NUM; ++i) {
		tempClan->privileges[0].set(i);
	}

	// залоадим сразу хранилище
	const auto chest = world_objects.create_from_prototype_by_vnum(CLAN_CHEST_VNUM);
	if (chest) {
		PlaceObjToRoom(chest.get(), GetRoomRnum(tempClan->chest_room));
	}

	Clan::ClanList.push_back(tempClan);
	Clan::ClanSave();
	Boards::Static::ClanInit();

	// уведомляем счастливых воеводу и имма
	DescriptorData *d = DescriptorByUid(unique);
	if (d) {
		Clan::SetClanData(d->character.get());
		SendMsgToChar(d->character.get(), "Вы стали хозяином нового замка. Добро пожаловать!\r\n");
	}

	SendMsgToChar(ch, "Дружина '%s' создана!\r\n", abbrev.c_str());
}

// удаление дружины (hcontrol destroy)
void Clan::HcontrolDestroy(CharData *ch, std::string &buffer) {
	int rent = atoi(buffer.c_str());

	ClanListType::iterator clan;
	for (clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan) {
		if ((*clan)->rent == rent) {
			break;
		}
	}

	if (clan == Clan::ClanList.end()) {
		SendMsgToChar(ch, "Дружины с номером %d не существует.\r\n", rent);
		return;
	}
	char smallbuf[128];
	sprintf(smallbuf, "распущена_богами");
	DestroyClan(*clan, smallbuf);

	SendMsgToChar("Дружина распущена.\r\n", ch);
}

void Clan::fix_clan_members_load_room(Clan::shared_ptr clan) {
	CharData *cbuf;
	DescriptorData *tch;

	for (std::size_t i = 0; i < player_table.size(); i++) {
		const auto unique = player_table[i].uid();
		auto member_i = clan->m_members.find(unique);
		if (member_i == clan->m_members.end()) {
			continue;
		}

		for (tch = descriptor_list; tch; tch = tch->next) {
			// it is possible to have character == nullptr because character is being created later than descriptor
			if (nullptr == tch->character) {
				continue;
			}

			if (isname(player_table[i].name(), tch->character->GetCharAliases())) {
				GET_LOADROOM(tch->character) = mortal_start_room;
				char_from_room(tch->character);
				char_to_room(tch->character, GetRoomRnum(mortal_start_room));

				break;
			}
		}

		if (!tch) {
			cbuf = new Player;
			if (LoadPlayerCharacter(player_table[i].name().c_str(), cbuf, ELoadCharFlags::kFindId) > -1) {
				GET_LOADROOM(cbuf) = mortal_start_room;
				cbuf->save_char();
			}
			delete cbuf;
		}

		sprintf(buf, "CLAN: Роспуск, удаляю игрока %s [%s]", player_table[i].name().c_str(), clan->name.c_str());
		log("%s", buf);
	}
}

void Clan::DestroyClan(Clan::shared_ptr clan, char *reason) {
	fix_clan_members_load_room(clan);
	const auto members = clan->m_members;    // copy members

	for (const auto &clanVictim : Clan::ClanList) { //для всех кланов выставляем нейтралитет (тупо удаляем)
		if (clan->rent != clanVictim->rent)
			clanVictim->SetPolitics(clan->rent, kPoliticsNeutral);
	}
	clan->set_rep(0);
	clan->builtOn = 0;
	clan->exp = 0;
	clan->clan_exp = 0;
	clan->clan_level = 0;
	clan->politics.clear();
	clan->bank = 1000;
	clan->gold_tax_pct_ = 0;
	clan->ingr_chest_room_rnum_ = 0;
	clan->storehouse = 0;
	clan->pkList.clear();
	clan->frList.clear();
	ObjData *temp, *chest, *obj_next;
	for (chest = world[GetRoomRnum(clan->chest_room)]->contents; chest; chest = chest->get_next_content()) {
		if (Clan::is_clan_chest(chest)) {
			for (temp = chest->get_contains(); temp; temp = obj_next) {
				obj_next = temp->get_next_content();
				RemoveObjFromObj(temp);
				ExtractObjFromWorld(temp);
			}
			break;
		}
	}
	// пуржим ингры, если есть
	clan->purge_ingr_chest();
	clan->exp_history.fulldelete();
	clan->last_exp.fulldelete();
	Clan::ClanSave();

	char smallbuf[128];
	for (const auto &it : members) {
		DescriptorData *d = DescriptorByUid(it.first);
		if (d) {
			Clan::SetClanData(d->character.get());
			SendMsgToChar(d->character.get(), "Ваша дружина распущена. Желаем удачи!\r\n");
		}
		Player p_vict;
		CharData *vict = &p_vict;
		if (LoadPlayerCharacter(it.second->name.c_str(), vict, ELoadCharFlags::kFindId) > -1) {
			sprintf(smallbuf, "Исключен(а) из дружины '%s'. (распущена)", clan->get_abbrev().c_str());
			AddKarma(vict, smallbuf, reason);
			vict->save_char();
		}

	}
	clan->m_members.clear();                    // remove all members from clan

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
bool check_online_state(long uid) {
	for (const auto &tch : character_list) {
		if (tch->IsNpc()
			|| tch->get_uid() != uid
			|| (!tch->desc && !NORENTABLE(tch))) {
			continue;
		}

		return true;
	}
	return false;
}

// * Распечатка пкл/дрл с учетом режима 'пкфортмат'.
void print_pkl(CharData *ch, std::ostringstream &stream, ClanPkList::const_iterator &it) {
	static char timeBuf[11];

	if (ch->IsFlagged(EPrf::kPkFormatMode))
		stream << it->second->victimName << " : " << it->second->text << "\n";
	else {
		strftime(timeBuf, sizeof(timeBuf), "%d/%m/%Y", localtime(&(it->second->time)));
		stream << fmt::format("{} [{}] :: {}\r\n{}\r\n\r\n",
							  timeBuf, it->second->authorName, it->second->victimName, it->second->text);
	}
}

// кладем в сундук (при наличии привилегии)
// если предмет - деньги, то автоматом идут в клан-казну, контейнеры только пустые
bool Clan::PutChest(CharData *ch, ObjData *obj, ObjData *chest) {
	const bool prohibited = ch->IsNpc() || !CLAN(ch)
		|| GetRoomRnum(CLAN(ch)->chest_room) != ch->in_room
		|| !CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_CHEST_PUT];
	if (prohibited) {
		SendMsgToChar("Не имеете таких правов!\r\n", ch);
		return false;
	}

	if (obj->get_type() == EObjType::kMoney) {
		long gold = GET_OBJ_VAL(obj, 0);
		if (IS_IMMORTAL(ch)) {
			RemoveObjFromChar(obj);
			ExtractObjFromWorld(obj);
			ch->add_gold(gold);
			SendMsgToChar(ch, "Вам это не положено! Вы вновь обрели %ld %s.\r\n",
						  gold, GetDeclensionInNumber(gold, EWhat::kMoneyU));
			return true;
		}
		// здесь и далее: в случае переполнения  - кладем сколько можем, остальное возвращаем чару
		if ((CLAN(ch)->bank + gold) < 0) {
			long over = std::numeric_limits<long>::max() - CLAN(ch)->bank;
			CLAN(ch)->bank += over;
			CLAN(ch)->m_members.add_money(ch->get_uid(), over);
			gold -= over;
			ch->add_gold(gold);
			RemoveObjFromChar(obj);
			ExtractObjFromWorld(obj);
			SendMsgToChar(ch,
						  "Вам удалось вложить в казну дружины только %ld %s.\r\n",
						  over,
						  GetDeclensionInNumber(over, EWhat::kMoneyU));
			return true;
		}
		CLAN(ch)->bank += gold;
		CLAN(ch)->m_members.add_money(ch->get_uid(), gold);
		RemoveObjFromChar(obj);
		ExtractObjFromWorld(obj);
		SendMsgToChar(ch, "Вы вложили в казну дружины %ld %s.\r\n", gold, GetDeclensionInNumber(gold, EWhat::kMoneyU));

	} else if (obj->has_flag(EObjFlag::kNodrop)
		|| obj->is_unrentable()
		|| obj->has_flag(EObjFlag::kNamed)
		|| obj->get_owner()) {
		act("Неведомая сила помешала положить $o3 в $O3.", false, ch, obj, chest, kToChar);
	} else if (obj->get_type() == EObjType::kContainer
		&& obj->get_contains()) {
		act("В $o5 что-то лежит.", false, ch, obj, nullptr, kToChar);
	} else if (SetSystem::is_norent_set(ch, obj, true) && obj->has_flag(EObjFlag::kNotOneInClanChest)) {
		snprintf(buf, kMaxStringLength, "%s - требуется две и более вещи из набора.\r\n", obj->get_PName(ECase::kNom).c_str());
		SendMsgToChar(utils::CAP(buf), ch);
		return false;
	} else {
		if ((chest->get_weight() + obj->get_weight()) > CLAN(ch)->ChestMaxWeight()
			|| CLAN(ch)->chest_objcount == CLAN(ch)->ChestMaxObjects()) {
			act("Вы попытались запихнуть $o3 в $O3, но не смогли - там просто нет места.",
				false,
				ch,
				obj,
				chest,
				kToChar);
			return false;
		}
		obj = dungeons::SwapOriginalObject(obj);
		RemoveObjFromChar(obj);
		PlaceObjIntoObj(obj, chest);
		ObjSaveSync::add(ch->get_uid(), CLAN(ch)->GetRent(), ObjSaveSync::CLAN_SAVE);

		CLAN(ch)->chest_log.add(fmt::format("{} сдал{} {}{}\r\n",
											ch->get_name(), GET_CH_SUF_1(ch), obj->get_PName(ECase::kAcc),
											clan_get_custom_label(obj, CLAN(ch))));

		// канал хранилища
		for (DescriptorData *d = descriptor_list; d; d = d->next) {
			if (d->character
				&& d->state == EConState::kPlaying
				&& !AFF_FLAGGED(d->character, EAffect::kDeafness)
				&& CLAN(d->character)
				&& CLAN(d->character) == CLAN(ch)
				&& d->character->IsFlagged(EPrf::kTakeMode)) {
				SendMsgToChar(d->character.get(), "[Хранилище]: %s'%s сдал%s %s%s.'%s\r\n",
							  kColorBoldRed, GET_NAME(ch), GET_CH_SUF_1(ch), obj->get_PName(ECase::kAcc).c_str(),
							  clan_get_custom_label(obj, CLAN(ch)).c_str(), kColorNrm);
			}
		}

		if (!ch->IsFlagged(EPrf::kDecayMode)) {
			act("Вы положили $o3 в $O3.", false, ch, obj, chest, kToChar);
		}

		CLAN(ch)->chest_objcount++;
	}

	return true;
}

// берем из клан-сундука (при наличии привилегии)
bool Clan::TakeChest(CharData *ch, ObjData *obj, ObjData *chest) {
	if (ch->IsNpc()
		|| !CLAN(ch)
		|| GetRoomRnum(CLAN(ch)->chest_room) != ch->in_room
		|| !CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_CHEST_TAKE]) {
		SendMsgToChar("Не имеете таких правов!\r\n", ch);
		return false;
	}

	RemoveObjFromObj(obj);
	PlaceObjToInventory(obj, ch);
	ObjSaveSync::add(ch->get_uid(), CLAN(ch)->GetRent(), ObjSaveSync::CLAN_SAVE);

	if (obj->get_carried_by() == ch) {
		std::string log_text = fmt::format("{} забрал{} {}{}\r\n",
										   GET_NAME(ch), GET_CH_SUF_1(ch), obj->get_PName(ECase::kAcc),
										   clan_get_custom_label(obj, CLAN(ch)));
		CLAN(ch)->chest_log.add(log_text);

		// канал хранилища
		for (DescriptorData *d = descriptor_list; d; d = d->next) {
			if (d->character
				&& d->state == EConState::kPlaying
				&& !AFF_FLAGGED(d->character, EAffect::kDeafness)
				&& CLAN(d->character)
				&& CLAN(d->character) == CLAN(ch)
				&& d->character->IsFlagged(EPrf::kTakeMode)) {
				SendMsgToChar(d->character.get(), "[Хранилище]: %s'%s забрал%s %s%s.'%s\r\n",
							  kColorBoldRed, GET_NAME(ch), GET_CH_SUF_1(ch),
							  obj->get_PName(ECase::kAcc).c_str(),
							  clan_get_custom_label(obj, CLAN(d->character)).c_str(),
							  kColorNrm);
			}
		}

		if (!ch->IsFlagged(EPrf::kTakeMode)) {
			act("Вы взяли $o3 из $O1.", false, ch, obj, chest, kToChar);
		}
		CLAN(ch)->chest_objcount--;
	}
	return true;
}

void Clan::save_chest() {
	log("Save obj: %s", this->abbrev.c_str());
	ObjSaveSync::check(this->GetRent(), ObjSaveSync::CLAN_SAVE);

	std::string buffer = this->abbrev;
	for (unsigned i = 0; i != buffer.length(); ++i)
		buffer[i] = LOWER(AtoL(buffer[i]));
	std::string filename = LIB_HOUSE + buffer + "/" + buffer + ".obj";
	for (ObjData *chest = world[GetRoomRnum(this->chest_room)]->contents; chest; chest = chest->get_next_content()) {
		if (Clan::is_clan_chest(chest)) {
			std::stringstream out;
			out << "* Items file\n";
			for (ObjData *temp = chest->get_contains(); temp; temp = temp->get_next_content()) {
				write_one_object(out, temp, 0);
			}
			out << "\n$\n$\n";

			std::ofstream file(filename.c_str());
			if (!file.is_open()) {
				log("Error open file: %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
				return;
			}
			file << out.rdbuf();
			file.close();
			break;
		}
	}
}

// сохраняем все сундуки в файлы
// пользует write_one_object (мне кажется это разуменее, чем плодить свои форматы везде и потом
// заниматься с ними сексом при изменении параметров на шмотках)
void Clan::SaveChestAll() {
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan) {
		(*clan)->save_chest();
	}
}

// чтение файлов клановых сундуков
// пользует read_one_object_new для чтения шмоток плееров в ренте
void Clan::ChestLoad() {
	ObjData *temp, *obj_next;

	// TODO: при сильном желании тут можно пробегать все зоны замков или вообще все зоны/предметы и пуржить все chest
	// предметы и их содержимое, на случай релоада кланов и изменения комнаты с хранилищем (чтобы в маде не пуржить руками)

	// на случай релоада - чистим перед этим все что было в сундуках
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan) {
		for (auto chest = world[GetRoomRnum((*clan)->chest_room)]->contents; chest; chest = chest->get_next_content()) {
			if (Clan::is_clan_chest(chest)) {
				for (temp = chest->get_contains(); temp; temp = obj_next) {
					obj_next = temp->get_next_content();
					RemoveObjFromObj(temp);
					ExtractObjFromWorld(temp);
				}
				ExtractObjFromWorld(chest);
				break;
			}
		}
	}

	int error = 0, fsize = 0;
	FILE *fl;
	char *data, *databuf;
	std::string buffer;

	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan) {
		buffer = (*clan)->abbrev;
		for (unsigned i = 0; i != buffer.length(); ++i) {
			buffer[i] = LOWER(AtoL(buffer[i]));
		}
		std::string filename = LIB_HOUSE + buffer + "/" + buffer + ".obj";

		//лоадим сундук. в зонах его лоадить не нужно.
		const auto chest = world_objects.create_from_prototype_by_vnum(CLAN_CHEST_VNUM);
		if (chest) {
			PlaceObjToRoom(chest.get(), GetRoomRnum((*clan)->chest_room));
		}

		if (!chest) {
			log("<Clan> Chest load error '%s'! (%s %s %d)", (*clan)->abbrev.c_str(), __FILE__, __func__, __LINE__);
			return;
		}

		if (!(fl = fopen(filename.c_str(), "r+b"))) {
			continue;
		}

		fseek(fl, 0L, SEEK_END);
		fsize = ftell(fl);
		if (!fsize) {
			fclose(fl);
			log("<Clan> Empty obj file '%s'. (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
			continue;
		}
		databuf = new char[fsize + 1];

		fseek(fl, 0L, SEEK_SET);
		if (!fread(databuf, fsize, 1, fl) || ferror(fl) || !databuf) {
			fclose(fl);
			log("<Clan> Error reading obj file '%s'. (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
			continue;
		}
		fclose(fl);

		data = databuf;
		*(data + fsize) = '\0';

		for (fsize = 0; *data && *data != '$'; fsize++) {
			const auto obj = read_one_object_new(&data, &error);
			if (!obj) {
				if (error) {
					log("<Clan> Items reading fail for %s error %d.", filename.c_str(), error);
				}

				continue;
			}

			if (!NamedStuff::check_named(nullptr,
										 obj.get()))//Если объект есть в списке именных то ему нечего делать в хранилище
			{
				PlaceObjIntoObj(obj.get(), chest.get());
			} else {
				ExtractObjFromWorld(obj.get());
			}
		}
		delete[] databuf;
	}
}

// смотрим чего в сундуках и берем из казны за ренту
void Clan::ChestUpdate() {
	double i, cost;

	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan) {
		cost = (*clan)->ChestTax() + (*clan)->ingr_chest_tax();
		cost += (*clan)->calculate_clan_tax();
		// расчет и снимание за ренту (целой части по возможности)
		cost = (cost * kChestUpdatePeriod) / (60 * 24);

		(*clan)->bankBuffer += cost;
		std::modf((*clan)->bankBuffer, &i);
		if (i >= 1.0f) {
			(*clan)->bank -= static_cast<unsigned>(i);
			(*clan)->bankBuffer -= i;
		}
		// при нулевом счете все шмотки в сундуке пуржим
		// TODO: а тут придется опять искать, ибо выше сундук уже похерен, надо фиксить
		if ((*clan)->bank < 0) {
			(*clan)->bank = 0;
			ObjData *temp, *chest, *obj_next;
			for (chest = world[GetRoomRnum((*clan)->chest_room)]->contents; chest; chest = chest->get_next_content()) {
				if (Clan::is_clan_chest(chest)) {
					for (temp = chest->get_contains(); temp; temp = obj_next) {
						obj_next = temp->get_next_content();
						RemoveObjFromObj(temp);
						ExtractObjFromWorld(temp);
					}
					break;
				}
			}
			// пуржим ингры, если есть
			(*clan)->purge_ingr_chest();
		}
	}
}

// * Запись сообщения дружины в файл и поле клана.
void Clan::write_mod(const std::string &arg) {
	std::string abbrev = this->get_abbrev();
	for (unsigned i = 0; i != abbrev.length(); ++i) {
		abbrev[i] = LOWER(AtoL(abbrev[i]));
	}
	std::string filename = LIB_HOUSE + abbrev + "/" + abbrev + ".mod";

	std::ofstream file(filename.c_str());
	if (!file.is_open()) {
		log("Error open file: %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
		return;
	}
	file << arg;
	file.close();

	mod_text = arg;
}

// * Распечатка сообщения дружины чару при входе.
bool Clan::print_mod(CharData *ch) const {
	if (!mod_text.empty()) {
		SendMsgToChar(ch, "\r\n%s%s%s\r\n",
					  kColorWht, mod_text.c_str(), kColorNrm);
		return true;
	}

	return false;
}

// * Загрузка сообщения дружины.
void Clan::load_mod() {
	std::string abbrev = this->get_file_abbrev();
	std::string filename = LIB_HOUSE + abbrev + "/" + abbrev + ".mod";

	std::ifstream file(filename.c_str(), std::ios::binary);
	if (!file.is_open()) {
		log("Error open file: %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
		return;
	}

	std::ostringstream out;
	out << file.rdbuf();
	out.str().swap(mod_text);
}

// казна дружины... команды теже самые с приставкой 'казна' в начале
// смотреть/вкладывать могут все, снимать по привилегии, висит на стандартных банкирах
bool Clan::BankManage(CharData *ch, char *arg) {
	if (ch->IsNpc() || !CLAN(ch) || GetRealLevel(ch) >= kLvlImmortal)
		return false;

	std::string buffer = arg, buffer2;
	GetOneParam(buffer, buffer2);

	if (CompareParam(buffer2, "баланс") || CompareParam(buffer2, "balance")) {
		SendMsgToChar(ch,
					  "На счету вашей дружины ровно %ld %s.\r\n",
					  CLAN(ch)->bank,
					  GetDeclensionInNumber(CLAN(ch)->bank, EWhat::kMoneyA));
		return true;

	} else if (CompareParam(buffer2, "вложить") || CompareParam(buffer2, "deposit")) {
		GetOneParam(buffer, buffer2);
		long gold = 0;
		try {
			gold = std::stol(buffer2, nullptr, 10);
			if (gold <= 0) {
				SendMsgToChar("Сколько вы хотите вложить?\r\n", ch);
				return true;
			}
			if (ch->get_gold() < gold) {
				SendMsgToChar("О такой сумме вы можете только мечтать!\r\n", ch);
				return true;
			}
		}
		catch (const std::invalid_argument &) {
			SendMsgToChar("Формат команды казна вложить <число>\r\n", ch);
		}
		// на случай переполнения казны
		if ((CLAN(ch)->bank + gold) < 0) {
			long over = std::numeric_limits<long int>::max() - CLAN(ch)->bank;
			CLAN(ch)->bank += over;
			CLAN(ch)->m_members.add_money(ch->get_uid(), over);
			ch->remove_gold(over);
			SendMsgToChar(ch,
						  "Вам удалось вложить в казну дружины только %ld %s.\r\n",
						  over,
						  GetDeclensionInNumber(over, EWhat::kMoneyU));
			act("$n произвел$g финансовую операцию.", true, ch, nullptr, nullptr, kToRoom);
			std::string log_text = fmt::format("{} вложил%s в казну {} {}\r\n",
											   GET_NAME(ch), GET_CH_SUF_1(ch), over,
											   GetDeclensionInNumber(over, EWhat::kMoneyU));
			CLAN(ch)->chest_log.add(log_text);
			return true;
		}
		ch->remove_gold(gold);
		CLAN(ch)->bank += gold;
		CLAN(ch)->m_members.add_money(ch->get_uid(), gold);
		SendMsgToChar(ch, "Вы вложили %ld %s.\r\n", gold, GetDeclensionInNumber(gold, EWhat::kMoneyU));
		act("$n произвел$g финансовую операцию.", true, ch, 0, nullptr, kToRoom);
		std::string log_text = fmt::format("{} вложил%s в казну {} {}\r\n",
										   GET_NAME(ch),
										   GET_CH_SUF_1(ch),
										   gold,
										   GetDeclensionInNumber(gold, EWhat::kMoneyU));
		CLAN(ch)->chest_log.add(log_text);
		return true;
	} else if (CompareParam(buffer2, "получить") || CompareParam(buffer2, "withdraw")) {
		if (!CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_BANK]) {
			SendMsgToChar("К сожалению, у вас нет возможности транжирить средства дружины.\r\n", ch);
			return true;
		}
		GetOneParam(buffer, buffer2);
		long gold = atol(buffer2.c_str());

		if (gold <= 0) {
			SendMsgToChar("Уточните количество денег, которые вы хотите получить?\r\n", ch);
			return true;
		}
		if (CLAN(ch)->bank < gold) {
			SendMsgToChar("К сожалению, ваша дружина не так богата.\r\n", ch);
			return true;
		}

		// на случай переполнения персонажа
		if ((ch->get_gold() + gold) < 0) {
			long over = std::numeric_limits<long>::max() - CLAN(ch)->bank;
			ch->add_gold(over);
			CLAN(ch)->bank -= over;
			CLAN(ch)->m_members.sub_money(ch->get_uid(), over);
			SendMsgToChar(ch,
						  "Вам удалось снять только %ld %s.\r\n",
						  over,
						  GetDeclensionInNumber(over, EWhat::kMoneyU));
			act("$n произвел$g финансовую операцию.", true, ch, 0, nullptr, kToRoom);
			std::string log_text = fmt::format("{} получил%s из казны {} {}\r\n",
											   GET_NAME(ch), GET_CH_SUF_1(ch), over,
											   GetDeclensionInNumber(over, EWhat::kMoneyU));
			CLAN(ch)->chest_log.add(log_text);
			return true;
		}
		CLAN(ch)->bank -= gold;
		CLAN(ch)->m_members.sub_money(ch->get_uid(), gold);
		ch->add_gold(gold);
		SendMsgToChar(ch, "Вы сняли %ld %s.\r\n", gold, GetDeclensionInNumber(gold, EWhat::kMoneyU));
		act("$n произвел$g финансовую операцию.", true, ch, nullptr, nullptr, kToRoom);
		std::string log_text = fmt::format("{} получил%s из казны {} {}\r\n",
										   GET_NAME(ch), GET_CH_SUF_1(ch),
										   gold, GetDeclensionInNumber(gold, EWhat::kMoneyU));
		CLAN(ch)->chest_log.add(log_text);
		return true;
	} else
		SendMsgToChar(ch, "Формат команды: казна вложить|получить|баланс сумма.\r\n");
	return true;
}

void Clan::Manage(DescriptorData *d, const char *arg) {
	unsigned num = atoi(arg);

	switch (d->clan_olc->mode) {
		case CLAN_MAIN_MENU:
			switch (*arg) {
				case 'в':
				case 'В':
				case 'q':
				case 'Q':
					// есть вариант, что за время в олц в клане изменят кол-во званий
					if (d->clan_olc->clan->privileges.size() != d->clan_olc->privileges.size()) {
						SendMsgToChar(
							"Во время редактирования привилегий в вашей дружине было изменено количество званий.\r\n"
							"Во избежание несоответствий зайдите в меню еще раз.\r\n"
							"Редактирование отменено.\r\n",
							d->character.get());
						d->clan_olc.reset();
						d->state = EConState::kPlaying;
						return;
					}

					SendMsgToChar("Вы желаете сохранить изменения? Y(Д)/N(Н) : ", d->character.get());
					d->clan_olc->mode = CLAN_SAVE_MENU;
					return;

				default:
					if (!*arg || !num) {
						SendMsgToChar("Неверный выбор!\r\n", d->character.get());
						d->clan_olc->clan->MainMenu(d);
						return;
					}

					// тут парсим циферки уже (меню начинается с 1 + ранг ниже себя)
					unsigned choise = num + CLAN_MEMBER(d->character)->rank_num;
					if (choise >= d->clan_olc->clan->ranks.size()) {
						unsigned i = choise - static_cast<unsigned>(d->clan_olc->clan->ranks.size());
						if (i == 0) {
							d->clan_olc->clan->AllMenu(d, i);
						} else if (i == 1) {
							d->clan_olc->clan->AllMenu(d, i);
						} else if (i == 2 && !CLAN_MEMBER(d->character)->rank_num) {
							if (d->clan_olc->clan->storehouse)
								d->clan_olc->clan->storehouse = 0;
							else
								d->clan_olc->clan->storehouse = 1;
							d->clan_olc->clan->MainMenu(d);
							return;
						} else if (i == 3 && !CLAN_MEMBER(d->character)->rank_num) {
							if (d->clan_olc->clan->exp_info)
								d->clan_olc->clan->exp_info = 0;
							else
								d->clan_olc->clan->exp_info = 1;
							d->clan_olc->clan->MainMenu(d);
							return;
						} else if (i == 4 && !CLAN_MEMBER(d->character)->rank_num) {
							d->clan_olc->clan->set_ingr_chest(d->character.get());
							d->clan_olc->clan->MainMenu(d);
							return;
						} else if (i == 5 && !CLAN_MEMBER(d->character)->rank_num) {
							if (!ingr_chest_active()) {
								SendMsgToChar("Неверный выбор!\r\n", d->character.get());
							} else {
								d->clan_olc->clan->disable_ingr_chest(d->character.get());
							}
							d->clan_olc->clan->MainMenu(d);
							return;
						} else {
							SendMsgToChar("Неверный выбор!\r\n", d->character.get());
							d->clan_olc->clan->MainMenu(d);
							return;
						}
						return;
					}

					if (choise >= d->clan_olc->clan->ranks.size()) {
						SendMsgToChar("Неверный выбор!\r\n", d->character.get());
						d->clan_olc->clan->MainMenu(d);
						return;
					}
					d->clan_olc->rank = choise;
					d->clan_olc->clan->PrivilegeMenu(d, choise);
					return;
			}
			break;

		case CLAN_PRIVILEGE_MENU:
			switch (*arg) {
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
					if (!*arg) {
						SendMsgToChar("Неверный выбор!\r\n", d->character.get());
						d->clan_olc->clan->PrivilegeMenu(d, d->clan_olc->rank);
						return;
					}

					if (num > CLAN_PRIVILEGES_NUM || num <= 0) {
						SendMsgToChar("Неверный выбор!\r\n", d->character.get());
						d->clan_olc->clan->PrivilegeMenu(d, d->clan_olc->rank);
						return;
					}

					// мы выдаем только доступные привилегии в меню и нормально парсим их тут
					unsigned parse_num;
					for (parse_num = 0; parse_num <= CLAN_PRIVILEGES_NUM; ++parse_num) {
						if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][parse_num]) {
							if (!--num)
								break;
						}
					}

					if (d->clan_olc->privileges[d->clan_olc->rank][parse_num]) {
						d->clan_olc->privileges[d->clan_olc->rank][parse_num] = 0;
					} else {
						d->clan_olc->privileges[d->clan_olc->rank][parse_num] = 1;
					}

					d->clan_olc->clan->PrivilegeMenu(d, d->clan_olc->rank);
					return;
			}

			break;

		case CLAN_SAVE_MENU:
			switch (*arg) {
				case 'y':
				case 'Y':
				case 'д':
				case 'Д': d->clan_olc->clan->privileges.clear();
					d->clan_olc->clan->privileges = d->clan_olc->privileges;
					d->clan_olc.reset();
					// Clan::ClanSave();
					d->state = EConState::kPlaying;
					SendMsgToChar("Изменения сохранены.\r\n", d->character.get());
					return;

				case 'n':
				case 'N':
				case 'н':
				case 'Н': d->clan_olc.reset();
					d->state = EConState::kPlaying;
					SendMsgToChar("Редактирование отменено.\r\n", d->character.get());
					return;

				default:
					SendMsgToChar("Неверный выбор!\r\nВы желаете сохранить изменения? Y(Д)/N(Н) : ",
								  d->character.get());
					d->clan_olc->mode = CLAN_SAVE_MENU;
					return;
			}

			break;

		case CLAN_ADDALL_MENU:
			switch (*arg) {
				case 'в':
				case 'В':
				case 'q':
				case 'Q':
					// выход в общее меню с изменением всех званий
					for (unsigned i = 0; i < CLAN_PRIVILEGES_NUM; ++i) {
						if (d->clan_olc->all_ranks[i]) {
							unsigned j = CLAN_MEMBER(d->character)->rank_num + 1;
							for (; j <= d->clan_olc->clan->ranks.size() - 1; ++j) {
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
						SendMsgToChar("Неверный выбор!\r\n", d->character.get());
						d->clan_olc->clan->AllMenu(d, 0);
						return;
					}

					if (num > CLAN_PRIVILEGES_NUM) {
						SendMsgToChar("Неверный выбор!\r\n", d->character.get());
						d->clan_olc->clan->AllMenu(d, 0);
						return;
					}

					unsigned parse_num;
					for (parse_num = 0; parse_num < CLAN_PRIVILEGES_NUM; ++parse_num) {
						if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][parse_num]) {
							if (!--num)
								break;
						}
					}

					if (d->clan_olc->all_ranks[parse_num]) {
						d->clan_olc->all_ranks[parse_num] = false;
					} else {
						d->clan_olc->all_ranks[parse_num] = true;
					}

					d->clan_olc->clan->AllMenu(d, 0);
					return;
			}
			break;

		case CLAN_DELALL_MENU:
			switch (*arg) {
				case 'в':
				case 'В':
				case 'q':
				case 'Q':
					// выход в общее меню с изменением всех званий
					for (unsigned i = 0; i < CLAN_PRIVILEGES_NUM; ++i) {
						if (d->clan_olc->all_ranks[i]) {
							unsigned j = CLAN_MEMBER(d->character)->rank_num + 1;
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
						SendMsgToChar("Неверный выбор!\r\n", d->character.get());
						d->clan_olc->clan->AllMenu(d, 1);
						return;
					}

					if (num > CLAN_PRIVILEGES_NUM) {
						SendMsgToChar("Неверный выбор!\r\n", d->character.get());
						d->clan_olc->clan->AllMenu(d, 1);
						return;
					}

					unsigned parse_num;
					for (parse_num = 0; parse_num < CLAN_PRIVILEGES_NUM; ++parse_num) {
						if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][parse_num]) {
							if (!--num)
								break;
						}
					}

					if (d->clan_olc->all_ranks[parse_num]) {
						d->clan_olc->all_ranks[parse_num] = 0;
					} else {
						d->clan_olc->all_ranks[parse_num] = 1;
					}

					d->clan_olc->clan->AllMenu(d, 1);
					return;
			}
			break;

		default: log("No case, wtf!? mode: %d (%s %s %d)", d->clan_olc->mode, __FILE__, __func__, __LINE__);
	}
}

void Clan::MainMenu(DescriptorData *d) {
	std::ostringstream buffer;
	buffer << "Раздел добавления и удаления привилегий.\r\n"
		   << "Выберите редактируемое звание или действие:\r\n";
	// звания ниже своего
	int rank = CLAN_MEMBER(d->character)->rank_num + 1;
	int num = 0;
	for (std::vector<std::string>::const_iterator it = d->clan_olc->clan->ranks.begin() + rank;
		 it != d->clan_olc->clan->ranks.end(); ++it)
		buffer << kColorGrn << std::setw(2) << ++num << kColorNrm << ") " << (*it)
			   << "\r\n";
	buffer << kColorGrn << std::setw(2) << ++num << kColorNrm
		   << ") " << "добавить всем\r\n";
	buffer << kColorGrn << std::setw(2) << ++num << kColorNrm
		   << ") " << "убрать у всех\r\n";
	if (!CLAN_MEMBER(d->character)->rank_num) {
		buffer << kColorGrn << std::setw(2) << ++num << kColorNrm
			   << ") " << "Выборка из хранилища по параметрам предметов\r\n"
			   << "    + просмотр хранилища вне замка в качестве бонуса (1000 кун в день) ";
		if (this->storehouse)
			buffer << "(отключить)\r\n";
		else
			buffer << "(включить)\r\n";
		buffer << kColorGrn << std::setw(2) << ++num << kColorNrm
			   << ") " << "Моментально обновлять очки дружины в топе (при отключении обновление раз в 6 часов) ";
		if (this->exp_info)
			buffer << "(отключить)\r\n";
		else
			buffer << "(включить)\r\n";

		// хранилище ингров (включить/переместить)
		buffer << kColorGrn << std::setw(2) << ++num
			   << kColorNrm << ") ";
		if (ingr_chest_active()) {
			buffer << "Переместить в данную комнату хранилище для ингредиентов\r\n";
		} else {
			buffer << "Установить в данной комнате хранилище для ингредиентов (1000 кун/день)\r\n";
		}

		// хранилище ингров (выключить)
		if (ingr_chest_active()) {
			buffer << kColorGrn << std::setw(2) << ++num
				   << kColorNrm << ") "
				   << "Отключить хранилище для ингредиентов" << "\r\n";
		} else {
			buffer << kColorBoldBlk << std::setw(2) << ++num
				   << ") " << "Отключить хранилище для ингредиентов"
				   << kColorNrm << "\r\n";
		}
	}
	buffer << kColorGrn << " В(Q)" << kColorNrm
		   << ") Выход\r\n" << "Ваш выбор:";

	SendMsgToChar(buffer.str(), d->character.get());
	d->clan_olc->mode = CLAN_MAIN_MENU;
}

void Clan::PrivilegeMenu(DescriptorData *d, unsigned num) {
	if (num > d->clan_olc->clan->ranks.size()) {
		log("Different size clan->ranks and clan->privileges! (%s %s %d)", __FILE__, __func__, __LINE__);
		if (d->character) {
			d->clan_olc.reset();
			d->state = EConState::kPlaying;
			SendMsgToChar(d->character.get(), "Случилось что-то страшное, сообщите Богам!");
		}
		return;
	}
	std::ostringstream buffer;
	buffer << "Список привилегий для звания '" << kColorBoldRed
		   << d->clan_olc->clan->ranks[num] << kColorNrm << "':\r\n";

	int count = 0;
	for (unsigned i = 0; i < CLAN_PRIVILEGES_NUM; ++i) {
		switch (i) {
			case MAY_CLAN_INFO:
				if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_INFO]) {
					buffer << kColorGrn << std::setw(2) << ++count << kColorNrm
						   << ") ";
					if (d->clan_olc->privileges[num][MAY_CLAN_INFO])
						buffer << "[x] информация о дружине (клан информация)\r\n";
					else
						buffer << "[ ] информация о дружине (клан информация)\r\n";
				}
				break;
			case MAY_CLAN_ADD:
				if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_ADD]) {
					buffer << kColorGrn << std::setw(2) << ++count << kColorNrm
						   << ") ";
					if (d->clan_olc->privileges[num][MAY_CLAN_ADD])
						buffer << "[x] принятие в дружину (клан принять)\r\n";
					else
						buffer << "[ ] принятие в дружину (клан принять)\r\n";
				}
				break;
			case MAY_CLAN_REMOVE:
				if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_REMOVE]) {
					buffer << kColorGrn << std::setw(2) << ++count << kColorNrm
						   << ") ";
					if (d->clan_olc->privileges[num][MAY_CLAN_REMOVE])
						buffer << "[x] изгнание из дружины (клан изгнать)\r\n";
					else
						buffer << "[ ] изгнание из дружины (клан изгнать)\r\n";
				}
				break;
			case MAY_CLAN_PRIVILEGES:
				if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_PRIVILEGES]) {
					buffer << kColorGrn << std::setw(2) << ++count << kColorNrm
						   << ") ";
					if (d->clan_olc->privileges[num][MAY_CLAN_PRIVILEGES])
						buffer << "[x] редактирование привилегий (клан привилегии)\r\n";
					else
						buffer << "[ ] редактирование привилегий (клан привилегии)\r\n";
				}
				break;
			case MAY_CLAN_CHANNEL:
				if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_CHANNEL]) {
					buffer << kColorGrn << std::setw(2) << ++count << kColorNrm
						   << ") ";
					if (d->clan_olc->privileges[num][MAY_CLAN_CHANNEL])
						buffer << "[x] клан-канал (гдругам)\r\n";
					else
						buffer << "[ ] клан-канал (гдругам)\r\n";
				}
				break;
			case MAY_CLAN_POLITICS:
				if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_POLITICS]) {
					buffer << kColorGrn << std::setw(2) << ++count << kColorNrm
						   << ") ";
					if (d->clan_olc->privileges[num][MAY_CLAN_POLITICS])
						buffer << "[x] изменение политики дружины (политика)\r\n";
					else
						buffer << "[ ] изменение политики дружины (политика)\r\n";
				}
				break;
			case MAY_CLAN_NEWS:
				if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_NEWS]) {
					buffer << kColorGrn << std::setw(2) << ++count << kColorNrm
						   << ") ";
					if (d->clan_olc->privileges[num][MAY_CLAN_NEWS])
						buffer << "[x] добавление новостей дружины (дрновости)\r\n";
					else
						buffer << "[ ] добавление новостей дружины (дрновости)\r\n";
				}
				break;
			case MAY_CLAN_PKLIST:
				if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_PKLIST]) {
					buffer << kColorGrn << std::setw(2) << ++count << kColorNrm
						   << ") ";
					if (d->clan_olc->privileges[num][MAY_CLAN_PKLIST])
						buffer << "[x] добавление в пк-лист (пклист)\r\n";
					else
						buffer << "[ ] добавление в пк-лист (пклист)\r\n";
				}
				break;
			case MAY_CLAN_CHEST_PUT:
				if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_CHEST_PUT]) {
					buffer << kColorGrn << std::setw(2) << ++count << kColorNrm
						   << ") ";
					if (d->clan_olc->privileges[num][MAY_CLAN_CHEST_PUT])
						buffer << "[x] класть вещи в хранилище\r\n";
					else
						buffer << "[ ] класть вещи в хранилище\r\n";
				}
				break;
			case MAY_CLAN_CHEST_TAKE:
				if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_CHEST_TAKE]) {
					buffer << kColorGrn << std::setw(2) << ++count << kColorNrm
						   << ") ";
					if (d->clan_olc->privileges[num][MAY_CLAN_CHEST_TAKE])
						buffer << "[x] брать вещи из хранилища\r\n";
					else
						buffer << "[ ] брать вещи из хранилища\r\n";
				}
				break;
			case MAY_CLAN_BANK:
				if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_BANK]) {
					buffer << kColorGrn << std::setw(2) << ++count << kColorNrm
						   << ") ";
					if (d->clan_olc->privileges[num][MAY_CLAN_BANK])
						buffer << "[x] брать из казны дружины\r\n";
					else
						buffer << "[ ] брать из казны дружины\r\n";
				}
				break;
			case MAY_CLAN_EXIT:
				if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_EXIT]) {
					buffer << kColorGrn << std::setw(2) << ++count << kColorNrm
						   << ") ";
					if (d->clan_olc->privileges[num][MAY_CLAN_EXIT])
						buffer << "[x] свободный выход из дружины\r\n";
					else
						buffer << "[ ] свободный выход из дружины\r\n";
				}
				break;
			case MAY_CLAN_MOD:
				if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_MOD]) {
					buffer << kColorGrn << std::setw(2) << ++count
						   << kColorNrm << ") "
						   << (d->clan_olc->privileges[num][MAY_CLAN_MOD] ? "[x]" : "[ ]")
						   << " написание сообщения дружины\r\n";
				}
				break;
			case MAY_CLAN_TAX:
				if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_TAX]) {
					buffer << kColorGrn << std::setw(2) << ++count
						   << kColorNrm << ") "
						   << (d->clan_olc->privileges[num][MAY_CLAN_TAX] ? "[x]" : "[ ]")
						   << " установка налога для ратников\r\n";
				}
				break;
			case MAY_CLAN_BOARD:
				if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_BOARD]) {
					buffer << kColorGrn << std::setw(2) << ++count
						   << kColorNrm << ") "
						   << (d->clan_olc->privileges[num][MAY_CLAN_BOARD] ? "[x]" : "[ ]")
						   << " сообщения в дрвече\r\n";
				}
				break;
		} // case
	}
	buffer << kColorGrn << " В(Q)" << kColorNrm
		   << ") Выход\r\n" << "Ваш выбор:";
	SendMsgToChar(buffer.str(), d->character.get());
	d->clan_olc->mode = CLAN_PRIVILEGE_MENU;
}

// меню добавления/удаления привилегий у всех званий, решил не нагружать все в одну функцию
// flag 0 - добавление, 1 - удаление
void Clan::AllMenu(DescriptorData *d, unsigned flag) {
	std::ostringstream buffer;
	if (flag == 0)
		buffer << "Выберите привилегии, которые вы хотите " << kColorBoldRed
			   << "добавить всем" << kColorNrm << " званиям:\r\n";
	else
		buffer << "Выберите привилегии, которые вы хотите " << kColorBoldRed
			   << "убрать у всех" << kColorNrm << " званий:\r\n";

	int count = 0;
	for (unsigned i = 0; i < CLAN_PRIVILEGES_NUM; ++i) {
		switch (i) {
			case MAY_CLAN_INFO:
				if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_INFO]) {
					buffer << kColorGrn << std::setw(2) << ++count << kColorNrm
						   << ") ";
					if (d->clan_olc->all_ranks[MAY_CLAN_INFO])
						buffer << "[x] информация о дружине (клан информация)\r\n";
					else
						buffer << "[ ] информация о дружине (клан информация)\r\n";
				}
				break;
			case MAY_CLAN_ADD:
				if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_ADD]) {
					buffer << kColorGrn << std::setw(2) << ++count << kColorNrm
						   << ") ";
					if (d->clan_olc->all_ranks[MAY_CLAN_ADD])
						buffer << "[x] принятие в дружину (клан принять)\r\n";
					else
						buffer << "[ ] принятие в дружину (клан принять)\r\n";
				}
				break;
			case MAY_CLAN_REMOVE:
				if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_REMOVE]) {
					buffer << kColorGrn << std::setw(2) << ++count << kColorNrm
						   << ") ";
					if (d->clan_olc->all_ranks[MAY_CLAN_REMOVE])
						buffer << "[x] изгнание из дружины (клан изгнать)\r\n";
					else
						buffer << "[ ] изгнание из дружины (клан изгнать)\r\n";
				}
				break;
			case MAY_CLAN_PRIVILEGES:
				if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_PRIVILEGES]) {
					buffer << kColorGrn << std::setw(2) << ++count << kColorNrm
						   << ") ";
					if (d->clan_olc->all_ranks[MAY_CLAN_PRIVILEGES])
						buffer << "[x] редактирование привилегий (клан привилегии)\r\n";
					else
						buffer << "[ ] редактирование привилегий (клан привилегии)\r\n";
				}
				break;
			case MAY_CLAN_CHANNEL:
				if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_CHANNEL]) {
					buffer << kColorGrn << std::setw(2) << ++count << kColorNrm
						   << ") ";
					if (d->clan_olc->all_ranks[MAY_CLAN_CHANNEL])
						buffer << "[x] клан-канал (гдругам)\r\n";
					else
						buffer << "[ ] клан-канал (гдругам)\r\n";
				}
				break;
			case MAY_CLAN_POLITICS:
				if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_POLITICS]) {
					buffer << kColorGrn << std::setw(2) << ++count << kColorNrm
						   << ") ";
					if (d->clan_olc->all_ranks[MAY_CLAN_POLITICS])
						buffer << "[x] изменение политики дружины (политика)\r\n";
					else
						buffer << "[ ] изменение политики дружины (политика)\r\n";
				}
				break;
			case MAY_CLAN_NEWS:
				if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_NEWS]) {
					buffer << kColorGrn << std::setw(2) << ++count << kColorNrm
						   << ") ";
					if (d->clan_olc->all_ranks[MAY_CLAN_NEWS])
						buffer << "[x] добавление новостей дружины (дрновости)\r\n";
					else
						buffer << "[ ] добавление новостей дружины (дрновости)\r\n";
				}
				break;
			case MAY_CLAN_PKLIST:
				if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_PKLIST]) {
					buffer << kColorGrn << std::setw(2) << ++count << kColorNrm
						   << ") ";
					if (d->clan_olc->all_ranks[MAY_CLAN_PKLIST])
						buffer << "[x] добавление в пк-лист (пклист)\r\n";
					else
						buffer << "[ ] добавление в пк-лист (пклист)\r\n";
				}
				break;
			case MAY_CLAN_CHEST_PUT:
				if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_CHEST_PUT]) {
					buffer << kColorGrn << std::setw(2) << ++count << kColorNrm
						   << ") ";
					if (d->clan_olc->all_ranks[MAY_CLAN_CHEST_PUT])
						buffer << "[x] класть вещи в хранилище\r\n";
					else
						buffer << "[ ] класть вещи в хранилище\r\n";
				}
				break;
			case MAY_CLAN_CHEST_TAKE:
				if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_CHEST_TAKE]) {
					buffer << kColorGrn << std::setw(2) << ++count << kColorNrm
						   << ") ";
					if (d->clan_olc->all_ranks[MAY_CLAN_CHEST_TAKE])
						buffer << "[x] брать вещи из хранилища\r\n";
					else
						buffer << "[ ] брать вещи из хранилища\r\n";
				}
				break;
			case MAY_CLAN_BANK:
				if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_BANK]) {
					buffer << kColorGrn << std::setw(2) << ++count << kColorNrm
						   << ") ";
					if (d->clan_olc->all_ranks[MAY_CLAN_BANK])
						buffer << "[x] брать из казны дружины\r\n";
					else
						buffer << "[ ] брать из казны дружины\r\n";
				}
				break;
			case MAY_CLAN_EXIT:
				if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_EXIT]) {
					buffer << kColorGrn << std::setw(2) << ++count << kColorNrm
						   << ") ";
					if (d->clan_olc->all_ranks[MAY_CLAN_EXIT])
						buffer << "[x] свободный выход из дружины\r\n";
					else
						buffer << "[ ] свободный выход из дружины\r\n";
				}
				break;
			case MAY_CLAN_MOD:
				if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_MOD]) {
					buffer << kColorGrn << std::setw(2) << ++count
						   << kColorNrm << ") "
						   << (d->clan_olc->all_ranks[MAY_CLAN_MOD] ? "[x]" : "[ ]")
						   << " написание сообщения дружины\r\n";
				}
				break;
			case MAY_CLAN_TAX:
				if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_TAX]) {
					buffer << kColorGrn << std::setw(2) << ++count
						   << kColorNrm << ") "
						   << (d->clan_olc->all_ranks[MAY_CLAN_TAX] ? "[x]" : "[ ]")
						   << " установка налога для ратников\r\n";
				}
				break;
			case MAY_CLAN_BOARD:
				if (d->clan_olc->privileges[CLAN_MEMBER(d->character)->rank_num][MAY_CLAN_BOARD]) {
					buffer << kColorGrn << std::setw(2) << ++count
						   << kColorNrm << ") "
						   << (d->clan_olc->all_ranks[MAY_CLAN_BOARD] ? "[x]" : "[ ]")
						   << " сообщения в дрвече\r\n";
				}
				break;
		} // case
	}
	buffer << kColorGrn << " В(Q)" << kColorNrm
		   << ") Применить\r\n" << "Ваш выбор:";
	SendMsgToChar(buffer.str(), d->character.get());
	if (flag == 0)
		d->clan_olc->mode = CLAN_ADDALL_MENU;
	else
		d->clan_olc->mode = CLAN_DELALL_MENU;
}

void Clan::add_offline_member(const std::string &name, int uid, int rank) {
	const auto tmp_member = std::make_shared<ClanMember>();
	tmp_member->name = name;
	tmp_member->rank_num = rank;
	this->m_members.set(uid, tmp_member);
}

// игрок ранг
void Clan::ClanAddMember(CharData *ch, int rank, std::string invite_name) {
	if (ch->get_name_str().empty()) {
		log("SYSERROR: zero player name (uid = %ld) (%s:%d %s)", ch->get_uid(),
			__FILE__, __LINE__, __func__);
		return;
	}

	this->add_offline_member(ch->get_name_str(), ch->get_uid(), rank);
	Clan::SetClanData(ch);
	DescriptorData *d;
	for (d = descriptor_list; d; d = d->next) {
		if (d->character
			&& CLAN(d->character)
			&& d->state == EConState::kPlaying
			&& !AFF_FLAGGED(d->character, EAffect::kDeafness)
			&& this->GetRent() == CLAN(d->character)->GetRent()
			&& ch != d->character.get()) {
			SendMsgToChar(d->character.get(),
						  "%s%s приписан%s к вашей дружине, статус - '%s'.%s\r\n",
						  kColorWht,
						  GET_NAME(ch),
						  GET_CH_SUF_6(ch),
						  (this->ranks[rank]).c_str(),
						  kColorNrm);
		}
	}

	SendMsgToChar(ch, "%sВас приписали к дружине '%s', статус - '%s'.%s\r\n",
				  kColorWht, this->name.c_str(), (this->ranks[rank]).c_str(), kColorNrm);
	sprintf(buf, "Принят в дружину '%s', статус - '%s'", this->name.c_str(), (this->ranks[rank]).c_str());
	AddKarma(ch, buf, invite_name.c_str());
	return;
}

// передача воеводства
void Clan::HouseOwner(CharData *ch, std::string &buffer) {
	std::string buffer2;
	GetOneParam(buffer, buffer2);
	long unique = GetUniqueByName(buffer2);
	DescriptorData *d = DescriptorByUid(unique);

	if (buffer2.empty())
		SendMsgToChar("Укажите имя персонажа.\r\n", ch);
	else if (!unique)
		SendMsgToChar("Неизвестный персонаж.\r\n", ch);
	else if (unique == ch->get_uid())
		SendMsgToChar("Сменить себя на самого себя? Вы бредите.\r\n", ch);
	else if (!d || !CAN_SEE(ch, d->character))
		SendMsgToChar("Этого персонажа нет в игре!\r\n", ch);
	else if (CLAN(d->character) && CLAN(ch) != CLAN(d->character))
		SendMsgToChar("Вы не можете передать свои права члену другой дружины.\r\n", ch);
	else {
		buffer2[0] = UPPER(buffer2[0]);
		// воевода идет рангом ниже
		this->m_members.set_rank(ch->get_uid(), 1);
		Clan::SetClanData(ch);
		// ставим нового воеводу (если он был в клане - меняем только ранг)
		if (CLAN(d->character)) {
			this->m_members.set_rank(d->character->get_uid(), 0);
			Clan::SetClanData(d->character.get());
		} else {
			this->ClanAddMember(d->character.get(), 0, ch->get_name());
		}
		this->owner = buffer2;
		SendMsgToChar(ch, "Поздравляем, вы передали свои полномочия %s!\r\n", GET_PAD(d->character, 2));
		if (IS_MALE(ch))
			sprintf(buf,
					"&RВнимание!!!&n %s ушел на пенсию и добровольно передал руководство дружины %s игроку %s.\r\n",
					GET_NAME(ch),
					CLAN(d->character)->GetAbbrev(),
					GET_PAD(d->character, 2));
		else
			sprintf(buf,
					"&RВнимание!!!&n %s ушла на пенсию и добровольно передала руководство дружины %s игроку %s.\r\n",
					GET_NAME(ch),
					CLAN(d->character)->GetAbbrev(),
					GET_PAD(d->character, 2));
		SendMsgToAll(buf);
	}
}

/**
* hcontrol owner vnum имя - передача воеводы клана внум игроку имя
* Новый воевода не может быть мембером другого клана, старый исключается из клана.
* Оба могут быть оффлайн во время выполнения команды иммом.
*/
void Clan::hcon_owner(CharData *ch, std::string &text) {
	std::string buffer;

	GetOneParam(text, buffer);
	const int vnum = atoi(buffer.c_str());

	ClanListType::iterator clan = std::find_if(ClanList.begin(), ClanList.end(),
											   [&](const Clan::shared_ptr &clan) {
												 return clan->rent == vnum;
											   });

	if (clan == Clan::ClanList.end()) {
		SendMsgToChar(ch, "Дружины с номером %d не существует.\r\n", vnum);
		return;
	}

	std::string name;
	GetOneParam(text, name);
	long member_uid = GetUniqueByName(name);

	if (!member_uid) {
		SendMsgToChar("Неизвестный персонаж.\r\n", ch);
		return;
	}

	name_convert(name);

	for (const auto &tmp_clan : Clan::ClanList) {
		const auto it = tmp_clan->m_members.find(member_uid);
		if (it != tmp_clan->m_members.end()) {
			if (vnum != tmp_clan->GetRent()) {
				SendMsgToChar(ch, "%s состоит в другой дружине.\r\n", name.c_str());
				return;
			} else if (!it->second->rank_num) {
				SendMsgToChar(ch, "%s и так является воеводой этой дружины.\r\n", name.c_str());
				return;
			}
		}
	}

	// убираем старого воеводу из клана
	for (const auto &it : (*clan)->m_members) {
		if (!it.second->rank_num) {
			const auto member_uid = it.first;
			// ахтунг, удаляется элемент дерева, по которому мы и идем в цикле
			char tmpstr[kMaxInputLength];
			sprintf(tmpstr, "смена воеводы");
			(*clan)->remove_member(member_uid, tmpstr);
			break;
		}
	}

	// вписываем нового
	if ((*clan)->m_members.find(member_uid) != (*clan)->m_members.end()) {
		// уже был в клане
		(*clan)->m_members.set_rank(member_uid, 0);
	} else {
		(*clan)->add_offline_member(name, member_uid, 0);
	}

	// новый воевода онлайн
	DescriptorData *d = DescriptorByUid(member_uid);
	if (d && d->character) {
		Clan::SetClanData(d->character.get());
		SendMsgToChar(d->character.get(), "%sВы стали новым воеводой дружины %s. Желаем удачи!%s\r\n",
					  kColorBoldGrn, (*clan)->get_abbrev().c_str(), kColorNrm);
	}
	if ((*clan)->m_members.size() > 0) {
		// оповещение
		for (DescriptorData *d = descriptor_list; d; d = d->next) {
			if (d->character && CLAN(d->character) && CLAN(d->character)->GetRent() == (*clan)->GetRent()) {
				SendMsgToChar(d->character.get(),
							  "%sОсуществлена принудительная смена воеводы вашей дружины: %s -> %s.%s\r\n",
							  kColorBoldGrn,
							  (*clan)->owner.c_str(),
							  name.c_str(),
							  kColorNrm);
			}
		}
	} else {
		(*clan)->builtOn = time(0);  //не ставит текущую дату
	}
	(*clan)->owner = name;
	Clan::ClanSave();
	SendMsgToChar("Сделано.\r\n", ch);
}

void Clan::CheckPkList(CharData *ch) {
	if (ch->IsNpc())
		return;
	ClanPkList::iterator it;
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan) {
		// пкл
		if ((it = (*clan)->pkList.find(ch->get_uid())) != (*clan)->pkList.end())
			SendMsgToChar(ch,
						  "Находитесь в списке врагов дружины '%s', добавивший: %s.\r\n",
						  (*clan)->name.c_str(),
						  it->second->authorName.c_str());
		// дрл
		if ((it = (*clan)->frList.find(ch->get_uid())) != (*clan)->frList.end())
			SendMsgToChar(ch,
						  "Находитесь в списке друзей дружины '%s', добавивший: %s.\r\n",
						  (*clan)->name.c_str(),
						  it->second->authorName.c_str());
	}
}

void Clan::HouseStat(CharData *ch, std::string &buffer) {
	std::string buffer2;
	GetOneParam(buffer, buffer2);
	bool all{false}, name{false};

	// параметр сортировки
	enum ESortParam {
	  SORT_STAT_BY_EXP,
	  SORT_STAT_BY_CLANEXP,
	  SORT_STAT_BY_MONEY,
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
	// клан стат [!опыт/!заработанным/!последнему/!имя] [имя/все]
	sortParameter = SORT_STAT_BY_EXP;
	if (buffer2.length() > 1) {
		if ((buffer2[0] == '!') && (utils::IsAbbr(buffer2.c_str() + 1, "опыту"))) // опыту дружине
			sortParameter = SORT_STAT_BY_CLANEXP;
		else if ((buffer2[0] == '!') && (utils::IsAbbr(buffer2.c_str() + 1, "заработанным")))
			sortParameter = SORT_STAT_BY_MONEY;
		else if ((buffer2[0] == '!') && (utils::IsAbbr(buffer2.c_str() + 1, "последнему")))
			sortParameter = SORT_STAT_BY_LOGON;
		else if ((buffer2[0] == '!') && (utils::IsAbbr(buffer2.c_str() + 1, "имя")))
			sortParameter = SORT_STAT_BY_NAME;

		// берем следующий параметр
		if (buffer2[0] == '!') GetOneParam(buffer, buffer2);
	}

	std::ostringstream out;
	out << kColorWht;
	if (CompareParam(buffer2, "очистить") || CompareParam(buffer2, "удалить")) {
		if (CLAN_MEMBER(ch)->rank_num) {
			SendMsgToChar("У вас нет прав удалять статистику.\r\n", ch);
			return;
		}
		// можно почистить тока деньги для удобства сбора с мемберов налога
		GetOneParam(buffer, buffer2);
		bool money = false;
		if (CompareParam(buffer2, "деньги") || CompareParam(buffer2, "money"))
			money = true;

		for (const auto &it : m_members) {
			it.second->money = 0;
			if (money) {
				continue;
			}
			it.second->exp = 0;
			it.second->clan_exp = 0;
		}

		if (money) {
			SendMsgToChar("Статистика доходов вашей дружины очищена.\r\n", ch);
		} else {
			SendMsgToChar("Статистика вашей дружины полностью очищена.\r\n", ch);
		}
		return;
	} else if (CompareParam(buffer2, "все")) {
		all = true;
		out << "Статистика вашей дружины ";
	} else if (!buffer2.empty()) {
		name = true;
		out << "Статистика вашей дружины (поиск по имени '" << buffer2 << "') ";
	} else {
		out << "Статистика вашей дружины (находящиеся онлайн) ";
	}

	// вывод режима сортировки
	out << "(сортировка по: ";
	switch (sortParameter) {
		case SORT_STAT_BY_EXP: out << "рейтинговым очкам";
			break;
		case SORT_STAT_BY_CLANEXP: out << "опыту дружине";
			break;
		case SORT_STAT_BY_MONEY: out << "заработанным кунам";
			break;
		case SORT_STAT_BY_LOGON: out << "последнему заходу в игру";
			break;
		case SORT_STAT_BY_NAME: out << "первой букве имени";
			break;

			// этого быть не должно
		default: out << "чему БОГ пошлет";
	}
	out << "):" << "\r\n";

	// multimap ибо могут быть совпадения
	std::multimap<long long, std::pair<std::string, ClanMember::shared_ptr> > temp_list;
	for (const auto &it : m_members) {
		it.second->level = 0;
		if (!all && !name) {
			DescriptorData *d = DescriptorByUid(it.first);
			if (!d) {
				continue;
			} else if (!IS_IMMORTAL(d->character)) {
				it.second->level = GetRealLevel(d->character);
				it.second->class_abbr = MUD::Class(d->character->GetClass()).GetAbbr();
				it.second->remort = GET_GOD_FLAG(d->character, EGf::kRemort) ? true : false;
				it.second->remorts_amount = d->character->get_remort();
			}
		} else if (name) {
			if (!CompareParam(buffer2, it.second->name)) {
				continue;
			}
		}
		char timeBuf[17];
		time_t tmp_time = GetLastlogonByUnique(it.first);
		if (tmp_time <= 0) tmp_time = time(nullptr);
		strftime(timeBuf, sizeof(timeBuf), "%d-%m-%Y", localtime(&tmp_time));

		// сортировка по...
		switch (sortParameter) {
			case SORT_STAT_BY_EXP: lSortParam = it.second->exp;
				break;
			case SORT_STAT_BY_CLANEXP: lSortParam = it.second->clan_exp;
				break;
			case SORT_STAT_BY_MONEY: lSortParam = it.second->money;
				break;
			case SORT_STAT_BY_LOGON: lSortParam = GetLastlogonByUnique(it.first);
				break;
			case SORT_STAT_BY_NAME: {
				pcFirstChar[0] = LOWER(it.second->name[0]);
				pcFirstChar[1] = '\0';
				char const *pTmp = strpbrk(pSortAlph, pcFirstChar);
				if (pTmp) lSortParam = pTmp - pSortAlph; // индекс первой буквы в массиве
				else lSortParam = pcFirstChar[0]; // или не русская буква или я хз
				break;
			}
				// на всякий случай
			default: lSortParam = it.second->exp;
		}

		temp_list.insert(std::make_pair(lSortParam, std::make_pair(std::string(timeBuf), it.second)));
	}

	table_wrapper::Table table;
	table << table_wrapper::kHeader << "Имя" << "Ур" << "Прв" << "Класс" << "Рейт. очков"
		  << "Опыта дружины" << "Внесено кун" << "Был в игре" << table_wrapper::kEndRow;
	std::string lvl_str;
	for (auto it = temp_list.rbegin(); it != temp_list.rend(); ++it) {
		table << it->second.second->name;
		lvl_str = (it->second.second->level > 0 ? std::to_string(it->second.second->level) : "--");
		lvl_str += (it->second.second->remort ? "+" : " ");
		table << lvl_str
			  << it->second.second->remorts_amount
			  << it->second.second->class_abbr
			  << PrintNumberByDigits(it->second.second->exp)
			  << PrintNumberByDigits(it->second.second->clan_exp)
			  << PrintNumberByDigits(it->second.second->money)
			  << it->second.first << table_wrapper::kEndRow;
	}
	table.SetColumnAlign(4, table_wrapper::align::kRight);
	table.SetColumnAlign(5, table_wrapper::align::kRight);
	table.SetColumnAlign(6, table_wrapper::align::kRight);

	table_wrapper::DecorateZebraTextTable(ch, table, table_wrapper::color::kGreen);
	table_wrapper::PrintTableToStream(out, table);

	page_string(ch->desc, out.str());
}

// напоминалка про кончину денег в казне дружины
void Clan::ChestInvoice() {
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan) {
		int cost = (*clan)->ChestTax() + (*clan)->ingr_chest_tax();
		cost += (*clan)->calculate_clan_tax();

		if (!cost) {
			continue; // чем черт не шутит
		}

		// опаньки
		if ((*clan)->bank >= cost) {
			continue;
		}

		for (DescriptorData *d = descriptor_list; d; d = d->next) {
			if (d->character && d->state == EConState::kPlaying
				&& !AFF_FLAGGED(d->character, EAffect::kDeafness)
				&& CLAN(d->character)
				&& CLAN(d->character) == *clan) {
				SendMsgToChar(d->character.get(),
							  "[Хранилище]: %s'Напоминаю, что средств в казне дружины хватит менее, чем на сутки!'%s\r\n",
							  kColorBoldRed,
							  kColorNrm);
			}
		}
	}
}

int Clan::ChestTax() {
	ObjData *temp, *chest;
	int cost = 0;
	int count = 0;
	for (chest = world[GetRoomRnum(this->chest_room)]->contents; chest; chest = chest->get_next_content()) {
		if (Clan::is_clan_chest(chest)) {
			// перебираем шмот
			for (temp = chest->get_contains(); temp; temp = temp->get_next_content()) {
				cost += temp->get_rent_on();
				++count;
			}
			this->chest_weight = chest->get_weight();
			break;
		}
	}
	this->chest_objcount = count;
	this->chest_discount = MAX(50, 75 - 5 * (this->clan_level));
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
bool Clan::ChestShow(ObjData *obj, CharData *ch) {
	if (!ch->desc || !Clan::is_clan_chest(obj)) {
		return false;
	}

	if (CLAN(ch) && GetRoomRnum(CLAN(ch)->chest_room) == obj->get_in_room()) {
		SendMsgToChar("Хранилище вашей дружины:\r\n", ch);
		int cost = CLAN(ch)->ChestTax();
		SendMsgToChar(ch,
					  "Всего вещей: %d, Рента в день: %d %s\r\n\r\n",
					  CLAN(ch)->chest_objcount,
					  cost,
					  GetDeclensionInNumber(cost, EWhat::kMoneyA));
		list_obj_to_char(obj->get_contains(), ch, 1, 3);
	} else {
		SendMsgToChar("Не на что тут глазеть, пусто, вот те крест.\r\n",
					  ch); // засланым казачкам показываем хер, а не хранилище
	}
	return true;
}

// +/- клан-экспы
void Clan::SetClanExp(CharData *ch, int add) {
	// шоб не читили
	if (GetRealLevel(ch) >= kLvlImmortal) {
		return;
	}

	// обнулять не надо минуса
	CLAN_MEMBER(ch)->clan_exp += add;

	this->clan_exp += add;
	if (this->clan_exp < 0) {
		this->clan_exp = 0;
	}

	if (this->clan_exp > clan_level_exp[this->clan_level + 1]
		&& this->clan_level < kMaxClanlevel) {
		this->clan_level++;
		for (DescriptorData *d = descriptor_list; d; d = d->next) {
			if (d->character && d->state == EConState::kPlaying
				&& !AFF_FLAGGED(d->character, EAffect::kDeafness)
				&& CLAN(d->character)
				&& CLAN(d->character)->GetRent() == this->rent) {
				SendMsgToChar(d->character.get(),
							  "&GВаш замок достиг нового, %d уровня! Поздравляем!&n\r\n",
							  this->clan_level);
			}
		}
	} else if (this->clan_exp < clan_level_exp[this->clan_level]
		&& this->clan_level > 0) {
		this->clan_level--;
		for (DescriptorData *d = descriptor_list; d; d = d->next) {
			if (d->character && d->state == EConState::kPlaying
				&& !AFF_FLAGGED(d->character, EAffect::kDeafness)
				&& CLAN(d->character)
				&& CLAN(d->character)->GetRent() == this->rent) {
				SendMsgToChar(d->character.get(),
							  "%sВаш замок потерял уровень! Теперь он %d уровня! Поздравляем!%s\r\n",
							  kColorBoldRed, this->clan_level,
							  kColorNrm);
			}
		}
	}
}

// добавление экспы для топа кланов и мемберу в зачетку
void Clan::AddTopExp(CharData *ch, int add_exp) {
	// шоб не читили
	if (GetRealLevel(ch) >= kLvlImmortal)
		return;

	CLAN_MEMBER(ch)->exp += add_exp;
	if (CLAN_MEMBER(ch)->exp < 0)
		CLAN_MEMBER(ch)->exp = 0;

	// в буффер или сразу в топ
	if (this->exp_info) {
		this->exp += add_exp;
		if (this->exp < 0)
			this->exp = 0;
	} else
		this->exp_buf += add_exp; // тут обнулять не надо
}

// синхронизация клановой экспы для топ с буффером, если есть режим запрета показа в ран-тайме
void Clan::SyncTopExp() {
	for (auto clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan)
		if (!(*clan)->exp_info) {
			(*clan)->exp += (*clan)->exp_buf;
			if ((*clan)->exp < 0)
				(*clan)->exp = 0;
			(*clan)->exp_buf = 0;
		}
}

// установка режима оповещения об изменениях в хранилище
void SetChestMode(CharData *ch, std::string &buffer) {
	if (ch->IsNpc())
		return;
	if (!CLAN(ch)) {
		SendMsgToChar("Для начала обзаведитесь дружиной.\r\n", ch);
		return;
	}
	if (CompareParam(buffer, "нет")) {
		ch->UnsetFlag(EPrf::kDecayMode);
		ch->UnsetFlag(EPrf::kTakeMode);
		SendMsgToChar("Ладушки.\r\n", ch);
	} else if (CompareParam(buffer, "рассыпание")) {
		ch->SetFlag(EPrf::kDecayMode);
		ch->UnsetFlag(EPrf::kTakeMode);
		SendMsgToChar("Ладушки.\r\n", ch);
	} else if (CompareParam(buffer, "изменение")) {
		ch->UnsetFlag(EPrf::kDecayMode);
		ch->SetFlag(EPrf::kTakeMode);
		SendMsgToChar("Ладушки.\r\n", ch);
	} else if (CompareParam(buffer, "полный")) {

		ch->SetFlag(EPrf::kDecayMode);
		ch->SetFlag(EPrf::kTakeMode);
		SendMsgToChar("Ладушки.\r\n", ch);
	} else {
		SendMsgToChar("Задается режим оповещения об изменениях в хранилище дружины.\r\n"
					  "Формат команды: режим хранилище <нет|рассыпание|изменение|полный>\r\n"
					  " нет - выключение канала хранилища\r\n"
					  " рассыпание - получать только сообщения о рассыпании вещей\r\n"
					  " изменение - получать только сообщения о взятии/добавлении вещей\r\n"
					  " полный - получать оба вида сообщений\r\n", ch);
		return;
	}
}

// шоб не засорять в режиме, а выдать строку сразу
std::string GetChestMode(CharData *ch) {
	if (ch->IsFlagged(EPrf::kDecayMode)) {
		if (ch->IsFlagged(EPrf::kTakeMode))
			return "полный";
		else
			return "рассыпание";
	} else if (ch->IsFlagged(EPrf::kTakeMode))
		return "изменение";
	else
		return "выкл";
}

// неужто реально глючит?
int Clan::GetRent() {
	return this->rent;
}

int Clan::GetOutRent() {
	return this->out_rent;
}

// * Удаление чара из клана, клан берется не через поля чара, а ищем по всем кланам
void Clan::remove_from_clan(long unique) {
	for (const auto &clan : Clan::ClanList) {
		const auto it = clan->m_members.find(unique);
		if (it != clan->m_members.end()) {
			clan->m_members.erase(it);
			return;
		}
	}
}

void Clan::init_chest_rnum() {
	CLAN_CHEST_RNUM = GetObjRnum(CLAN_CHEST_VNUM);
	INGR_CHEST_RNUM = GetObjRnum(INGR_CHEST_VNUM);
}

bool Clan::is_ingr_chest(ObjData *obj) {
	if (INGR_CHEST_RNUM < 0
		|| obj->get_rnum() != INGR_CHEST_RNUM) {
		return false;
	}
	return true;
}
bool Clan::is_clan_chest(ObjData *obj) {
	if (CLAN_CHEST_RNUM < 0
		|| obj->get_rnum() != CLAN_CHEST_RNUM) {
		return false;
	}
	return true;
}

bool Clan::is_clan_member(int unique) {
	return m_members.find(unique) != m_members.end();
}

bool Clan::is_alli_member(int unique) {
	for (const auto &clan : Clan::ClanList) {
		if (clan->rent == rent) {
			continue;
		}

		if (clan->is_clan_member(unique)
			&& clan->CheckPolitics(GetRent()) == kPoliticsAlliance) {
			return true;
		}
	}

	return false;
}

bool ClanSystem::is_ingr_chest(ObjData *obj) {
	if (INGR_CHEST_RNUM < 0
		|| obj->get_rnum() != INGR_CHEST_RNUM) {
		return false;
	}
	return true;
}

/**
* Оповещение соклановцев о входе/выходе друг друга.
* \param enter 1 - вход чара, 0 - выход.
*/
void Clan::clan_invoice(CharData *ch, bool enter) {
	if (ch->IsNpc() || !CLAN(ch)) {
		return;
	}

	for (DescriptorData *d = descriptor_list; d; d = d->next) {
		if (d->character
			&& d->character.get() != ch
			&& d->state == EConState::kPlaying
			&& CLAN(d->character) == CLAN(ch)
			&& d->character->IsFlagged(EPrf::kClanmembersMode)) {
			if (enter) {
				SendMsgToChar(d->character.get(), "%sДружинни%s %s вош%s в мир.%s\r\n",
							  kColorBoldBlk, IS_MALE(ch) ? "к" : "ца", GET_NAME(ch),
							  GET_CH_SUF_5(ch), kColorNrm);
			} else {
				SendMsgToChar(d->character.get(), "%sДружинни%s %s покинул%s мир.%s\r\n",
							  kColorBoldBlk, IS_MALE(ch) ? "к" : "ца", GET_NAME(ch),
							  GET_CH_SUF_1(ch), kColorNrm);
			}
		}
	}
}

// иммское где для кланхранилищ
std::string Clan::print_imm_where_obj(const ObjData *obj) {
	std::string str;
	for (Clan::shared_ptr ptr_clan : Clan::ClanList) {
		for (ObjData *chest = world[GetRoomRnum(ptr_clan->chest_room)]->contents; chest;
			 chest = chest->get_next_content()) {
			if (Clan::is_clan_chest(chest)) {
				for (ObjData *chest_content = chest->get_contains(); chest_content; chest_content = chest_content->get_next_content()) {
					if (obj->get_id() == chest_content->get_id()) {
						str = fmt::format("наход{}ся в хранилище дружины '{}'.\r\n",
								  GET_OBJ_POLY_1(ch, (chest_content)),
								  ptr_clan->GetAbbrev());
						return str;
					}
				}
			}
		}
	}
	return str;
}

int Clan::print_spell_locate_object(CharData *ch, int count, std::string name) {
	for (ClanListType::const_iterator clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan) {
		ObjData *temp, *chest;
		for (chest = world[GetRoomRnum((*clan)->chest_room)]->contents; chest; chest = chest->get_next_content()) {
			if (Clan::is_clan_chest(chest)) {
				for (temp = chest->get_contains(); temp; temp = temp->get_next_content()) {
					if (!IS_GOD(ch)) {
						if (number(1, 100) > (40 + MAX((GetRealInt(ch) - 25) * 2, 0))) {
							continue;
						}
						if (temp->has_flag(EObjFlag::kNolocate)) {
							continue;
						}
					}

					if (!isname(name, temp->get_aliases().c_str())) {
						continue;
					}

					sprintf(buf, "%s наход%sся в хранилище дружины '%s'.",
							temp->get_short_description().c_str(),
							GET_OBJ_POLY_1(ch, temp),
							(*clan)->GetAbbrev());
//					CAP(buf);
					if (IS_GRGOD(ch)) {
						sprintf(buf2, " Vnum предмета: %d", GET_OBJ_VNUM(temp));
						strcat(buf, buf2);
					}
					strcat(buf, "\r\n");
					SendMsgToChar(buf, ch);
					if (--count <= 0) {
						return count;
					}
				}
				break;
			}
		}
	}
	return count;
}

int Clan::GetClanWars(CharData *ch) {
	int result = 0, p1 = 0;
	if (ch->IsNpc() || !CLAN(ch)) {
		return false;
	}

	ClanListType::const_iterator clanVictim;
	for (clanVictim = Clan::ClanList.begin(); clanVictim != Clan::ClanList.end(); ++clanVictim) {
		if (*clanVictim == CLAN(ch)) {
			continue;
		}
		p1 = CLAN(ch)->CheckPolitics((*clanVictim)->rent);
		if (p1 == kPoliticsWar) result++;
	}

	return result;
}

void Clan::add_remember(std::string text, int flag) {
	std::string buffer = Remember::time_format();
	buffer += text;

	switch (flag) {
		case Remember::CLAN: remember_.push_back(buffer);
			if (remember_.size() > Remember::MAX_REMEMBER_NUM) {
				remember_.erase(remember_.begin());
			}
			break;

		case Remember::ALLY: remember_ally_.push_back(buffer);
			if (remember_ally_.size() > Remember::MAX_REMEMBER_NUM) {
				remember_ally_.erase(remember_ally_.begin());
			}
			break;

		default:
			log("SYSERROR: мы не должны были сюда попасть, flag: %d, func: %s",
				flag, __func__);
			break;
	}
}

std::string Clan::get_remember(unsigned int num, int flag) const {
	std::string buffer;

	switch (flag) {
		case Remember::CLAN: {
			Remember::RememberListType::const_iterator it = remember_.begin();
			if (remember_.size() > num) {
				std::advance(it, remember_.size() - num);
			}
			for (; it != remember_.end(); ++it) {
				buffer += *it;
			}
			break;
		}
		case Remember::ALLY: {
			Remember::RememberListType::const_iterator it = remember_ally_.begin();
			if (remember_ally_.size() > num) {
				std::advance(it, remember_ally_.size() - num);
			}
			for (; it != remember_ally_.end(); ++it) {
				buffer += *it;
			}
			break;
		}
		default:
			log("SYSERROR: мы не должны были сюда попасть, flag: %d, func: %s",
				flag, __func__);
			break;
	}
	if (buffer.empty()) {
		buffer = "Вам нечего вспомнить.\r\n";
	}
	return buffer;
}

std::string Clan::get_file_abbrev() const {
	std::string text = this->get_abbrev();
	for (unsigned i = 0; i != text.length(); ++i) {
		text[i] = LOWER(AtoL(text[i]));
	}
	return text;
}

void Clan::save_pk_log() {
	for (ClanListType::const_iterator i = Clan::ClanList.begin(); i != Clan::ClanList.end(); ++i) {
		(*i)->pk_log.save((*i)->get_file_abbrev());
	}
}

void Clan::init_ingr_chest() {
	if (!ingr_chest_active()) {
		return;
	}

	// на случай релоада
	for (ObjData *chest = world[get_ingr_chest_room_rnum()]->contents; chest; chest = chest->get_next_content()) {
		if (is_ingr_chest(chest)) {
			ObjData *obj_next;
			for (ObjData *temp = chest->get_contains(); temp; temp = obj_next) {
				obj_next = temp->get_next_content();
				RemoveObjFromObj(temp);
				ExtractObjFromWorld(temp);
			}
			ExtractObjFromWorld(chest);
			break;
		}
	}

	std::string file_abbrev = get_file_abbrev();
	std::string filename = LIB_HOUSE + file_abbrev + "/" + file_abbrev + ".ing";

	const auto chest = world_objects.create_from_prototype_by_vnum(INGR_CHEST_VNUM);
	if (!chest) {
		log("<Clan> IngrChest load error '%d'! (%s %s %d)", GetRent(), __FILE__, __func__, __LINE__);
		return;
	}
	//лоадим в комнату сам хран
	PlaceObjToRoom(chest.get(), get_ingr_chest_room_rnum());

	FILE *fl = fopen(filename.c_str(), "r+b");
	if (!fl) {
		return;
	}

	fseek(fl, 0L, SEEK_END);
	int fsize = ftell(fl);
	if (!fsize) {
		fclose(fl);
		log("<Clan> Empty file '%s'. (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
		return;
	}

	char *databuf = new char[fsize + 1];

	fseek(fl, 0L, SEEK_SET);
	if (!fread(databuf, fsize, 1, fl)
		|| ferror(fl)
		|| !databuf) {
		fclose(fl);
		log("<Clan> Error reading file '%s'. (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
		delete[] databuf;
		return;
	}
	fclose(fl);

	char *data = databuf;
	*(data + fsize) = '\0';

	int error = 0;
	std::list<ObjData *> obj_list;
	for (fsize = 0; *data && *data != '$'; fsize++) {
		const auto obj = read_one_object_new(&data, &error);
		if (!obj) {
			if (error) {
				log("<Clan> Items reading fail for %s error %d.", filename.c_str(), error);
			}
			continue;
		}
		obj_list.push_front(obj.get());
	}
	for (auto it : obj_list) {
		PlaceObjIntoObj(it, chest.get());
	}
	delete[] databuf;
}

// сохраняем храны ингров всех кланов в файлы
void ClanSystem::save_ingr_chests() {
	for (const auto &i : Clan::ClanList) {

		if (!i->ingr_chest_active()) {
			continue;
		}
		utils::CExecutionTimer timer;

		std::string file_abbrev = i->get_file_abbrev();
		std::string filename = LIB_HOUSE + file_abbrev + "/" + file_abbrev + ".ing";

		for (ObjData *chest = world[i->get_ingr_chest_room_rnum()]->contents; chest; chest = chest->get_next_content()) {

			if (!is_ingr_chest(chest)) {
				continue;
			}
			utils::CExecutionTimer timer;

			std::stringstream out;
			out << "* Items file\n";
			for (ObjData *temp = chest->get_contains(); temp; temp = temp->get_next_content()) {
				write_one_object(out, temp, 0);
			}
			out << "\n$\n$\n";
			std::ofstream file(filename.c_str());
			if (!file.is_open()) {
				log("Error open file: %s! (%s %s %d)", filename.c_str(), __FILE__, __func__, __LINE__);
				return;
			}
			file << out.rdbuf();
			file.close();
			break;
		}
		log(fmt::format("saving clan chest {} done, timer {:.10f}", i->GetAbbrev(), timer.delta().count()));
	}
}

bool Clan::put_ingr_chest(CharData *ch, ObjData *obj, ObjData *chest) {
	if (ch->IsNpc()
		|| !CLAN(ch)
		|| CLAN(ch)->GetRent() / 100 != GET_ROOM_VNUM(ch->in_room) / 100) {
		SendMsgToChar("Не имеете таких правов!\r\n", ch);
		return false;
	}

	if (obj->get_type() != EObjType::kMagicIngredient
		&& obj->get_type() != EObjType::kCraftMaterial) {
		SendMsgToChar(ch, "%s - Хранилище ингредиентов не предназначено для предметов данного типа.\r\n",
					  obj->get_PName(ECase::kNom).c_str());

		if (obj->get_type() == EObjType::kMoney) {
			int howmany = GET_OBJ_VAL(obj, 0);
			RemoveObjFromChar(obj);
			ExtractObjFromWorld(obj);
			ch->add_gold(howmany);
			SendMsgToChar(ch, "Вы вновь обрели %d %s.\r\n", howmany, GetDeclensionInNumber(howmany, EWhat::kMoneyU));
		}
	} else if (obj->has_flag(EObjFlag::kNodrop)
		|| obj->has_flag(EObjFlag::kZonedecay)
		|| obj->has_flag(EObjFlag::kRepopDecay)
		|| obj->has_flag(EObjFlag::kNorent)
		|| obj->get_rent_off() < 0
		|| obj->get_rnum() <= kNothing) {
		act("Неведомая сила помешала положить $o3 в $O3.", false, ch, obj, chest, kToChar);
	} else {
		if (CLAN(ch)->ingr_chest_objcount_ >= CLAN(ch)->ingr_chest_max_objects()) {
			act("Вы попытались запихнуть $o3 в $O3, но не смогли - там просто нет места.",
				false,
				ch,
				obj,
				chest,
				kToChar);
			return false;
		}
		dungeons::SwapOriginalObject(obj);
		RemoveObjFromChar(obj);
		PlaceObjIntoObj(obj, chest);
		act("Вы положили $o3 в $O3.", false, ch, obj, chest, kToChar);
		CLAN(ch)->ingr_chest_objcount_++;
	}
	return true;
}

bool Clan::take_ingr_chest(CharData *ch, ObjData *obj, ObjData *chest) {
	if (ch->IsNpc() || !CLAN(ch)
		|| CLAN(ch)->GetRent() / 100 != GET_ROOM_VNUM(ch->in_room) / 100) {
		SendMsgToChar("Не имеете таких правов!\r\n", ch);
		return false;
	}

	RemoveObjFromObj(obj);
	PlaceObjToInventory(obj, ch);
	if (obj->get_carried_by() == ch) {
		act("Вы взяли $o3 из $O1.", false, ch, obj, chest, kToChar);
		CLAN(ch)->ingr_chest_objcount_--;
	}
	return true;
}

bool ClanSystem::show_ingr_chest(ObjData *obj, CharData *ch) {
	if (!ch->desc || !is_ingr_chest(obj)) {
		return false;
	}

	if (CLAN(ch)
		&& CLAN(ch)->ingr_chest_active()
		&& CLAN(ch)->GetRent() / 100 == GET_ROOM_VNUM(ch->in_room) / 100) {
		SendMsgToChar("Хранилище ингредиентов вашей дружины:\r\n", ch);
		int cost = CLAN(ch)->ingr_chest_tax();
		SendMsgToChar(ch, "Всего вещей: %d/%d, Рента в день: %d %s\r\n\r\n",
					  CLAN(ch)->get_ingr_chest_objcount(), CLAN(ch)->ingr_chest_max_objects(),
					  cost, GetDeclensionInNumber(cost, EWhat::kMoneyA));
		list_obj_to_char(obj->get_contains(), ch, 1, 4);
	} else {
		SendMsgToChar("Не на что тут глазеть, пусто, вот те крест.\r\n", ch);
	}

	return true;
}

// * Расчет суточной ренты хранилища ингров.
int Clan::ingr_chest_tax() {
	if (!ingr_chest_active()) {
		return false;
	}

	int cost = 0;
	int count = 0;

	for (ObjData *chest = world[get_ingr_chest_room_rnum()]->contents; chest; chest = chest->get_next_content()) {
		if (is_ingr_chest(chest)) {
			for (ObjData *temp = chest->get_contains(); temp; temp = temp->get_next_content()) {
				cost += temp->get_rent_off();
				++count;
			}
			break;
		}
	}

	ingr_chest_objcount_ = count;
	return cost;
}

// * Очистка хранилища ингров при нулевом клан-банке.
void Clan::purge_ingr_chest() {
	if (!ingr_chest_active()) {
		return;
	}
	for (ObjData *chest = world[get_ingr_chest_room_rnum()]->contents; chest; chest = chest->get_next_content()) {
		if (is_ingr_chest(chest)) {
			ObjData *obj_next;
			for (ObjData *temp = chest->get_contains(); temp; temp = obj_next) {
				obj_next = temp->get_next_content();
				RemoveObjFromObj(temp);
				ExtractObjFromWorld(temp);
			}
			break;
		}
	}
}

int Clan::calculate_clan_tax() const {
	int cost = kClanTax + (storehouse * kClanStorehouseTax);

	if (ingr_chest_active()) {
		cost += INGR_CHEST_TAX;
	}

	return cost;
}

bool Clan::ingr_chest_active() const {
	if (ingr_chest_room_rnum_ > 0) {
		return true;
	}
	return false;
}

void Clan::set_ingr_chest(CharData *ch) {
	if (GetRent() / 100 != GET_ROOM_VNUM(ch->in_room) / 100) {
		SendMsgToChar("Данная комната находится вне зоны вашего замка.\r\n", ch);
		return;
	}

	bool chest_moved = false;
	// хран под ингры уже был
	if (ingr_chest_active()) {
		for (ObjData *chest = world[get_ingr_chest_room_rnum()]->contents; chest; chest = chest->get_next_content()) {
			if (is_ingr_chest(chest)) {
				RemoveObjFromRoom(chest);
				PlaceObjToRoom(chest, ch->in_room);
				chest_moved = true;
				break;
			}
		}
	}

	ingr_chest_room_rnum_ = ch->in_room;
	// Clan::ClanSave();

	if (!chest_moved) {
		const auto chest = world_objects.create_from_prototype_by_vnum(INGR_CHEST_VNUM);
		if (chest) {
			PlaceObjToRoom(chest.get(), get_ingr_chest_room_rnum());
		}
		SendMsgToChar("Хранилище установлено.\r\n", ch);
	} else {
		SendMsgToChar("Хранилище перенесено.\r\n", ch);
	}
}

void Clan::disable_ingr_chest(CharData *ch) {
	if (!ingr_chest_active()) {
		SendMsgToChar("У вас и так нет хранилища для ингредиентов.\r\n", ch);
		return;
	}

	for (ObjData *chest = world[get_ingr_chest_room_rnum()]->contents; chest; chest = chest->get_next_content()) {
		if (is_ingr_chest(chest)) {
			if (chest->get_contains()) {
				SendMsgToChar("Во избежание недоразумений отключить можно только пустое хранилище.\r\n", ch);
				return;
			}
			ExtractObjFromWorld(chest);
			break;
		}
	}
	ingr_chest_room_rnum_ = -1;
	SendMsgToChar("Хранилище отключено.\r\n", ch);
}

int Clan::ingr_chest_max_objects() {
	return 600 * MAX(1, this->clan_level) + this->last_exp.get_exp() / 10000000;
}

////////////////////////////////////////////////////////////////////////////////

namespace ClanSystem {

void save_chest_log() {
	for (const auto &i : Clan::ClanList) {
		i->chest_log.save(i->get_file_abbrev());
	}
}

// * Генерация справки 'сайтыдружин'.
void init_xhelp() {
	std::stringstream out;
	out << "  В данном разделе приведены адреса сайтов,  принадлежащим той или иной дружине.\r\n"
		   "Как  правило,  на  подобных  сайтах  вы  можете  ознакомиться с уставом дружины,\r\n"
		   "узнать условия вступления в дружину, а также получить довольно много  информации\r\n"
		   "о  политике дружины, ее составе, важных событиях, участие в которых принимали ее\r\n"
		   "ратники и многое другое.\r\n\r\n"
		   "  Список сайтов дружин:\r\n\r\n";

	for (const auto &i : Clan::ClanList) {
		out << "    $COLORW" << std::setw(7) << std::left
			<< i->GetAbbrev() << "$COLORn --   $COLORC"
			<< (i->get_web_url().empty() ? "$COLORW[ НЕТ ИНФОРМАЦИИ ]" : i->get_web_url())
			<< "$COLORn\r\n";
	}

	out << "\r\n  Официальный сайт мада МПМ Былины:$COLORc www.mud.ru$COLORn\r\n"
		<< "  Сайт истории мада МПМ Былины:$COLORc mudhistory.nm.ru$COLORn\r\n"
		<< "\r\nСм. также:$COLORC ДРУЖИНЫ $COLORn\r\n";

	HelpSystem::add_dynamic("САЙТЫДРУЖИН", out.str());
	HelpSystem::add_dynamic("CLANSITES", out.str());
	HelpSystem::add_dynamic("INTERNETLINKS", out.str());
}

const char *GOLD_TAX_FORMAT =
	"Формат команды: клан налог <число от 0 до 25>\r\n"
	"Устанавливает процент автоматических отчислений в казну дружины.\r\n";

void tax_manage(CharData *ch, std::string &buffer) {
	if (!CLAN(ch)) return;

	utils::Trim(buffer);
	if (!buffer.empty()) {
		try {
			int tax = std::stoi(buffer, nullptr, 10);
			if (tax <= MAX_GOLD_TAX_PCT) {
				CLAN(ch)->set_gold_tax_pct(tax);
				SendMsgToChar(ch, "Налог для ратников дружины установлен в %d%%\r\n", tax);
			} else {
				SendMsgToChar(GOLD_TAX_FORMAT, ch);
			}
		}
		catch (const std::invalid_argument &) {
			SendMsgToChar(GOLD_TAX_FORMAT, ch);
		}
	} else {
		SendMsgToChar(ch, "Текущий налог для ратников дружины: %ld%%\r\n%s",
					  CLAN(ch)->get_gold_tax_pct(), GOLD_TAX_FORMAT);
	}
}

long do_gold_tax(CharData *ch, long gold) {
	if (gold >= MIN_GOLD_TAX_AMOUNT
#ifndef TEST_BUILD
		&& !IS_IMMORTAL(ch)
#endif
		&& CLAN(ch) && CLAN(ch)->get_gold_tax_pct() > 0
		&& CLAN_MEMBER(ch)) {
		const long tax = (gold * CLAN(ch)->get_gold_tax_pct()) / 100;
		if (tax <= 0) return false;

		// TODO: мб вынести как в desc_count для версии с окончаниями?
		if ((tax % 100 >= 11 && tax % 100 <= 14)
			|| tax % 10 >= 5
			|| tax % 10 == 0) {
			SendMsgToChar(ch,
						  "%ld %s было немедленно отправлено в казну вашей дружины.\r\n",
						  tax, GetDeclensionInNumber(tax, EWhat::kMoneyA));
		} else if (tax % 10 == 1) {
			SendMsgToChar(ch,
						  "%ld %s была немедленно отправлена в казну вашей дружины.\r\n",
						  tax, GetDeclensionInNumber(tax, EWhat::kMoneyA));
		} else {
			SendMsgToChar(ch,
						  "%ld %s были немедленно отправлены в казну вашей дружины.\r\n",
						  tax, GetDeclensionInNumber(tax, EWhat::kMoneyA));
		}
		// 1 куну за транзакцию, если сумма налога позволяет
		const long real_tax = tax > 1 ? tax - 1 : tax;
		CLAN(ch)->set_bank(CLAN(ch)->get_bank() + real_tax);
		CLAN_MEMBER(ch)->money += real_tax;
		// возврат полного налога, снятого с чара
		return tax;
	}
	return false;
}

} // ClanSystem

////////////////////////////////////////////////////////////////////////////////

// клан сайт <текст>
void Clan::house_web_url(CharData *ch, const std::string &buffer) {
	const unsigned MAX_URL_LENGTH = 40;
	std::istringstream tmp(buffer);
	std::string url;
	tmp >> url;

	if (url.size() > MAX_URL_LENGTH) {
		url = url.substr(0, MAX_URL_LENGTH);
		SendMsgToChar(ch, "Строка была обрезана до %u символов.\r\n", MAX_URL_LENGTH);
	}

	if (url.empty()) {
		SendMsgToChar("Адрес сайта вашей дружины удален.\r\n"
					  "Обновление справки 'сайтыдружин' состоится в течении минуты.\r\n", ch);
		this->web_url_.clear();
	} else {
		this->web_url_ = url;
		SendMsgToChar("Адрес сайта вашей дружины установлен.\r\n"
					  "Обновление справки 'сайтыдружин' состоится в течении минуты.\r\n", ch);

		snprintf(buf, sizeof(buf), "%s sets new clan website: %s", GET_NAME(ch), url.c_str());
		mudlog(buf, LGH, kLvlImmortal, SYSLOG, true);
	}

	HelpSystem::need_update = true;
}

// для использования с кланами (в "клан лог", сообщениях хранилища и т.д.):
// возвращает клан-метку с ведущим пробелом
std::string clan_get_custom_label(ObjData *obj, Clan::shared_ptr clan) {
	if (obj->get_custom_label()
		&& obj->get_custom_label()->text_label
		&& obj->get_custom_label()->clan_abbrev
		&& clan
		&& !strcmp(clan->GetAbbrev(), obj->get_custom_label()->clan_abbrev)) {
		return fmt::format(" *{}*", obj->get_custom_label()->text_label);
	} else {
		return "";
	}
}
bool is_alliance_by_abbr(const CharData *ch, char *abbrev) {
	if (!str_cmp(CLAN(ch)->get_abbrev().c_str(), abbrev))
		return true;
	for (auto &ClanTmp : Clan::ClanList) {
		if ((CLAN(ch)->CheckPolitics(ClanTmp->GetRent()) == kPoliticsAlliance
			&& ClanTmp->CheckPolitics(CLAN(ch)->GetRent()) == kPoliticsAlliance
			&& !str_cmp(ClanTmp->get_abbrev().c_str(), abbrev))) {
			return true;
		}
	}
	return false;
}

bool CHECK_CUSTOM_LABEL_CORE(const ObjData *obj, const CharData *ch) {
	return (obj->get_custom_label()->author == (ch)->get_uid()
		&& !(obj->get_custom_label()->clan_abbrev))
		|| IS_IMPL(ch)
		|| ((ch)->player_specials->clan
			&& obj->get_custom_label()->clan_abbrev != nullptr
			&& is_alliance_by_abbr(ch, obj->get_custom_label()->clan_abbrev))
		|| (obj->get_custom_label()->author_mail
			&& !strcmp(GET_EMAIL(ch), obj->get_custom_label()->author_mail));
}

bool CHECK_CUSTOM_LABEL(const char *arg, const ObjData *obj, const CharData *ch) {
	return obj->get_custom_label()
		&& obj->get_custom_label()->text_label
		&& (ch->IsNpc()
			? ((IS_CHARMICE(ch) && ch->has_master())
			   ? CHECK_CUSTOM_LABEL_CORE(obj, ch->get_master())
			   : 0)
			: CHECK_CUSTOM_LABEL_CORE(obj, ch))
		&& isname(arg, obj->get_custom_label()->text_label);
}

bool AUTH_CUSTOM_LABEL(const ObjData *obj, const CharData *ch) {
	return obj->get_custom_label()
		&& obj->get_custom_label()->text_label
		&& (ch->IsNpc()
			? ((IS_CHARMICE(ch) && ch->has_master())
			   ? CHECK_CUSTOM_LABEL_CORE(obj, ch->get_master())
			   : 0)
			: CHECK_CUSTOM_LABEL_CORE(obj, ch));
}

void Clan::set_gold_tax_pct(long num) {
	gold_tax_pct_ = num;
}

long Clan::get_gold_tax_pct() const {
	return gold_tax_pct_;
}

void Clan::set_bank(long num) {
	bank = num;
}

long Clan::get_bank() const {
	return bank;
}

// проверка, находится ли левый чар в замке,
// если да, то выпинываем его оттуда
void ClanSystem::check_player_in_house() {
	for (auto d = descriptor_list; d; d = d->next) {
		if (d->character
			&& (!Clan::MayEnter(d->character.get(), d->character->in_room, kHouseAtrium))) {
			const auto clan = Clan::GetClanByRoom(d->character->in_room);
			if (clan) {
				char_from_room(d->character);
				act("$n был$g выдворен$a за пределы замка!", true, d->character.get(), 0, 0, kToRoom);
				SendMsgToChar("Вы были выдворены за пределы замка!\r\n", d->character.get());
				char_to_room(d->character, GetRoomRnum(clan->GetOutRent()));
				look_at_room(d->character.get(), GetRoomRnum(clan->GetOutRent()));
				act("$n свалил$u с небес, выкрикивая какие-то ругательства!", true, d->character.get(), 0, 0, kToRoom);
			}
		}
	}
}

// показывает, является ли чар союзником такой-то дружине
bool ClanSystem::is_alliance(CharData *ch, char *clan_abbr) {
	std::string abbrev = clan_abbr;
	if (!CLAN(ch)) {
		return false;
	}
	utils::Trim(abbrev);
	for (const auto &clan : Clan::ClanList) {
		if (CompareParam(abbrev, clan->get_abbrev())) {
			if (clan == CLAN(ch)) {
				return true;
			}

			if (clan->CheckPolitics(CLAN(ch)->GetRent()) == kPoliticsAlliance
				&& CLAN(ch)->CheckPolitics(clan->GetRent()) == kPoliticsAlliance) {
				return true;
			}
		}
	}

	return false;
}

namespace ClanSystem {

void DoShowWars(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc()) return;
	std::string buffer = argument;
	std::ostringstream buffer3;
	buffer3 << "Дружины, находящиеся в состоянии войны:\r\n";
	if (!buffer.empty()) {
		Clan::ClanListType::const_iterator clan1;
		for (clan1 = Clan::ClanList.begin(); clan1 != Clan::ClanList.end(); ++clan1) {
			if (CompareParam(buffer, (*clan1)->abbrev)) {
				break;
			}
		}

		if (clan1 == Clan::ClanList.end() || (*clan1)->m_members.size() == 0) {
			SendMsgToChar("Такая дружина не зарегистрирована\r\n", ch);
			return;
		}

		Clan::ClanListType::const_iterator clan2;
		for (clan2 = Clan::ClanList.begin(); clan2 != Clan::ClanList.end(); ++clan2) {
			if (clan2 == clan1) {
				continue;
			}

			if ((*clan1)->CheckPolitics((*clan2)->rent) == kPoliticsWar) {
				buffer3 << " " << (*clan1)->abbrev << " против " << (*clan2)->abbrev << "\r\n";
			}
		}
	} else {
		for (const auto &clan1 : Clan::ClanList) {
			for (const auto &clan2 : Clan::ClanList) {
				if (clan2 == clan1) {
					continue;
				}

				if (clan1->CheckPolitics(clan2->rent) == kPoliticsWar) {
					buffer3 << " " << clan1->abbrev << " против " << clan2->abbrev << "\r\n";
				}
			}
		}
	}
	buffer3 << "\r\n";
	SendMsgToChar(buffer3.str(), ch);

}

// обработка клан-канала и канала союзников, как игрока, так и имма
// клановые БОГи ниже 34 не могут говорить другим дружинам, и им и остальным спокойнее
// для канала союзников нужен обоюдный альянс дружин
void DoClanChannel(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	if (ch->IsNpc())
		return;

	std::string buffer = argument;

	// большой неклановый или 34 клановый БОГ говорит какой-то дружине
	if (IS_IMPL(ch) || (IS_GRGOD(ch) && !CLAN(ch))) {
		std::string buffer2;
		GetOneParam(buffer, buffer2);

		Clan::ClanListType::const_iterator clan;
		for (clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan) {
			if (CompareParam((*clan)->abbrev, buffer2, true)) {
				break;    // found
			}
		}

		if (clan == Clan::ClanList.end()) {
			if (!CLAN(ch)) // неклановый 34 ошибся аббревиатурой
				SendMsgToChar("Дружина с такой аббревиатурой не найдена.\r\n", ch);
			else   // клановый 34 ошибиться не может, идет в его клан-канал
			{
				buffer = argument; // финт ушами
				CLAN(ch)->CharToChannel(ch, buffer, subcmd);
			}
			return;
		}

		(*clan)->GodToChannel(ch, buffer, subcmd);
		// остальные говорят только в свою дружину
	} else {
		if (!CLAN(ch)) {
			SendMsgToChar("Вы не принадлежите ни к одной дружине.\r\n", ch);
			return;
		}

		// ограничения на клан-канал не канают на любое звание, если это БОГ
		if (!IS_IMMORTAL(ch)
			&& (!(CLAN(ch))->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_CHANNEL]
				|| ch->IsFlagged(EPlrFlag::kDumbed))) {
			SendMsgToChar("Вы не можете пользоваться каналом дружины.\r\n", ch);
			return;
		}

		CLAN(ch)->CharToChannel(ch, buffer, subcmd);
	}
}

void DoClanList(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc()) {
		return;
	}
	std::string buffer = argument;
	if (buffer.empty()) {
		// сортировка кланов по экспе
		std::multimap<long long, Clan::shared_ptr> sort_clan;
		for (const auto &clan : Clan::ClanList) {
			sort_clan.insert(std::make_pair(clan->exp, clan));
		}
		std::ostringstream out;
		// \todo Тут нужно использовать table_wrapper::Table а не формат.
		std::string_view clanTopFormat{" {:5}  {:6}   {:<30} {:14}{:14} {:9}\r\n"};
		out << "В игре зарегистрированы следующие дружины:\r\n"
			<< "     #           Название                       Всего опыта   За 30 дней         Человек\r\n\r\n";
		int count = 1;
		for (const auto &it : reverse(sort_clan)) {
			if (it.second->m_members.size() == 0) {
				continue;
			}
			const auto &clan = it.second;
			if (clan->is_pk())
				out << fmt::format(fmt::runtime(clanTopFormat), count, clan->abbrev, clan->name, ExpFormat(clan->exp),
								   ExpFormat(clan->last_exp.get_exp()), clan->m_members.size());
			else
				out << "&g"
					<< fmt::format(fmt::runtime(clanTopFormat), count, clan->abbrev, clan->name, ExpFormat(clan->exp),
								   ExpFormat(clan->last_exp.get_exp()), clan->m_members.size()) << "&n";
			++count;
		}
		SendMsgToChar(out.str(), ch);
		return;
	}

	bool all = false;

	Clan::ClanListType::const_iterator clan;
	for (clan = Clan::ClanList.begin(); clan != Clan::ClanList.end(); ++clan) {
		if (CompareParam(buffer, (*clan)->abbrev) && !((*clan)->m_members.size() == 0)) {
			break;
		}
	}

	if (clan == Clan::ClanList.end()) {
		if (CompareParam(buffer, "все")) {
			all = true;
		} else {
			SendMsgToChar("Такая дружина не зарегистрирована\r\n", ch);
			return;
		}
	}

	// спам-контроль применяем только если запросили полный список или не свою дружину
	if (all || !ch->player_specials->clan || !CompareParam(ch->player_specials->clan->GetAbbrev(), (*clan)->abbrev)) {
		if (PerformWhoSpamcontrol(ch, kWhoListclan)) {
			return;
		}
	}

	// строится список членов дружины или всех дружин (по флагу all)
	std::vector<CharData::shared_ptr> temp_list;
	for (auto d = descriptor_list; d; d = d->next) {
		if (d->character
			&& d->character->in_room != kNowhere
			&& CLAN(d->character)
			&& CAN_SEE_CHAR(ch, d->character)
			&& !IS_IMMORTAL(d->character)
			&& !d->character->IsFlagged(EPrf::kCoderinfo)
			&& (all || CLAN(d->character) == *clan)) {
			temp_list.push_back(d->character);
		}
	}

	// до кучи сортировка по рангам
	std::sort(temp_list.begin(), temp_list.end(), SortRank());

	std::ostringstream buffer2;
	buffer2 << "В игре зарегистрированы следующие дружины:\r\n" << "     #                  Глава Название\r\n\r\n";
	std::string_view clanFormat{" {:5}  {:6} {:15} {}\r\n"};
	std::string_view memberFormat{" {:>10} {}{}{} {}{}{}\r\n"};
	// если искали конкретную дружину - выводим ее
	if (!all) {
		buffer2 << fmt::format(fmt::runtime(clanFormat), 1, (*clan)->abbrev, (*clan)->owner, (*clan)->name);
		for (const auto &it : temp_list) {
			buffer2 << fmt::format(fmt::runtime(memberFormat), (IS_MALE(it) ? (*clan)->ranks[CLAN_MEMBER(it)->rank_num]
																			: (*clan)->ranks_female[CLAN_MEMBER(it)->rank_num]),
								   GetPkNameColor(it), (it)->GetNameWithTitleOrRace(),
								   kColorNrm, kColorBoldRed,
								   (it->IsFlagged(EPlrFlag::kKiller) ? "(ДУШЕГУБ)" : ""),
								   kColorNrm);
		}
	}
		// просто выводим все дружины и всех членов (без параметра 'все' в списке будут только дружины)
	else {
		int count = 1;
		for (auto clan_i = Clan::ClanList.begin(); clan_i != Clan::ClanList.end(); ++clan_i) {
			if ((*clan_i)->m_members.size() == 0) {
				continue;
			}

			buffer2 << fmt::format(fmt::runtime(clanFormat),
								   count, (*clan_i)->abbrev, (*clan_i)->owner, (*clan_i)->name);
			for (const auto &it : temp_list) {
				if (CLAN(it) == *clan_i) {
					buffer2 << fmt::format(fmt::runtime(memberFormat), (*clan_i)->ranks[CLAN_MEMBER(it)->rank_num],
										   GetPkNameColor(it), it->GetNameWithTitleOrRace(),
										   kColorNrm, kColorBoldRed,
										   (it->IsFlagged(EPlrFlag::kKiller) ? "(ДУШЕГУБ)" : ""),
										   kColorNrm);
				}
			}
			++count;
		}
	}

	buffer2 << "\r\nВсего игроков - " << temp_list.size() << "\r\n";
	SendMsgToChar(buffer2.str(), ch);
}

void DoClanPkList(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	if (ch->IsNpc() || !CLAN(ch)) {
		SendMsgToChar("Чаво?\r\n", ch);
		return;
	}
	std::string buffer = argument, buffer2;
	GetOneParam(buffer, buffer2);
	std::ostringstream info;

	//std::string frmt{"%s [%s] :: %s\r\n%s\r\n\r\n"};
	if (buffer2.empty()) {
		// выводим список тех, кто онлайн
		SendMsgToChar(ch,
					  "%sОтображаются только находящиеся в игре персонажи:%s\r\n\r\n",
					  kColorWht,
					  kColorNrm);
		ClanPkList::const_iterator it;
		// вобщем чтобы словить чаров, находящихся в лд - придется гонять по чарактер-листу
		for (const auto &tch : character_list) {
			if (tch->IsNpc())
				continue;
			// пкл
			if (!subcmd) {
				it = CLAN(ch)->pkList.find(tch->get_uid());
				if (it != CLAN(ch)->pkList.end())
					print_pkl(ch, info, it);
			}
				// дрл
			else {
				it = CLAN(ch)->frList.find(tch->get_uid());
				if (it != CLAN(ch)->frList.end())
					print_pkl(ch, info, it);
			}
		}
		if (info.str().empty())
			info << "Записи в выбранном списке отсутствуют.\r\n";
		page_string(ch->desc, info.str());

	} else if (CompareParam(buffer2, "все") || CompareParam(buffer2, "all")) {
		// выводим весь список
		SendMsgToChar(ch, "%sСписок отображается полностью:%s\r\n\r\n", kColorWht, kColorNrm);
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

		page_string(ch->desc, info.str());

	} else if ((CompareParam(buffer2, "добавить") || CompareParam(buffer2, "add"))
		&& CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_PKLIST]) {
		// добавляем нового
		GetOneParam(buffer, buffer2);
		if (buffer2.empty()) {
			SendMsgToChar("Кого добавить то?\r\n", ch);
			return;
		}
		long unique = GetUniqueByName(buffer2, 1);

		if (!unique) {
			SendMsgToChar("Интересующий вас персонаж не найден.\r\n", ch);
			return;
		}
		if (unique < 0) {
			SendMsgToChar("Не дело это, Богов добавлять куда не надо....\r\n", ch);
			return;
		}
		const auto it = CLAN(ch)->m_members.find(unique);
		if (it != CLAN(ch)->m_members.end()) {
			SendMsgToChar("Давайте не будем засорять список всяким бредом?\r\n", ch);
			return;
		}
		if (buffer.empty()) {
			SendMsgToChar("Потрудитесь прокомментировать, за что вы его так.\r\n", ch);
			return;
		}

		// тож надо проверять
		ClanPkList::iterator it2;
		if (!subcmd) {
			it2 = CLAN(ch)->pkList.find(unique);
		} else {
			it2 = CLAN(ch)->frList.find(unique);
		}

		if ((!subcmd && it2 != CLAN(ch)->pkList.end())
			|| (subcmd && it2 != CLAN(ch)->frList.end())) {
			// уид тут саавсем не обязательно, что валидный
			const auto rank_it = CLAN(ch)->m_members.find(it2->second->author);
			if (rank_it != CLAN(ch)->m_members.end()
				&& rank_it->second->rank_num < CLAN_MEMBER(ch)->rank_num) {
				if (!subcmd) {
					SendMsgToChar("Ваша жертва уже добавлена в список врагов старшим по званию.\r\n", ch);
				} else {
					SendMsgToChar("Персонаж уже добавлен в список друзей старшим по званию.\r\n", ch);
				}
				return;
			}
		}

		// собственно пишем новую жертву/друга
		ClanPkPtr tempRecord(new ClanPk);
		tempRecord->author = ch->get_uid();
		tempRecord->authorName = GET_NAME(ch);
		tempRecord->victimName = buffer2;
		name_convert(tempRecord->victimName);
		tempRecord->time = time(nullptr);
		tempRecord->text = buffer;
		if (!subcmd)
			CLAN(ch)->pkList[unique] = tempRecord;
		else
			CLAN(ch)->frList[unique] = tempRecord;

		DescriptorData *d = DescriptorByUid(unique);
		if (d && d->character->IsFlagged(EPrf::kPklMode)) {
			if (!subcmd) {
				SendMsgToChar(d->character.get(),
							  "%sДружина '%s' добавила вас в список своих врагов!%s\r\n",
							  kColorBoldRed,
							  CLAN(ch)->name.c_str(),
							  kColorNrm);
			} else {
				SendMsgToChar(d->character.get(),
							  "%sДружина '%s' добавила вас в список своих друзей!%s\r\n",
							  kColorGrn,
							  CLAN(ch)->name.c_str(),
							  kColorNrm);
			}
			SetWait(ch, 1, false);
		}

		SendMsgToChar("Ладушки, добавили.\r\n", ch);
	} else if ((CompareParam(buffer2, "удалить") || CompareParam(buffer2, "delete"))
		&& CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_PKLIST]) {
		// удаление записи
		GetOneParam(buffer, buffer2);
		if (CompareParam(buffer2, "все", 1) || CompareParam(buffer2, "all", 1)) {
			if (CLAN_MEMBER(ch)->rank_num) {
				SendMsgToChar("Полная очистка списка доступна только воеводе.\r\n", ch);
				return;
			}
			// пкл
			if (!subcmd)
				CLAN(ch)->pkList.erase(CLAN(ch)->pkList.begin(), CLAN(ch)->pkList.end());
				// дрл
			else
				CLAN(ch)->frList.erase(CLAN(ch)->frList.begin(), CLAN(ch)->frList.end());
			SendMsgToChar("Список очищен.\r\n", ch);
			return;
		}
		long unique = GetUniqueByName(buffer2, true);

		if (unique <= 0) {
			SendMsgToChar("Интересующий вас персонаж не найден.\r\n", ch);
			return;
		}
		// пкл, раздельно они мне больше нравятся, чем по пять раз subcmd проверять
		bool removed = false;
		if (!subcmd) {
			ClanPkList::iterator it;
			it = CLAN(ch)->pkList.find(unique);
			if (it != CLAN(ch)->pkList.end()) {
				const auto pk_rank_it = CLAN(ch)->m_members.find(it->second->author);
				if (pk_rank_it != CLAN(ch)->m_members.end()
					&& pk_rank_it->second->rank_num < CLAN_MEMBER(ch)->rank_num) {
					SendMsgToChar("Ваша жертва была добавлена старшим по званию.\r\n", ch);
					return;
				}
				CLAN(ch)->pkList.erase(it);
				removed = true;
			}
			// дрл
		} else {
			ClanPkList::iterator it;
			it = CLAN(ch)->frList.find(unique);
			if (it != CLAN(ch)->frList.end()) {
				const auto fr_rank_it = CLAN(ch)->m_members.find(it->second->author);
				if (fr_rank_it != CLAN(ch)->m_members.end()
					&& fr_rank_it->second->rank_num < CLAN_MEMBER(ch)->rank_num) {
					SendMsgToChar("Персонаж был добавлен старшим по званию.\r\n", ch);
					return;
				}
				CLAN(ch)->frList.erase(it);
				removed = true;
			}
		}

		if (removed) {
			SendMsgToChar("Запись удалена.\r\n", ch);
			DescriptorData *d;
			if ((d = DescriptorByUid(unique))
				&& d->character->IsFlagged(EPrf::kPklMode)) {
				if (!subcmd) {
					SendMsgToChar(d->character.get(),
								  "%sДружина '%s' удалила вас из списка своих врагов!%s\r\n",
								  kColorGrn,
								  CLAN(ch)->name.c_str(),
								  kColorNrm);
				} else {
					SendMsgToChar(d->character.get(),
								  "%sДружина '%s' удалила вас из списка своих друзей!%s\r\n",
								  kColorBoldRed,
								  CLAN(ch)->name.c_str(),
								  kColorNrm);
				}
				SetWait(ch, 1, false);
			}
		} else {
			SendMsgToChar("Запись не найдена.\r\n", ch);
		}
	} else {
		utils::Trim(buffer);
		bool online = 1;

		if (CompareParam(buffer, "all") || CompareParam(buffer, "все"))
			online = 0;

		if (online)
			SendMsgToChar(ch,
						  "%sОтображаются только находящиеся в игре персонажи:%s\r\n\r\n",
						  kColorWht,
						  kColorNrm);
		else
			SendMsgToChar(ch, "%sСписок отображается полностью:%s\r\n\r\n", kColorWht, kColorNrm);

		std::ostringstream out;

		if (!subcmd) {
			for (ClanPkList::const_iterator it = CLAN(ch)->pkList.begin(); it != CLAN(ch)->pkList.end(); ++it) {
				if (CompareParam(buffer2, it->second->victimName, 1)) {
					if (!online || check_online_state(it->first))
						print_pkl(ch, out, it);
				}
			}
		} else {
			for (ClanPkList::const_iterator it = CLAN(ch)->frList.begin(); it != CLAN(ch)->frList.end(); ++it) {
				if (CompareParam(buffer2, it->second->victimName, 1)) {
					if (!online || check_online_state(it->first))
						print_pkl(ch, out, it);
				}
			}
		}
		if (!out.str().empty())
			page_string(ch->desc, out.str());
		else {
			SendMsgToChar("По вашему запросу никого не найдено.\r\n", ch);
			if (CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_PKLIST]) {
				buffer = CLAN_PKLIST_FORMAT[0];
				buffer += CLAN_PKLIST_FORMAT[1];
				SendMsgToChar(buffer, ch);
			} else
				SendMsgToChar(CLAN_PKLIST_FORMAT[0], ch);
		}
	}
}

// обработка клановых привилегий (команда house)
void DoHouse(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc())
		return;

	std::string buffer = argument, buffer2;
	GetOneParam(buffer, buffer2);

	// если игрок неклановый, то есть вариант с приглашением
	if (!CLAN(ch)) {
		if (CompareParam(buffer2, "согласен") && ch->desc->clan_invite)
			ch->desc->clan_invite->clan->ClanAddMember(ch,
													   ch->desc->clan_invite->rank,
													   ch->desc->clan_invite->invite_name);
		else if (CompareParam(buffer2, "отказать") && ch->desc->clan_invite) {
			ch->desc->clan_invite.reset();
			SendMsgToChar("Приглашение дружины отклонено.\r\n", ch);
			return;
		} else
			SendMsgToChar("Данная команда доступна только высоко привилегированным членам дружин,\r\n"
						  "а если вас пригласили вступить в оную, то так и пишите 'клан согласен' или 'клан отказать'.\r\n"
						  "Никто не сможет послать вам новое приглашение до тех пор, пока вы не разберетесь с этим.\r\n",
						  ch);
		return;
	}

	if (CompareParam(buffer2, "информация") && CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_INFO])
		CLAN(ch)->HouseInfo(ch);
	else if (CompareParam(buffer2, "принять") && CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_ADD])
		CLAN(ch)->HouseAdd(ch, buffer);
	else if (CompareParam(buffer2, "изгнать") && CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_REMOVE])
		CLAN(ch)->HouseRemove(ch, buffer);
	else if (CompareParam(buffer2, "привилегии")
		&& CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_PRIVILEGES]) {
		std::shared_ptr<struct ClanOLC> temp_clan_olc(new ClanOLC);
		temp_clan_olc->clan = CLAN(ch);
		temp_clan_olc->privileges = CLAN(ch)->privileges;
		ch->desc->clan_olc = temp_clan_olc;
		ch->desc->state = EConState::kClanedit;
		CLAN(ch)->MainMenu(ch->desc);
	} else if (CompareParam(buffer2, "воевода") && !CLAN_MEMBER(ch)->rank_num)
		CLAN(ch)->HouseOwner(ch, buffer);
	else if (CompareParam(buffer2, "статистика"))
		CLAN(ch)->HouseStat(ch, buffer);
	else if (CompareParam(buffer2, "покинуть", true) && CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_EXIT])
		CLAN(ch)->HouseLeave(ch);
	else if (CompareParam(buffer2, "налог") && CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_TAX])
		tax_manage(ch, buffer);
	else if (CompareParam(buffer2, "сообщение") && CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_MOD]) {
		prepare_write_mod(ch, buffer);
	} else if (CompareParam(buffer2, "пк")) {
		CLAN(ch)->pk_log.print(ch);
	} else if (CompareParam(buffer2, "лог")) {
		CLAN(ch)->chest_log.print(ch, buffer);
	} else if (CompareParam(buffer2, "сайт") && !CLAN_MEMBER(ch)->rank_num) {
		CLAN(ch)->house_web_url(ch, buffer);
	} else {
		// обработка списка доступных команд по званию персонажа
		buffer = "Доступные вам привилегии дружины:\r\n";
		for (unsigned i = 0; i < CLAN_PRIVILEGES_NUM; ++i)
			if (CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][i])
				buffer += HOUSE_FORMAT[i];
		// воевода до кучи может сам сменить у дружины воеводу
		if (!CLAN_MEMBER(ch)->rank_num) {
			buffer += "  клан воевода (имя)\r\n";
			buffer += "  клан сайт (адрес сайта вашей дружины для 'справка сайтыдружин')\r\n";
		}
		if (CLAN(ch)->storehouse)
			buffer += "  хранилище <фильтры>\r\n";
		buffer += "  клан статистика <!опыт/!заработанным/!налог/!последнему/!имя>";
		if (!CLAN_MEMBER(ch)->rank_num)
			buffer += " <имя|все|очистить|очистить деньги>\r\n";
		else
			buffer += " <имя|все>\r\n";
		if (!CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_POLITICS]) {
			buffer += "  политика (только просмотр)\r\n";
		}
		buffer += "  клан пк (список последних сражений)\r\n";
		buffer += "  клан лог <без параметров|строка поиска>\r\n";
		SendMsgToChar(buffer, ch);
	}
}

// ктодружина (список соклановцев, находящихся онлайн)
void DoWhoClan(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc() || !CLAN(ch)) {
		SendMsgToChar("Чаво?\r\n", ch);
		return;
	}

	std::ostringstream buffer;
	buffer << fmt::format(" Ваша дружина: {}{}{}.\r\n", kColorBoldRed, CLAN(ch)->abbrev, kColorNrm);
	buffer << fmt::format(" {}Сейчас в игре Ваши соратники:{}\r\n\r\n", kColorWht, kColorNrm);
	DescriptorData *d;
	int num = 0;

	for (d = descriptor_list, num = 0; d; d = d->next)
		if (d->character && d->state == EConState::kPlaying && CLAN(d->character) == CLAN(ch)) {
			buffer << "    " << d->character->race_or_title().c_str() << "\r\n";
			++num;
		}
	buffer << "\r\n Всего: " << num << ".\r\n";
	SendMsgToChar(buffer.str(), ch);
}

void DoShowPolitics(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc() || !CLAN(ch)) {
		SendMsgToChar("Чаво?\r\n", ch);
		return;
	}

	std::string buffer = argument;
	if (!buffer.empty() && CLAN(ch)->privileges[CLAN_MEMBER(ch)->rank_num][MAY_CLAN_POLITICS]) {
		CLAN(ch)->ManagePolitics(ch, buffer);
		return;
	}

	int p1 = 0, p2 = 0;
	std::ostringstream buffer2;
	buffer2 << "Отношения Вашей дружины с другими дружинами:\r\n" <<
			"Название     Отношение Вашей дружины     Отношение к вашей дружине\r\n";

	for (const auto &clanVictim : Clan::ClanList) {
		if ((clanVictim == CLAN(ch)) || ((*clanVictim).m_members.size() == 0)) {
			continue;
		}
		p1 = CLAN(ch)->CheckPolitics(clanVictim->rent);
		p2 = clanVictim->CheckPolitics(CLAN(ch)->rent);
		buffer2 << fmt::format("  {:>3}             {}{:>11}{}                 {}{:>11}{}\r\n", clanVictim->abbrev,
							   (p1 == kPoliticsWar ? kColorBoldRed : (p1 == kPoliticsAlliance ? kColorGrn
																							  : kColorNrm)),
							   politicsnames[p1], kColorNrm,
							   (p2 == kPoliticsWar ? kColorBoldRed : (p2 == kPoliticsAlliance ? kColorGrn
																							  : kColorNrm)),
							   politicsnames[p2], kColorNrm);
	}
	SendMsgToChar(buffer2.str(), ch);
}

void do_show_alliance(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc()) {
		return;
	}
	std::string buffer = argument;
	std::ostringstream buffer3;
	buffer3 << "Дружины, находящиеся в состоянии союза:\r\n";

	if (!buffer.empty()) {
		Clan::ClanListType::const_iterator clan1;
		for (clan1 = Clan::ClanList.begin(); clan1 != Clan::ClanList.end(); ++clan1) {
			if (CompareParam(buffer, (*clan1)->abbrev)) {
				break;
			}
		}

		if (clan1 == Clan::ClanList.end() || (*clan1)->m_members.size() == 0) {
			SendMsgToChar("Такая дружина не зарегистрирована\r\n", ch);
			return;
		}

		Clan::ClanListType::const_iterator clan2;
		for (clan2 = Clan::ClanList.begin(); clan2 != Clan::ClanList.end(); ++clan2) {
			if (clan2 == clan1) {
				continue;
			}

			if ((*clan1)->CheckPolitics((*clan2)->rent) == kPoliticsAlliance) {
				buffer3 << " " << (*clan1)->abbrev << " помогает " << (*clan2)->abbrev << "\r\n";
			}
		}
	} else {
		for (const auto &clan1 : Clan::ClanList) {
			for (const auto &clan2 : Clan::ClanList) {
				if (clan2 == clan1) {
					continue;
				}

				if (clan1->CheckPolitics(clan2->rent) == kPoliticsAlliance) {
					buffer3 << " " << clan1->abbrev << " помогает " << clan2->abbrev << "\r\n";
				}
			}
		}
	}
	buffer3 << "\r\n";
	SendMsgToChar(buffer3.str(), ch);

}

// вобщем это копи-паст из биржи + флаги
void DoStoreHouse(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	ObjData *chest;
	if (ch->IsNpc() || !CLAN(ch)) {
		SendMsgToChar("Чаво?\r\n", ch);
		return;
	}
	if (!CLAN(ch)->storehouse) {
		SendMsgToChar("Ваш воевода зажал денег и отключил эту возможность! :(\r\n", ch);
		return;
	}
	char *stufina = one_argument(argument, arg);

	if (!str_cmp(arg, "все") || !str_cmp(arg, "all")) {
		for (chest = world[GetRoomRnum(CLAN(ch)->chest_room)]->contents; chest; chest = chest->get_next_content()) {
			if (Clan::is_clan_chest(chest)) {
				Clan::ChestShow(chest, ch);
				return;
			}
		}
	}
	if (utils::IsAbbr(arg, "характеристики") || utils::IsAbbr(arg, "identify") || utils::IsAbbr(arg, "опознать")) {
		if ((ch->get_bank() < kChestIdentPay) && (GetRealLevel(ch) < kLvlImplementator)) {
			SendMsgToChar("У вас недостаточно денег в банке для такого исследования.\r\n", ch);
			return;
		}

		for (chest = world[GetRoomRnum(CLAN(ch)->chest_room)]->contents; chest; chest = chest->get_next_content()) {
			if (Clan::is_clan_chest(chest)) {
				ObjData *tmp_obj = get_obj_in_list_vis(ch, stufina, chest->get_contains());
				if (tmp_obj) {
					SendMsgToChar(ch, "Характеристики предмета: %s\r\n", stufina);
					mort_show_obj_values(tmp_obj, ch, 200);
					ch->remove_bank(kChestIdentPay);
					SendMsgToChar(ch,
								  "%sЗа информацию о предмете с вашего банковского счета сняли %d %s%s\r\n",
								  kColorBoldGrn,
								  kChestIdentPay,
								  GetDeclensionInNumber(kChestIdentPay, EWhat::kMoneyU),
								  kColorNrm);
					return;
				}
			}
		}
		sprintf(buf1, "Ничего похожего на %s в хранилище ненайдено! Будьте внимательнее.\r\n", stufina);
		SendMsgToChar(buf1, ch);
		return;
	}

	ParseFilter filter(ParseFilter::CLAN);

	if (!filter.parse_filter(ch, filter, argument)) {
		return;
	}
	SendMsgToChar(ch, "Выборка: %s\r\n", filter.print().c_str());
	SetWait(ch, 1, false);

	std::string out;
	for (chest = world[GetRoomRnum(CLAN(ch)->chest_room)]->contents;
		 chest; chest = chest->get_next_content()) {
		if (Clan::is_clan_chest(chest)) {
			for (ObjData *obj = chest->get_contains(); obj; obj = obj->get_next_content()) {
				if (filter.check(obj, ch)) {
					out += show_obj_to_char(obj, ch, 1, 3, 1);
				}
			}
			break;
		}
	}

	if (!out.empty())
		page_string(ch->desc, out);
	else
		SendMsgToChar("Ничего не найдено.\r\n", ch);
}

void do_clanstuff(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	int vnum, rnum;
	int cnt = 0, gold_total = 0;

	if (!CLAN(ch)) {
		SendMsgToChar("Сначала вступите в какой-нибудь клан.\r\n", ch);
		return;
	}

	if (GET_ROOM_VNUM(ch->in_room) != CLAN(ch)->GetRent()) {
		SendMsgToChar("Получить клановую экипировку вы можете только в центре вашего замка.\r\n", ch);
		return;
	}

	std::string title = CLAN(ch)->GetClanTitle();

	half_chop(argument, arg, buf);

	auto it = CLAN(ch)->clanstuff.begin();
	for (; it != CLAN(ch)->clanstuff.end(); it++) {
		vnum = kClanStuffZone * 100 + it->num;
		const auto obj = world_objects.create_from_prototype_by_vnum(vnum);
		if (!obj) {
			continue;
		}
		rnum = obj->get_rnum();

		if (rnum == kNothing) {
			continue;
		}

		if (*arg && !strstr(obj_proto[rnum]->get_short_description().c_str(), arg)) {
			continue;
		}

		sprintf(buf, "%s %s clan%d!", it->name.c_str(), title.c_str(), CLAN(ch)->GetRent());
		obj->set_aliases(buf);
		obj->set_short_description(it->PNames[0] + " " + title);

		for (int i = ECase::kFirstCase; i <= ECase::kLastCase; i++) {
			obj->set_PName(static_cast<ECase>(i), it->PNames[i] + " " + title);
		}

		obj->set_description(it->desc);

		if (it->longdesc.length() > 0) {
			ExtraDescription::shared_ptr new_descr(new ExtraDescription());
			new_descr->keyword = str_dup(obj->get_short_description().c_str());
			new_descr->description = str_dup(it->longdesc.c_str());
			new_descr->next = nullptr;
			obj->set_ex_description(new_descr);
		}

		if (!cnt) {
			act("$n открыл$g крышку сундука со стандартной экипировкой\r\n", false, ch, nullptr, nullptr, kToRoom);
			act("Вы открыли крышку сундука со стандартной экипировкой\r\n", false, ch, nullptr, nullptr, kToChar);
		}

		int gold = obj->get_cost();

		if (ch->get_gold() >= gold) {
			ch->remove_gold(gold);
			gold_total += gold;
		} else {
			SendMsgToChar(ch, "Кончились денюжки!\r\n");
			break;
		}

		PlaceObjToInventory(obj.get(), ch);
		cnt++;

		sprintf(buf, "$n взял$g %s из сундука", obj->get_PName(ECase::kNom).c_str());
		sprintf(buf2, "Вы взяли %s из сундука", obj->get_PName(ECase::kNom).c_str());
		act(buf, false, ch, 0, 0, kToRoom);
		act(buf2, false, ch, 0, 0, kToChar);
	}

	if (cnt) {
		sprintf(buf2, "\r\nЭкипировка обошлась вам в %d %s.", gold_total,
				GetDeclensionInNumber(gold_total, EWhat::kMoneyU));
		act("\r\n$n закрыл$g крышку сундука", false, ch, 0, 0, kToRoom);
		act(buf2, false, ch, 0, 0, kToChar);
	} else {
		act("\r\n$n порыл$u в сундуке со стандартной экипировкой, но ничего не наш$y", false, ch, 0, 0, kToRoom);
		act("\r\nВы порылись в сундуке со стандартной экипировкой, но не нашли ничего подходящего",
			false,
			ch,
			0,
			0,
			kToChar);
	}

	SetWait(ch, 1, false);
}

// божественный hcontrol
void DoHcontrol(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc())
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
	else if (CompareParam(buffer2, "pk") && !buffer.empty()) {
		Clan::SetPk(ch, buffer);
	} else if (CompareParam(buffer2, "save")) {
		Clan::ClanSave();
		Clan::ChestUpdate();
		Clan::SaveChestAll();
		Clan::save_pk_log();
		save_ingr_chests();
		save_clan_exp();
		save_chest_log();
	} else if (CompareParam(buffer2, "title") && !buffer.empty()) {
		Clan::hcontrol_title(ch, buffer);
	} else if (CompareParam(buffer2, "rank") && !buffer.empty()) {
		Clan::hcontrol_rank(ch, buffer);
	} else if (CompareParam(buffer2, "exphistory")) {
		Clan::hcontrol_exphistory(ch, buffer);
	} else if (CompareParam(buffer2, "ingr")) {
		Clan::hcontrol_set_ingr_chest(ch, buffer);
	} else if (CompareParam(buffer2, "owner") && !buffer.empty()) {
		Clan::hcon_owner(ch, buffer);
	} else
		SendMsgToChar(HCONTROL_FORMAT, ch);
}

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
