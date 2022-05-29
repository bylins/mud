#ifndef BYLINS_SRC_GAME_SKILLS_FIT_H_
#define BYLINS_SRC_GAME_SKILLS_FIT_H_

class CharData;

const int kScmdDoAdapt = 0;
const int kScmdMakeOver = 1;

void DoFit(CharData *ch, char *argument, int/* cmd*/, int subcmd);

#endif //BYLINS_SRC_GAME_SKILLS_FIT_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
