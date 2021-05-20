#include "boot.index.hpp"

#include "logger.hpp"
#include "utils.h"
#include "boot.constants.hpp"

class IndexFileImplementation : public IndexFile
{
public:
	IndexFileImplementation();

	virtual bool open();
	virtual int load();

protected:
	virtual EBootType mode() const = 0;
	const auto& get_file_prefix() const { return prefixes(mode()); }
	const auto& file() const { return m_file; }
	const auto& line() const { return m_buffer; }
	void getline();

private:
	virtual int process_line() = 0;

	std::ifstream m_file;
	std::string m_buffer;
};

void IndexFileImplementation::getline()
{
	std::getline(m_file, m_buffer);
	if (0 < m_buffer.size()
			&& '\r' == m_buffer[m_buffer.size() - 1])
	{
		m_buffer.resize(m_buffer.size() - 1);
	}
}

IndexFileImplementation::IndexFileImplementation()
{
}

bool IndexFileImplementation::open()
{
	constexpr int BUFFER_SIZE = 1024;
	char filename[BUFFER_SIZE];

	const auto& prefix = get_file_prefix();
	if (prefix.empty())
	{
		log("SYSERR: Unknown subcommand %d to index_boot!", mode());
		return false;
	}

	const auto written = snprintf(filename, BUFFER_SIZE, "%s%s", prefix.c_str(), INDEX_FILE);
	if (written >= BUFFER_SIZE)
	{
		log("SYSERR: buffer size for index file name is not enough.");
		return false;
	}

	m_file.open(filename, std::ios::in);
	if (!m_file.good())
	{
		log("SYSERR: opening index file '%s': %s", filename, strerror(errno));
		return false;
	}

	return true;
}

int IndexFileImplementation::load()
{
	int rec_count = 0;

	const auto& prefix = get_file_prefix();

	getline();
	clear();
	while (file().good()
		&& (0 == line().size() || line()[0] != '$'))
	{
		push_back(line());
		rec_count += process_line();
		getline();
	}

	// Exit if 0 records, unless this is shops
	if (!rec_count)
	{
		log("SYSERR: boot error - 0 records counted in %s/%s.", prefix.c_str(), INDEX_FILE);
		exit(1);
	}

	return rec_count;
}

class ZoneIndexFile : public IndexFileImplementation
{
public:
	static shared_ptr create() { return shared_ptr(new ZoneIndexFile()); }

private:
	virtual EBootType mode() const { return DB_BOOT_ZON; }
	virtual int process_line() { return 1; }
};

class FilesIndexFile : public IndexFileImplementation
{
protected:
	const auto& entry_file() const { return m_entry_file; }
	const auto& entry_line() const { return m_buffer; }
	void get_next_entry_line() { std::getline(m_entry_file, m_buffer); }

private:
	virtual int process_line();
	virtual int process_file() = 0;

	std::ifstream m_entry_file;
	std::string m_buffer;
};

int FilesIndexFile::process_line()
{
	const auto& prefix = get_file_prefix();
	const std::string filename = prefix + line();
	m_entry_file.open(filename, std::ios::in);
	if (!m_entry_file.good())
	{
		log("SYSERR: File '%s' listed in '%s/%s' (will be skipped): %s", filename.c_str(), prefix.c_str(), INDEX_FILE, strerror(errno));
		return 0;
	}

	const auto result = process_file();

	m_entry_file.close();
	return result;
}

class HelpIndexFile : public FilesIndexFile
{
public:
	HelpIndexFile() : m_messages(0), m_keywords(0), m_unexpected_eof(false) {}
	static shared_ptr create() { return shared_ptr(new HelpIndexFile()); }

protected:
	virtual int process_file();

	auto get_keywords() const { return m_keywords; }
	auto get_messages() const { return m_messages; }

private:
	virtual EBootType mode() const { return DB_BOOT_HLP; }
	int count_social_records();
	bool skip_entry_body();
	bool read_entry();

	void set_unexpected_eof() { m_unexpected_eof = true; }
	bool get_unexpected_eof() const { return m_unexpected_eof; }

	int m_messages;
	int m_keywords;

	std::string m_keywords_string;
	bool m_unexpected_eof;
	char next_key[READ_SIZE];
};

int HelpIndexFile::process_file()
{
	return count_social_records();
}

int HelpIndexFile::count_social_records()
{
	while (read_entry())
	{
		if (m_unexpected_eof)
		{
			log("SYSERR: Unexpected end of help file.");
			exit(1);
		}
	}

	const auto result = get_keywords();
	return result;
}

bool HelpIndexFile::skip_entry_body()
{
	while (entry_file().good())
	{
		get_next_entry_line();
		if (0 < entry_line().size() && '#' == entry_line()[0])
		{
			return true;
		}
	}

	return false;
}

bool HelpIndexFile::read_entry()
{
	while (file().good())
	{
		get_next_entry_line();
		if (0 < entry_line().size() && '$' == entry_line()[0])
		{
			break;
		}
		m_keywords_string = entry_line();

		if (!skip_entry_body())
		{
			m_unexpected_eof = true;
			break;
		}

		++m_messages;
		auto scan = m_keywords_string.c_str();
		do
		{
			scan = one_word(scan, next_key);
			if (*next_key)
			{
				++m_keywords;
			}
		} while (*next_key);
	}

	if (!file().good())
	{
		m_unexpected_eof = true;
	}

	return false;
}

class SocialIndexFile : public HelpIndexFile
{
public:
	/// TODO: get rid of references
	SocialIndexFile(int& messages, int& keywords) : m_messages_ref(messages), m_keywords_ref(keywords) {}

	static shared_ptr create(int& messages, int& keywords) { return shared_ptr(new SocialIndexFile(messages, keywords)); }

private:
	virtual EBootType mode() const { return DB_BOOT_SOCIAL; }
	virtual int process_file();

	int& m_messages_ref;
	int& m_keywords_ref;
};

int SocialIndexFile::process_file()
{
	const auto result = HelpIndexFile::process_file();

	m_messages_ref = get_messages();
	m_keywords_ref = get_keywords();

	return result;
}

class HashSeparatedIndexFile : public FilesIndexFile
{
private:
	virtual int process_file();
	int count_hash_records();

	std::string m_buffer;
};

int HashSeparatedIndexFile::process_file()
{
	return count_hash_records();
}

// function to count how many hash-mark delimited records exist in a file
int HashSeparatedIndexFile::count_hash_records()
{
	int count = 0;
	for (get_next_entry_line(); entry_file().good(); get_next_entry_line())
	{
		if (0 < entry_line().size() && entry_line()[0] == '#')
		{
			count++;
		}
	}

	return count;
}

class MobileIndexFile : public HashSeparatedIndexFile
{
public:
	static shared_ptr create() { return shared_ptr(new MobileIndexFile()); }

private:
	virtual EBootType mode() const { return DB_BOOT_MOB; }
};

class ObjectIndexFile : public HashSeparatedIndexFile
{
public:
	static shared_ptr create() { return shared_ptr(new ObjectIndexFile()); }

private:
	virtual EBootType mode() const { return DB_BOOT_OBJ; }
};

class TriggerIndexFile : public HashSeparatedIndexFile
{
public:
	static shared_ptr create() { return shared_ptr(new TriggerIndexFile()); }

private:
	virtual EBootType mode() const { return DB_BOOT_TRG; }
};

class WorldIndexFile : public IndexFileImplementation
{
public:
	static shared_ptr create() { return shared_ptr(new WorldIndexFile()); }

private:
	virtual EBootType mode() const { return DB_BOOT_WLD; }
	virtual int process_line() { return 1; }
};

extern int number_of_social_messages;	// TODO: get rid of me
extern int number_of_social_commands;	// TODO: get rid of me

IndexFile::shared_ptr IndexFileFactory::get_index(const EBootType mode)
{
	switch (mode)
	{
	case DB_BOOT_MOB:
		return MobileIndexFile::create();

	case DB_BOOT_OBJ:
		return ObjectIndexFile::create();

	case DB_BOOT_TRG:
		return TriggerIndexFile::create();

	case DB_BOOT_WLD:
		return WorldIndexFile::create();

	case DB_BOOT_ZON:
		return ZoneIndexFile::create();

	case DB_BOOT_HLP:
		return HelpIndexFile::create();

	case DB_BOOT_SOCIAL:
		return SocialIndexFile::create(number_of_social_messages, number_of_social_commands);

	default:
		return nullptr;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
