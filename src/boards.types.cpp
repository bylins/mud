#include "boards.types.hpp"

#include "boards.constants.hpp"
#include "utils.h"
#include "screen.h"

#include <fstream>
#include <sstream>

namespace Boards
{
	Board::Board(Boards::BoardTypes in_type) :
		type_(in_type),
		last_write_(0),
		clan_rent_(0),
		pers_unique_(0),
		blind_(false)
	{
	}

	void Board::erase_message(const size_t index)
	{
		messages.erase(messages.begin() + index);
		int count = 0;
		for (auto it = messages.rbegin(); it != messages.rend(); ++it)
		{
			(*it)->num = count++;
		}
	}

	int Board::count_unread(time_t last_read) const
	{
		int unread = 0;
		for (auto i = messages.rbegin(); i != messages.rend(); ++i)
		{
			if ((*i)->date > last_read)
			{
				++unread;
			}
			else
			{
				break;
			}
		}
		return unread;
	}

	// подгружаем доску, если она существует
	void Board::Load()
	{
		if (file_.empty())
		{
			return;
		}
		std::ifstream file(file_.c_str());
		if (!file.is_open())
		{
			this->Save();
			return;
		}

		std::string buffer;
		while (file >> buffer)
		{
			if (buffer == "Message:")
			{
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

				// не помешает глянуть че мы там залоадили
				if (message->author.empty()
					|| !message->unique
					|| !message->level
					|| !message->date)
				{
					continue;
				}

				messages.push_back(message);
			}
			else if (buffer == "Type:")
			{
				// type у доски в любом случае уже есть, здесь остается только
				// его проверить на всякий случай, это удобно еще и тем, что
				// тип пишется в файле первым полем и дальше можно все стопнуть
				int num;
				file >> num;
				if (num != type_)
				{
					log("SYSERROR: wrong board type=%d from %s, expected=%d (%s %s %d)",
						num, file_.c_str(), type_, __FILE__, __func__, __LINE__);
					return;
				}
			}
			else if (buffer == "Clan:")
				file >> clan_rent_;
			else if (buffer == "PersUID:")
				file >> pers_unique_;
			else if (buffer == "PersName:")
				file >> pers_name_;
		}
		file.close();

		this->Save();
	}

	void Board::format_board(Formatter::shared_ptr formatter) const
	{
		for (MessageListType::const_reverse_iterator message_i = messages.crbegin(); message_i != messages.crend(); ++message_i)
		{
			if (!formatter->format(*message_i))
			{
				break;
			}
		}
	}

	bool Board::is_special() const
	{
		if (get_type() == ERROR_BOARD
			|| get_type() == MISPRINT_BOARD
			|| get_type() == SUGGEST_BOARD)
		{
			return true;
		}
		return false;
	}

	void Board::add_message_implementation(Message::shared_ptr msg, const bool to_front)
	{
		if ((get_type() == CODER_BOARD)
			&& messages.size() >= MAX_REPORT_MESSAGES)
		{
			messages.erase(messages.begin());
		}
		if (!is_special()
			&& get_type() != NEWS_BOARD
			&& get_type() != GODNEWS_BOARD
			&& get_type() != CODER_BOARD
			&& messages.size() >= MAX_BOARD_MESSAGES)
		{
			messages.erase(messages.begin());
		}

		// чтобы время написания разделялось
		if (last_message_date() >= msg->date)
		{
			msg->date = last_message_date() + 1;
		}

		if (to_front)
		{
			messages.push_front(msg);
		}
		else
		{
			messages.push_back(msg);
		}

		int count = 0;
		for (auto it = messages.rbegin(); it != messages.rend(); ++it)
		{
			(*it)->num = count++;
		}
		set_lastwrite(msg->unique);
		Save();
	}

	time_t Board::last_message_date() const
	{
		if (!messages.empty())
		{
			return (*messages.rbegin())->date;
		}
		return 0;
	}

	// сохраняем доску
	void Board::Save()
	{
		if (file_.empty())
		{
			return;
		}
		std::ofstream file(file_.c_str());
		if (!file.is_open())
		{
			log("Error open file: %s! (%s %s %d)", file_.c_str(), __FILE__, __func__, __LINE__);
			return;
		}

		file << "Type: " << type_ << " "
			<< "Clan: " << clan_rent_ << " "
			<< "PersUID: " << pers_unique_ << " "
			<< "PersName: " << (pers_name_.empty() ? "none" : pers_name_) << "\n";
		for (MessageListType::const_iterator message = this->messages.begin(); message != this->messages.end(); ++message)
		{
			file << "Message: " << (*message)->num << "\n"
				<< (*message)->author << " "
				<< (*message)->unique << " "
				<< (*message)->level << " "
				<< (*message)->date << " "
				<< (*message)->rank << "\n"
				<< (*message)->subject << "~\n"
				<< (*message)->text << "~\n";
		}
		file.close();
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
