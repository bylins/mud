#include "boards.changelog.loaders.hpp"

#include "logger.hpp"
#include "boards.h"
#include "boards.message.hpp"
#include "boards.constants.hpp"

#include <boost/algorithm/string/trim.hpp>

#include <functional>
#include <sstream>
#include <iomanip>

namespace Boards {
class ChangeLogLoaderImplementation : public ChangeLogLoader {
 public:
  ChangeLogLoaderImplementation(const Board::shared_ptr board) : m_board(board) {}
  virtual bool load(std::istream &is) = 0;

 protected:
  void add_message(const std::string &author, const std::string &desc, time_t parsed_time);
  void renumerate_messages() { m_board->renumerate_messages(); }

 private:
  Board::shared_ptr m_board;
};

void ChangeLogLoaderImplementation::add_message(const std::string &author,
                                                const std::string &desc,
                                                time_t parsed_time) {
  const auto message = std::make_shared<Message>();
  // имя автора
  std::string author_copy = author;
  const std::size_t e_pos = author_copy.find('<');
  const std::size_t s_pos = author_copy.find(' ');
  if (e_pos != std::string::npos
      || s_pos != std::string::npos) {
    author_copy = author_copy.substr(0, std::min(e_pos, s_pos));
  }
  boost::trim(author_copy);
  message->author = author_copy;

  std::string desc_copy = desc;
  boost::trim(desc_copy);
  message->text = desc_copy;

  // из текста первая строка в заголовок
  std::string subj(message->text.begin(), std::find(message->text.begin(), message->text.end(), '\n'));
/*		if (subj.size() > 40)
		{
			subj = subj.substr(0, 40);
		} */
  boost::trim(subj);
  message->subject = subj;
  message->date = parsed_time;
  message->unique = 1;
  message->level = 1;

  m_board->add_message(message);
}

const char *mon_name[] =
    {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", "\n"
    };

time_t parse_asctime(const std::string &str) {
  char tmp[10], month[10];
  std::tm tm_time;
  tm_time.tm_mon = -1;

  int parsed = sscanf(str.c_str(), "%3s %3s %d %d:%d:%d %d",
                      tmp, month,
                      &tm_time.tm_mday,
                      &tm_time.tm_hour,
                      &tm_time.tm_min,
                      &tm_time.tm_sec,
                      &tm_time.tm_year);

  int i = 0;
  for (/**/; *mon_name[i] != '\n'; ++i) {
    if (!strcmp(month, mon_name[i])) {
      tm_time.tm_mon = i;
      break;
    }
  }

  time_t time = 0;
  if (tm_time.tm_mon != -1 && parsed == 7) {
    tm_time.tm_year -= 1900;
    tm_time.tm_isdst = -1;
    time = std::mktime(&tm_time);
  }
  return time;
}

class GitChangeLogLoader : public ChangeLogLoaderImplementation {
 public:
  GitChangeLogLoader(const Board::shared_ptr board) : ChangeLogLoaderImplementation(board) {}

  virtual bool load(std::istream &is) override;

  static shared_ptr create(const Board::shared_ptr board) { return std::make_shared<GitChangeLogLoader>(board); }

 private:
  class GitCommitReader {
   public:
    bool parse_revision(const std::string &buffer);
    bool parse_author(std::string &buffer, std::istream &is, int &line);
    bool parse_date(const std::string &buffer);
    bool parse_comment(std::string &buffer, std::istream &is, int &line);

    const auto &get_author() const { return m_author; }
    const auto &get_comment() const { return m_comment; }
    const auto &get_date() const { return m_date; }

   private:
    static constexpr int REVISION_LENGTH = 40;        ///< length of sha1

    static bool next_line(std::string &buffer, std::istream &is, int &line);

    const std::string REVISION_PREFIX = "commit ";
    const std::string MERGE_PREFIX = "Merge: ";
    const std::string AUTHOR_PREFIX = "Author: ";
    const std::string DATE_PREFIX = "Date: ";
    const std::string COMMENT_PREFIX = "    ";

    std::string m_revision;
    std::string m_author;
    time_t m_date;
    std::string m_comment;
  };
};

bool GitChangeLogLoader::GitCommitReader::parse_revision(const std::string &buffer) {
  if (0 != strncmp(REVISION_PREFIX.c_str(), buffer.c_str(), REVISION_PREFIX.size())) {
    return false;
  }

  const auto revision_length = buffer.size() - REVISION_PREFIX.size();
  if (REVISION_LENGTH != revision_length) {
    return false;
  }
  m_revision = buffer.substr(REVISION_PREFIX.size(), revision_length);

  return true;
}

bool GitChangeLogLoader::GitCommitReader::parse_author(std::string &buffer, std::istream &is, int &line) {
  if (0 == strncmp(MERGE_PREFIX.c_str(), buffer.c_str(), MERGE_PREFIX.size())) {
    if (!next_line(buffer, is, line)) {
      return false;
    }
  }

  if (0 != strncmp(AUTHOR_PREFIX.c_str(), buffer.c_str(), AUTHOR_PREFIX.size())) {
    return false;
  }

  const auto author_length = buffer.size() - AUTHOR_PREFIX.size();
  m_author = buffer.substr(AUTHOR_PREFIX.size(), author_length);

  return true;
}

bool GitChangeLogLoader::GitCommitReader::parse_date(const std::string &buffer) {
  if (0 != strncmp(DATE_PREFIX.c_str(), buffer.c_str(), DATE_PREFIX.size())) {
    return false;
  }

  const auto date_length = buffer.size() - DATE_PREFIX.size();
  std::string date = buffer.substr(DATE_PREFIX.size(), date_length);

  m_date = parse_asctime(date);

  return true;
}

bool GitChangeLogLoader::GitCommitReader::parse_comment(std::string &buffer, std::istream &is, int &line) {
  if (!buffer.empty())    // first line must be empty
  {
    return false;
  }

  if (!next_line(buffer, is, line)) {
    return false;
  }

  std::stringstream comment;
  do {
    if (0 != strncmp(COMMENT_PREFIX.c_str(), buffer.c_str(), COMMENT_PREFIX.size())) {
      return false;
    }

    comment << (4 + buffer.c_str());

    if (!next_line(buffer, is, line)) {
      break;
    }
  } while (!buffer.empty());

  m_comment = comment.str();

  return true;
}

bool GitChangeLogLoader::GitCommitReader::next_line(std::string &buffer, std::istream &is, int &line) {
  if (!std::getline(is, buffer)) {
    return false;
  }
  ++line;

  return true;
}

bool GitChangeLogLoader::load(std::istream &is) {
  enum {
    REVISION,
    AUTHOR,
    DATE,
    COMMENT
  } parser_state = REVISION;

  GitCommitReader commit;

  int line = 0;
  std::string buffer;
  while (std::getline(is, buffer)) {
    ++line;
    switch (parser_state) {
      case REVISION:
        if (!commit.parse_revision(buffer)) {
          log("SYSERR: could not read commit revision from changelog file at line %d. Wrong string: %s",
              line, buffer.c_str());
          return false;
        }
        parser_state = AUTHOR;
        break;

      case AUTHOR:
        if (!commit.parse_author(buffer, is, line)) {
          log("SYSERR: could not read commit author from changelog file at line %d. Wrong string: %s",
              line, buffer.c_str());
          return false;
        }
        parser_state = DATE;
        break;

      case DATE:
        if (!commit.parse_date(buffer)) {
          log("SYSERR: could not read commit date from changelog file at line %d. Wrong string: %s",
              line, buffer.c_str());
          return false;
        }
        parser_state = COMMENT;
        break;

      case COMMENT:
        if (!commit.parse_comment(buffer, is, line)) {
          log("SYSERR: could not read commit comment from changelog file at line %d. Wrong string: %s",
              line, buffer.c_str());
          return false;
        }
        parser_state = REVISION;

        add_message(commit.get_author(), commit.get_comment(), commit.get_date());
        break;

      default:
        log("SYSERR: error while reading changelog file. Went into unexpected parser state [%d] at line %d.",
            static_cast<int>(parser_state), line);
        break;
    }
  }

  renumerate_messages();
  return true;
}

class Loaders {
 public:
  static ChangeLogLoader::shared_ptr get(const std::string &kind, const Board::shared_ptr board);

 private:
  using loader_builder_t = std::function<ChangeLogLoader::shared_ptr(const Board::shared_ptr)>;
  using loaders_map_t = std::unordered_map<std::string, loader_builder_t>;
  using loaders_map_ptr_t = std::shared_ptr<loaders_map_t>;

  static void init_loaders();
  static loaders_map_ptr_t s_loaders;
};

ChangeLogLoader::shared_ptr Loaders::get(const std::string &kind, const Board::shared_ptr board) {
  init_loaders();

  const auto loader_i = s_loaders->find(kind);

  ChangeLogLoader::shared_ptr result;
  if (loader_i != s_loaders->end()) {
    result = loader_i->second(board);
  }
  return result;
}

void Loaders::init_loaders() {
  if (!s_loaders) {
    loaders_map_t map = {
        {std::string(constants::loader_formats::GIT), std::bind(GitChangeLogLoader::create, std::placeholders::_1)}
    };
    s_loaders = std::make_shared<loaders_map_t>(map);
  }
}

Loaders::loaders_map_ptr_t Loaders::s_loaders;

ChangeLogLoader::shared_ptr ChangeLogLoader::create(const std::string &kind, const Board::shared_ptr board) {
  return Loaders::get(kind, board);
}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
