// $RCSfile$     $Date$     $Revision$
// Part of Bylins http://www.mud.ru

#ifndef PLAYER_RACES_HPP_INCLUDED
#define PLAYER_RACES_HPP_INCLUDED

#define RACE_UNDEFINED -1
#define PLAYER_RACE_FILE "playerraces.xml"

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"

class PlayerKin;
class PlayerRace;

typedef boost::shared_ptr<PlayerRace> PlayerRacePtr;
typedef std::vector<PlayerRacePtr> PlayerRaceListType;
typedef boost::shared_ptr<PlayerKin> PlayerKinPtr;
typedef std::vector<PlayerKinPtr> PlayerKinListType;

class PlayerKin
{
public:
	PlayerRaceListType PlayerRaceList;

	PlayerKin();
	//	void ShowMenu(CHAR_DATA *ch);
	
	int KinNum;					//number of kin
	std::string KinMenuStr;	
	std::string KinItName;	
	std::string KinHeName;
	std::string KinSheName;
	std::string KinPluralName;
};

class PlayerRace
{
public:
	static PlayerKinListType PlayerKinList;

	void SetRaceNum(int Num)
	{
		_RaceNum = Num;
	};
	void SetRaceMenuStr(std::string MenuStr)
	{
		_RaceMenuStr = MenuStr;
	};
	void SetRaceItName(std::string Name)
	{
		_RaceItName = Name;
	};
	void SetRaceHeName(std::string Name)
	{
		_RaceHeName = Name;
	};
	void SetRaceSheName(std::string Name)
	{
		_RaceSheName = Name;
	};
	void SetRacePluralName(std::string Name)
	{
		_RacePluralName = Name;
	};
	int GetRaceNum()
	{
		return _RaceNum;
	};
	std::string GetMenuStr()
	{
		return _RaceMenuStr;
	};

	void AddRaceFeature(int feat);
	static void Load(const char *PathToFile);
	static PlayerRacePtr GetPlayerRace(int Kin,int Race);
	static std::vector<int> GetRaceFeatures(int Kin,int Race);
	static void GetKinNamesList(CHAR_DATA *ch);
	static bool FeatureCheck(int Kin,int Race,int Feat);

private:
	int _RaceNum;	//number of race
	std::string _RaceMenuStr;	
	std::string _RaceItName;	
	std::string _RaceHeName;
	std::string _RaceSheName;
	std::string _RacePluralName;
	std::vector<int> _RaceFeatureList; //list of race's features	

};

#endif // PLAYER_RACES_HPP_INCLUDED
