/*
 *  Файл назван так, чтобы не возникало конфликтов с библиотечным файлом features.h
 */
#ifndef BYLINS_SRC_CMD_DO_FEATURES_H_
#define BYLINS_SRC_CMD_DO_FEATURES_H_

class CharData;

void DisplayFeats(CharData *ch, CharData *vict, bool all_feats);
void DoFeatures(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_SRC_CMD_DO_FEATURES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
