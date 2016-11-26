#include "boards.changelog.loaders.hpp"

#include "boards.h"
#include "boards.message.hpp"

#include <boost/algorithm/string/trim.hpp>

#ifdef HAVE_ICONV
#include <iconv.h>
#endif

#include <functional>

namespace Boards
{
	std::string iconv_convert(const char *from, const char *to, std::string text)
	{
#ifdef HAVE_ICONV
		iconv_t cnv = iconv_open(to, from);
		if (cnv == (iconv_t)-1)
		{
			iconv_close(cnv);
			return "";
		}
		char *outbuf;
		if ((outbuf = (char *)malloc(text.length() * 2 + 1)) == NULL)
		{
			iconv_close(cnv);
			return "";
		}
		char *ip = (char *)text.c_str(), *op = outbuf;
		size_t icount = text.length(), ocount = text.length() * 2;

		if (iconv(cnv, &ip, &icount, &op, &ocount) != (size_t)-1)
		{
			outbuf[text.length() * 2 - ocount] = '\0';
			text = outbuf;
		}
		else
		{
			text = "";
		}

		free(outbuf);
		iconv_close(cnv);
#endif

		return text;
	}

	class ChangeLogLoaderImplementation : public ChangeLogLoader
	{
	public:
		ChangeLogLoaderImplementation(const Board::shared_ptr board) : m_board(board) {}
		virtual bool load(std::istream& is) = 0;

	protected:
		void add_message(std::string &author, std::string &desc, time_t parsed_time);

	private:
		Board::shared_ptr m_board;
	};

	void ChangeLogLoaderImplementation::add_message(std::string &author, std::string &desc, time_t parsed_time)
	{
		const auto message = std::make_shared<Message>();
		// имя автора
		const std::size_t e_pos = author.find('<');
		const std::size_t s_pos = author.find(' ');
		if (e_pos != std::string::npos
			|| s_pos != std::string::npos)
		{
			author = author.substr(0, std::min(e_pos, s_pos));
		}
		boost::trim(author);
		message->author = author;
		boost::trim(desc);
		message->text = iconv_convert("UTF-8", "KOI8-R", desc) + "\r\n";
		// из текста первая строка в заголовок
		std::string subj(message->text.begin(), std::find(message->text.begin(), message->text.end(), '\n'));
		if (subj.size() > 40)
		{
			subj = subj.substr(0, 40);
		}
		boost::trim(subj);
		message->subject = subj;
		message->date = parsed_time;
		message->unique = 1;
		message->level = 1;

		m_board->add_message_to_front(message);
	}

	const char *mon_name[] =
	{
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec", "\n"
	};

	time_t parse_asctime(const std::string &str)
	{
		char tmp[10], month[10];
		std::tm tm_time;
		tm_time.tm_mon = -1;

		int parsed = sscanf(str.c_str(), "%3s %3s %d %d:%d:%d %d",
			tmp, month,
			&tm_time.tm_mday,
			&tm_time.tm_hour, &tm_time.tm_min, &tm_time.tm_sec,
			&tm_time.tm_year);

		int i = 0;
		for (/**/; *mon_name[i] != '\n'; ++i)
		{
			if (!strcmp(month, mon_name[i]))
			{
				tm_time.tm_mon = i;
				break;
			}
		}

		time_t time = 0;
		if (tm_time.tm_mon != -1 && parsed == 7)
		{
			tm_time.tm_year -= 1900;
			tm_time.tm_isdst = -1;
			time = std::mktime(&tm_time);
		}
		return time;
	}

	class HgChangeLogLoader : public ChangeLogLoaderImplementation
	{
	public:
		HgChangeLogLoader(const Board::shared_ptr board) : ChangeLogLoaderImplementation(board) {}

		virtual bool load(std::istream& is) override;

		static shared_ptr create(const Board::shared_ptr board) { return std::make_shared<HgChangeLogLoader>(board); }
	};

	bool HgChangeLogLoader::load(std::istream& is)
	{
		std::string buf_str, author, date, desc;
		bool description = false;

		while (std::getline(is, buf_str))
		{
			const std::size_t pos = buf_str.find(' ');
			if (pos != std::string::npos)
			{
				std::string tmp_str = buf_str.substr(0, pos);
				if (tmp_str == "changeset:")
				{
					const time_t parsed_time = parse_asctime(date);
					if (parsed_time >= 1326121851 // переход на меркуриал
						&& !author.empty() && !date.empty() && !desc.empty())
					{
						add_message(author, desc, parsed_time);
					}
					description = false;
					author.clear();
					date.clear();
					desc.clear();
					continue;
				}
				if (description)
				{
					desc += buf_str + "\r\n";
					continue;
				}
				else
				{
					if (tmp_str == "user:")
					{
						author = buf_str.substr(pos);
						boost::trim(author);
					}
					else if (tmp_str == "date:")
					{
						date = buf_str.substr(pos);
						boost::trim(date);
					}
				}
			}
			else if (buf_str == "description:")
			{
				description = true;
			}
			else
			{
				desc += buf_str + "\r\n";
			}
		}

		return true;
	}

	class GitChangeLogLoader : public ChangeLogLoaderImplementation
	{
	public:
		GitChangeLogLoader(const Board::shared_ptr board) : ChangeLogLoaderImplementation(board) {}

		virtual bool load(std::istream& is) override;

		static shared_ptr create(const Board::shared_ptr board) { return std::make_shared<GitChangeLogLoader>(board); }
	};

	bool GitChangeLogLoader::load(std::istream& /*is*/)
	{
		return true;
	}

	namespace constants
	{
		namespace loader_formats
		{
			const char* MERCURIAL = "mercurial";
			const char* GIT = "git";
		}
	}

	using loader_builder_t = std::function<ChangeLogLoader::shared_ptr(const Board::shared_ptr)>;
	std::unordered_map<std::string, loader_builder_t> loaders = {
		{ constants::loader_formats::MERCURIAL, std::bind(HgChangeLogLoader::create, std::placeholders::_1) },
		{ constants::loader_formats::GIT, std::bind(GitChangeLogLoader::create, std::placeholders::_1) }
	};

	ChangeLogLoader::shared_ptr ChangeLogLoader::create(const std::string& kind, const Board::shared_ptr board)
	{
		const auto loader_i = loaders.find(kind);

		ChangeLogLoader::shared_ptr result;
		if (loader_i != loaders.end())
		{
			result = loader_i->second(board);
		}
		return result;
	}
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
