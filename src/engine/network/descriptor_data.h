/**
 \authors Created by Sventovit
 \date 26.05.2021.
 \brief Интерфейс DescriptorData
 \details Заголовок/интерфейс класса, описывающего состояние соединения клиента с севером.
*/
#ifndef BYLINS_SRC_STRUCTS_DESCRIPTOR_DATA_H_
#define BYLINS_SRC_STRUCTS_DESCRIPTOR_DATA_H_

#include "gameplay/communication/boards/boards_types.h"
#include "gameplay/economics/ext_money.h"
#include "engine/structs/structs.h"
#include "engine/core/sysdep.h"
#include "engine/core/iosystem.h"
#include "engine/core/conf.h"
#include "utils/utils_string.h"

#include <string>

namespace Glory {
	class spend_glory;
}
namespace GloryConst {
	struct glory_olc;
}
namespace NamedStuff {
	struct stuff_node;
}
namespace MapSystem {
	struct Options;
}
namespace  obj_sets_olc {
	class sedit;
}

enum class EConState : uint8_t {
  kPlaying = 0,        // Playing - Nominal state //
  kClose = 1,            // Disconnecting     //
  kGetName = 2,        // By what name ..?     //
  kNameConfirm = 3,    // Did I get that right, x?   //
  kPassword = 4,        // Password:         //
  kNewpasswd = 5,        // Give me a password for x   //
  kCnfpasswd = 6,        // Please retype password: //
  kQsex = 7,            // Sex?           //
  kQclass = 8,        // Class?         //
  kRmotd = 9,            // PRESS RETURN after MOTD //
  kMenu = 10,            // Your choice: (main menu)   //
  kExdesc = 11,        // Enter a new description:   //
  kChpwdGetOld = 12, // Changing passwd: get old   //
  kChpwdGetNew = 13, // Changing passwd: get new   //
  kChpwdVrfy = 14,    // Verify new password     //
  kDelcnf1 = 15,        // Delete confirmation 1   //
  kDelcnf2 = 16,        // Delete confirmation 2   //
  kDisconnect = 17,    // In-game disconnection   //
  kOedit = 18,        //. OLC mode - object edit     . //
  kRedit = 19,        //. OLC mode - room edit       . //
  kZedit = 20,        //. OLC mode - zone info edit  . //
  kMedit = 21,        //. OLC mode - mobile edit     . //
  kTrigedit = 22,        //. OLC mode - trigger edit    . //
  kName2 = 23,
  kName3 = 24,
  kName4 = 25,
  kName5 = 26,
  kName6 = 27,
  kQreligion = 28,
  kRace = 29,
//	kLows = 30,	// не используется
  kGetKeytable = 31,
  kGetEmail = 32,
  kRollStats = 33,
  kMredit = 34,        // OLC mode - make recept edit //
  kQkin = 35,
  kWriteNote = 36,		// пишет записку
//	kQclass = 37,		// Удалено, номер можно использовать
  kMapMenu = 38,
//	kColor = 39,	// не используется
  kWriteboard = 40,        // написание на доску
  kClanedit = 41,            // команда house
  kNewChar = 42,
  kSpendGlory = 43,        // вливание славы через команду у чара
  kResetStats = 44,        // реролл статов при входе в игру
  kBirthplace = 45,        // выбираем где начать игру
  kWriteMod = 46,        // пишет клановое сообщение дня
  kGloryConst = 47,        // вливает славу2
  kNamedStuff = 48,        // редактирует именной стаф
  kResetKin = 49,        // выбор расы после смены/удаления оной (или иного испоганивания значения)
  kResetRace = 50,        // выбор РОДА посла смены/сброса оного
  kConsole = 51,            // Интерактивная скриптовая консоль
  kTorcExch = 52,        // обмен гривен
  kMenuStats = 53,        // оплата сброса стартовых статов из главного меню
  kSedit = 54,            // sedit - редактирование сетов
  kResetReligion = 55,    // сброс религии из меню сброса статов
  kRandomNumber = 56,    // Verification code entry: where player enter the game from new location
  kInit = 57,               // just connected
  kAdminAPI = 58            // Admin API connection
};
// Номера оставлены, чтобы было удобней ориентироваться в списке описаний -- Svent
// не забываем отражать новые состояния в connection_descriptions -- Krodo

const char *GetConDescription(EConState state);

struct DescriptorData {
	DescriptorData();

	void msdp_support(bool on);
	void msdp_add_report_variable(const std::string &name) { m_msdp_requested_report.insert(name); }
	void msdp_remove_report_variable(const std::string &name) { m_msdp_requested_report.erase(name); }
	bool msdp_need_report(const std::string &name) {
		return m_msdp_requested_report.find(name) != m_msdp_requested_report.end();
	}
	void msdp_report(const std::string &name);
	void msdp_report_changed_vars();

	void string_to_client_encoding(const char *in_str, char *out_str) const;
	auto get_character() const { return original ? original : character; }

	socket_t descriptor{};    // file descriptor for socket    //
	char host[kHostLength + 1]{};    // hostname          //
	byte bad_pws;        // number of bad pw attemps this login //
	byte idle_tics;        // tics idle at password prompt     //
	EConState state;        // state of 'connectedness'    //
	int desc_num;        // unique num assigned to desc      //
	time_t input_time;
	time_t login_time;    // when the person connected     //
	char *showstr_head;    // for keeping track of an internal str   //
	char **showstr_vector;    // for paging through texts      //
	int showstr_count;        // number of pages to page through  //
	int showstr_page;        // which page are we currently showing?   //
	utils::AbstractStringWriter::shared_ptr writer;        // for the modify-str system     //
	size_t max_str;        //      -        //
	char *backstr;        // added for handling abort buffers //
	int mail_to;        // uid for mail system
	int has_prompt;        // is the user at a prompt?             //
	char inbuf[kMaxRawInputLength]{};    // buffer for raw input    //
	char last_input[kMaxInputLength]{};    // the last input       //
	char small_outbuf[kSmallBufsize]{};    // standard output buffer      //
	char *output;        // ptr to the current output buffer //
	char **history;        // History of commands, for ! mostly.  //
	int history_pos;        // Circular array position.      //
	size_t bufptr;            // ptr to end of current output  //
	size_t bufspace;        // space left in the output buffer  //
	struct iosystem::TextBlock *large_outbuf;    // ptr to large buffer, if we need it //
	struct iosystem::TextBlocksQueue input;            // q of unprocessed input     //

	std::shared_ptr<CharData> character;    // linked to char       //
	std::shared_ptr<CharData> original;    // original char if switched     //

	DescriptorData *snooping;    // Who is this char snooping  //
	DescriptorData *snoop_by;    // And who is snooping this char //
	DescriptorData *next;    // link to next descriptor     //
	struct olc_data *olc;    //. OLC info - defined in olc.h   . //
	ubyte keytable;
	int options;        // descriptor flags       //
	z_stream *deflate;    // compression engine        //
	int mccp_version;
	unsigned long ip; // ип адрес в виде числа для внутреннего пользования
	std::weak_ptr<Boards::Board> board; // редактируемая доска
	Message::shared_ptr message; // редактируемое сообщение
	std::shared_ptr<struct ClanOLC> clan_olc; // редактирование привилегий клана
	std::shared_ptr<struct ClanInvite> clan_invite; // приглашение в дружину
	bool registered_email; // чтобы не шарить каждую секунду по списку мыл
	FILE *pers_log; // чтобы не открывать файл на каждую команду чара при персональном логе
	std::shared_ptr<class Glory::spend_glory> glory; // вливание славы
	std::shared_ptr<GloryConst::glory_olc> glory_const; // вливание славы2
	std::shared_ptr<NamedStuff::stuff_node> named_obj;    // редактируемая именная шмотка
#if defined WITH_SCRIPTING
	//std::shared_ptr<scripting::Console> console;	// Скриптовая консоль
#endif
	ObjVnum cur_vnum;                    // текущий внум именной шмотки (и что ЭТО тут делает?! убрать)
	ObjVnum old_vnum;                    // старый внум именной шмотки
	std::shared_ptr<MapSystem::Options> map_options; // редактирование опций режима карты
	bool snoop_with_map; // показывать снуперу карту цели с опциями самого снупера
	std::array<int, ExtMoney::kTotalTypes> ext_money{}; // обмен доп.денег
	std::shared_ptr<obj_sets_olc::sedit> sedit; // редактирование сетов
	bool mxp{}; // Для MXP

	// Admin API support
	bool admin_api_mode{}; // Admin API connection (JSON protocol)
	bool admin_api_authenticated{}; // Admin API authenticated flag
	std::string admin_api_large_buffer; // Buffer for large JSON messages
	long admin_user_id{}; // Authenticated user ID (UID)
	std::string admin_user_name; // Authenticated user name

 private:
	bool m_msdp_support;
	std::unordered_set<std::string> m_msdp_requested_report;
	int m_msdp_last_max_hit, m_msdp_last_max_move;
};

#endif //BYLINS_SRC_STRUCTS_DESCRIPTOR_DATA_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
