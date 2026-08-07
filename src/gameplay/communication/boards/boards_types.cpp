#include "boards_types.h"
#include "utils/native_text.h"

#include "boards_constants.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include <sys/stat.h>

namespace Boards {
Board::Board(Boards::BoardTypes in_type) :
	type_(in_type),
	last_write_(0),
	clan_rent_(0),
	pers_unique_(0),
	blind_(false) {
}

void Board::erase_message(const size_t index) {
	messages.erase(messages.begin() + index);
	renumerate_messages();
}

int Board::count_unread(time_t last_read) const {
	int unread = 0;
	for (const auto &message : messages) {
		if (message->date > last_read) {
			++unread;
		} else {
			break;
		}
	}
	return unread;
}

// подгружаем доску, если она существует
void Board::Load() {
	if (file_.empty()) {
		return;
	}
	std::ifstream file(file_.c_str());
	if (!file.is_open()) {
		this->Save();    // case for missing file???
		return;
	}

	std::string buffer;
	while (file >> buffer) {
		if (buffer == "Message:") {
			const auto message = std::make_shared<Message>();
			file >> message->num;
			file >> message->author;
			file >> message->unique;
			file >> message->level;
			file >> message->date;
			file >> message->rank;

			ReadEndString(file);
			std::getline(file, message->subject, '~');
			ReadEndString(file);
			std::getline(file, message->text, '~');

			// Доски лежат на диске в KOI8-R и читаются потоком, минуя FBFILE, поэтому границу
			// кодировки проходим здесь -- по текстовым полям (issue #3681). Под KOI8-R это
			// тождество.
			message->author = native_text::from_disk_line(message->author.c_str());
			message->subject = native_text::from_disk_line(message->subject.c_str());
			message->text = native_text::from_disk_line(message->text.c_str());

			// не помешает глянуть че мы там залоадили
			if (message->author.empty()
				|| !message->unique
				|| !message->level
				|| !message->date) {
				continue;
			}

			messages.push_front(message);
		} else if (buffer == "Type:") {
			// type у доски в любом случае уже есть, здесь остается только
			// его проверить на всякий случай, это удобно еще и тем, что
			// тип пишется в файле первым полем и дальше можно все стопнуть
			int num;
			file >> num;
			if (num != type_) {
				log("SYSERROR: wrong board type=%d from %s, expected=%d (%s %s %d)",
					num, file_.c_str(), type_, __FILE__, __func__, __LINE__);
				return;
			}
		} else if (buffer == "Clan:")
			file >> clan_rent_;
		else if (buffer == "PersUID:")
			file >> pers_unique_;
		else if (buffer == "PersName:")
			file >> pers_name_;
	}
	file.close();
}

bool Board::is_special() const {
	if (get_type() == ERROR_BOARD
		|| get_type() == MISPRINT_BOARD
		|| get_type() == SUGGEST_BOARD) {
		return true;
	}
	return false;
}

void Board::add_message(Message::shared_ptr msg) {
	const bool coder_overflow = get_type() == CODER_BOARD
		&& messages.size() >= MAX_REPORT_MESSAGES;
	const bool board_overflow = !is_special()
		&& get_type() != NEWS_BOARD
		&& get_type() != GODNEWS_BOARD
		&& get_type() != CODER_BOARD
		&& messages.size() >= MAX_BOARD_MESSAGES;
	if (coder_overflow
		|| board_overflow) {
		messages.erase(--messages.end());
	}

	// чтобы время написания разделялось
	if (last_message_date() >= msg->date) {
		msg->date = last_message_date() + 1;
	}

	messages.push_front(msg);
}

time_t Board::last_message_date() const {
	if (!messages.empty()) {
		return (*messages.begin())->date;
	}
	return 0;
}

// сохраняем доску
void Board::Save() {
	if (file_.empty()) {
		return;
	}
	std::ofstream file(file_.c_str());
	if (!file.is_open()) {
		log("Error open file: %s! (%s %s %d)", file_.c_str(), __FILE__, __func__, __LINE__);
		return;
	}
	// Собираем всё в строку и приводим к кодировке диска одним куском: граница записи -- зеркало
	// границы чтения (доски читаются через from_disk_line), иначе первое же сохранение доски
	// переводит её файл в UTF-8, а мир вокруг остаётся в KOI8-R (issue #3681).
	std::ostringstream out;
	out << "Type: " << type_ << " "
		<< "Clan: " << clan_rent_ << " "
		<< "PersUID: " << pers_unique_ << " "
		<< "PersName: " << (pers_name_.empty() ? "none" : pers_name_) << "\n";
	for (MessageListType::const_reverse_iterator message = messages.rbegin(); message != messages.rend(); ++message) {
		out << "Message: " << (*message)->num << "\n"
			<< (*message)->author << " "
			<< (*message)->unique << " "
			<< (*message)->level << " "
			<< (*message)->date << " "
			<< (*message)->rank << "\n"
			<< (*message)->subject << "~\n"
			<< (*message)->text << "~\n";
	}
	const std::string on_disk = native_text::to_disk(out.str());
	file.write(on_disk.data(), static_cast<std::streamsize>(on_disk.size()));
	file.close();
}

void Board::renumerate_messages() {
	int count = 0;
	for (auto &message : messages) {
		message->num = count++;
	}
}

void Board::write_message(Message::shared_ptr message) {
	add_message(message);
	renumerate_messages();
	set_lastwrite_uid(message->unique);
	Save();
}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
