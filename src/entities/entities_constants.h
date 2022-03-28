/**
 \authors Created by Sventovit
 \date 13.01.2022.
 \brief Константы для персонажей, комнат и предметов..
 \details Тут должны располагаться исключительно константы и минимум кода.
*/

#ifndef BYLINS_SRC_ENTITY_ROOMS_ROOM_CONSTANTS_H_
#define BYLINS_SRC_ENTITY_ROOMS_ROOM_CONSTANTS_H_

#include "structs/structs.h"

/*
 * ========================================================================================
 *  								General entities constants
 * ========================================================================================
 */

enum class ESex : byte {
	kNeutral = 0,
	kMale = 1,
	kFemale = 2,
	kPoly = 3,
	kLast
};

template<>
ESex ITEM_BY_NAME<ESex>(const std::string &name);
template<>
const std::string &NAME_BY_ITEM(ESex item);

/*
 * ========================================================================================
 *  								Mutual PC-NPC constants
 * ========================================================================================
 */

/*
 * Character positions.
 */
enum class EPosition {
	kIncorrect = -1, // Это неправильно, но в классах Hit и Damage есть позиция -1, надо переделывать.
	kDead = 0,
	kPerish = 1,	// mortally wounded  //
	kIncap = 2,
	kStun = 3,
	kSleep = 4,
	kRest = 5,
	kSit = 6,
	kFight = 7,
	kStand = 8,
	kLast};

int operator-(EPosition p1,  EPosition p2);
EPosition operator--(const EPosition &p);

/*
 * Character base stats ids.
 */
enum class EBaseStat : int {
	kStr = 0,
	kDex = 1,
	kCon = 2,
	kWis = 3,
	kInt = 4,
	kCha = 5,
};

template<>
const std::string &NAME_BY_ITEM<EBaseStat>(EBaseStat item);
template<>
EBaseStat ITEM_BY_NAME<EBaseStat>(const std::string &name);

/*
 * Character savings ids.
 */
enum class ESaving : int {
	kWill = 0,
	kCritical = 1,
	kStability = 2,
	kReflex = 3,
	kFirst = kWill,
	kLast = kReflex, // Не забываем менять при добаввлении новых элементов.
};

ESaving& operator++(ESaving &s);

template<>
const std::string &NAME_BY_ITEM<ESaving>(ESaving item);
template<>
ESaving ITEM_BY_NAME<ESaving>(const std::string &name);

/*
 * Character equipment positions: used as index for char_data.equipment[]
 * MOTE: Don't confuse these constants with the ITEM_ bitvectors which control
 * the valid places you can wear a piece of equipment
 */
enum EEquipPos : int {
	kLight = 0,
	kFingerR = 1,
	kFingerL = 2,
	kNeck = 3,
	kChest = 4,
	kBody = 5,
	kHead = 6,
	kLegs = 7,
	kFeet = 8,
	kHands = 9,
	kArms = 10,
	kShield = 11,
	kShoulders = 12,
	kWaist = 13,
	kWristR = 14,
	kWristL = 15,
	kWield = 16,      // правая рука
	kHold = 17,      // левая рука
	kBoths = 18,      // обе руки
	kQuiver = 19,      // под лук (колчан)
	kNumEquipPos = 20    // This must be the # of eq positions!! //
};

/*
 * ========================================================================================
 *  								PC constants
 * ========================================================================================
 */

/**
 * Preference flags: used by char_data.player_specials.pref
 */
enum EPrf : Bitvector {
	kBrief = 1 << 0,        // Room descs won't normally be shown //
	kCompact = 1 << 1,    // No extra CRLF pair before prompts  //
	kNoHoller = 1 << 2,    // Не слышит команду "орать"   //
	kNoTell = 1 << 3,        // Не слышит команду "сказать" //
	kDispHp = 1 << 4,        // Display hit points in prompt   //
	kDispMana = 1 << 5,    // Display mana points in prompt   //
	kDispMove = 1 << 6,    // Display move points in prompt   //
	kAutoexit = 1 << 7,    // Display exits in a room      //
	kNohassle = 1 << 8,    // Aggr mobs won't attack    //
	KSummonable = 1 << 9,  // Can be summoned         //
	kQuest = 1 << 10,        // On quest                       //
	kNoRepeat = 1 << 11,   // No repetition of comm commands  //
	kHolylight = 1 << 12,  // Can see in dark        //
	kColor1 = 1 << 13,    // Color (low bit)       //
	kColor2 = 1 << 14,    // Color (high bit)         //
	kNoWiz = 1 << 15,        // Can't hear wizline       //
	kLog1 = 1 << 16,        // On-line System Log (low bit)   //
	kLog2 = 1 << 17,        // On-line System Log (high bit)  //
	kNoAuction = 1 << 18,        // Can't hear auction channel     //
	kNoGossip = 1 << 19,        // Не слышит команду "болтать" //
	kDispFight = 1 << 20,  // Видит свое состояние в бою      //
	kRoomFlags = 1 << 21,  // Can see room flags (ROOM_x)  //
	kDispExp = 1 << 22,
	kDispExits = 1 << 23,
	kDispLvl = 1 << 24,
	kDispMoney = 1 << 25,
	kDispTick = 1 << 26,
	kPunctual = 1 << 27,
	kAwake = 1 << 28,
	kCoderinfo = 1 << 29,

	kAutomem = kIntOne | 1 << 0,
	kNoShout = kIntOne | 1 << 1,                // Не слышит команду "кричать"  //
	kGoAhead = kIntOne | 1 << 2,                // Добавление IAC GA после промпта //
	kShowGroup = kIntOne | 1 << 3,            // Показ полного состава группы //
	kAutoassist = kIntOne | 1 << 4,            // Автоматическое вступление в бой //
	kAutoloot = kIntOne | 1 << 5,                // Autoloot //
	kAutosplit = kIntOne | 1 << 6,            // Autosplit //
	kAutomoney = kIntOne | 1 << 7,            // Automoney //
	kNoArena = kIntOne | 1 << 8,                // Не слышит арену //
	kNoExchange = kIntOne | 1 << 9,            // Не слышит базар //
	kNoClones = kIntOne | 1 << 10,            // Не видит в группе чужих клонов //
	kNoInvistell = kIntOne | 1 << 11,            // Не хочет, чтобы телял "кто-то" //
	kPowerAttack = kIntOne | 1 << 12,            // мощная атака //
	kGreatPowerAttack = kIntOne | 1 << 13,    // улучшеная мощная атака //
	kAimingAttack = kIntOne | 1 << 14,        // прицельная атака //
	kGreatAimingAttack = kIntOne | 1 << 15,    // улучшеная прицельная атака //
	kNewsMode = kIntOne | 1 << 16,            // вариант чтения новостей мада и дружины
	kBoardMode = kIntOne | 1 << 17,            // уведомления о новых мессагах на досках
	kDecayMode = kIntOne | 1 << 18,            // канал хранилища, рассыпание шмота
	kTakeMode = kIntOne | 1 << 19,            // канал хранилища, положили/взяли
	kPklMode = kIntOne | 1 << 20,            // уведомления о добавлении/убирании в пкл
	kPolitMode = kIntOne | 1 << 21,            // уведомления об изменении политики, своей и чужой
	kIronWind = kIntOne | 1 << 22,            // включен скилл "железный ветер"
	kPkFormatMode = kIntOne | 1 << 23,        // формат пкл/дрл
	kClanmembersMode = kIntOne | 1 << 24,        // показ входов/выходов соклановцев
	kOfftopMode = kIntOne | 1 << 25,        // вкл/выкл канала оффтопа
	kAntiDcMode = kIntOne | 1 << 26,        // режим защиты от дисконекта в бою
	kNoIngrMode = kIntOne | 1 << 27,        // не показывать продажу/покупку ингров в канале базара
	kNoIngrLoot = kIntOne | 1 << 28,        // не лутить ингры в режиме автограбежа
	kDispTimed = kIntOne | 1 << 29,            // показ задержек для умений и способностей

	kStopOfftop = kIntTwo | 1 << 0,            // для стоп-списка оффтоп
	kExecutor = kIntTwo | 1 << 1,            // палач
	kDrawMap = kIntTwo | 1 << 2,            // отрисовка карты при осмотре клетки
	kCanRemort = kIntTwo | 1 << 3,            // разрешение на реморт через жертвование гривн
	kShowZoneNameOnEnter = kIntTwo | 1 << 4,  // вывод названия/среднего уровня при входе в зону
	kShowUnread = kIntTwo | 1 << 5,            // показ непрочитанных сообщений на доске опечаток при входе
	kBriefShields = kIntTwo | 1 << 6,        // краткий режим сообщений при срабатывании маг.щитов
	kAutonosummon = kIntTwo | 1 << 7,        // автоматическое включение режима защиты от призыва ('реж призыв') после удачного суммона/пенты
	kDemigodChat = kIntTwo | 1 << 8,          // Для канала демигодов
	kBlindMode = kIntTwo | 1 << 9,            // Режим слабовидящего игрока
	kMapper = kIntTwo | 1 << 10,                // Показывает хеши рядом с названием комнаты
	kTester = kIntTwo | 1 << 11,                // отображать допинфу при годсфлаге тестер
	kIpControl = kIntTwo | 1 << 12,            // отправлять код на мыло при заходе из новой подсети
	kSkirmisher = kIntTwo | 1 << 13,            // персонаж "в строю" в группе
	kDoubleThrow = kIntTwo | 1 << 14,        // готов использовать двойной бросок
	kTripleThrow = kIntTwo | 1 << 15,        // готов использовать тройной бросок
	kShadowThrow = kIntTwo | 1 << 16,        // применяет "теневой бросок"
	kDispCooldowns = kIntTwo | 1 << 17,        // Показывать кулдауны скиллов в промпте
	kTelegram = kIntTwo | 1 << 18            // Активирует телеграм-канал у персонажа
};

// при добавлении не забываем про preference_bits[]

/**
 * PC religions
 * \todo Все, связанное с религиями, нужно вынести в отдельный модуль.
 */
const __uint8_t kReligionPoly = 0;
const __uint8_t kReligionMono = 1;

typedef std::array<const char *, static_cast<std::size_t>(ESex::kLast)> religion_genders_t;
typedef std::array<religion_genders_t, 3> religion_names_t;
extern const religion_names_t religion_name;

/**
 *	Player flags: used by char_data.char_specials.act
 */
constexpr Bitvector PLR_KILLER = 1 << 0;            // Player is a player-killer     //
constexpr Bitvector PLR_THIEF = 1 << 1;            // Player is a player-thief      //
constexpr Bitvector PLR_FROZEN = 1 << 2;            // Player is frozen        //
constexpr Bitvector PLR_DONTSET = 1 << 3;            // Don't EVER set (ISNPC bit)  //
constexpr Bitvector PLR_WRITING = 1 << 4;            // Player writing (board/mail/olc)  //
constexpr Bitvector PLR_MAILING = 1 << 5;            // Player is writing mail     //
constexpr Bitvector PLR_CRASH = 1 << 6;            // Player needs to be crash-saved   //
constexpr Bitvector PLR_SITEOK = 1 << 7;            // Player has been site-cleared  //
constexpr Bitvector PLR_MUTE = 1 << 8;            // Player not allowed to shout/goss/auct  //
constexpr Bitvector PLR_NOTITLE = 1 << 9;            // Player not allowed to set title  //
constexpr Bitvector PLR_DELETED = 1 << 10;        // Player deleted - space reusable  //
constexpr Bitvector PLR_LOADROOM = 1 << 11;        // Player uses nonstandard loadroom  (не используется) //
constexpr Bitvector PLR_AUTOBOT = 1 << 12;        // Player автоматический игрок //
constexpr Bitvector PLR_NODELETE = 1 << 13;        // Player shouldn't be deleted //
constexpr Bitvector PLR_INVSTART = 1 << 14;        // Player should enter game wizinvis //
constexpr Bitvector PLR_CRYO = 1 << 15;            // Player is cryo-saved (purge prog)   //
constexpr Bitvector PLR_HELLED = 1 << 16;            // Player is in Hell //
constexpr Bitvector PLR_NAMED = 1 << 17;            // Player is in Names Room //
constexpr Bitvector PLR_REGISTERED = 1 << 18;
constexpr Bitvector PLR_DUMB = 1 << 19;            // Player is not allowed to tell/emote/social //
constexpr Bitvector PLR_SCRIPTWRITER = 1 << 20;   // скриптер
constexpr Bitvector PLR_SPAMMER = 1 << 21;        // спаммер
// свободно
constexpr Bitvector PLR_DELETE = 1 << 28;            // RESERVED - ONLY INTERNALLY (MOB_DELETE) //
constexpr Bitvector PLR_FREE = 1 << 29;            // RESERVED - ONLY INTERBALLY (MOB_FREE)//

/**
 *	Gods flags.
 */
constexpr Bitvector GF_GODSLIKE = 1 << 0;
constexpr Bitvector GF_GODSCURSE = 1 << 1;
constexpr Bitvector GF_HIGHGOD = 1 << 2;
constexpr Bitvector GF_REMORT = 1 << 3;
constexpr Bitvector GF_DEMIGOD = 1 << 4;    // Морталы с привилегиями богов //
constexpr Bitvector GF_PERSLOG = 1 << 5;
constexpr Bitvector GF_TESTER = 1 << 6;

/**
 *	Modes of ignoring
 *	\todo Перенести в модуль с соственно игнорированием каналов.
 */
constexpr Bitvector IGNORE_TELL = 1 << 0;
constexpr Bitvector IGNORE_SAY = 1 << 1;
constexpr Bitvector IGNORE_CLAN = 1 << 2;
constexpr Bitvector IGNORE_ALLIANCE = 1 << 3;
constexpr Bitvector IGNORE_GOSSIP = 1 << 4;
constexpr Bitvector IGNORE_SHOUT = 1 << 5;
constexpr Bitvector IGNORE_HOLLER = 1 << 6;
constexpr Bitvector IGNORE_GROUP = 1 << 7;
constexpr Bitvector IGNORE_WHISPER = 1 << 8;
constexpr Bitvector IGNORE_ASK = 1 << 9;
constexpr Bitvector IGNORE_EMOTE = 1 << 10;
constexpr Bitvector IGNORE_OFFTOP = 1 << 11;

/*
 * ========================================================================================
 *  								NPC's constants
 * ========================================================================================
 */

/**
 * NPC races
 */
enum ENpcRace : int {
	kBasic = 100,
	kHuman = 101,
	kBeastman = 102,
	kBird = 103,
	kAnimal = 104,
	kReptile = 105,
	kFish = 106,
	kInsect = 107,
	kPlant = 108,
	kConstruct = 109,
	kZombie = 110,
	kGhost = 111,
	kBoggart = 112,
	kSpirit = 113,
	kMagicCreature = 114,
	kLastNpcRace = kMagicCreature // Не забываем менять при добавлении новых
};

/**
 * Virtual NPC races
 */
const int NPC_BOSS = 200;
const int NPC_UNIQUE = 201;

/**
 * Mobile flags: used by char_data.char_specials.act
 */
constexpr Bitvector MOB_SPEC = 1 << 0;            // Mob has a callable spec-proc  //
constexpr Bitvector MOB_SENTINEL = 1 << 1;        // Mob should not move     //
constexpr Bitvector MOB_SCAVENGER = 1 << 2;    // Mob picks up stuff on the ground //
constexpr Bitvector MOB_ISNPC = 1 << 3;            // (R) Automatically set on all Mobs   //
constexpr Bitvector MOB_AWARE = 1 << 4;            // Mob can't be backstabbed      //
constexpr Bitvector MOB_AGGRESSIVE = 1 << 5;    // Mob hits players in the room  //
constexpr Bitvector MOB_STAY_ZONE = 1 << 6;    // Mob shouldn't wander out of zone //
constexpr Bitvector MOB_WIMPY = 1 << 7;            // Mob flees if severely injured //
constexpr Bitvector MOB_AGGR_DAY = 1 << 8;        // //
constexpr Bitvector MOB_AGGR_NIGHT = 1 << 9;    // //
constexpr Bitvector MOB_AGGR_FULLMOON = 1 << 10;  // //
constexpr Bitvector MOB_MEMORY = 1 << 11;            // remember attackers if attacked   //
constexpr Bitvector MOB_HELPER = 1 << 12;            // attack PCs fighting other NPCs   //
constexpr Bitvector MOB_NOCHARM = 1 << 13;        // Mob can't be charmed    //
constexpr Bitvector MOB_NOSUMMON = 1 << 14;        // Mob can't be summoned      //
constexpr Bitvector MOB_NOSLEEP = 1 << 15;        // Mob can't be slept      //
constexpr Bitvector MOB_NOBASH = 1 << 16;            // Mob can't be bashed (e.g. trees) //
constexpr Bitvector MOB_NOBLIND = 1 << 17;        // Mob can't be blinded    //
constexpr Bitvector MOB_MOUNTING = 1 << 18;
constexpr Bitvector MOB_NOHOLD = 1 << 19;
constexpr Bitvector MOB_NOSIELENCE = 1 << 20;
constexpr Bitvector MOB_AGGRMONO = 1 << 21;
constexpr Bitvector MOB_AGGRPOLY = 1 << 22;
constexpr Bitvector MOB_NOFEAR = 1 << 23;
constexpr Bitvector MOB_NOGROUP = 1 << 24;
constexpr Bitvector MOB_CORPSE = 1 << 25;
constexpr Bitvector MOB_LOOTER = 1 << 26;
constexpr Bitvector MOB_PROTECT = 1 << 27;
constexpr Bitvector MOB_DELETE = 1 << 28;            // RESERVED - ONLY INTERNALLY //
constexpr Bitvector MOB_FREE = 1 << 29;            // RESERVED - ONLY INTERBALLY //

constexpr Bitvector MOB_SWIMMING = kIntOne | (1 << 0);
constexpr Bitvector MOB_FLYING = kIntOne | (1 << 1);
constexpr Bitvector MOB_ONLYSWIMMING = kIntOne | (1 << 2);
constexpr Bitvector MOB_AGGR_WINTER = kIntOne | (1 << 3);
constexpr Bitvector MOB_AGGR_SPRING = kIntOne | (1 << 4);
constexpr Bitvector MOB_AGGR_SUMMER = kIntOne | (1 << 5);
constexpr Bitvector MOB_AGGR_AUTUMN = kIntOne | (1 << 6);
constexpr Bitvector MOB_LIKE_DAY = kIntOne | (1 << 7);
constexpr Bitvector MOB_LIKE_NIGHT = kIntOne | (1 << 8);
constexpr Bitvector MOB_LIKE_FULLMOON = kIntOne | (1 << 9);
constexpr Bitvector MOB_LIKE_WINTER = kIntOne | (1 << 10);
constexpr Bitvector MOB_LIKE_SPRING = kIntOne | (1 << 11);
constexpr Bitvector MOB_LIKE_SUMMER = kIntOne | (1 << 12);
constexpr Bitvector MOB_LIKE_AUTUMN = kIntOne | (1 << 13);
constexpr Bitvector MOB_NOFIGHT = kIntOne | (1 << 14);
constexpr Bitvector MOB_EADECREASE = kIntOne | (1 << 15); // понижает количество своих атак по мере убывания тек.хп
constexpr Bitvector MOB_HORDE = kIntOne | (1 << 16);
constexpr Bitvector MOB_CLONE = kIntOne | (1 << 17);
constexpr Bitvector MOB_NOTKILLPUNCTUAL = kIntOne | (1 << 18);
constexpr Bitvector MOB_NOTRIP = kIntOne | (1 << 19);
constexpr Bitvector MOB_ANGEL = kIntOne | (1 << 20);
constexpr Bitvector MOB_GUARDIAN = kIntOne | (1 << 21); //моб-стражник, ставится программно из файла guards.xml
constexpr Bitvector MOB_IGNORE_FORBIDDEN = kIntOne | (1 << 22); // игнорирует печать
constexpr Bitvector MOB_NO_BATTLE_EXP = kIntOne | (1 << 23); // не дает экспу за удары
constexpr Bitvector MOB_NOMIGHTHIT = kIntOne | (1 << 24); // нельзя оглушить богатырским молотом
constexpr Bitvector MOB_GHOST = kIntOne | (1 << 25); // Используется для ментальной тени
constexpr Bitvector MOB_PLAYER_SUMMON = kIntOne | (1 << 26); // (ангел, тень, храны, трупы, умки)

constexpr Bitvector MOB_FIREBREATH = kIntTwo | (1 << 0);
constexpr Bitvector MOB_GASBREATH = kIntTwo | (1 << 1);
constexpr Bitvector MOB_FROSTBREATH = kIntTwo | (1 << 2);
constexpr Bitvector MOB_ACIDBREATH = kIntTwo | (1 << 3);
constexpr Bitvector MOB_LIGHTBREATH = kIntTwo | (1 << 4);
constexpr Bitvector MOB_NOTRAIN = kIntTwo | (1 << 5);
constexpr Bitvector MOB_NOREST = kIntTwo | (1 << 6);
constexpr Bitvector MOB_AREA_ATTACK = kIntTwo | (1 << 7);
constexpr Bitvector MOB_NOSTUPOR = kIntTwo | (1 << 8);
constexpr Bitvector MOB_NOHELPS = kIntTwo | (1 << 9);
constexpr Bitvector MOB_OPENDOOR = kIntTwo | (1 << 10);
constexpr Bitvector MOB_IGNORNOMOB = kIntTwo | (1 << 11);
constexpr Bitvector MOB_IGNORPEACE = kIntTwo | (1 << 12);
constexpr Bitvector MOB_RESURRECTED = kIntTwo | (1 << 13); // !поднять труп! или !оживить труп! только програмно//
constexpr Bitvector MOB_RUSICH = kIntTwo | (1 << 14);
constexpr Bitvector MOB_VIKING = kIntTwo | (1 << 15);
constexpr Bitvector MOB_STEPNYAK = kIntTwo | (1 << 16);
constexpr Bitvector MOB_AGGR_RUSICHI = kIntTwo | (1 << 17);
constexpr Bitvector MOB_AGGR_VIKINGI = kIntTwo | (1 << 18);
constexpr Bitvector MOB_AGGR_STEPNYAKI = kIntTwo | (1 << 19);
constexpr Bitvector MOB_NORESURRECTION = kIntTwo | (1 << 20);
constexpr Bitvector MOB_AWAKE = kIntTwo | (1 << 21);
constexpr Bitvector MOB_IGNORE_FORMATION = kIntTwo | (1 << 22);

constexpr Bitvector NPC_NORTH = 1 << 0;
constexpr Bitvector NPC_EAST = 1 << 1;
constexpr Bitvector NPC_SOUTH = 1 << 2;
constexpr Bitvector NPC_WEST = 1 << 3;
constexpr Bitvector NPC_UP = 1 << 4;
constexpr Bitvector NPC_DOWN = 1 << 5;
constexpr Bitvector NPC_POISON = 1 << 6;
constexpr Bitvector NPC_INVIS = 1 << 7;
constexpr Bitvector NPC_SNEAK = 1 << 8;
constexpr Bitvector NPC_CAMOUFLAGE = 1 << 9;
constexpr Bitvector NPC_MOVEFLY = 1 << 11;
constexpr Bitvector NPC_MOVECREEP = 1 << 12;
constexpr Bitvector NPC_MOVEJUMP = 1 << 13;
constexpr Bitvector NPC_MOVESWIM = 1 << 14;
constexpr Bitvector NPC_MOVERUN = 1 << 15;
constexpr Bitvector NPC_AIRCREATURE = 1 << 20;
constexpr Bitvector NPC_WATERCREATURE = 1 << 21;
constexpr Bitvector NPC_EARTHCREATURE = 1 << 22;
constexpr Bitvector NPC_FIRECREATURE = 1 << 23;
constexpr Bitvector NPC_HELPED = 1 << 24;
constexpr Bitvector NPC_FREEDROP = 1 << 25;
constexpr Bitvector NPC_NOINGRDROP = 1 << 26;

constexpr Bitvector NPC_STEALING = kIntOne | (1 << 0);
constexpr Bitvector NPC_WIELDING = kIntOne | (1 << 1);
constexpr Bitvector NPC_ARMORING = kIntOne | (1 << 2);
constexpr Bitvector NPC_USELIGHT = kIntOne | (1 << 3);
constexpr Bitvector NPC_NOTAKEITEMS = kIntOne | (1 << 4);

/*
 * ========================================================================================
 *  								Room's constants
 * ========================================================================================
 */

extern std::unordered_map<int, std::string> SECTOR_TYPE_BY_VALUE;

/**
 * The cardinal directions: used as index to room_data.dir_option[]
 */
const __uint8_t kDirNorth = 0;
const __uint8_t kDirEast = 1;
const __uint8_t kDirSouth = 2;
const __uint8_t kDirWest = 3;
const __uint8_t kDirUp = 4;
const __uint8_t kDirDown = 5;
const __uint8_t kDirMaxNumber = 6;        // number of directions in a room (nsewud) //

/**
 * Room flags: used in room_data.room_flags
 * WARNING: In the world files, NEVER set the bits marked "R" ("Reserved")
 */
constexpr Bitvector ROOM_DARK = 1 << 0;
constexpr Bitvector ROOM_DEATH =  1 << 1;    // Death trap      //
constexpr Bitvector ROOM_NOMOB = 1 << 2;
constexpr Bitvector ROOM_INDOORS = 1 << 3;
constexpr Bitvector ROOM_PEACEFUL = 1 << 4;
constexpr Bitvector ROOM_SOUNDPROOF = 1 << 5;
constexpr Bitvector ROOM_NOTRACK = 1 << 6;
constexpr Bitvector ROOM_NOMAGIC = 1 << 7;
constexpr Bitvector ROOM_TUNNEL = 1 << 8;
constexpr Bitvector ROOM_NOTELEPORTIN = 1 << 9;
constexpr Bitvector ROOM_GODROOM = 1 << 10;    // kLevelGod+ only allowed //
constexpr Bitvector ROOM_HOUSE = 1 << 11;    // (R) Room is a house  //
constexpr Bitvector ROOM_HOUSE_CRASH = 1 << 12;    // (R) House needs saving   //
constexpr Bitvector ROOM_ATRIUM = 1 << 13;    // (R) The door to a house //
constexpr Bitvector ROOM_OLC = 1 << 14;    // (R) Modifyable/!compress   //
constexpr Bitvector ROOM_BFS_MARK = 1 << 15;    // (R) breath-first srch mrk   //
constexpr Bitvector ROOM_MAGE = 1 << 16;
constexpr Bitvector ROOM_CLERIC = 1 << 17;
constexpr Bitvector ROOM_THIEF = 1 << 18;
constexpr Bitvector ROOM_WARRIOR = 1 << 19;
constexpr Bitvector ROOM_ASSASINE = 1 << 20;
constexpr Bitvector ROOM_GUARD = 1 << 21;
constexpr Bitvector ROOM_PALADINE = 1 << 22;
constexpr Bitvector ROOM_RANGER = 1 << 23;
constexpr Bitvector ROOM_POLY = 1 << 24;
constexpr Bitvector ROOM_MONO = 1 << 25;
constexpr Bitvector ROOM_SMITH = 1 << 26;
constexpr Bitvector ROOM_MERCHANT = 1 << 27;
constexpr Bitvector ROOM_DRUID = 1 << 28;
constexpr Bitvector ROOM_ARENA = 1 << 29;

constexpr Bitvector ROOM_NOSUMMON = kIntOne | (1 << 0);
constexpr Bitvector ROOM_NOTELEPORTOUT = kIntOne | (1 << 1);
constexpr Bitvector ROOM_NOHORSE = kIntOne | (1 << 2);
constexpr Bitvector ROOM_NOWEATHER = kIntOne | (1 << 3);
constexpr Bitvector ROOM_SLOWDEATH = kIntOne | (1 << 4);
constexpr Bitvector ROOM_ICEDEATH = kIntOne | (1 << 5);
constexpr Bitvector ROOM_NORELOCATEIN = kIntOne | (1 << 6);
constexpr Bitvector ROOM_ARENARECV = kIntOne | (1 << 7);  // комната в которой слышно сообщения арены
constexpr Bitvector ROOM_ARENASEND = kIntOne | (1 << 8);   // комната из которой отправляются сообщения арены
constexpr Bitvector ROOM_NOBATTLE = kIntOne | (1 << 9);
constexpr Bitvector ROOM_QUEST = kIntOne | (1 << 10);
constexpr Bitvector ROOM_LIGHT = kIntOne | (1 << 11);
constexpr Bitvector ROOM_NOMAPPER = kIntOne | (1 << 12);  //нет внумов комнат


constexpr Bitvector ROOM_NOITEM = kIntTwo | (1 << 0);    // Передача вещей в комнате запрещена
constexpr Bitvector ROOM_ARENA_DOMINATION = kIntTwo | (1 << 1); // комната арены доминирования

/**
 * Exit info: used in room_data.dir_option.exit_info
 */
constexpr Bitvector EX_ISDOOR = 1 << 0;    	// Exit is a door     //
constexpr Bitvector EX_CLOSED = 1 << 1;   	// The door is closed //
constexpr Bitvector EX_LOCKED = 1 << 2; 	   	// The door is locked //
constexpr Bitvector EX_PICKPROOF = 1 << 3;    // Lock can't be picked  //
constexpr Bitvector EX_HIDDEN = 1 << 4;
constexpr Bitvector EX_BROKEN = 1 << 5; 		//Polud замок двери сломан
constexpr Bitvector EX_DUNGEON_ENTRY = 1 << 6;    // When character goes through this door then he will get into a copy of the zone behind the door.

/**
 * Sector types: used in room_data.sector_type
 */
const __uint8_t kSectInside = 0;
const __uint8_t kSectCity = 1;
const __uint8_t kSectField = 2;
const __uint8_t kSectForest = 3;
const __uint8_t kSectHills = 4;
const __uint8_t kSectMountain = 5;
const __uint8_t kSectWaterSwim = 6;		// Swimmable water      //
const __uint8_t kSectWaterNoswim = 7;	// Water - need a boat  //
const __uint8_t kSectOnlyFlying = 8;	// Wheee!         //
const __uint8_t kSectUnderwater = 9;
const __uint8_t kSectSecret = 10;
const __uint8_t kSectStoneroad = 11;
const __uint8_t kSectRoad = 12;
const __uint8_t kSectWildroad = 13;
// надо не забывать менять NUM_ROOM_SECTORS в olc.h
// Values for weather changes //
const __uint8_t kSectFieldSnow = 20;
const __uint8_t kSectFieldRain = 21;
const __uint8_t kSectForestSnow = 22;
const __uint8_t kSectForestRain = 23;
const __uint8_t kSectHillsSnow = 24;
const __uint8_t kSectHillsRain = 25;
const __uint8_t kSectMountainSnow = 26;
const __uint8_t kSectThinIce = 27;
const __uint8_t kSectNormalIce = 28;
const __uint8_t kSectThickIce = 29;

/*
 * ========================================================================================
 *  								Object's constants
 * ========================================================================================
 */

/**
 * Magic books types.
 */
const __uint8_t BOOK_SPELL = 0;    // Книга заклинания //
const __uint8_t BOOK_SKILL = 1;    // Книга умения //
const __uint8_t BOOK_UPGRD = 2;    // Увеличение умения //
const __uint8_t BOOK_RECPT = 3;    // Книга рецепта //
const __uint8_t BOOK_FEAT = 4;        // Книга способности (feats) //

/**
 * Take/Wear flags: used by obj_data.obj_flags.wear_flags
 */
enum class EWearFlag : Bitvector {
	kUndefined = 0,    // Special value
	kTake = 1 << 0,    // Item can be takes      //
	kFinger = 1 << 1,
	kNeck = 1 << 2,
	kBody = 1 << 3,
	kHead = 1 << 4,
	kLegs = 1 << 5,
	kFeet = 1 << 6,
	kHands = 1 << 7,
	kArms = 1 << 8,
	kShield = 1 << 9,
	kShoulders = 1 << 10,
	kWaist = 1 << 11,
	kWrist = 1 << 12,
	kWield = 1 << 13,
	kHold = 1 << 14,
	kBoth = 1 << 15,
	kQuiver = 1 << 16
};

template<>
const std::string &NAME_BY_ITEM<EWearFlag>(EWearFlag item);
template<>
EWearFlag ITEM_BY_NAME<EWearFlag>(const std::string &name);

/**
 * Extra object flags: used by obj_data.obj_flags.extra_flags
 */
enum class EObjFlag : Bitvector {
	kGlow = 1 << 0,
	kHum = 1 << 1,
	kNorent = 1 << 2,
	kNodonate = 1 << 3,
	kNoinvis = 1 << 4,
	kInvisible = 1 << 5,
	kMagic = 1 << 6,
	kNodrop = 1 << 7,
	kBless = 1 << 8,
	kNosell = 1 << 9,
	kDecay = 1 << 10,
	kZonedacay = 1 << 11,
	kNodisarm = 1 << 12,
	kNodecay = 1 << 13,
	kPoisoned = 1 << 14,
	kSharpen = 1 << 15,
	kArmored = 1 << 16,
	kAppearsDay = 1 << 17,
	kAppearsNight = 1 << 18,
	kAppearsFullmoon = 1 << 19,
	kAppearsWinter = 1 << 20,
	kAppearsSpring = 1 << 21,
	kAppearsSummer = 1 << 22,
	kAppearsAutumn = 1 << 23,
	kSwimming = 1 << 24,
	kFlying = 1 << 25,
	kThrowing = 1 << 26,
	kTicktimer = 1 << 27,
	kFire = 1 << 28,									//< ...горит
	kRepopDecay = 1 << 29,
	kNolocate = kIntOne | (1 << 0),
	kTimedLvl = kIntOne | (1 << 1),						//< для маг.предметов уровень уменьшается со временем
	kNoalter = kIntOne | (1 << 2),						//< свойства предмета не могут быть изменены магией
	kHasOneSlot = kIntOne | (1 << 3),
	kHasTwoSlots = kIntOne | (1 << 4),
	kHasThreeSlots = kIntOne | (1 << 5),
	KSetItem = kIntOne | (1 << 6),
	KNofail = kIntOne | (1 << 7),						//< не фейлится при изучении (в случае книги)
	kNamed = kIntOne | (1 << 8),
	kBloody = kIntOne | (1 << 9),
	k1inlaid = kIntOne | (1 << 10),						//< TODO: не используется, см convert_obj_values()
	k2inlaid = kIntOne | (1 << 11),
	k3inlaid = kIntOne | (1 << 12),
	kNopour = kIntOne | (1 << 13),						//< нельзя перелить
	kUnique = kIntOne | (1 << 14),						// объект уникальный, т.е. только один в экипировке
	kTransformed = kIntOne | (1 << 15),					// Наложено заклинание заколдовать оружие
	kFreeForUse = kIntOne | (1 << 16),					// пока свободно, можно использовать
	KLimitedTimer = kIntOne | (1 << 17),				// Не может быть нерушимой
	kBindOnPurchase = kIntOne | (1 << 18),				// станет именной при покупке в магазе
	kNotOneInClanChest = kIntOne | (1 << 19)			//1 штука из набора не лезет в хран
};

template<>
const std::string &NAME_BY_ITEM<EObjFlag>(EObjFlag item);
template<>
EObjFlag ITEM_BY_NAME<EObjFlag>(const std::string &name);

/**
 * Object no flags - who can't use this object.
 */
enum class ENoFlag : Bitvector {
	kMono = 1 << 0,
	kPoly = 1 << 1,
	kNeutral = 1 << 2,
	kMage = 1 << 3,
	kSorcerer = 1 << 4,
	kThief = 1 << 5,
	kWarrior = 1 << 6,
	kAssasine = 1 << 7,
	kGuard = 1 << 8,
	kPaladine = 1 << 9,
	kRanger = 1 << 10,
	kVigilant = 1 << 11,
	kMerchant = 1 << 12,
	kMagus = 1 << 13,
	kConjurer = 1 << 14,
	kCharmer = 1 << 15,
	kWIzard = 1 << 16,
	kNecromancer = 1 << 17,
	kFighter = 1 << 18,
	kKiller = kIntOne | 1 << 0,
	kColored = kIntOne | 1 << 1,    // нельзя цветным //
	kBattle = kIntOne | 1 << 2,
	kMale = kIntTwo | 1 << 6,
	kFemale = kIntTwo | 1 << 7,
	kCharmice = kIntTwo | 1 << 8,
	kFree1 = kIntTwo | 1 << 9,
	kFree2 = kIntTwo | 1 << 10,
	kFree3 = kIntTwo | 1 << 11,
	kFree4 = kIntTwo | 1 << 12,
	kFree5 = kIntTwo | 1 << 13,
	kFree6 = kIntTwo | 1 << 14,
	kFree7 = kIntTwo | 1 << 15,
	kFree8 = kIntTwo | 1 << 16,
	kFree9 = kIntTwo | 1 << 17,
	kFree10 = kIntTwo | 1 << 18,
	kFree11 = kIntTwo | 1 << 19,
	kFree12 = kIntTwo | 1 << 20,
	kFree13 = kIntThree | 1 << 0,
	kFree14 = kIntThree | 1 << 1,
	kFree15 = kIntThree | 1 << 2,
	kNoPkClan = kIntThree | (1 << 3)      // не может быть взята !пк кланом
};

template<>
const std::string &NAME_BY_ITEM<ENoFlag>(ENoFlag item);
template<>
ENoFlag ITEM_BY_NAME<ENoFlag>(const std::string &name);

/**
 * Object anti flags - who can't take or use this object.
 */
enum class EAntiFlag : Bitvector {
	ITEM_AN_MONO = 1 << 0,
	ITEM_AN_POLY = 1 << 1,
	ITEM_AN_NEUTRAL = 1 << 2,
	ITEM_AN_MAGIC_USER = 1 << 3,
	ITEM_AN_CLERIC = 1 << 4,
	ITEM_AN_THIEF = 1 << 5,
	ITEM_AN_WARRIOR = 1 << 6,
	ITEM_AN_ASSASINE = 1 << 7,
	ITEM_AN_GUARD = 1 << 8,
	ITEM_AN_PALADINE = 1 << 9,
	ITEM_AN_RANGER = 1 << 10,
	ITEM_AN_SMITH = 1 << 11,
	ITEM_AN_MERCHANT = 1 << 12,
	ITEM_AN_DRUID = 1 << 13,
	ITEM_AN_BATTLEMAGE = 1 << 14,
	ITEM_AN_CHARMMAGE = 1 << 15,
	ITEM_AN_DEFENDERMAGE = 1 << 16,
	ITEM_AN_NECROMANCER = 1 << 17,
	ITEM_AN_FIGHTER_USER = 1 << 18,
	ITEM_AN_KILLER = kIntOne | (1 << 0),
	ITEM_AN_COLORED = kIntOne | (1 << 1),    // нельзя цветным //
	ITEM_AN_BD = kIntOne | (1 << 2),
	ITEM_AN_SEVERANE = kIntTwo | 1 << 0,  // недоступность по родам
	ITEM_AN_POLANE = kIntTwo | 1 << 1,
	ITEM_AN_KRIVICHI = kIntTwo | 1 << 2,
	ITEM_AN_VATICHI = kIntTwo | 1 << 3,
	ITEM_AN_VELANE = kIntTwo | 1 << 4,
	ITEM_AN_DREVLANE = kIntTwo | 1 << 5,
	ITEM_AN_MALE = kIntTwo | 1 << 6,
	ITEM_AN_FEMALE = kIntTwo | 1 << 7,
	ITEM_AN_CHARMICE = kIntTwo | 1 << 8,
	ITEM_AN_POLOVCI = kIntTwo | 1 << 9,
	ITEM_AN_PECHENEGI = kIntTwo | 1 << 10,
	ITEM_AN_MONGOLI = kIntTwo | 1 << 11,
	ITEM_AN_YIGURI = kIntTwo | 1 << 12,
	ITEM_AN_KANGARI = kIntTwo | 1 << 13,
	ITEM_AN_XAZARI = kIntTwo | 1 << 14,
	ITEM_AN_SVEI = kIntTwo | 1 << 15,
	ITEM_AN_DATCHANE = kIntTwo | 1 << 16,
	ITEM_AN_GETTI = kIntTwo | 1 << 17,
	ITEM_AN_UTTI = kIntTwo | 1 << 18,
	ITEM_AN_XALEIGI = kIntTwo | 1 << 19,
	ITEM_AN_NORVEZCI = kIntTwo | 1 << 20,
	ITEM_AN_RUSICHI = kIntThree | 1 << 0,
	ITEM_AN_STEPNYAKI = kIntThree | 1 << 1,
	ITEM_AN_VIKINGI = kIntThree | 1 << 2,
	kNoPkClan = kIntThree | (1 << 3)      // не может быть взята !пк кланом
};

template<>
const std::string &NAME_BY_ITEM<EAntiFlag>(EAntiFlag item);
template<>
EAntiFlag ITEM_BY_NAME<EAntiFlag>(const std::string &name);

/**
 * Container flags - value[1]
 */
constexpr Bitvector CONT_CLOSEABLE = 1 << 0;    // Container can be closed //
constexpr Bitvector CONT_PICKPROOF = 1 << 1;    // Container is pickproof  //
constexpr Bitvector CONT_CLOSED = 1 << 2;        // Container is closed     //
constexpr Bitvector CONT_LOCKED = 1 << 3;        // Container is locked     //
constexpr Bitvector CONT_BROKEN = 1 << 4;        // Container is locked     //

#endif //BYLINS_SRC_ENTITY_ROOMS_ROOM_CONSTANTS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :