#ifndef BYLINS_SRC_CMD_SIGN_H_
#define BYLINS_SRC_CMD_SIGN_H_

#include <string>

class CharData;

void DoSign(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
std::string char_get_custom_label(ObjData *obj, CharData *ch);

#endif //BYLINS_SRC_CMD_SIGN_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :