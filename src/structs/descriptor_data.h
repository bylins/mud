/**
 \authors Created by Sventovit
 \date 26.05.2021.
 \brief Интерфейс DescriptorData
 \details Заголовок/интерфейс класса, описывающего состояние соединения клиента с севером.
*/
#ifndef BYLINS_SRC_STRUCTS_DESCRIPTOR_DATA_H_
#define BYLINS_SRC_STRUCTS_DESCRIPTOR_DATA_H_

#include "boards/boards_types.h"
#include "structs.h"
#include "sysdep.h"
#include "utils/utils_string.h"

#include <string>

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
	int connected;        // mode of 'connectedness'    //
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
	struct TextBlock *large_outbuf;    // ptr to large buffer, if we need it //
	struct TextBlocksQueue input;            // q of unprocessed input     //

	std::shared_ptr<CHAR_DATA> character;    // linked to char       //
	std::shared_ptr<CHAR_DATA> original;    // original char if switched     //

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

 private:
	bool m_msdp_support;
	std::unordered_set<std::string> m_msdp_requested_report;
	int m_msdp_last_max_hit, m_msdp_last_max_move;
};

// 		Modes of connectedness: used by descriptor_data.state	//
// 		ОБЕЗАТЕЛЬНО ДОБАВИТЬ В connected_types[]!!!!				//
const __uint8_t CON_PLAYING = 0;        // Playing - Nominal state //
const __uint8_t CON_CLOSE = 1;            // Disconnecting     //
const __uint8_t CON_GET_NAME = 2;        // By what name ..?     //
const __uint8_t CON_NAME_CNFRM = 3;    // Did I get that right, x?   //
const __uint8_t CON_PASSWORD = 4;        // Password:         //
const __uint8_t CON_NEWPASSWD = 5;        // Give me a password for x   //
const __uint8_t CON_CNFPASSWD = 6;        // Please retype password: //
const __uint8_t CON_QSEX = 7;            // Sex?           //
const __uint8_t CON_QCLASS = 8;        // Class?         //
const __uint8_t CON_RMOTD = 9;            // PRESS RETURN after MOTD //
const __uint8_t CON_MENU = 10;            // Your choice: (main menu)   //
const __uint8_t CON_EXDESC = 11;        // Enter a new description:   //
const __uint8_t CON_CHPWD_GETOLD = 12; // Changing passwd: get old   //
const __uint8_t CON_CHPWD_GETNEW = 13; // Changing passwd: get new   //
const __uint8_t CON_CHPWD_VRFY = 14;    // Verify new password     //
const __uint8_t CON_DELCNF1 = 15;        // Delete confirmation 1   //
const __uint8_t CON_DELCNF2 = 16;        // Delete confirmation 2   //
const __uint8_t CON_DISCONNECT = 17;    // In-game disconnection   //
const __uint8_t CON_OEDIT = 18;        //. OLC mode - object edit     . //
const __uint8_t CON_REDIT = 19;        //. OLC mode - room edit       . //
const __uint8_t CON_ZEDIT = 20;        //. OLC mode - zone info edit  . //
const __uint8_t CON_MEDIT = 21;        //. OLC mode - mobile edit     . //
const __uint8_t CON_TRIGEDIT = 22;        //. OLC mode - trigger edit    . //
const __uint8_t CON_NAME2 = 23;
const __uint8_t CON_NAME3 = 24;
const __uint8_t CON_NAME4 = 25;
const __uint8_t CON_NAME5 = 26;
const __uint8_t CON_NAME6 = 27;
const __uint8_t CON_RELIGION = 28;
const __uint8_t CON_RACE = 29;
const __uint8_t CON_LOWS = 30;
const __uint8_t CON_GET_KEYTABLE = 31;
const __uint8_t CON_GET_EMAIL = 32;
const __uint8_t CON_ROLL_STATS = 33;
const __uint8_t CON_MREDIT = 34;        // OLC mode - make recept edit //
const __uint8_t CON_QKIN = 35;
const __uint8_t CON_QCLASSV = 36;
const __uint8_t CON_QCLASSS = 37;
const __uint8_t CON_MAP_MENU = 38;
const __uint8_t CON_COLOR = 39;
const __uint8_t CON_WRITEBOARD = 40;        // написание на доску
const __uint8_t CON_CLANEDIT = 41;            // команда house
const __uint8_t CON_NEW_CHAR = 42;
const __uint8_t CON_SPEND_GLORY = 43;        // вливание славы через команду у чара
const __uint8_t CON_RESET_STATS = 44;        // реролл статов при входе в игру
const __uint8_t CON_BIRTHPLACE = 45;        // выбираем где начать игру
const __uint8_t CON_WRITE_MOD = 46;        // пишет клановое сообщение дня
const __uint8_t CON_GLORY_CONST = 47;        // вливает славу2
const __uint8_t CON_NAMED_STUFF = 48;        // редактирует именной стаф
const __uint8_t CON_RESET_KIN = 49;        // выбор расы после смены/удаления оной (или иного испоганивания значения)
const __uint8_t CON_RESET_RACE = 50;        // выбор РОДА посла смены/сброса оного
const __uint8_t CON_CONSOLE = 51;            // Интерактивная скриптовая консоль
const __uint8_t CON_TORC_EXCH = 52;        // обмен гривен
const __uint8_t CON_MENU_STATS = 53;        // оплата сброса стартовых статов из главного меню
const __uint8_t CON_SEDIT = 54;            // sedit - редактирование сетов
const __uint8_t CON_RESET_RELIGION = 55;    // сброс религии из меню сброса статов
const __uint8_t CON_RANDOM_NUMBER = 56;    // Verification code entry: where player enter in the game from new location
const __uint8_t CON_INIT = 57;                // just connected
// не забываем отражать новые состояния в connected_types -- Krodo

#endif //BYLINS_SRC_STRUCTS_DESCRIPTOR_DATA_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
