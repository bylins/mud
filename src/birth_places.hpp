//  $RCSfile$     $Date$     $Revision$
//  Part of Bylins http://www.mud.ru
// Модуль содержит все необходимое для загрузки и работы со списком точек входа в игру

#ifndef BIRTH_PLACES_HPP_INCLUDED
#define BIRTH_PLACES_HPP_INCLUDED

//Для тех, у кого нет нормального файла рас, и нет зон.
//Хотя по-хорошему внумам в коде делать вообще нефик
#define DEFAULT_LOADROOM 4056
#define BIRTH_PLACES_FILE "birthplaces.xml"
#define BIRTH_PLACE_UNDEFINED    -1
#define BIRTH_PLACE_NAME_UNDEFINED "Undefined: у кодера какие-то проблемы"
#define BIRTH_PLACE_MAIN_TAG "birthplaces"
#define BIRTH_PLACE_ERROR_STR "...birth places reading fail"

#include "conf.h"
#include "sysdep.h"
#include "structs.h"

#include <pugixml.hpp>

#include <vector>

class BirthPlace;

typedef std::shared_ptr<BirthPlace> BirthPlacePtr;
typedef std::vector<BirthPlacePtr> BirthPlaceListType;

class BirthPlace
{
public:
    //static void Load(const char *PathToFile);               // Загрузка файла настроек
    static void Load(pugi::xml_node XMLBirthPlaceList);
    static int GetLoadRoom(short Id);                       // Получение внума загрузочной комнаты по ID
    static std::string GetMenuStr(short Id);                // Получение строчки для меню по ID
    static std::vector<int> GetItemList(short Id);          // Получение списка выдаваемых итемов по ID
    static std::string ShowMenu(std::vector<int> BPList);   // Получение меню выбора точек одной строкой
    static short ParseSelect(char *arg);                    // Поиск точки по текстовому вводу и описанию (description)
    static bool CheckId(short Id);                          // Проверка наличия точки с указанным ID
    static int GetIdByRoom(int room_vnum);                  // Выяснение ID через текущую комнату
    static std::string GetRentHelp(short Id);               // Фраза рентера нубу после смерти по ID

    // Доступ к свойствам класса.
    // Не особенно нужно, но пусть будет
    short Id() const {return this->_Id;}
    std::string Name() {return this->_Name;}
    std::string Description() {return this->_Description;}
    std::string MenuStr() {return this->_MenuStr;}
    int LoadRoom() {return this->_LoadRoom;}
    std::vector<int> ItemsList() const {return this->_ItemsList;}
    std::string RentHelp() {return this->_RentHelp;}

private:
    short _Id;                                  // Идентификатор - целое число
    std::string _Name;                          // Название точки
    std::string _Description;                   // Короткое описание для парсинга
    std::string _MenuStr;                       // Название в меню
    int _LoadRoom;                              // Номер комнаты входа
    std::vector<int> _ItemsList;                // Список предметов, которые дополнительно выдаются на руки в этой точке
    std::string _RentHelp;                      // Фраза, выдаваемая нубу мобом-рентером при выходе на ренту после смерти

    static BirthPlaceListType BirthPlaceList;   // Список точек входа в игру новых персонажей
    static void LoadBirthPlace(pugi::xml_node BirthPlaceNode);  // Парсинг описания одной точки входа
    static BirthPlacePtr GetBirthPlaceById(short Id);           // Получение ссылки на точку хода по ее ID
};

#endif // BIRTH_PLACES_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
