#include "boot.index.hpp"

class IndexFileImplementation : public IndexFile
{
public:
	IndexFileImplementation(const EBootType mode);

	virtual bool open();
	virtual int load();

protected:
	auto mode() const { return m_mode; }
	const auto& get_file_prefix() const { return prefixes(mode()); }
	const auto& file() const { return m_file; }
	void getline(std::string& line) { std::getline(m_file, line); }

private:
	virtual int process_line(const std::string& line) = 0;

	std::ifstream m_file;
	const EBootType m_mode;
};

IndexFileImplementation::IndexFileImplementation(const EBootType mode) : m_mode(mode)
{
}

bool IndexFileImplementation::open()
{
	constexpr int BUFFER_SIZE = 1024;
	char filename[BUFFER_SIZE];

	const auto& prefix = get_file_prefix();
	if (prefix.empty())
	{
		log("SYSERR: Unknown subcommand %d to index_boot!", m_mode);
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

	std::string line;
	getline(line);
	clear();
	while (file().good()
		&& (0 == line.size() || line[0] != '$'))
	{
		push_back(line);
		rec_count += process_line(line);
		getline(line);
	}

	// Exit if 0 records, unless this is shops
	if (!rec_count)
	{
		log("SYSERR: boot error - 0 records counted in %s/%s.", prefix.c_str(), INDEX_FILE);
		exit(1);
	}

	// Any idea why you put this here Jeremy?
	rec_count++;

	return rec_count;
}

class ZoneIndexFile : public IndexFileImplementation
{
public:
	ZoneIndexFile() : IndexFileImplementation(DB_BOOT_ZON) {}

private:
	virtual int process_line(const std::string&) { return 1; }
};

class FilesIndexFile : public IndexFileImplementation
{
public:
	FilesIndexFile(const EBootType mode) : IndexFileImplementation(mode) {}

protected:
	const auto& entry_file() const { return m_entry_file; }
	void get_entry_line(std::string& line) { std::getline(m_entry_file, line); }

private:
	virtual int process_line(const std::string& line);
	virtual int process_file() = 0;

	std::ifstream m_entry_file;
};

int FilesIndexFile::process_line(const std::string& line)
{
	const auto& prefix = get_file_prefix();
	const std::string filename = prefix + line;
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

class SocialIndexFile : public FilesIndexFile
{
public:
	/// TODO: get rid of references
	SocialIndexFile(int& messages, int& keywords) : FilesIndexFile(DB_BOOT_SOCIAL), m_messages(messages), m_keywords(keywords) {}

private:
	virtual int process_file();
	int count_social_records();

	int& m_messages;
	int& m_keywords;
};

int SocialIndexFile::process_file()
{
	return count_social_records();
}

int SocialIndexFile::count_social_records()
{
	char next_key[READ_SIZE];
	const char *scan;

	std::string key;
	get_entry_line(key);
	while (0 < key.size() && key[0] != '$')  	// skip the text
	{
		std::string line;
		do
		{
			get_entry_line(line);
			if (!entry_file().good())
			{
				log("SYSERR: Unexpected end of help file.");
				exit(1);
			}
		} while (0 < line.size() && line[0] != '#');

		// now count keywords
		scan = key.c_str();
		++m_messages;
		do
		{
			scan = one_word(scan, next_key);
			if (*next_key)
			{
				++m_keywords;
			}
		} while (*next_key);

		get_entry_line(key);

		if (!entry_file().good())
		{
			log("SYSERR: Unexpected end of help file.");
		}
	}

	return 1;
}

class HelpIndexFile : public FilesIndexFile
{
public:
	HelpIndexFile() : FilesIndexFile(DB_BOOT_HLP) {}

private:
	virtual int process_file();
	int count_alias_records();
};

int HelpIndexFile::process_file()
{
	return count_alias_records();
}

/*
* Thanks to Andrey (andrey@alex-ua.com) for this bit of code, although I
* did add the 'goto' and changed some "while()" into "do { } while()".
*      -gg 6/24/98 (technically 6/25/98, but I care not.)
*/
int HelpIndexFile::count_alias_records()
{
	char next_key[READ_SIZE];
	const char *scan;
	int total_keywords = 0;

	std::string key;
	get_entry_line(key);

	while (0 < key.size() && key[0] != '$')  	// skip the text
	{
		std::string line;
		do
		{
			get_entry_line(line);
			if (!entry_file().good())
			{
				mudlog("SYSERR: Unexpected end of help file.", DEF, LVL_IMMORT, SYSLOG, TRUE);
				return total_keywords;
			}
		} while (0 < line.size() && line[0] != '#');

		// now count keywords
		scan = key.c_str();
		do
		{
			scan = one_word(scan, next_key);
			if (*next_key)
			{
				++total_keywords;
			}
		} while (*next_key);

		get_entry_line(key);

		if (!entry_file().good())
		{
			mudlog("SYSERR: Unexpected end of help file.", DEF, LVL_IMMORT, SYSLOG, TRUE);
			return total_keywords;
		}
	}

	return total_keywords;
}

class HashSeparatedIndexFile : public FilesIndexFile
{
public:
	HashSeparatedIndexFile(const EBootType mode) : FilesIndexFile(mode) {}

private:
	virtual int process_file();
	int count_hash_records();
};

int HashSeparatedIndexFile::process_file()
{
	return count_hash_records();
}

// function to count how many hash-mark delimited records exist in a file
int HashSeparatedIndexFile::count_hash_records()
{
	int count = 0;
	std::string line;
	for (get_entry_line(line); entry_file().good(); get_entry_line(line))
	{
		if (0 < line.size() && line[0] == '#')
		{
			count++;
		}
	}

	return (count);
}

class WorldIndexFile : public IndexFileImplementation
{
public:
	WorldIndexFile() : IndexFileImplementation(DB_BOOT_WLD) {}

private:
	virtual int process_line(const std::string&) { return 1; }
};

extern int top_of_socialm;	// TODO: get rid of me
extern int top_of_socialk;	// TODO: get rid of me

IndexFile::shared_ptr IndexFileFactory::get_index(const EBootType mode)
{
	switch (mode)
	{
	case DB_BOOT_TRG:
		return IndexFile::shared_ptr(new HashSeparatedIndexFile(mode));

	case DB_BOOT_WLD:
		return IndexFile::shared_ptr(new WorldIndexFile());

	case DB_BOOT_MOB:
		return IndexFile::shared_ptr(new HashSeparatedIndexFile(mode));

	case DB_BOOT_OBJ:
		return IndexFile::shared_ptr(new HashSeparatedIndexFile(mode));

	case DB_BOOT_ZON:
		return IndexFile::shared_ptr(new ZoneIndexFile());

	case DB_BOOT_HLP:
		return IndexFile::shared_ptr(new HelpIndexFile());

	case DB_BOOT_SOCIAL:
		return IndexFile::shared_ptr(new SocialIndexFile(top_of_socialm, top_of_socialk));

	default:
		return nullptr;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
