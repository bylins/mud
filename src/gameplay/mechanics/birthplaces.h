//  $RCSfile$     $Date$     $Revision$
//  Part of Bylins http://www.mud.ru
// Модуль содержит все необходимое для загрузки и работы со списком точек входа в игру

#ifndef BIRTH_PLACES_HPP_INCLUDED
#define BIRTH_PLACES_HPP_INCLUDED

//Для тех, у кого нет нормального файла рас, и нет зон.
const int kDefaultLoadroom = 4056;
const int kBirthplaceUndefined = -1;
#define BIRTH_PLACES_FILE "birthplaces.xml"
#define BIRTH_PLACE_NAME_UNDEFINED "Undefined: у кодера какие-то проблемы"
#define BIRTH_PLACE_MAIN_TAG "birthplaces"
#define BIRTH_PLACE_ERROR_STR "...birth places reading fail"

#include "engine/core/conf.h"
#include "engine/core/sysdep.h"
#include "engine/structs/structs.h"
#include "third_party_libs/pugixml/pugixml.h"

#include <vector>

class Birthplaces;

typedef std::shared_ptr<Birthplaces> BirthPlacePtr;
typedef std::vector<BirthPlacePtr> BirthPlaceListType;

class Birthplaces {
 public:
	static void Load(pugi::xml_node XMLBirthPlaceList);
	static int GetLoadRoom(int Id);                       // Получение внума загрузочной комнаты по ID
  [[maybe_unused]] static std::string GetMenuStr(int Id);                // Получение строчки для меню по ID
	static std::vector<int> GetItemList(int Id);          // Получение списка выдаваемых итемов по ID
	static std::string ShowMenu(const std::vector<int> &BPList);   // Получение меню выбора точек одной строкой
	static int ParseSelect(const char *arg);                    // Поиск точки по текстовому вводу и описанию (description)
	static bool CheckId(int Id);                          // Проверка наличия точки с указанным ID
	static int GetIdByRoom(int vnumum);                  // Выяснение ID через текущую комнату
	static std::string GetRentHelp(int Id);               // Фраза рентера нубу после смерти по ID

	// Доступ к свойствам класса.
	// Не особенно нужно, но пусть будет
	[[nodiscard]] int Id() const { return this->id_; }
	std::string Name() { return this->name_; }
	std::string Description() { return this->description_; }
	std::string MenuStr() { return this->menu_str_; }
	[[nodiscard]] int LoadRoom() const { return this->load_room_; }
	[[nodiscard]] std::vector<int> ItemsList() const { return this->items_list_; }
	std::string RentHelp() { return this->rent_help_; }

 private:
	int id_;                                  // Идентификатор - целое число
	std::string name_;                          // Название точки
	std::string description_;                   // Короткое описание для парсинга
	std::string menu_str_;                       // Название в меню
	int load_room_;                              // Номер комнаты входа
	std::vector<int> items_list_;                // Список предметов, которые дополнительно выдаются на руки в этой точке
	std::string rent_help_;                      // Фраза, выдаваемая нубу мобом-рентером при выходе на ренту после смерти

	static BirthPlaceListType BirthPlaceList;   // Список точек входа в игру новых персонажей
	static void LoadBirthPlace(pugi::xml_node BirthPlaceNode);  // Парсинг описания одной точки входа
	static BirthPlacePtr GetBirthPlaceById(int Id);           // Получение ссылки на точку хода по ее ID
};

#endif // BIRTH_PLACES_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
