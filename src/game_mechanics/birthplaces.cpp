// $RCSfile$     $Date$     $Revision$
// Part of Bylins http://www.mud.ru

#include "utils/utils.h"
#include "third_party_libs/pugixml/pugixml.h"

const char *DEFAULT_RENT_HELP = "Попроси нашего кладовщика помочь тебе с экипировкой и припасами.";

BirthPlaceListType Birthplaces::BirthPlaceList;

//Создаем новую точку входа и заполняем ее поля значениями из файлда
void Birthplaces::LoadBirthPlace(pugi::xml_node BirthPlaceNode) {
	pugi::xml_node CurNode;
	BirthPlacePtr TmpBirthPlace(new Birthplaces);

	//Парсим параметры точки создания
	TmpBirthPlace->id_ = BirthPlaceNode.attribute("id").as_int();
	TmpBirthPlace->name_ = BirthPlaceNode.child("name").child_value();
	TmpBirthPlace->description_ = BirthPlaceNode.child("shortdesc").child_value();
	TmpBirthPlace->menu_str_ = BirthPlaceNode.child("menustring").child_value();
	CurNode = BirthPlaceNode.child("room");
	TmpBirthPlace->load_room_ = CurNode.attribute("vnum").as_int();
	TmpBirthPlace->rent_help_ = BirthPlaceNode.child("renthelp").child_value();

	//Парсим список предметов
	CurNode = BirthPlaceNode.child("items");
	for (CurNode = CurNode.child("item"); CurNode; CurNode = CurNode.next_sibling("item")) {
		TmpBirthPlace->items_list_.push_back(CurNode.attribute("vnum").as_int());
	}
	//Добавляем новую точку в список
	Birthplaces::BirthPlaceList.push_back(TmpBirthPlace);
}

//Загрузка параметров точек создания персонажей
void Birthplaces::Load(pugi::xml_node XMLBirthPlaceList) {
	pugi::xml_node CurNode;

	for (CurNode = XMLBirthPlaceList.child("birthplace"); CurNode; CurNode = CurNode.next_sibling("birthplace")) {
		LoadBirthPlace(CurNode);
	}
}

// Надо было map использовать. %) Поздно сообразил
// Если руки дойдут - потом переделаю.

// Получение ссылки на точку входа по ее ID
BirthPlacePtr Birthplaces::GetBirthPlaceById(int Id) {
	BirthPlacePtr BPPtr;
	for (auto & it : BirthPlaceList) {
      if (Id == it->Id()) {
        BPPtr = it;
      }
    }

	return BPPtr;
}

// Получение внума комнаты по ID точки входа
int Birthplaces::GetLoadRoom(int Id) {
	BirthPlacePtr BPPtr = Birthplaces::GetBirthPlaceById(Id);
	if (BPPtr)
		return BPPtr->LoadRoom();

	return kDefaultLoadroom;
}

// Получение списка предметов, которые выдаются в этой точке при первом входе в игру
std::vector<int> Birthplaces::GetItemList(int Id) {
	std::vector<int> BirthPlaceItemList;
	BirthPlacePtr BPPtr = Birthplaces::GetBirthPlaceById(Id);
	if (BPPtr)
		BirthPlaceItemList = BPPtr->ItemsList();

	return BirthPlaceItemList;
}

// Получение строчки меню для точки входа по ID
[[maybe_unused]] std::string Birthplaces::GetMenuStr(int Id) {
	BirthPlacePtr BPPtr = Birthplaces::GetBirthPlaceById(Id);
	if (BPPtr != nullptr)
		return BPPtr->MenuStr();

	return BIRTH_PLACE_NAME_UNDEFINED;
}

// Генерация меню по списку точек входа
std::string Birthplaces::ShowMenu(const std::vector<int> &BPList) {
	int i;
	BirthPlacePtr BPPtr;
	std::ostringstream buffer;
	i = 1;
	for (const auto it : BPList) {
		BPPtr = Birthplaces::GetBirthPlaceById(it);
		if (BPPtr != nullptr) {
			buffer << " " << i << ") " << BPPtr->menu_str_ << "\r\n";
			i++;
		}
	}

	return buffer.str();
}

// Сравнение текстового ввода с описанием точек
// Добавлено, чтоб не травмировать нежную психику мортящихся
int Birthplaces::ParseSelect(const char *argument) {
	std::string select = argument;
	utils::ConvertToLow(select);
	for (auto & it : BirthPlaceList) {
      if (select == it->Description()) {
        return it->Id();
      }
    }

	return kBirthplaceUndefined;
}

// Проверка наличия точки входа с указанным ID
bool Birthplaces::CheckId(int Id) {
	BirthPlacePtr BPPtr = Birthplaces::GetBirthPlaceById(Id);
	return (BPPtr != nullptr);
}

int Birthplaces::GetIdByRoom(int vnumum) {
	for (auto & i : BirthPlaceList) {
		if (i->LoadRoom() / 100 == vnumum / 100) {
			return i->Id();
		}
	}
	return -1;
}

std::string Birthplaces::GetRentHelp(int Id) {
	BirthPlacePtr BPPtr = Birthplaces::GetBirthPlaceById(Id);
	if (BPPtr != nullptr && !BPPtr->RentHelp().empty()) {
		return BPPtr->RentHelp();
	}
	return DEFAULT_RENT_HELP;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
