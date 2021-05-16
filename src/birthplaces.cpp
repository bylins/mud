// $RCSfile$     $Date$     $Revision$
// Part of Bylins http://www.mud.ru

#include "conf.h"
#include "sysdep.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "birthplaces.h"
#include "pugixml.hpp"

#include <algorithm>
#include <sstream>

const char *DEFAULT_RENT_HELP = "Попроси нашего кладовщика помочь тебе с экипировкой и припасами.";

BirthPlaceListType BirthPlace::BirthPlaceList;

//Создаем новую точку входа и заполняем ее поля значениями из файлда
void BirthPlace::LoadBirthPlace(pugi::xml_node BirthPlaceNode)
{
	pugi::xml_node CurNode;
	BirthPlacePtr TmpBirthPlace(new BirthPlace);

	//Парсим параметры точки создания
    TmpBirthPlace->_Id = BirthPlaceNode.attribute("id").as_int();
	TmpBirthPlace->_Name = BirthPlaceNode.child("name").child_value();
    TmpBirthPlace->_Description = BirthPlaceNode.child("shortdesc").child_value();
    TmpBirthPlace->_MenuStr = BirthPlaceNode.child("menustring").child_value();
    CurNode = BirthPlaceNode.child("room");
    TmpBirthPlace->_LoadRoom = CurNode.attribute("vnum").as_int();
    TmpBirthPlace->_RentHelp = BirthPlaceNode.child("renthelp").child_value();

	//Парсим список предметов
	CurNode = BirthPlaceNode.child("items");
	for (CurNode = CurNode.child("item"); CurNode; CurNode = CurNode.next_sibling("item"))
	{
		TmpBirthPlace->_ItemsList.push_back(CurNode.attribute("vnum").as_int());
	}
	//Добавляем новую точку в список
	BirthPlace::BirthPlaceList.push_back(TmpBirthPlace);
}

//Загрузка параметров точек создания персонажей
void BirthPlace::Load(pugi::xml_node XMLBirthPlaceList)
{
    pugi::xml_node CurNode;

	for (CurNode = XMLBirthPlaceList.child("birthplace"); CurNode; CurNode = CurNode.next_sibling("birthplace"))
	{
		LoadBirthPlace(CurNode);
	}
}

// Надо было map использовать. %) Поздно сообразил
// Если руки дойдут - потом переделаю.

// Получение ссылки на точку входа по ее ID
BirthPlacePtr BirthPlace::GetBirthPlaceById(short Id)
{
    BirthPlacePtr BPPtr;
    for (BirthPlaceListType::iterator it =  BirthPlaceList.begin();it != BirthPlaceList.end();++it)
        if (Id == (*it)->Id())
            BPPtr = *it;

    return BPPtr;
};

// Получение внума комнаты по ID точки входа
int BirthPlace::GetLoadRoom(short Id)
{
    BirthPlacePtr BPPtr = BirthPlace::GetBirthPlaceById(Id);
    if (BPPtr)
        return BPPtr->LoadRoom();

    return DEFAULT_LOADROOM;
};

// Получение списка предметов, которые выдаются в этой точке при первом входе в игру
std::vector<int> BirthPlace::GetItemList(short Id)
{
    std::vector<int> BirthPlaceItemList;
    BirthPlacePtr BPPtr = BirthPlace::GetBirthPlaceById(Id);
    if (BPPtr)
        BirthPlaceItemList = BPPtr->ItemsList();

    return BirthPlaceItemList;
};

// Получение строчки меню для точки входа по ID
std::string BirthPlace::GetMenuStr(short Id)
{
    BirthPlacePtr BPPtr = BirthPlace::GetBirthPlaceById(Id);
    if (BPPtr != NULL)
        return BPPtr->MenuStr();

    return BIRTH_PLACE_NAME_UNDEFINED;
};

// Генерация меню по списку точек входа
std::string BirthPlace::ShowMenu(std::vector<int> BPList)
{
    int i;
    BirthPlacePtr BPPtr;
    std::ostringstream buffer;
    i = 1;
    for (std::vector<int>::iterator it = BPList.begin();it != BPList.end();++it)
    {
        BPPtr = BirthPlace::GetBirthPlaceById(*it);
        if (BPPtr != NULL)
        {
            buffer << " " << i << ") " << BPPtr->_MenuStr << "\r\n";
            i++;
        };
    };

     return buffer.str();
};

// Сравнение текстового ввода с описанием точек
// Добавлено, чтоб не травмировать нежную психику мортящихся
short BirthPlace::ParseSelect(char *arg)
{
    std::string select = arg;
    lower_convert(select);
//    std::transform(select.begin(), select.end(), select.begin(), _tolower);
    for (BirthPlaceListType::iterator it = BirthPlaceList.begin();it != BirthPlaceList.end();++it)
        if (select == (*it)->Description())
            return (*it)->Id();

    return BIRTH_PLACE_UNDEFINED;
};

// Проверка наличия точки входа с указанным ID
bool BirthPlace::CheckId(short Id)
{
    BirthPlacePtr BPPtr = BirthPlace::GetBirthPlaceById(Id);
    if (BPPtr != NULL)
        return true;

    return false;
};

int BirthPlace::GetIdByRoom(int room_vnum)
{
    for (auto i = BirthPlaceList.begin(); i != BirthPlaceList.end(); ++i)
	{
        if ((*i)->LoadRoom() / 100 == room_vnum / 100)
		{
			return (*i)->Id();
		}
	}
	return -1;
}

std::string BirthPlace::GetRentHelp(short Id)
{
    BirthPlacePtr BPPtr = BirthPlace::GetBirthPlaceById(Id);
    if (BPPtr != NULL && !BPPtr->RentHelp().empty())
    {
        return BPPtr->RentHelp();
    }
    return DEFAULT_RENT_HELP;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
