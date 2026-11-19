#include <gtest/gtest.h>

#include "engine/network/naws.h"
#include "engine/network/telnet.h"

#include <string>

// NAWS: клиент сообщает размер окна, и по нему ширина переноса совпадает с настоящим окном.
// В последовательности две ловушки -- байт 255 внутри удваивается, и она может прийти
// не целиком; обе проверяются здесь, потому что сетевой цикл на них уже спотыкался бы молча.

namespace {

std::string Naws(std::initializer_list<int> bytes) {
	std::string out;
	out += static_cast<char>(IAC);
	out += static_cast<char>(SB);
	out += static_cast<char>(TELOPT_NAWS);
	for (int byte : bytes) {
		out += static_cast<char>(byte);
	}
	return out;
}

std::string End() {
	std::string out;
	out += static_cast<char>(IAC);
	out += static_cast<char>(SE);
	return out;
}

}  // namespace

TEST(NawsParse, ReadsWidthAndHeight) {
	const auto bytes = Naws({0, 120, 0, 40}) + End();

	const auto parsed = naws::Parse(bytes);

	EXPECT_TRUE(parsed.complete);
	EXPECT_EQ(120, parsed.width);
	EXPECT_EQ(40, parsed.height);
	EXPECT_EQ(bytes.size(), parsed.consumed);
}

TEST(NawsParse, ReadsWideWindow) {
	// 300 знаков = 0x012C: старший байт значащий
	const auto parsed = naws::Parse(Naws({1, 0x2C, 0, 50}) + End());

	EXPECT_TRUE(parsed.complete);
	EXPECT_EQ(300, parsed.width);
	EXPECT_EQ(50, parsed.height);
}

TEST(NawsParse, UnescapesDoubledIac) {
	// Окно 255 на 255: байт 255 передаётся как IAC IAC -- и в ширине, и в высоте,
	// иначе он оборвал бы разбор
	const auto parsed = naws::Parse(Naws({0, 255, 255, 0, 255, 255}) + End());

	EXPECT_TRUE(parsed.complete);
	EXPECT_EQ(255, parsed.width);
	EXPECT_EQ(255, parsed.height);
}

TEST(NawsParse, IncompleteSequenceWaitsForMore) {
	// Пришли только два байта размера -- вырезать нечего, ждём следующего чтения
	const auto parsed = naws::Parse(Naws({0, 120}));

	EXPECT_FALSE(parsed.complete);
	EXPECT_EQ(0u, parsed.consumed);
}

TEST(NawsParse, GarbageInsteadOfEndIsCutOff) {
	// Вместо IAC SE пришло что-то другое: размер не берём, но байты съедаем,
	// иначе они уедут игроку в строку ввода
	std::string bytes = Naws({0, 120, 0, 40});
	bytes += static_cast<char>(IAC);
	bytes += static_cast<char>(GA);

	const auto parsed = naws::Parse(bytes);

	EXPECT_FALSE(parsed.complete);
	EXPECT_GT(parsed.consumed, 0u);
	EXPECT_LE(parsed.consumed, bytes.size());
}

TEST(NawsParse, ExtraBytesAfterEndAreLeftAlone) {
	const auto bytes = Naws({0, 90, 0, 24}) + End() + "смотреть";

	const auto parsed = naws::Parse(bytes);

    EXPECT_TRUE(parsed.complete);
	EXPECT_EQ(90, parsed.width);
	// Съедена только сама последовательность: команда игрока остаётся в буфере
	EXPECT_EQ(9u, parsed.consumed);
}
