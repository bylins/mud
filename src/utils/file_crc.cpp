// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#include "file_crc.h"
#include "engine/db/player_index.h"

#include "engine/db/db.h"
#include "logger.h"
#include "utils/utils.h"
#include "engine/ui/interpreter.h"
#include "engine/core/comm.h"

namespace FileCRC {
void add_message(const char *text, ...) __attribute__((format(printf, 1, 2)));

class PlayerCRC {
 public:
	PlayerCRC() : player(0), textobjs(0), timeobjs(0) {};
	std::string name; // имя игрока
	uint32_t player; // crc .player
	uint32_t textobjs; // crc .textobjs
	uint32_t timeobjs; // crc .timeobjs
	// TODO: мб кланы еще
};

typedef std::shared_ptr<PlayerCRC> CRCListPtr;
typedef std::map<long, CRCListPtr> CRCListType;
const char CRC_UID = '*';

CRCListType crc_list;
// лог иммам
std::string message;
// флажок необходимости записи файла чексумм
bool need_save = false;

// * Вывод сообщения в сислог, иммлог и лог show crc.
void add_message(const char *text, ...) {
	if (!text) return;
	va_list args;
	char out[kMaxStringLength];

	va_start(args, text);
	vsprintf(out, text, args);
	va_end(args);

	mudlog(out, DEF, kLvlImmortal, SYSLOG, true);
	message += out + std::string("\r\n");
}

unsigned int CRC32_function(const char *buf, unsigned long len) {
	unsigned long crc_table[256];
	unsigned long crc;
	for (int i = 0; i < 256; i++)
	{
		crc = i;
		for (int j = 0; j < 8; j++)
			crc = crc & 1 ? (crc >> 1) ^ 0xEDB88320UL : crc >> 1;
		crc_table[i] = crc;
	};
	crc = 0xFFFFFFFFUL;
	while (len--)
	crc = crc_table[(crc ^ *buf++) & 0xFF] ^ (crc >> 8);
	return crc ^ 0xFFFFFFFFUL;
}

// * Подсчет crc для строки.
uint32_t calculate_str_crc(const std::string &text) {
	return CRC32_function(text.c_str(), text.size());
}

// * Загрузка глобального списка crc.
void load() {
	const char *file_name = LIB_PLRSTUFF"crc.lst";
	std::ifstream file(file_name);
	if (!file.is_open()) {
		add_message("SYSERROR: не удалось открыть файл на чтение: %s", file_name);
		return;
	}

	std::string buffer, textobjs_buf, timeobjs_buf;
	std::getline(file, buffer, CRC_UID);
	std::stringstream stream(buffer);

	long uid;
	while (stream >> uid) {
		if (!(stream >> buffer >> textobjs_buf >> timeobjs_buf)) {
			add_message("SYSERROR: не удалось прочитать файл: %s, last uid: %ld", file_name, uid);
			return;
		}

		std::string name = GetNameByUnique(uid);
		if (name.empty()) {
			log("FileCRC: UID %ld - персонажа не существует.", uid);
			continue;
		}
		CRCListPtr tmp_crc(new PlayerCRC);
		tmp_crc->name = name;
		try {
			tmp_crc->player = std::stoul(buffer, nullptr, 10);
			tmp_crc->textobjs = std::stoul(textobjs_buf, nullptr, 10);
			tmp_crc->timeobjs = std::stoul(timeobjs_buf, nullptr, 10);
		}
		catch (const std::invalid_argument &) {
			add_message("FileCrc: ошибка чтения crc (%s, %s), uid: %ld", buffer.c_str(), textobjs_buf.c_str(), uid);
			break;
		}
		crc_list[uid] = tmp_crc;
	}

	bool checked = false;
	if ((file >> buffer)) {
		uint32_t prev_crc;
		try {
			prev_crc = std::stoul(buffer, nullptr, 10);
		}
		catch (const std::invalid_argument &) {
			add_message("SYSERROR: ошибка чтения total crc (%s)", buffer.c_str());
			return;
		}
		stream.clear();
		const uint32_t crc = calculate_str_crc(stream.str());
		if (crc != prev_crc) {
			add_message("SYSERROR: несовпадение контрольной суммы файла: %s", file_name);
			return;
		}
		checked = true;
	}

	if (!checked)
		add_message("SYSERROR: не произведена сверка контрольной суммы файла: %s", file_name);
}

/**
* Запись глобального списка чек-сумм (дергается раз в секунду для проверки необходимости сейва).
* \param force_save - запись файла в любом случае (при шатдауне мада), по умолчанию false.
*/
void save(bool force_save) {
	if (!need_save && !force_save) return;

	need_save = false;
	std::stringstream out;

	for (CRCListType::const_iterator it = crc_list.begin(); it != crc_list.end(); ++it) {
		out << it->first << " "
			<< it->second->player << " "
			<< it->second->textobjs << " "
			<< it->second->timeobjs << "\r\n";
	}

	// crc32 самого файла пишется ему же в конец
	const uint32_t crc = calculate_str_crc(out.str());

	// это все в конце, чтобы не завалить на креше файл
	const char *file_name = LIB_PLRSTUFF"crc.lst";
	std::ofstream file(file_name);
	if (!file.is_open()) {
		add_message("SYSERROR: не удалось открыть файл на запись: %s", file_name);
		return;
	}
	file << out.rdbuf() << CRC_UID << " " << crc << "\r\n";
	file.close();
}

void create_message(std::string &name, EType file, const uint32_t &expected, const uint32_t &calculated) {
	char time_buf[20];
	time_t ct = time(nullptr);
	strftime(time_buf, sizeof(time_buf), "%d-%m-%y %H:%M:%S", localtime(&ct));
	std::string file_type;
	switch (file) {
		case kPlayer:   file_type = "player";   break;
		case kTextObjs: file_type = "textobjs"; break;
		case kTimeObjs: file_type = "timeobjs"; break;
	}

	add_message("%s несовпадение контрольной суммы %s файла: %s (expected: %u; calculated: %u)",
				time_buf,
				file_type.c_str(),
				name.c_str(),
				static_cast<unsigned>(expected),
				static_cast<unsigned>(calculated));
}

// * Вывод лога событий имму по show crc.
void show(CharData *ch) {
	if (message.empty())
		SendMsgToChar("Вроде ничего не происходило...\r\n", ch);
	else
		SendMsgToChar(message.c_str(), ch);
}

// Внутренний upsert: создаёт запись uid при отсутствии и кладёт CRC в поле
// по типу файла. Флаг need_save -- забота вызывающего.
static void set_crc_field(long uid, EType file, uint32_t crc) {
	auto it = crc_list.find(uid);
	if (it == crc_list.end()) {
		it = crc_list.emplace(uid, CRCListPtr(new PlayerCRC)).first;
	}
	switch (file) {
		case kPlayer:   it->second->player = crc;   break;
		case kTextObjs: it->second->textobjs = crc; break;
		case kTimeObjs: it->second->timeobjs = crc; break;
	}
	it->second->name = GetNameByUnique(uid);
}

// Чтение текущего CRC поля по типу файла (для сверки).
static uint32_t get_crc_field(const PlayerCRC &p, EType file) {
	switch (file) {
		case kPlayer:   return p.player;
		case kTextObjs: return p.textobjs;
		case kTimeObjs: return p.timeobjs;
	}
	return 0;  // недостижимо: switch исчерпывающий по EType
}

// Записывает CRC из готового буфера в памяти, без перечитывания файла.
// Вызывается при сохранении: CRC считается из того же буфера, что и на диск.
void update_from_content(long uid, EType file, const char *data, std::size_t len) {
	set_crc_field(uid, file, CRC32_function(data, static_cast<unsigned long>(len)));
	need_save = true;
}

// Сброс CRC файла игрока в 0: файл удалён (Crash_delete_files). Раньше для
// этого звали check_crc на уже удалённом файле, чтобы получить 0 чтением.
std::size_t forget_all() {
	const std::size_t forgotten = crc_list.size();
	crc_list.clear();
	need_save = true;
	save(true);
	return forgotten;
}

void reset(long uid, EType file) {
	set_crc_field(uid, file, 0);
	need_save = true;
}

// Сверка CRC из готового буфера со снимком в crc_list (вместо повторного
// чтения файла на загрузке). Возвращает true при совпадении; при расхождении
// -- сообщение имму в лог и false. Если снимка ещё нет -- заводит базовый
// (как делал check_crc для режимов сверки) и возвращает true.
bool verify_from_content(long uid, EType file, const char *data, std::size_t len) {
	const uint32_t crc = CRC32_function(data, static_cast<unsigned long>(len));
	auto it = crc_list.find(uid);
	if (it == crc_list.end()) {
		set_crc_field(uid, file, crc);
		return true;
	}
	const uint32_t stored = get_crc_field(*it->second, file);
	// Ноль в поле player означает "снимка нет", а не "ожидаем нулевую сумму". Запись могла
	// появиться из-за сброса соседнего поля: истекла рента -> reset для objs заводит запись со
	// всеми нулями, а в player CRC ни разу не писали. Раньше такой ноль сверялся как настоящий
	// ожидаемый CRC, и игрок при каждом входе давал имму ложную тревогу; сам собой этот ноль не
	// чинился, потому что при несовпадении снимок не обновляется.
	//
	// Только для player: файл персонажа существует всегда, и reset для него не вызывается. У
	// файлов объектов ноль осмыслен -- это "файл удален сервером", и появление такого файла
	// заново как раз должно поднимать тревогу.
	if (file == kPlayer && stored == 0) {
		set_crc_field(uid, file, crc);
		return true;
	}
	if (stored != crc) {
		create_message(it->second->name, file, stored, crc);
		return false;
	}
	return true;
}

} // namespace FileCRC

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
