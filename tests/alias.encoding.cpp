// Файл синонимов и кодировка -- issue #3787.
//
// Это самый хрупкий формат в userdata: он хранит ДЛИНЫ строк в байтах, а читатель берёт
// ровно столько байт, сколько записано, и только потом приводит их к нативной кодировке.
// Значит длина обязана считаться по тем же байтам, что уходят в файл. Пока движок писал
// KOI8-R, длины были koi8-байтовые; теперь пишется нативный UTF-8, где русская буква
// занимает два байта, и длина должна вырасти вместе с ним. Разъедется одно -- чтение
// пойдёт с середины строки, и посыплются все синонимы игрока разом.
//
// Тесты держат три вещи: длина совпадает с тем, что реально записано; цикл запись-чтение
// возвращает текст без потерь; файл, оставшийся в KOI8-R (со старыми длинами), читается
// по-прежнему.
//
// Сам тест -- чистый ASCII: кириллица записана байтовыми escape-последовательностями UTF-8
// и приводится к нативной кодировке через native_text.

#include "engine/ui/alias.h"

#include "engine/db/db.h"
#include "engine/entities/char_data.h"
#include "simulator/character_builder.h"
#include "utils/native_text.h"
#include "utils/translit_koi8.h"
#include "utils/utils.h"

#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace {

// "смотреть" и "смотреть на север" -- синоним и замена, оба с русскими буквами.
const char *const kAliasUtf8 = "\xD1\x81\xD0\xBC";
const char *const kReplacementUtf8 =
	"\xD1\x81\xD0\xBC\xD0\xBE\xD1\x82\xD1\x80\xD0\xB5\xD1\x82\xD1\x8C \xD0\xBD\xD0\xB0 \xD1\x81\xD0\xB5\xD0\xB2\xD0\xB5\xD1\x80";

// Имя латиницей: транслитерация имени в имя файла нас тут не проверяется, а латиница
// даёт предсказуемый путь.
const char *const kPlayerName = "Aliastest";

std::string AliasToNative(const char *utf8) {
	return native_text::from_koi8(codepages::Utf8ToKoi8(utf8));
}

std::string AliasFilePath(const char *name) {
	char path[512];
	EXPECT_TRUE(get_filename(name, path, kAliasFile));
	return path;
}

std::string ReadFileBytes(const std::string &path) {
	std::ifstream in(path, std::ios::binary);
	std::ostringstream buffer;
	buffer << in.rdbuf();
	return buffer.str();
}

// Один синоним в списке персонажа. Владение отдаётся CharData, как и в игре.
void SetSingleAlias(const CharData::shared_ptr &ch, const std::string &alias, const std::string &replacement) {
	auto *node = new alias_data;
	node->alias = str_dup(alias.c_str());
	node->replacement = str_dup(replacement.c_str());
	node->type = kAliasComplex;
	node->next = nullptr;
	GET_ALIASES(ch.get()) = node;
}

CharData::shared_ptr MakeAliasOwner(simulator::CharacterBuilder &builder) {
	builder.make_basic_player(static_cast<short>(ECharClass::kSorcerer), 10);
	builder.set_name(kPlayerName);
	return builder.get();
}

// Каталог движок заводит на старте; тесты этот шаг не проходят, поэтому создаём сами.
class AliasFile {
 public:
	AliasFile() : path_(AliasFilePath(kPlayerName)) {
		std::filesystem::create_directories(std::filesystem::path(path_).parent_path());
		std::filesystem::remove(path_);
	}
	~AliasFile() { std::error_code ec; std::filesystem::remove(path_, ec); }
	const std::string &path() const { return path_; }

 private:
	std::string path_;
};

}  // namespace

TEST(AliasEncoding, DeclaredLengthMatchesWhatWasWritten) {
	AliasFile file;
	simulator::CharacterBuilder builder;
	auto ch = MakeAliasOwner(builder);
	const std::string alias = AliasToNative(kAliasUtf8);
	const std::string replacement = AliasToNative(kReplacementUtf8);
	SetSingleAlias(ch, alias, replacement);

	WriteAliases(ch.get());

	// Формат: <длина>\n<строка>\n<длина>\n<строка>\n<тип>\n. Разбираем ровно так, как это
	// делает ReadAliases -- по объявленной длине, а не по переводу строки.
	const std::string body = ReadFileBytes(file.path());
	ASSERT_FALSE(body.empty()) << "файл синонимов не записался";

	// Разделитель пропускаем по факту, а не считая его одним байтом: движок открывает файл
	// синонимов в текстовом режиме (fopen "w"/"r"), и на Windows на диск уходит \r\n. Самому
	// движку это безразлично -- он и пишет, и читает в одном режиме, -- а тест смотрит байты.
	const auto skip_eol = [&body](std::size_t pos) {
		if (pos < body.size() && body[pos] == '\r') {
			++pos;
		}
		if (pos < body.size() && body[pos] == '\n') {
			++pos;
		}
		return pos;
	};

	std::size_t pos = 0;
	for (const std::string *expected : {&alias, &replacement}) {
		const std::size_t eol = body.find('\n', pos);
		ASSERT_NE(eol, std::string::npos) << "нет длины";
		std::string digits = body.substr(pos, eol - pos);
		if (!digits.empty() && digits.back() == '\r') {
			digits.pop_back();
		}
		const int declared = std::stoi(digits);
		pos = eol + 1;
		EXPECT_EQ(static_cast<std::size_t>(declared), expected->size())
				<< "длина в файле должна совпадать с длиной записанной строки в байтах";
		EXPECT_EQ(body.substr(pos, static_cast<std::size_t>(declared)), *expected);
		pos = skip_eol(pos + static_cast<std::size_t>(declared));
	}
}

TEST(AliasEncoding, WriteReadRoundTrip) {
	AliasFile file;
	simulator::CharacterBuilder builder;
	auto ch = MakeAliasOwner(builder);
	const std::string alias = AliasToNative(kAliasUtf8);
	const std::string replacement = AliasToNative(kReplacementUtf8);
	SetSingleAlias(ch, alias, replacement);

	WriteAliases(ch.get());

	simulator::CharacterBuilder reader_builder;
	auto reloaded = MakeAliasOwner(reader_builder);
	ReadAliases(reloaded.get());

	ASSERT_NE(GET_ALIASES(reloaded.get()), nullptr) << "синонимы не прочитались";
	EXPECT_STREQ(GET_ALIASES(reloaded.get())->alias, alias.c_str());
	EXPECT_STREQ(GET_ALIASES(reloaded.get())->replacement, replacement.c_str());
}

TEST(AliasEncoding, ReadsAFileLeftInKoi8) {
	// Старый файл: и текст, и длины -- в KOI8-R. Он обязан читаться по-прежнему, иначе
	// перевод данных пришлось бы делать одномоментно со сменой бинаря.
	AliasFile file;
	const std::string alias_koi8 = codepages::Utf8ToKoi8(kAliasUtf8);
	const std::string replacement_koi8 = codepages::Utf8ToKoi8(kReplacementUtf8);
	{
		std::ofstream out(file.path(), std::ios::binary);
		out << alias_koi8.size() << "\n" << alias_koi8 << "\n"
			<< replacement_koi8.size() << "\n" << replacement_koi8 << "\n"
			<< kAliasComplex << "\n";
	}

	simulator::CharacterBuilder builder;
	auto ch = MakeAliasOwner(builder);
	ReadAliases(ch.get());

	ASSERT_NE(GET_ALIASES(ch.get()), nullptr) << "старый файл не прочитался";
	EXPECT_STREQ(GET_ALIASES(ch.get())->alias, AliasToNative(kAliasUtf8).c_str());
	EXPECT_STREQ(GET_ALIASES(ch.get())->replacement, AliasToNative(kReplacementUtf8).c_str());
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
