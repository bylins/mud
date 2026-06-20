#ifndef BYLINS_SRC_GAME_SKILLS_SPELL_CAPABLE_H_
#define BYLINS_SRC_GAME_SKILLS_SPELL_CAPABLE_H_

class CharData;

void DoSpellCapable(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);

// Clone-mob death trigger: fire its stored "capable" spell at the killer (issue.fight-stuff).
void check_spell_capable(CharData *ch, CharData *killer);

#endif //BYLINS_SRC_GAME_SKILLS_SPELL_CAPABLE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
