#ifndef BYLINS_SRC_CMD_DO_SKILLS_H_
#define BYLINS_SRC_CMD_DO_SKILLS_H_

class CharData;

void DoSkills(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void DisplaySkills(CharData *ch, CharData *vict, const char *filter = nullptr);

#endif //BYLINS_SRC_CMD_DO_SKILLS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
