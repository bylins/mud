// by bodrich (2014)
// http://mud.ru

#include <vector>
#include <string>
#include "quest.hpp"

Quest::Quest(int id, int time_start, int time_end, std::string text_quest, std::string tquest, int var_quest)
{
	this->id = id;
	this->time_start = time_start;
	this->time_end = time_end;
	this->text_quest = text_quest;
	this->tquest = tquest;
	this->var_quest = var_quest;
}

int Quest::get_id()
{
	return this->id;
}

int Quest::get_time_start()
{
	return this->time_start;
}

int Quest::get_time_end()
{
	return this->time_end;
}

int Quest::get_time()
{
	return this->time_end-this->time_start;
}

std::string Quest::get_text()
{
	return this->text_quest;
}

std::string Quest::get_tquest()
{
	return this->tquest;
}

int Quest::get_var_quest()
{
	return this->var_quest;
}

int Quest::pquest()
{
	return this->pvar_quest;
}

void Quest::set_pvar(int pvar)
{
	this->pvar_quest = pvar;
}




// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
