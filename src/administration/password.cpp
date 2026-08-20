// password.cpp
// Copyright (c) 2007 Krodo
// Part of Bylins http://www.mud.ru

#if defined(__APPLE__) || defined(__MACH__)
#include <unistd.h>
#endif
#include "password.h"
#include "engine/ui/interpreter.h"
#include "engine/entities/char_data.h"
#include "engine/entities/char_player.h"
#include "utils/native_text.h"
// для ручного отключения крипования (на локалке лучше собирайте через make test и не парьтесь)
//#define NOCRYPT
// в случае сборки без криптования просто пишем пароль в открытом виде
#if defined(NOCRYPT)
#define CRYPT(a,b) ((char *) (a))
#else
#if defined(__unix__) || defined(__unix) || defined(__APPLE__)
#include <sys/param.h>
#endif
#ifdef BSD
#include <unistd.h>
#else
#include <crypt.h>
#endif
#define CRYPT(a, b) ((char *) crypt((a),(b)))
#endif

namespace Password {

// Хэш пароля -- сохранённые данные, и посчитан он когда-то по дисковым байтам (KOI8-R).
// Движок теперь держит текст нативным, поэтому кириллический пароль дал бы другие байты
// и не сошёлся бы с сохранённым хэшем. Приводим к дисковой форме перед crypt: старые хэши
// продолжают сходиться, а новые остаются пригодными для отката (issue #3681).
static std::string password_bytes(const std::string &pwd) {
	return native_text::to_disk(pwd);
}

const char *BAD_PASSWORD = "Пароль должен быть от 8 до 50 символов и не должен быть именем персонажа.";
const unsigned int MIN_PWD_LENGTH = 8;
const unsigned int MAX_PWD_LENGTH = 50;

// * Генерация хэша с более-менее рандомным сальтом
std::string generate_md5_hash(const std::string &pwd) {
#ifdef NOCRYPT
	// И здесь дисковая форма: сравнение всё равно приводит пароль к ней, а хранить
	// нативную значило бы, что кириллический пароль не сойдётся сам с собой. Сборки
	// без crypt() -- это Windows, macOS и -Dnocrypt=true (issue #3681).
	return password_bytes(pwd);
#else
	char key[14];
	key[0] = '$';
	key[1] = '1';
	key[2] = '$';
	for (int i = 3; i < 12; i++) {
		int c = number(0, 63);
		if (c < 26)
			key[i] = c + 'a';
		else if (c < 52)
			key[i] = c - 26 + 'A';
		else if (c < 62)
			key[i] = c - 52 + '0';
		else
			key[i] = '/';
	}
	key[12] = '$';
	key[13] = '\0';
	return CRYPT(password_bytes(pwd).c_str(), key);
#endif
}

/**
* Генерируем новый хэш и пишем его чару
* TODO: в принципе можно и совместить с методом плеера.
*/
void set_password(CharData *ch, const std::string &pwd) {
	ch->set_passwd(generate_md5_hash(pwd));
}

// отправляет пароль на мыло через внешний скрипт
// такое, конечно же, правильнее делать через либу openssl прямо в плюсах
// но там гемора много
void send_password(std::string email, std::string password, std::string name) {
	std::string cmd_line = "python3 change_pass.py " + email + " " + password + " " + name + " &";
	auto result = system(cmd_line.c_str());
	UNUSED_ARG(result);
}

void send_password(std::string email, std::string password) {
	std::string cmd_line = "python3 change_pass.py " + email + " " + password + " &";
	auto result = system(cmd_line.c_str());
	UNUSED_ARG(result);
}

// Дубликат set_password, который отправляет пароль на мыло
void set_password_to_email(CharData *ch, const std::string &pwd) {
	ch->set_passwd(generate_md5_hash(pwd));
	send_password(std::string(GET_EMAIL(ch)), pwd.c_str(), std::string(GET_NAME(ch)));
}

// дубликат set_password, который отправляет пароль на мыло
// и говорит, что всем его персонажам изменены пароли
void set_all_password_to_email(const char *email, const std::string &pwd, const std::string &name) {
	send_password(std::string(email), pwd.c_str(), name.c_str());
}

/**
* Тип хэша у плеера
* \return  0 - des, 1 - md5
*/
bool get_password_type(const CharData *ch) {
	return CompareParam("$1$", ch->get_passwd());
}

/**
* Сравнение хэшей и конверт при необходимости в мд5
* \return 0 - не сошлось, 1 - сошлось
*/
bool compare_password(CharData *ch, const std::string &pwd) {
	bool result = 0;
	if (get_password_type(ch))
		result = CompareParam(ch->get_passwd(), CRYPT(password_bytes(pwd).c_str(), ch->get_passwd().c_str()), 1);
	else {
		// если пароль des сошелся - конвертим сразу в md5 (10 - бывший MAX_PWD_LENGTH)
		char *s = (char *) CRYPT(password_bytes(pwd).c_str(), ch->get_passwd().c_str());
		if (s && !strncmp(s, ch->get_passwd().c_str(), 10)) {
			set_password(ch, pwd);
			result = 1;
		} else if (s == nullptr) {
			SendMsgToChar("Возникли проблемы при проверке вашего пароля. Обратитесь к старшим богам для его сброса.\r\n",
						 ch);
			result = 0;
		}
	}
	return result;
}

/**
* Проверка пароля на длину и тупость
* \return 0 - некорректный пароль, 1 - корректный
*/
bool check_password(const CharData *ch, const char *pwd) {
// при вырубленном криптовании на локалке пароль можно ставить любой
#ifndef NOCRYPT
	if (!pwd) {
		return 0;
	}
	// Длина считается в символах: с UTF-8 кириллица занимает по два байта, и по strlen
	// восьмибуквенный русский пароль выглядел бы шестнадцатисимвольным (issue #3681).
	const std::size_t length = native_text::char_count(pwd);
	if (!str_cmp(pwd, GET_PC_NAME(ch)) || length > MAX_PWD_LENGTH || length < MIN_PWD_LENGTH)
		return 0;
#else
	UNUSED_ARG(ch);
	UNUSED_ARG(pwd);
#endif
	return 1;
}

/**
* Более универсальный аналог compare_password.
* \return 0 - не сошлось, 1 - сошлось
*/
bool compare_password(std::string const &hash, std::string const &pass) {
	return CompareParam(hash.c_str(), CRYPT(password_bytes(pass).c_str(), hash.c_str()), 1);
}

} // namespace Password

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
