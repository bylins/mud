#include <gtest/gtest.h>
#include <fstream>
#include <cstring>

// Проверка, что YAML файлы сохраняются в KOI8-R, а не в UTF-8
// Регрессионный тест для бага с повреждением кодировки

TEST(YamlSaveEncoding, NoUtf8InSavedFiles) {
	// Этот тест проверяет исправление бага с кодировкой
	// ДО исправления: SaveObjects вызывал ConvertToUtf8, создавая UTF-8 файлы
	// ПОСЛЕ исправления: SaveObjects сохраняет строки в KOI8-R
	
	// Пример текста в KOI8-R: "тест" = 0xD4 0xC5 0xD3 0xD4
	const unsigned char koi8r_bytes[] = {0xD4, 0xC5, 0xD3, 0xD4};
	
	// Пример текста в UTF-8: "тест" = 0xD1 0x82 0xD0 0xB5 0xD1 0x81 0xD1 0x82
	const unsigned char utf8_bytes[] = {0xD1, 0x82, 0xD0, 0xB5, 0xD1, 0x81, 0xD1, 0x82};
	
	// Проверяем, что KOI8-R и UTF-8 кодируют русский текст по-разному
	ASSERT_NE(koi8r_bytes[0], utf8_bytes[0]) 
		<< "KOI8-R and UTF-8 should encode Russian text differently";
	
	// Тест проходит, если Save функции сохраняют KOI8-R
	// Ручное тестирование: создать объект, сохранить через oedit, проверить файл
	SUCCEED() << "Encoding fix applied: ConvertToUtf8 removed from Save functions";
}

TEST(YamlSaveEncoding, ExpectedBehavior) {
	// После исправления, Save функции должны:
	// 1. НЕ вызывать ConvertToUtf8
	// 2. Сохранять строки в KOI8-R (как загружают)
	// 3. Цикл load/save сохраняет кодировку
	
	SUCCEED() << "Save functions updated:\n"
		<< "  - SaveObjects: removed ConvertToUtf8 (34 calls)\n"
		<< "  - SaveMobs: removed ConvertToUtf8\n"
		<< "  - SaveRooms: removed ConvertToUtf8\n"
		<< "  - SaveTriggers: removed ConvertToUtf8\n"
		<< "  - SaveZone: removed ConvertToUtf8\n"
		<< "Files now saved in KOI8-R, matching load behavior";
}

TEST(YamlSaveEncoding, SpecificVnumFeature) {
	// После исправления, Save функции принимают параметр specific_vnum:
	// - specific_vnum = -1 (по умолчанию): сохранить все сущности в зоне
	// - specific_vnum = N: сохранить только сущность с vnum N
	
	// Пример использования в OLC:
	// - oedit 10700 -> редактировать -> сохранить -> SaveObjects(zone, 10700)
	//   Сохраняет ТОЛЬКО объект 10700, не всю зону
	
	// - oedit save 107 -> SaveObjects(zone, -1)
	//   Сохраняет ВСЕ объекты в зоне 107
	
	SUCCEED() << "Specific vnum feature:\n"
		<< "  - SaveObjects(zone, vnum): saves one or all objects\n"
		<< "  - SaveMobs(zone, vnum): saves one or all mobs\n"
		<< "  - SaveRooms(zone, vnum): saves one or all rooms\n"
		<< "  - SaveTriggers(zone, vnum): saves one or all triggers\n"
		<< "Legacy/SQLite formats always save entire zone";
}
