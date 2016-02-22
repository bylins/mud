#ifndef MORPH_HPP_INCLUDED
#define MORPH_HPP_INCLUDED

#include <list>
#include <vector>
#include <boost/shared_ptr.hpp>
#include "comm.h"

using std::list;
using std::string;
using std::map;
using std::vector;

extern short MIN_WIS_FOR_MORPH;

typedef struct
{
	int fromLevel;
	string desc;
} DescNode;

typedef list<DescNode> DescListType;

class IMorph;
class AnimalMorph;

typedef boost::shared_ptr<IMorph> MorphPtr;
typedef boost::shared_ptr<AnimalMorph> AnimalMorphPtr;
MorphPtr GetNormalMorphNew(CHAR_DATA *ch);
typedef map<string, AnimalMorphPtr> MorphListType;

typedef map<int, int> MorphSkillsList;

class IMorph {
public:
	typedef std::vector<EAffectFlags> affects_list_t;

	IMorph() {};
	virtual ~IMorph() {};
	virtual string Name()=0;
	virtual string PadName()=0;
	virtual string GetMorphDesc()=0;
	virtual string GetMorphTitle()=0;
	virtual void InitSkills(int value) {};
	virtual void InitAbils() {};
	virtual void SetAbilsParams(short toStr, short toDex, short toCon, short toInt, short toCha) {};
	virtual void SetChar(CHAR_DATA *ch) {};
	virtual string CoverDesc() {return "";};
	virtual bool isAffected(const EAffectFlags flag) const {return false;}
	virtual const affects_list_t& GetAffects() {return affects_list_t();}
	virtual string GetMessageToRoom() {return string();};
	virtual string GetMessageToChar() {return string();};

	virtual int GetStr() const =0;
	virtual void SetStr(int str)=0;
	virtual int GetIntel() const =0;
	virtual void SetIntel(int intel)=0;
	virtual int GetWis() const =0;
	virtual void SetWis(int wis)=0;
	virtual int GetDex() const =0;
	virtual void SetDex(int dex)=0;
	virtual int GetCha() const =0;
	virtual void SetCha(int cha)=0;
	virtual int GetCon() const =0;
	virtual void SetCon(int con)=0;

	virtual void set_skill(int skill_num, int percent)=0;
	virtual int get_trained_skill(int skill_num)=0;
};



class NormalMorph : public IMorph
{	
public:
	NormalMorph (CHAR_DATA *ch) {ch_=ch;}
	CHAR_DATA *ch_;

	~NormalMorph () {};
	

	string GetMorphDesc();
	string GetMorphTitle();
	string Name() {return "Обычная";}
	string PadName() {return "Человеком";};
	void SetChar(CHAR_DATA *ch) {ch_=ch;};

	void set_skill(int skill_num, int percent);
	int get_trained_skill(int skill_num);

	virtual int GetStr() const;
	virtual void SetStr(int str);
	virtual int GetIntel() const;
	virtual void SetIntel(int intel);
	virtual int GetWis() const;
	virtual void SetWis(int wis);
	virtual int GetDex() const;
	virtual void SetDex(int dex);
	virtual int GetCha() const;
	virtual void SetCha(int cha);
	virtual int GetCon() const;
	virtual void SetCon(int con);

};

class AnimalMorph : public IMorph
{
	CHAR_DATA *ch_;
	string id_;
	string name_;
	string padName_;
	DescListType descList_;
	MorphSkillsList skills_;
	string coverDesc_;
	string speech_;
	short toStr_;
	short toDex_;
	short toCon_;
	short toInt_;
	short toCha_;
	int str_;
	int intel_;
	int wis_;
	int dex_;
	int cha_;
	int con_;
	std::vector<EAffectFlags> affects_;
	string messageToRoom_, messageToChar_;

public:
	AnimalMorph(string id, string name, string padName, DescListType descList, MorphSkillsList skills, string coverDesc, string speech) :
		id_(id),
		name_(name),
		padName_(padName),
		descList_(descList),
		skills_(skills),
		coverDesc_(coverDesc),
		speech_(speech)
	{};

	~AnimalMorph() {};

	string GetMorphDesc();
	string Name() {return name_;}
	string PadName() {return padName_;}
	string GetMorphTitle();
	string CoverDesc() {return coverDesc_;}
	void InitSkills(int value);
	void InitAbils();
	void SetChar(CHAR_DATA *ch);
	void SetAbilsParams(short toStr, short toDex, short toCon, short toInt, short toCha)
	{
		toStr_ = toStr;
		toDex_ = toDex;
		toCon_ = toCon;
		toCha_ = toCha;
		toInt_ = toInt;
	};
	bool isAffected(const EAffectFlags flag) const;
	void AddAffect(const EAffectFlags flag);
	const affects_list_t& GetAffects();
	void SetAffects(const affects_list_t&);
	void SetMessages(string toRoom, string toChar) {
		messageToRoom_ = toRoom;
		messageToChar_ = toChar;
	};
	string GetMessageToRoom() {return messageToRoom_;}
	string GetMessageToChar() {return messageToChar_;}


	virtual int GetStr() const {return str_;}
	virtual void SetStr(int str) {str_=str;}
	virtual int GetIntel() const {return intel_;}
	virtual void SetIntel(int intel) {intel_=intel;}
	virtual int GetWis() const {return wis_;}
	virtual void SetWis(int wis) {wis_=wis;}
	virtual int GetDex() const {return dex_;}
	virtual void SetDex(int dex) {dex_=dex;}
	virtual int GetCha() const {return cha_;}
	virtual void SetCha(int cha) {cha_=cha;}
	virtual int GetCon()  const {return con_;}
	virtual void SetCon(int con) {con_=con;}

	void set_skill(int skill_num, int percent);
	int get_trained_skill(int skill_num);
};


void load_morphs();
void set_god_morphs(CHAR_DATA *ch);
void morphs_save(CHAR_DATA*, FILE*);
void morphs_load(CHAR_DATA*, std::string);

#endif // MORPH_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
