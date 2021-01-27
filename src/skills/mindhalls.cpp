//
// sudo apt-get install nlohmann-json3-dev helped me
//
#include <house.h>
#include "mindhalls.h"

#include "handler.h"
#include "magic.h"
#include "world.objects.hpp"
#include "object.prototypes.hpp"
#include "screen.h"

#include "core/mindhalls.parameters.h"

using namespace std::chrono;

MindHallsParams mindHallsParameters;
extern CObjectPrototypes obj_proto;

bool nsMindHalls::initMindHalls(){
    if (!mindHallsParameters.load())
        return false;
    return true;
}

void nsMindHalls::do_mindhalls(CHAR_DATA *ch, char *argument, int, int) {

}

bool nsMindHalls::load(CHAR_DATA *owner, char *line) {
    if (owner == nullptr){
        sprintf(buf, "Ошибка загрузки чертогов, параметры NULL: player: %s, file: %s",
                owner? owner->get_name().c_str() : "NULL",
                *line?  "NULL" : line);
        mudlog(buf, BRF, LVL_GOD, SYSLOG, 1);
        return false;
    }
    owner->player_specials->saved._mindHalls = std::make_shared<MindHalls>();
    auto m = owner->getMindHalls();
    if (!m->load(owner, line))
        return false;
    return true;
}

bool nsMindHalls::save(CHAR_DATA *owner, FILE *playerFile) {
    std::string line;
    if (owner == nullptr || playerFile == nullptr){
        sprintf(buf, "Ошибка сохранения чертогов, параметры NULL: player: %s, file: %s",
                owner? owner->get_name().c_str() : "NULL",
                playerFile == nullptr?  "NULL" : "есть");
        mudlog(buf, BRF, LVL_BUILDER, SYSLOG, 1);
        return false;
    }
    if (owner->getMindHalls() == nullptr)
        return true;
    line = owner->getMindHalls()->to_string();
    fprintf(playerFile, "MndH: %s\n", line.c_str());
    return true;
}

OBJ_DATA* MindHalls::operator[](obj_rnum runeId) const {
    auto it = _runes.find(runeId);
    if (it == _runes.end())
        return nullptr;
    return nullptr;
}

void MindHalls::erase(u_short remort) {
    _runes.clear();
    _limits = mindHallsParameters.getHallsCapacity(remort);
}

HRESULT MindHalls::addRune(CHAR_DATA* owner, char* runeName) {
    if (owner == nullptr || !*runeName)
        return MH_NOT_FOUND;
    auto rune = get_obj_in_list_vis(owner, runeName, owner->carrying);
    if (rune == nullptr || !mag_item_ok(owner, rune, SPELL_RUNES))
        return MH_NOT_FOUND;
    if (_runes[rune->get_vnum()] != nullptr)
        return MH_ALREADY_STORED;
    if (_limits - mindHallsParameters.getRunePower(rune->get_vnum()) < 0)
        return MH_OVER_LIMIT;
    auto runeToStore = world_objects.create_from_prototype_by_vnum(rune->get_vnum());
    _runes.emplace(rune->get_vnum(), runeToStore);
    _limits -= mindHallsParameters.getRunePower(rune->get_vnum());
    return MH_OK;
}

HRESULT MindHalls::removeRune(char* runeName) {
    for (auto it : _runes){
        auto r = it.second.get();
        if (isname(runeName, r->get_aliases())) {
            _limits += mindHallsParameters.getRunePower(it.first);
            _runes.erase(it.first);
            return MH_OK;
        }
    }
    return MH_NOT_FOUND;
}

MindHalls::MindHalls() {

}

std::string MindHalls::to_string() {
    std::vector<short> tmpval;
    for (auto it: _runes)
        tmpval.push_back(it.first);
    json j_tmp(tmpval);
    std::string s = j_tmp.dump();
    return s;
}

u_short MindHalls::getFreeStore() {
    return _limits;
}

bool MindHalls::load(CHAR_DATA* owner, char* runes) {
    _limits = mindHallsParameters.getHallsCapacity(owner->get_remort());
    json r = json::parse(runes, nullptr, false);
    if (r.is_discarded()) {
        sprintf(buf, "Ошибка инициализации рун из файла игрока: %s, строка: %s ",
                owner->get_name().c_str(),
                runes);
        mudlog(buf, BRF, LVL_GOD, SYSLOG, 1);
        return false;
    }
    for (const auto& it : r.items()) {
        auto runeToStore = 	std::make_shared<OBJ_DATA>(*obj_proto[real_object(it.value())]);
        _runes.emplace(it.value(), runeToStore);
        _limits -= mindHallsParameters.getRunePower(it.value());
    }
    return true;
}

std::string MindHalls::getContents(CHAR_DATA* owner) {
    bool empty = true;
    sprintf(buf, "\r\n%sВ чертогах разума:%s\r\n", CCBLU(owner, C_NRM) , CCNRM(owner, C_NRM));
    for (const auto& it: _runes) {
        sprintf(buf + strlen(buf), "%s\r\n", it.second->get_PName(0).c_str());
        empty = false;
    }
    if (empty)
        sprintf(buf + strlen(buf), "ничего нет, и свободно %d места.\r\n", _limits);
    return std::string(buf);
}

