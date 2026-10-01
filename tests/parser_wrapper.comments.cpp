// Комментарии XML-конфига должны пережить цикл загрузки и сохранения (issue.xml-comments).
//
// Пояснения в cfg писали люди, и их много: в shops.xml 57 комментариев, в affects.xml 15, включая
// блоки в шапке. pugixml по умолчанию (parse_default) выбрасывает их ещё при разборе, так что
// первое же сохранение из ведуна стирало их все -- молча, вместе с правкой одного поля.
//
// Здесь закреплено и обратное: наружу, в обход по DataNode, комментарии не попадают. Иначе
// Children() отдавал бы загрузчику узел без имени, а MoveChildUp/Down считали бы комментарий
// соседним элементом.

#include "utils/parser_wrapper.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace comments_test {

std::string ReadWhole(const std::filesystem::path &path) {
	std::ifstream in(path, std::ios::binary);
	std::ostringstream buffer;
	buffer << in.rdbuf();
	return buffer.str();
}

void WriteWhole(const std::filesystem::path &path, const std::string &text) {
	std::ofstream out(path, std::ios::binary);
	out.write(text.data(), static_cast<std::streamsize>(text.size()));
}

// Черновой файл, удаляемый в деструкторе: упавшая проверка не оставит мусора.
class TempFile {
 public:
	explicit TempFile(const char *name) : path_(std::filesystem::temp_directory_path() / name) {}
	~TempFile() { std::error_code ec; std::filesystem::remove(path_, ec); }
	[[nodiscard]] const std::filesystem::path &path() const { return path_; }

 private:
	std::filesystem::path path_;
};

const char *const kConfigWithComments =
	"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
	"<!-- shapka: zachem etot fail nuzhen -->\n"
	"<shops>\n"
	"\t<!-- poyasnenie k lavke -->\n"
	"\t<shop vnum=\"1\" />\n"
	"\t<shop vnum=\"2\" />\n"
	"</shops>\n";

}  // namespace comments_test

TEST(ParserWrapperComments, SurviveLoadSaveCycle) {
	comments_test::TempFile file("bylins_cfg_comments.xml");
	comments_test::WriteWhole(file.path(), comments_test::kConfigWithComments);

	{
		parser_wrapper::DataNode doc(file.path());
		ASSERT_TRUE(doc.IsNotEmpty());
		ASSERT_TRUE(doc.Save(file.path()));
	}

	const std::string saved = comments_test::ReadWhole(file.path());
	EXPECT_NE(saved.find("shapka: zachem etot fail nuzhen"), std::string::npos)
		<< "комментарий в шапке потерялся при сохранении";
	EXPECT_NE(saved.find("poyasnenie k lavke"), std::string::npos)
		<< "комментарий внутри корня потерялся при сохранении";
}

TEST(ParserWrapperComments, AreInvisibleToTheReader) {
	comments_test::TempFile file("bylins_cfg_comments_walk.xml");
	comments_test::WriteWhole(file.path(), comments_test::kConfigWithComments);

	parser_wrapper::DataNode doc(file.path());
	ASSERT_TRUE(doc.IsNotEmpty());

	// Обход детей отдаёт только лавки: комментарий между ними в счёт не идёт.
	std::size_t shops = 0;
	for (auto &child : doc.Children()) {
		EXPECT_EQ(child.GetName(), std::string("shop")) << "в обход попал узел, который не элемент";
		++shops;
	}
	EXPECT_EQ(shops, 2u) << "ожидались ровно две лавки";

	// Переход к первому ребёнку тоже минует комментарий.
	ASSERT_TRUE(doc.GoToChild("shop"));
	EXPECT_EQ(doc.GetName(), std::string("shop"));
	EXPECT_TRUE(doc.HaveNext());
	doc.GoToNext();
	EXPECT_EQ(doc.GetName(), std::string("shop"));
	EXPECT_FALSE(doc.HaveNext()) << "после второй лавки элементов больше нет";
}
