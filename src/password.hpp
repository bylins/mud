// password.hpp
// Copyright (c) 2007 Krodo
// Part of Bylins http://www.mud.ru

#ifndef PASSWORD_HPP_INCLUDED
#define PASSWORD_HPP_INCLUDED

#include "conf.h"
#include <string>

#include "sysdep.h"
#include "structs.h"

/**
* Вобщем вынес всю работу с паролями в один файл + прикрутил md5, хотя толку с него сейчас тоже уже не особо много.
* Зато если вдруг надумается подключить какую-нить либу с ядреным алгоритмом - все уже под это дело готово.
*/
namespace Password
{

void set_password(CHAR_DATA *ch, const std::string &pwd);
void set_password_to_email(CHAR_DATA *ch, const std::string &pwd);
void send_password(std::string email, std::string password);
void send_password(std::string email, std::string password, std::string name);
void set_all_password_to_email(const char* email, const std::string &pwd, const std::string &name);
bool compare_password(CHAR_DATA *ch, const std::string &pwd);
bool compare_password(std::string const &hash, std::string const &pass);
bool check_password(const CHAR_DATA *ch, const char *pwd);

std::string generate_md5_hash(const std::string &pwd);

extern const unsigned int MIN_PWD_LENGTH;
extern const unsigned int MAX_PWD_LENGTH;
extern const char *BAD_PASSWORD;

} // namespace Password

#endif // PASSWORD_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
