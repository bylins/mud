//
// sudo apt-get install nlohmann-json3-dev helped me
//
#include "mindhalls.h"

#include "handler.h"
#include "magic.h"
#include "world.objects.hpp"

#include "core/mindhalls.parameters.h"

using namespace nsMindHalls;

MindHallsParams mindHallsParameters;

bool nsMindHalls::initMindHalls(){
    if (!mindHallsParameters.load())
        return false;
    return true;
}

OBJ_DATA* MindHalls::operator[](obj_rnum runeId) const {
    auto it = _runes.find(runeId);
    if (it == _runes.end())
        return nullptr;
    return it->second.get();
}

void MindHalls::erase(u_short remort) {
    _runes.clear();
    _limits = mindHallsParameters.getHallsCapacity(remort);
}

HRESULT MindHalls::addRune(CHAR_DATA* owner, char* runeName) {
    if (owner == nullptr || !*runeName)
        return HRESULT::NOT_FOUND;
    auto rune = get_obj_in_list_vis(owner, runeName, owner->carrying);
    if (rune == nullptr || !mag_item_ok(owner, rune, SPELL_RUNES))
        return HRESULT::NOT_FOUND;
    if (_runes[rune->get_vnum()] != nullptr)
        return HRESULT::ALREADY_STORED;
    if (_limits - mindHallsParameters.getRunePower(rune->get_vnum()) < 0)
        return HRESULT::OVER_LIMIT;
    auto runeToStore = world_objects.create_from_prototype_by_vnum(rune->get_vnum());
    _runes.emplace(rune->get_vnum(), runeToStore);
    _limits -= mindHallsParameters.getRunePower(rune->get_vnum());
    return HRESULT::OK;
}

HRESULT MindHalls::removeRune(char* runeName) {
    for (auto it : _runes){
        auto r = it.second.get();
        if (isname(runeName, r->get_aliases())) {
            _limits += mindHallsParameters.getRunePower(it.first);
            _runes.erase(it.first);
            return HRESULT::OK;
        }
    }
    return HRESULT::NOT_FOUND;
}

MindHalls::MindHalls(CHAR_DATA* owner, char* runes) {
    _limits = mindHallsParameters.getHallsCapacity(owner->get_remort());
    json r = json::parse(runes, nullptr, false);
    if (r.is_discarded()) {
        sprintf(buf, "Ошибка инициализации рун из файла игрока: %s ", owner->get_name().c_str());
        mudlog(buf, BRF, LVL_BUILDER, SYSLOG, 1);
        return;
    }
    for (auto it : r.items()) {
        auto runeToStore = world_objects.create_from_prototype_by_vnum(it.value());
        _runes.emplace(it.value(), runeToStore);
        _limits -= mindHallsParameters.getRunePower(it.value());
    }
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
