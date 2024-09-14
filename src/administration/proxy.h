/**
\file proxy.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 14.09.2024.
\brief Система регистрации прокси-серверов.
\detail Список зарегистрированных прокси, загрузка-сохранение, система регистрации.
*/

#ifndef BYLINS_SRC_ADMINISTRATION_PROXY_H_
#define BYLINS_SRC_ADMINISTRATION_PROXY_H_

#include <string>
#include <memory>
#include <map>

class CharData;

namespace RegisterSystem {

void add(CharData *ch, const char *text, const char *reason);
void remove(CharData *ch);
bool IsRegistered(CharData *ch);
bool IsRegisteredEmail(const std::string &email);
std::string ShowComment(const std::string &email);
void load();
void save();
void LoadProxyList();
} // namespace RegisterSystem


void SaveProxyList();
unsigned long TxtToIp(std::string &text);

struct ProxyIp {
  unsigned long ip2{0L};   // конечный ип в случае диапазона
  int num{0};             // кол-во максимальных коннектов
  std::string text;    // комментарий
  std::string textIp;  // ип в виде строки
  std::string textIp2; // конечный ип в виде строки
};

using ProxyIpPtr = std::shared_ptr<ProxyIp>;
using ProxyListType = std::map<unsigned long, ProxyIpPtr>;
extern ProxyListType proxy_list;
extern const int kMaxProxyConnect;

#endif //BYLINS_SRC_ADMINISTRATION_PROXY_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
