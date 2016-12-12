#ifndef __BOARDS_CONSTANTS_HPP__
#define __BOARDS_CONSTANTS_HPP__

#include <string>

namespace Boards
{
	// предметы, на которые вешаем спешиалы для старого способа работы с досками
	constexpr int GODGENERAL_BOARD_OBJ = 250;
	constexpr int GENERAL_BOARD_OBJ = 251;
	constexpr int GODCODE_BOARD_OBJ = 253;
	constexpr int GODPUNISH_BOARD_OBJ = 257;
	constexpr int GODBUILD_BOARD_OBJ = 259;
	// максимальный размер сообщения
	constexpr int MAX_MESSAGE_LENGTH = 4096;
	// мин.левел для поста на общих досках
	constexpr int MIN_WRITE_LEVEL = 6;
	// максимальное кол-во сообщений на одной доске
	constexpr unsigned int MAX_BOARD_MESSAGES = 200;
	// максимальное кол-во сообщений на спец.досках
	constexpr unsigned int MAX_REPORT_MESSAGES = 999;

	namespace constants
	{
		extern const char *OVERFLOW_MESSAGE;
		extern const char* CHANGELOG_FILE_NAME;

		namespace loader_formats
		{
			extern const char* GIT;
		}
	}
}

#endif // __BOARDS_CONSTANTS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
