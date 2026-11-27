#include "boot_index.h"

#include "utils/logger.h"
#include "gameplay/communication/social.h"

class IndexFileImplementation : public IndexFile {
 public:
	using impl_ptr = std::shared_ptr<IndexFileImplementation>;

	IndexFileImplementation();

	virtual bool open();
	virtual int load();
	void set_files(const LegacyFiles &files) { m_files = &files; }

 protected:
	virtual EBootType mode() const = 0;
	const LegacyFiles &files() const { return *m_files; }
	const auto &file() const { return m_file; }
	const auto &line() const { return m_buffer; }
	void getline();

 private:
	virtual int process_line() = 0;

	const LegacyFiles *m_files{nullptr};
	std::ifstream m_file;
	std::string m_buffer;
};

void IndexFileImplementation::getline() {
	std::getline(m_file, m_buffer);
	if (0 < m_buffer.size()
		&& '\r' == m_buffer[m_buffer.size() - 1]) {
		m_buffer.resize(m_buffer.size() - 1);
	}
}

IndexFileImplementation::IndexFileImplementation() {
}

bool IndexFileImplementation::open() {
	const auto filename = files().Index(mode());
	if (filename.empty()) {
		log("SYSERR: Unknown subcommand %d to BootIndex!", mode());
		return false;
	}

	m_file.open(filename, std::ios::in);
	if (!m_file.good()) {
		log("SYSERR: opening index file '%s': %s", filename.c_str(), strerror(errno));
		return false;
	}

	return true;
}

int IndexFileImplementation::load() {
	int rec_count = 0;

	getline();
	clear();
	while (file().good()
		&& (0 == line().size() || line()[0] != '$')) {
		push_back(line());
		rec_count += process_line();
		getline();
	}

	// Exit if 0 records, unless this is shops
	if (!rec_count) {
		fatal_log("SYSERR: boot error - 0 records counted in %s.", files().Index(mode()).c_str());
	}

	return rec_count;
}

class ZoneIndexFile : public IndexFileImplementation {
 public:
	static impl_ptr create() { return impl_ptr(new ZoneIndexFile()); }

 private:
	virtual EBootType mode() const { return DB_BOOT_ZON; }
	virtual int process_line() { return 1; }
};

class FilesIndexFile : public IndexFileImplementation {
 protected:
	const auto &entry_file() const { return m_entry_file; }
	const auto &entry_line() const { return m_buffer; }
	void get_next_entry_line() { std::getline(m_entry_file, m_buffer); }

 private:
	virtual int process_line();
	virtual int process_file() = 0;

	std::ifstream m_entry_file;
	std::string m_buffer;
};

int FilesIndexFile::process_line() {
	const auto filename = files().Entry(mode(), line());
	m_entry_file.open(filename, std::ios::in);
	if (!m_entry_file.good()) {
		log("SYSERR: File '%s' listed in '%s' (will be skipped): %s",
			filename.c_str(),
			files().Index(mode()).c_str(),
			strerror(errno));
		return 0;
	}

	const auto result = process_file();

	m_entry_file.close();
	return result;
}

class HelpIndexFile : public FilesIndexFile {
 public:
	HelpIndexFile() : m_messages(0), m_keywords(0), m_unexpected_eof(false) {}
	static impl_ptr create() { return impl_ptr(new HelpIndexFile()); }

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

int HelpIndexFile::process_file() {
	return count_social_records();
}

int HelpIndexFile::count_social_records() {
	while (read_entry()) {
		if (m_unexpected_eof) {
			fatal_log("SYSERR: Unexpected end of help file.");
		}
	}

	const auto result = get_keywords();
	return result;
}

bool HelpIndexFile::skip_entry_body() {
	while (entry_file().good()) {
		get_next_entry_line();
		if (0 < entry_line().size() && '#' == entry_line()[0]) {
			return true;
		}
	}

	return false;
}

bool HelpIndexFile::read_entry() {
	while (file().good()) {
		get_next_entry_line();
		if (0 < entry_line().size() && '$' == entry_line()[0]) {
			break;
		}
		m_keywords_string = entry_line();

		if (!skip_entry_body()) {
			m_unexpected_eof = true;
			break;
		}

		++m_messages;
		auto scan = m_keywords_string.c_str();
		do {
			scan = one_word(scan, next_key);
			if (*next_key) {
				++m_keywords;
			}
		} while (*next_key);
	}

	if (!file().good()) {
		m_unexpected_eof = true;
	}

	return false;
}

class HashSeparatedIndexFile : public FilesIndexFile {
 private:
	virtual int process_file();
	int count_hash_records();

	std::string m_buffer;
};

int HashSeparatedIndexFile::process_file() {
	return count_hash_records();
}

// function to count how many hash-mark delimited records exist in a file
int HashSeparatedIndexFile::count_hash_records() {
	int count = 0;
	for (get_next_entry_line(); entry_file().good(); get_next_entry_line()) {
		if (0 < entry_line().size() && entry_line()[0] == '#') {
			count++;
		}
	}

	return count;
}

class MobileIndexFile : public HashSeparatedIndexFile {
 public:
	static impl_ptr create() { return impl_ptr(new MobileIndexFile()); }

 private:
	virtual EBootType mode() const { return DB_BOOT_MOB; }
};

class ObjectIndexFile : public HashSeparatedIndexFile {
 public:
	static impl_ptr create() { return impl_ptr(new ObjectIndexFile()); }

 private:
	virtual EBootType mode() const { return DB_BOOT_OBJ; }
};

class TriggerIndexFile : public HashSeparatedIndexFile {
 public:
	static impl_ptr create() { return impl_ptr(new TriggerIndexFile()); }

 private:
	virtual EBootType mode() const { return DB_BOOT_TRG; }
};

class WorldIndexFile : public IndexFileImplementation {
 public:
	static impl_ptr create() { return impl_ptr(new WorldIndexFile()); }

 private:
	virtual EBootType mode() const { return DB_BOOT_WLD; }
	virtual int process_line() { return 1; }
};

IndexFile::shared_ptr IndexFileFactory::get_index(const EBootType mode, const LegacyFiles &files) {
	IndexFileImplementation::impl_ptr index;
	switch (mode) {
		case DB_BOOT_MOB: index = MobileIndexFile::create();
			break;

		case DB_BOOT_OBJ: index = ObjectIndexFile::create();
			break;

		case DB_BOOT_TRG: index = TriggerIndexFile::create();
			break;

		case DB_BOOT_WLD: index = WorldIndexFile::create();
			break;

		case DB_BOOT_ZON: index = ZoneIndexFile::create();
			break;

		case DB_BOOT_HLP: index = HelpIndexFile::create();
			break;

		default: return nullptr;
	}
	index->set_files(files);
	return index;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
