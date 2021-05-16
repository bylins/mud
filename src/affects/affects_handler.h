#ifndef _AFFECTHANDLER_HPP_
#define _AFFECTHANDLER_HPP_
#include "chars/character.h"


// класс новых аффектов
class AffectParent {
	// чар, который кастанул цель
	CHAR_DATA *ch;
	// чар, на которого касстанули аффект
	CHAR_DATA *vict;
	// сколько тиков осталось висеть на чаре
	int time;
	// частота, которая показывает, как часто за тик дергается данный аффект
	const int freq;

public:
	AffectParent(CHAR_DATA *character, CHAR_DATA *victim, int time, int frequency);
	void DoAffect();
};



//классы для конфигурирования обработчиков Handler()
class DamageActorParameters {
public:
	DamageActorParameters(CHAR_DATA* act, CHAR_DATA* vict, int dam) : ch(act), opponent(vict), damage(dam) {};
	CHAR_DATA* ch;
	CHAR_DATA* opponent;
	int damage;
};

class DamageVictimParameters {
public:
	DamageVictimParameters(CHAR_DATA* act, CHAR_DATA* vict, int dam) : ch(vict), opponent(act), damage(dam) {};
	CHAR_DATA* ch;
	CHAR_DATA* opponent;
	int damage;
};

class BattleRoundParameters{
public:
	BattleRoundParameters(CHAR_DATA* actor) : ch(actor) {};
	CHAR_DATA* ch;
};

class StopFightParameters
{
public:
	StopFightParameters(CHAR_DATA* actor) : ch(actor) {};
	CHAR_DATA* ch;
};
//инфтерфейс для обработчиков аффектов
class IAffectHandler
{
public:
	IAffectHandler(void) {};
	virtual ~IAffectHandler(void) {};
	virtual void Handle(DamageActorParameters&/* params*/) {};
	virtual void Handle(DamageVictimParameters&/* params*/) {};
	virtual void Handle(BattleRoundParameters&/* params*/) {};
	virtual void Handle(StopFightParameters&/* params*/) {};
};

class LackyAffectHandler : public IAffectHandler{
public:
	LackyAffectHandler() : round_(0), damToMe_(false), damFromMe_(false) {};
	virtual ~LackyAffectHandler() {};
	virtual void Handle(DamageActorParameters& params);
	virtual void Handle(BattleRoundParameters& params);
	virtual void Handle(DamageVictimParameters& params);
	virtual void Handle(StopFightParameters& params);
private:
	int round_;
	bool damToMe_;
	bool damFromMe_;
};

//Polud функция, вызывающая обработчики аффектов, если они есть
template <class S>
void handle_affects(S& params) //тип params определяется при вызове функции
{
	for (const auto& aff : params.ch->affected)
	{
		if (aff->handler)
		{
			aff->handler->Handle(params); //в зависимости от типа params вызовется нужный Handler
		}
	}
}

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
