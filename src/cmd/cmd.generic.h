#ifndef BYLINS_CMD_GENERIC_H
#define BYLINS_CMD_GENERIC_H

#include "chars/char.hpp"
#include "skills.h"

#include <map>

#define MAXPRICE 9999999

void do_findhelpee(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_freehelpee(CHAR_DATA* ch, char* /* argument*/, int/* cmd*/, int/* subcmd*/);
int get_reformed_charmice_hp(CHAR_DATA * ch, CHAR_DATA * victim, int spellnum);

int mercenary(CHAR_DATA *ch, void* /*me*/, int cmd, char* argument);

struct MERCDATA
{
    int CharmedCount; // кол-во раз почармлено
    int spentGold; // если купец - сколько потрачено кун
    int deathCount; // кол-во раз, когда чармис сдох
    int currRemortAvail; // доступен на текущем реморте
    int isFavorite; // моб показывается в сокращенном списке
};

void do_order(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_quit(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd);
void do_retreat(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/);

#if defined(HAVE_TG)
#include <curl/curl.h>
#include <string>
#endif
class CHAR_DATA;

void do_telegram(CHAR_DATA *ch, char *argument, int, int);

class TelegramBot {
private:
#if defined(HAVE_TG)
    CURL *curl;
#endif
    const std::string msgStr = "chat_id=358708535&text=";
    const std::string chatIdParam = "chat_id=";
    const std::string textParam = "&text=";
    const std::string uri = "https://api.telegram.org/bot1330963555:AAHvh-gXBRxJHVKOmjsl8E73TJr0cO2eC50/sendMessage";
    const unsigned long debugChatId = 358708535;
public:
    TelegramBot();
    ~TelegramBot();
    bool sendMessage(unsigned long chatId, const std::string& msg);
};


#define CALC_TRACK(ch,vict) (calculate_skill(ch,SKILL_TRACK, 0))

int go_track(CHAR_DATA * ch, CHAR_DATA * victim, const ESkill skill_no);
void do_track(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_hidetrack(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/);
void do_whoami(CHAR_DATA *ch, char *, int, int);

#endif //BYLINS_CMD_GENERIC_H
