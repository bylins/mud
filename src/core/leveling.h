//
// Created by SKWIZ on 04.01.2021.
//

#ifndef BYLINS_LEVELING_H
#define BYLINS_LEVELING_H

#include "chars/char.hpp"

void levelup_events(CHAR_DATA *ch);

namespace ExpCalc {
    void gain_exp(CHAR_DATA * ch, int gain);
    void gain_exp_regardless(CHAR_DATA * ch, int gain);
    int get_extend_exp(int exp, CHAR_DATA * ch, CHAR_DATA * victim);
    int max_exp_gain_pc(CHAR_DATA * ch);
    int max_exp_loss_pc(CHAR_DATA * ch);
    float get_npc_long_live_exp_bounus(int vnum);
    int level_exp(CHAR_DATA * ch, int level);
    int calcDeathExp(CHAR_DATA* ch);
    void advance_level(CHAR_DATA * ch);
    void increaseExperience(CHAR_DATA * ch, CHAR_DATA * victim, int members, int koef);
}

namespace Remort
{
    extern const char *CONFIG_FILE;
    extern std::string WHERE_TO_REMORT_STR;
    bool can_remort_now(CHAR_DATA *ch);
    void init();
    bool need_torc(CHAR_DATA *ch);
} // namespace Remort


#endif //BYLINS_LEVELING_H
