// Загрузка доски с русским текстом: проверяет, что сообщения приходят в НАТИВНОЙ кодировке
// движка. Файл доски лежит на диске в KOI8-R и читается потоком, минуя FBFILE, поэтому это
// отдельная граница кодировки (issue #3681). Тест осмыслен в обеих сборках: литералы ниже
// компилируются в той же кодировке, в какой работает движок.

#include "gameplay/communication/boards/boards_types.h"
#include "gameplay/communication/boards/boards_formatters.h"
#include "utils/native_text.h"
#include "utils/utf8.h"

#include <gtest/gtest.h>

#include <fstream>
#include <string>

namespace {

const char *const kNewsSample = "data/boards/news.sample";

Boards::Board::shared_ptr LoadSampleBoard() {
	auto board = std::make_shared<Boards::Board>(Boards::NEWS_BOARD);
	board->set_file_name(kNewsSample);
	board->Load();
	return board;
}

}  // namespace

TEST(BoardsEncoding, MessagesLoadInTheNativeEncoding) {
	std::ifstream probe(kNewsSample);
	ASSERT_TRUE(probe.is_open()) << "нет тестовых данных " << kNewsSample;
	probe.close();

	const auto board = LoadSampleBoard();
	ASSERT_FALSE(board->empty()) << "доска не загрузилась";

	bool seen_cyrillic = false;
	for (std::size_t i = 0; i < board->messages_count(); ++i) {
		const auto message = board->get_message(i);
		for (const std::string *field : {&message->author, &message->subject, &message->text}) {
			if (field->empty()) {
				continue;
			}
			const bool has_high_byte = std::any_of(field->begin(), field->end(),
				[](char c) { return static_cast<unsigned char>(c) >= 0x80; });
			if (!has_high_byte) {
				continue;
			}
			seen_cyrillic = true;
			// В UTF-8-сборке текст обязан быть корректным UTF-8; в KOI8-R-сборке -- наоборот,
			// это KOI8-R, который валидным UTF-8 не является.
			EXPECT_TRUE(utf8::is_valid(*field))
				<< "поле не в нативной кодировке: " << *field;
		}
	}
	EXPECT_TRUE(seen_cyrillic) << "в тестовых данных не оказалось русского текста -- проверять нечего";
}

TEST(BoardsEncoding, AuthorNameSurvivesTheBoundary) {
	const auto board = LoadSampleBoard();
	ASSERT_FALSE(board->empty());

	// Автор одного из сообщений в news.sample -- "Стрибог".
	bool found = false;
	for (std::size_t i = 0; i < board->messages_count(); ++i) {
		if (board->get_message(i)->author == "Стрибог") {
			found = true;
			break;
		}
	}
	EXPECT_TRUE(found) << "имя автора не совпало -- текст доски пришёл не в той кодировке";
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

// issue #3681: тема сообщения на доске режется по ЗНАКАМ. Рез по длине строки под UTF-8
// укорачивал русскую тему вдвое и оставлял в конце один ведущий байт -- такая тема уезжала
// автору в "Тема сообщения укорочена до ..." и всем читающим доску в оповещении о новом
// сообщении, а клиенту уходил невалидный UTF-8.
TEST(BoardsEncoding, SubjectCutKeepsWholeCharacters) {
	// 45 русских знаков: под UTF-8 это 90 байт.
	std::string subject;
	for (int i = 0; i < 45; ++i) {
		subject += "я";
	}
	ASSERT_EQ(native_text::char_count(subject), 45u);

	const std::string cut = Boards::CutSubject(subject);
	EXPECT_EQ(native_text::char_count(cut), Boards::kSubjectMaxChars);
	EXPECT_EQ(utf8::find_invalid(cut), std::string_view::npos) << "тема обрезана посередине знака";

	// Тема короче предела не меняется -- вызывающий по этому и понимает, что резать не пришлось.
	const std::string short_subject = "короткая тема";
	EXPECT_EQ(Boards::CutSubject(short_subject), short_subject);
}
