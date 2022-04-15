// $RCSfile$     $Date$     $Revision$
// Part of Bylins http://www.mud.ru

#include "utils/utils.h"
#include "utils/pugixml/pugixml.h"

const char *DEFAULT_RENT_HELP = "Попроси нашего кладовщика помочь тебе с экипировкой и припасами.";

BirthPlaceListType Birthplaces::BirthPlaceList;

//Создаем новую точку входа и заполняем ее поля значениями из файлда
void Birthplaces::LoadBirthPlace(pugi::xml_node BirthPlaceNode) {
	pugi::xml_node CurNode;
	BirthPlacePtr TmpBirthPlace(new Birthplaces);

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
	for (CurNode = CurNode.child("item"); CurNode; CurNode = CurNode.next_sibling("item")) {
		TmpBirthPlace->_ItemsList.push_back(CurNode.attribute("vnum").as_int());
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
BirthPlacePtr Birthplaces::GetBirthPlaceById(short Id) {
	BirthPlacePtr BPPtr;
	for (BirthPlaceListType::iterator it = BirthPlaceList.begin(); it != BirthPlaceList.end(); ++it)
		if (Id == (*it)->Id())
			BPPtr = *it;

	return BPPtr;
};

// Получение внума комнаты по ID точки входа
int Birthplaces::GetLoadRoom(short Id) {
	BirthPlacePtr BPPtr = Birthplaces::GetBirthPlaceById(Id);
	if (BPPtr)
		return BPPtr->LoadRoom();

	return kDefaultLoadroom;
};

// Получение списка предметов, которые выдаются в этой точке при первом входе в игру
std::vector<int> Birthplaces::GetItemList(short Id) {
	std::vector<int> BirthPlaceItemList;
	BirthPlacePtr BPPtr = Birthplaces::GetBirthPlaceById(Id);
	if (BPPtr)
		BirthPlaceItemList = BPPtr->ItemsList();

	return BirthPlaceItemList;
};

// Получение строчки меню для точки входа по ID
std::string Birthplaces::GetMenuStr(short Id) {
	BirthPlacePtr BPPtr = Birthplaces::GetBirthPlaceById(Id);
	if (BPPtr != nullptr)
		return BPPtr->MenuStr();

	return BIRTH_PLACE_NAME_UNDEFINED;
};

// Генерация меню по списку точек входа
std::string Birthplaces::ShowMenu(std::vector<int> BPList) {
	int i;
	BirthPlacePtr BPPtr;
	std::ostringstream buffer;
	i = 1;
	for (std::vector<int>::iterator it = BPList.begin(); it != BPList.end(); ++it) {
		BPPtr = Birthplaces::GetBirthPlaceById(*it);
		if (BPPtr != nullptr) {
			buffer << " " << i << ") " << BPPtr->_MenuStr << "\r\n";
			i++;
		};
	};

	return buffer.str();
};

// Сравнение текстового ввода с описанием точек
// Добавлено, чтоб не травмировать нежную психику мортящихся
short Birthplaces::ParseSelect(char *arg) {
	std::string select = arg;
	utils::ConvertToLow(select);
//    std::transform(select.begin(), select.end(), select.begin(), _tolower);
	for (BirthPlaceListType::iterator it = BirthPlaceList.begin(); it != BirthPlaceList.end(); ++it)
		if (select == (*it)->Description())
			return (*it)->Id();

	return kBirthplaceUndefined;
};

// Проверка наличия точки входа с указанным ID
bool Birthplaces::CheckId(short Id) {
	BirthPlacePtr BPPtr = Birthplaces::GetBirthPlaceById(Id);
	if (BPPtr != nullptr)
		return true;

	return false;
};

int Birthplaces::GetIdByRoom(int room_vnum) {
	for (auto i = BirthPlaceList.begin(); i != BirthPlaceList.end(); ++i) {
		if ((*i)->LoadRoom() / 100 == room_vnum / 100) {
			return (*i)->Id();
		}
	}
	return -1;
}

std::string Birthplaces::GetRentHelp(short Id) {
	BirthPlacePtr BPPtr = Birthplaces::GetBirthPlaceById(Id);
	if (BPPtr != nullptr && !BPPtr->RentHelp().empty()) {
		return BPPtr->RentHelp();
	}
	return DEFAULT_RENT_HELP;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
