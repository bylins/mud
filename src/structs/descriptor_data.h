/**
 \authors Created by Sventovit
 \date 26.05.2021.
 \brief Интерфейс DescriptorData
 \details Заголовок/интерфейс класса, описывающего состояние соединения клиента с севером.
*/
#ifndef BYLINS_SRC_STRUCTS_DESCRIPTOR_DATA_H_
#define BYLINS_SRC_STRUCTS_DESCRIPTOR_DATA_H_

#include "structs.h"
#include "sysdep.h"

#include <string>

struct DESCRIPTOR_DATA {
	DESCRIPTOR_DATA();

	void msdp_support(bool on);
	void msdp_add_report_variable(const std::string &name) { m_msdp_requested_report.insert(name); }
	void msdp_remove_report_variable(const std::string &name) { m_msdp_requested_report.erase(name); }
	bool msdp_need_report(const std::string &name) {
		return m_msdp_requested_report.find(name) != m_msdp_requested_report.end();
	}
	void msdp_report(const std::string &name);
	void msdp_report_changed_vars();

	void string_to_client_encoding(const char *input, char *output) const;
	auto get_character() const { return original ? original : character; }

	socket_t descriptor;    // file descriptor for socket    //
	char host[kHostLength + 1];    // hostname          //
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
	AbstractStringWriter::shared_ptr writer;        // for the modify-str system     //
	size_t max_str;        //      -        //
	char *backstr;        // added for handling abort buffers //
	int mail_to;        // uid for mail system
	int has_prompt;        // is the user at a prompt?             //
	char inbuf[kMaxRawInputLength];    // buffer for raw input    //
	char last_input[kMaxInputLength];    // the last input       //
	char small_outbuf[kSmallBufsize];    // standard output buffer      //
	char *output;        // ptr to the current output buffer //
	char **history;        // History of commands, for ! mostly.  //
	int history_pos;        // Circular array position.      //
	size_t bufptr;            // ptr to end of current output  //
	size_t bufspace;        // space left in the output buffer  //
	struct txt_block *large_outbuf;    // ptr to large buffer, if we need it //
	struct txt_q input;            // q of unprocessed input     //

	std::shared_ptr<CHAR_DATA> character;    // linked to char       //
	std::shared_ptr<CHAR_DATA> original;    // original char if switched     //

	DESCRIPTOR_DATA *snooping;    // Who is this char snooping  //
	DESCRIPTOR_DATA *snoop_by;    // And who is snooping this char //
	DESCRIPTOR_DATA *next;    // link to next descriptor     //
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
	unsigned long cur_vnum;                    // текущий внум именной шмотки
	unsigned long old_vnum;                    // старый внум именной шмотки
	std::shared_ptr<MapSystem::Options> map_options; // редактирование опций режима карты
	bool snoop_with_map; // показывать снуперу карту цели с опциями самого снупера
	std::array<int, ExtMoney::kTotalTypes> ext_money; // обмен доп.денег
	std::shared_ptr<obj_sets_olc::sedit> sedit; // редактирование сетов
	bool mxp; // Для MXP

 private:
	bool m_msdp_support;
	std::unordered_set<std::string> m_msdp_requested_report;
	int m_msdp_last_max_hit, m_msdp_last_max_move;
};

#endif //BYLINS_SRC_STRUCTS_DESCRIPTOR_DATA_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :