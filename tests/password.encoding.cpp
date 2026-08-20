// Хэш пароля считается по ДИСКОВЫМ байтам (issue #3681).
//
// Хэш -- сохранённые данные, и посчитан он когда-то по кодировке диска. Когда движок
// перешёл на UTF-8, те же буквы стали давать другие байты, и у всех, чей пароль
// с кириллицей, вход отвалился: 'Bad PW' при верном пароле. Латинские работали, потому
// что в ASCII обе кодировки совпадают.
//
// Проверять хэш напрямую нельзя -- соль случайная, -- поэтому свойство проверяется через
// наблюдаемое следствие: сравнение смотрит на дисковую форму пароля, а не на байты
// в памяти.

#include "administration/password.h"
#include "utils/native_text.h"

#include <gtest/gtest.h>

#include <string>

namespace {

// "пароль" в UTF-8 байтами, чтобы тест не зависел от того, как редактор сохранил файл.
const std::string kCyrillic = "\xD0\xBF\xD0\xB0\xD1\x80\xD0\xBE\xD0\xBB\xD1\x8C";

}  // namespace

TEST(PasswordEncoding, CyrillicPasswordMatchesItself) {
	const std::string hash = Password::generate_md5_hash(kCyrillic);
	EXPECT_TRUE(Password::compare_password(hash, kCyrillic));
}

TEST(PasswordEncoding, WrongCyrillicPasswordIsRejected) {
	const std::string hash = Password::generate_md5_hash(kCyrillic);
	const std::string other = kCyrillic + "\xD1\x8B";   // тот же пароль плюс "ы"
	EXPECT_FALSE(Password::compare_password(hash, other));
}

TEST(PasswordEncoding, HashIsTakenOverTheOnDiskForm) {
	// Ключевое свойство. Два разных набора байт в памяти, у которых ОДНА дисковая форма:
	// длинное тире на диске становится обычным дефисом (в KOI8-R его попросту нет, см.
	// словарь замен). Если хэш считать по байтам памяти, эти пароли разойдутся; если по
	// дисковой форме -- совпадут. Именно на этом расхождении и отвалился вход.
	const std::string with_em_dash = "pa\xE2\x80\x94rol";   // pa—rol
	const std::string with_hyphen = "pa-rol";

	ASSERT_NE(with_em_dash, with_hyphen) << "проверка построена на том, что в памяти они разные";
	ASSERT_EQ(native_text::to_disk(with_em_dash), native_text::to_disk(with_hyphen))
		<< "...а на диске -- одинаковые";

	const std::string hash = Password::generate_md5_hash(with_em_dash);
	EXPECT_TRUE(Password::compare_password(hash, with_hyphen))
		<< "хэш обязан считаться по дисковой форме, иначе старые хэши не сойдутся";
}

TEST(PasswordEncoding, AsciiIsUnaffected) {
	// Латиница в обеих кодировках одинакова -- на ней баг и не проявлялся.
	//
	// Неверный пароль отличается ПЕРВОЙ буквой, а не последней, нарочно: на macOS crypt() --
	// классический DES, он смотрит только первые восемь символов. Пара, различающаяся
	// десятым символом, там даёт один и тот же хэш, и проверка «неверный пароль отвергнут»
	// молча превращается в свою противоположность.
	const std::string hash = Password::generate_md5_hash("parolparol");
	EXPECT_TRUE(Password::compare_password(hash, "parolparol"));
	EXPECT_FALSE(Password::compare_password(hash, "xarolparol"));
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
