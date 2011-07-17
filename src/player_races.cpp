// $RCSfile$     $Date$     $Revision$
// Part of Bylins http://www.mud.ru

#include <algorithm>

#include "conf.h"
#include "sysdep.h"
#include "utils.h"
#include "comm.h"
//#include "db.h"
//#include "dg_scripts.h"
//#include "char.hpp"
//#include "handler.h"

#include "player_races.hpp"
#include "pugixml.hpp"

PlayerKinListType PlayerRace::PlayerKinList;

void LoadRace(pugi::xml_node RaceNode, PlayerKinPtr KinPtr);
void LoadKin(pugi::xml_node KinNode);

PlayerKin::PlayerKin()
{
//Create kin
};

// Parse & create new player race
void LoadRace(pugi::xml_node RaceNode, PlayerKinPtr KinPtr)
{
	pugi::xml_node CurNode;
	PlayerRacePtr TmpRace(new PlayerRace);

	TmpRace->SetRaceNum(RaceNode.attribute("racenum").as_int());
	TmpRace->SetRaceMenuStr(RaceNode.child("menu").child_value());
	CurNode = RaceNode.child("racename");
	TmpRace->SetRaceItName(CurNode.child("itname").child_value());
	TmpRace->SetRaceHeName(CurNode.child("hename").child_value());
	TmpRace->SetRaceSheName(CurNode.child("shename").child_value());
	TmpRace->SetRacePluralName(CurNode.child("pluralname").child_value());
	//Add race features
	for (CurNode = RaceNode.child("feature"); CurNode; CurNode = CurNode.next_sibling("feature"))
	{
		TmpRace->AddRaceFeature(CurNode.attribute("featnum").as_int());
	}
	//Add new race in list
	KinPtr->PlayerRaceList.push_back(TmpRace);
}

// Parse & create new player kin
void LoadKin(pugi::xml_node KinNode)
{
	pugi::xml_node CurNode;
	PlayerKinPtr TmpKin(new PlayerKin);

	//Parse kin's parameters
	TmpKin->KinNum = KinNode.attribute("kinnum").as_int();
	CurNode = KinNode.child("menu");
	TmpKin->KinMenuStr = CurNode.child_value();
	CurNode = KinNode.child("kinname");
	TmpKin->KinItName = CurNode.child("itname").child_value();
	TmpKin->KinHeName = CurNode.child("hename").child_value();
	TmpKin->KinSheName = CurNode.child("shename").child_value();
	TmpKin->KinPluralName = CurNode.child("pluralname").child_value();

	//Parce kin races
	CurNode = KinNode.child("kinraces");
	for (CurNode = CurNode.child("race"); CurNode; CurNode = CurNode.next_sibling("race"))
	{
		LoadRace(CurNode, TmpKin);
	}
	//Add new kin in kin list
	PlayerRace::PlayerKinList.push_back(TmpKin);
}

// Load players races & kins
void PlayerRace::Load(const char *PathToFile)
{
	char buf[MAX_INPUT_LENGTH];
	pugi::xml_document Doc;
	pugi::xml_node KinList, KinNode;
	pugi::xml_parse_result Result;

	Result = Doc.load_file(PathToFile);
	if (!Result)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...%s", Result.description());
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}

	KinList = Doc.child("races");
	if (!KinList)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...players races reading fail");
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	} 
	for (KinNode = KinList.child("kin"); KinNode; KinNode = KinNode.next_sibling("kin"))
	{
		LoadKin(KinNode);
	}
}

void PlayerRace::AddRaceFeature(int feat)
{
	std::vector<int>::iterator RaceFeature = find(_RaceFeatureList.begin(), _RaceFeatureList.end(), feat);
	if (RaceFeature == _RaceFeatureList.end())
		_RaceFeatureList.push_back(feat);
};

PlayerRacePtr PlayerRace::GetPlayerRace(int Kin,int Race)
{
	PlayerRacePtr RacePtr;
	for (PlayerKinListType::iterator it =  PlayerKinList.begin();it != PlayerKinList.end();++it)
	{
		if ((*it)->KinNum == Kin)
		{
			for (PlayerRaceListType::iterator itr = (*it)->PlayerRaceList.begin();itr != (*it)->PlayerRaceList.end();++itr)
			{
				if ((*itr)->_RaceNum == Race)
				{
					RacePtr = *itr;
				}
			}
		}
	}
	return RacePtr;
};

bool PlayerRace::FeatureCheck(int Kin,int Race,int Feat)
{
	PlayerRacePtr RacePtr = PlayerRace::GetPlayerRace(Kin, Race);
	if (RacePtr == NULL)
		return false;
	std::vector<int>::iterator RaceFeature = find(RacePtr->_RaceFeatureList.begin(), RacePtr->_RaceFeatureList.end(), Feat);
	if (RaceFeature != RacePtr->_RaceFeatureList.end())
		return true;

	return false;
};

void PlayerRace::GetKinNamesList(CHAR_DATA *ch)
{
	//char buf[MAX_INPUT_LENGTH];
	//snprintf(buf, MAX_STRING_LENGTH, " %d \r\n", PlayerKinList[0]->PlayerRaceList[0]->GetFeatNum());
	//send_to_char(buf, ch);
	//for (PlayerKinListType::iterator it = PlayerKinList.begin();it != PlayerKinList.end();++it)
	//{
	//	snprintf(buf, MAX_STRING_LENGTH, " %s \r\n", (*it)->KinHeName.c_str());
	//	send_to_char(buf, ch);
	//}
	//test message
	//char buf33[MAX_INPUT_LENGTH];
	//snprintf(buf33, MAX_STRING_LENGTH, "!==!...%s", CurNode.child("shename").child_value());
	//mudlog(buf33, CMP, LVL_IMMORT, SYSLOG, TRUE);
}

std::vector<int> PlayerRace::GetRaceFeatures(int Kin,int Race)
{
	std::vector<int> RaceFeatures;
	PlayerRacePtr RacePtr = PlayerRace::GetPlayerRace(Kin, Race);
	if (RacePtr != NULL) {
		RaceFeatures = RacePtr->_RaceFeatureList;
	} else {
		RaceFeatures.push_back(0);
	}
	return RaceFeatures;
}
