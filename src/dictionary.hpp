#ifndef _DICTIONARY_H_
#define _DICTIONARY_H_
#include <string>
#include <vector>
#include <list>
#include <boost/shared_ptr.hpp>


const std::string DICTIONARY_RESULT_UNDEFINED = "Не найдено";

enum DictionaryMode {SHOP};

class DictionaryItem;
typedef boost::shared_ptr<DictionaryItem> DictionaryItemPtr;

class DictionaryItem
{
public:
	DictionaryItem() : DictionaryName(std::string()), DictionaryTID(std::string()) {};
	DictionaryItem(std::string name, std::string tid) : DictionaryName(name), DictionaryTID(tid) {};
	virtual void SetDictionaryName(std::string name) {DictionaryName = name;}
	virtual void SetDictionaryTID(std::string tid) {DictionaryTID = tid;}
	virtual std::string GetDictionaryName() {return DictionaryName;}
	virtual std::string GetDictionaryTID() {return DictionaryTID;}
	DictionaryItemPtr GetDictionaryItem();
private:
	std::string DictionaryName;
	std::string DictionaryTID;//символьный идентификатор
};

typedef std::vector<DictionaryItemPtr> DictionaryType;

class Dictionary
{
public:
	Dictionary() {};
	Dictionary(DictionaryMode mode);
	void AddToDictionary(DictionaryItemPtr item);
	int Size();

	std::string GetNameByTID(std::string tid);
	std::string GetNameByNID(unsigned nid);
	std::string GetTIDByNID(unsigned nid);

private:
	DictionaryType dictionary_;
};

typedef std::auto_ptr<Dictionary> DictionaryPtr;

#endif