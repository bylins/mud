#ifndef BYLINS_THROW_H
#define BYLINS_THROW_H

class CharData;

inline constexpr int kScmdPhysicalThrow{0};
inline constexpr int kScmdShadowThrow{1};

void GoThrow(CharData *ch, CharData *victim);
void DoThrow(CharData *ch, char *argument, int/* cmd*/, int subcmd);
void DoThrow(CharData *ch, CharData *victim);

#endif //BYLINS_THROW_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
