#ifndef MORPH_HPP_INCLUDED
#define MORPH_HPP_INCLUDED

#include "skills.h"
#include "comm.h"

#include <list>
#include <set>
#include <unordered_map>

extern short MIN_WIS_FOR_MORPH;

typedef struct
{
	int fromLevel;
	std::string desc;
} DescNode;

typedef std::list<DescNode> DescListType;

class IMorph;
class AnimalMorph;

typedef std::shared_ptr<IMorph> MorphPtr;
typedef std::shared_ptr<AnimalMorph> AnimalMorphPtr;
MorphPtr GetNormalMorphNew(CHAR_DATA *ch);
typedef std::map<std::string, AnimalMorphPtr> MorphListType;

typedef std::unordered_map<ESkill, int> MorphSkillsList;

class IMorph {
public:
	typedef std::set<EAffectFlag> affects_list_t;

	IMorph() {};
	virtual ~IMorph() {};
	virtual std::string Name() const = 0;
	virtual std::string PadName() const = 0;
	virtual std::string GetMorphDesc() const = 0;
	virtual std::string GetMorphTitle() const = 0;
	virtual void InitSkills(int/* value*/) {};
	virtual void InitAbils() {};
	virtual void SetAbilsParams(short/* toStr*/, short/* toDex*/, short/* toCon*/, short/* toInt*/, short/* toCha*/) {};
	virtual void SetChar(CHAR_DATA* /*ch*/) {};
	virtual std::string CoverDesc() { return ""; };
	virtual bool isAffected(const EAffectFlag/* flag*/) const { return false; }
	virtual const affects_list_t& GetAffects();
	virtual std::string GetMessageToRoom() { return std::string(); }
	virtual std::string GetMessageToChar() { return std::string(); }

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

	virtual void set_skill(const ESkill skill_num, int percent)=0;
	virtual int get_trained_skill(const ESkill skill_num)=0;
};



class NormalMorph : public IMorph
{	
public:
	NormalMorph (CHAR_DATA *ch) {ch_=ch;}
	CHAR_DATA *ch_;

	~NormalMorph () {};

	std::string GetMorphDesc() const;
	std::string GetMorphTitle() const;
	std::string Name() const { return "Обычная"; }
	std::string PadName() const { return "Человеком"; }
	void SetChar(CHAR_DATA *ch) {ch_=ch;};

	void set_skill(const ESkill skill_num, int percent);
	int get_trained_skill(const ESkill skill_num);

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
	std::string id_;
	std::string name_;
	std::string padName_;
	DescListType descList_;
	MorphSkillsList skills_;
	std::string coverDesc_;
	std::string speech_;
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
	affects_list_t affects_;
	std::string messageToRoom_, messageToChar_;

public:
	AnimalMorph(const std::string& id, const std::string& name, const std::string& padName, DescListType descList,
		MorphSkillsList skills, const std::string& coverDesc, const std::string& speech) :
		id_(id),
		name_(name),
		padName_(padName),
		descList_(descList),
		skills_(skills),
		coverDesc_(coverDesc),
		speech_(speech)
	{};

	~AnimalMorph() {};

	std::string GetMorphDesc() const;
	std::string Name() const { return name_; }
	std::string PadName() const { return padName_; }
	std::string GetMorphTitle() const;
	std::string CoverDesc() { return coverDesc_; }
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
	bool isAffected(const EAffectFlag flag) const;
	void AddAffect(const EAffectFlag flag);
	const affects_list_t& GetAffects();
	void SetAffects(const affects_list_t&);
	void SetMessages(const std::string& toRoom, const std::string& toChar) {
		messageToRoom_ = toRoom;
		messageToChar_ = toChar;
	};
	std::string GetMessageToRoom() { return messageToRoom_; }
	std::string GetMessageToChar() { return messageToChar_; }

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

	void set_skill(const ESkill skill_num, int percent);
	int get_trained_skill(const ESkill skill_num);
};


void load_morphs();
void set_god_morphs(CHAR_DATA *ch);
void morphs_save(CHAR_DATA*, FILE*);
void morphs_load(CHAR_DATA*, std::string);

#endif // MORPH_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
