#ifndef DICTIONARY_H_
#define DICTIONARY_H_

#include <string>
#include <utility>
#include <vector>
#include <list>
#include <memory>

//const std::string DICTIONARY_RESULT_UNDEFINED = "Не найдено";

enum DictionaryMode { SHOP };

class DictionaryItem;
typedef std::shared_ptr<DictionaryItem> DictionaryItemPtr;

class DictionaryItem {
 public:
	DictionaryItem() : DictionaryName(std::string()), DictionaryTID(std::string()) {}
	DictionaryItem(std::string name, std::string tid) : DictionaryName(std::move(name)), DictionaryTID(std::move(tid)) {}
	void SetDictionaryName(const std::string &name) { DictionaryName = name; }
	void SetDictionaryTID(const std::string &tid) { DictionaryTID = tid; }
	const std::string &GetDictionaryName() { return DictionaryName; }
	const std::string &GetDictionaryTID() { return DictionaryTID; }
	DictionaryItemPtr GetDictionaryItem();

 private:
	std::string DictionaryName;
	std::string DictionaryTID;//символьный идентификатор
};

typedef std::vector<DictionaryItemPtr> DictionaryType;

class Dictionary {
 public:
	Dictionary() = default;;
	explicit Dictionary(DictionaryMode mode);
	void AddToDictionary(const DictionaryItemPtr& item);
	size_t Size();

	static std::string GetNameByTID(const std::string& tid);
	std::string GetNameByNID(size_t nid);
	std::string GetTIDByNID(size_t nid);

 private:
	DictionaryType dictionary_;
};

typedef std::shared_ptr<Dictionary> DictionaryPtr;

#endif // DICTIONARY_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
