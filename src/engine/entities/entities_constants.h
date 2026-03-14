/**
 \authors Created by Sventovit
 \date 13.01.2022.
 \brief Константы для персонажей, комнат и предметов..
 \details Тут должны располагаться исключительно константы и минимум кода.
*/

#ifndef BYLINS_SRC_ENTITY_ROOMS_ROOM_CONSTANTS_H_
#define BYLINS_SRC_ENTITY_ROOMS_ROOM_CONSTANTS_H_

#include "engine/structs/structs.h"
#include "engine/structs/meta_enum.h"

/*
 * ========================================================================================
 *  								General entities constants
 * ========================================================================================
 */

enum class EGender : byte {
	kNeutral = 0,
	kMale = 1,
	kFemale = 2,
	kPoly = 3,
	kLast
};

template<>
EGender ITEM_BY_NAME<EGender>(const std::string &name);
template<>
const std::string &NAME_BY_ITEM(EGender item);

/*
 * ========================================================================================
 *  								Mutual PC-NPC constants
 * ========================================================================================
 */

/**
 * Character positions.
 */
enum class EPosition {
	kUndefined = -1, // Это неправильно, но в классах Hit и Damage есть позиция -1, надо переделывать.
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

template<>
const std::string &NAME_BY_ITEM<EPosition>(EPosition item);
template<>
EPosition ITEM_BY_NAME<EPosition>(const std::string &name);

/**
 * Character base stats ids.
 */
enum class EBaseStat : int {
	kStr = 0,
	kDex = 1,
	kCon = 2,
	kWis = 3,
	kInt = 4,
	kCha = 5,
	kFirst = kStr,
	kLast = kCha
};

EBaseStat& operator++(EBaseStat &s);

template<>
const std::string &NAME_BY_ITEM<EBaseStat>(EBaseStat item);
template<>
EBaseStat ITEM_BY_NAME<EBaseStat>(const std::string &name);

const int kDefaultBaseStatMin{10};
const int kDefaultBaseStatMax{25};
const int kDefaultBaseStatAutoGen{12};
const int kDefaultBaseStatCap{50};
const int kMobBaseStatCap{100};
const int kLeastBaseStat{1};

/*
 * Character savings ids.
 */
enum class ESaving : int {
	kWill = 0,
	kCritical = 1,
	kStability = 2,
	kReflex = 3,
	kNone = 4,
	kFirst = kWill,
	kLast = kReflex, // Не забываем менять при добаввлении новых элементов.
};

ESaving& operator++(ESaving &s);

template<>
const std::string &NAME_BY_ITEM<ESaving>(ESaving item);
template<>
ESaving ITEM_BY_NAME<ESaving>(const std::string &name);

/**
 * Magic damage resistance types.
 */
enum EResist {
	kFire = 0,
	kAir,
	kWater,
	kEarth,
	kVitality,
	kMind,
	kImmunity,
	kDark,
	kFirstResist = kFire,
	kLastResist = kDark
};

EResist& operator++(EResist &r);

template<>
const std::string &NAME_BY_ITEM<EResist>(EResist item);
template<>
EResist ITEM_BY_NAME<EResist>(const std::string &name);

const int kMaxPcResist = 75;

enum class EWhereObj : int {
	kNowhere = 0,
	kCharEquip,
	kCharInventory,
	kRoom,
	kBag,
	kClanChest,
	kDepotChest,
	kExchange,
	kPostal,
	kSeller,
};

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
	kFirstEquipPos = kLight,
	kNumEquipPos = 20    // This must be the # of eq positions!!
};

// Экстра флаги персонажа
// \TODO Переделать на enum или удалить
constexpr Bitvector EXTRA_FAILHIDE = 1 << 0;
constexpr Bitvector EXTRA_FAILSNEAK = 1 << 1;
constexpr Bitvector EXTRA_FAILCAMOUFLAGE = 1 << 2;
constexpr Bitvector EXTRA_GRP_KILL_COUNT = 1 << 3; // для избежания повторных записей моба в списки SetsDrop

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
	kPerformPowerAttack = kIntOne | 1 << 12,            // мощная атака //
	kPerformGreatPowerAttack = kIntOne | 1 << 13,    // улучшеная мощная атака //
	kPerformAimingAttack = kIntOne | 1 << 14,        // прицельная атака //
	kPerformGreatAimingAttack = kIntOne | 1 << 15,    // улучшеная прицельная атака //
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
	kTelegram = kIntTwo | 1 << 18,           // Активирует телеграм-канал у персонажа
	kPerformSerratedBlade = kIntTwo | 1 << 19 // Активирована "воровская заточка".
};

// при добавлении не забываем про preference_bits[]

/**
 * PC religions
 * \todo Все, связанное с религиями, нужно вынести в отдельный модуль.
 */
const uint8_t kReligionPoly = 0;
const uint8_t kReligionMono = 1;

typedef std::array<const char *, static_cast<std::size_t>(EGender::kLast)> religion_genders_t;
typedef std::array<religion_genders_t, 3> religion_names_t;
extern const religion_names_t religion_name;

/**
 *	Player flags: used by char_data.char_specials.act
 */
 enum EPlrFlag : Bitvector {
	kKiller = 1 << 0,            // Player is a player-killer     //
	kBurglar = 1 << 1,            // Player is a player-thief. Назван так, потому что конфликтует с константой класса //
	kFrozen = 1 << 2,            // Player is frozen        //
	kDontSet = 1 << 3,            // Don't EVER set (ISNPC bit)  //
	kWriting = 1 << 4,            // Player writing (board/mail/olc)  //
	kMailing = 1 << 5,            // Player is writing mail     //
	kCrashSave = 1 << 6,            // Player needs to be crash-saved   //
	kSiteOk = 1 << 7,            // Player has been site-cleared  //
	kMuted = 1 << 8,            // Player not allowed to shout/goss/auct  //
	kNoTitle = 1 << 9,            // Player not allowed to set title  //
	kDeleted = 1 << 10,        // Player deleted - space reusable  //
	kLoadroom = 1 << 11,        // Player uses nonstandard loadroom  (не используется) //
	kAutobot = 1 << 12,        // Player автоматический игрок //
	kNoDelete = 1 << 13,        // Player shouldn't be deleted //
	kInvStart = 1 << 14,        // Player should enter game wizinvis //
	kCryo = 1 << 15,            // Player is cryo-saved (purge prog)   //
	kHelled = 1 << 16,            // Player is in Hell //
	kNameDenied = 1 << 17,            // Player is in Names Room //
	kRegistred = 1 << 18,
	kDumbed = 1 << 19,            // Player is not allowed to tell/emote/social //
	kScriptWriter = 1 << 20,   // скриптер
	kSpamer = 1 << 21,        // спаммер
// свободно
	kDelete = 1 << 28,            // RESERVED - ONLY INTERNALLY (MOB_DELETE) //
	kFree = 1 << 29,            // RESERVED - ONLY INTERBALLY (MOB_FREE)//
};

/**
 *	Gods flags.
 */
enum EGf : Bitvector {
	kGodsLike = 1 << 0,
	kGodscurse = 1 << 1,
	kHighgod = 1 << 2,
	kRemort = 1 << 3,
	kDemigod = 1 << 4,    // Морталы с привилегиями богов //
	kPerslog = 1 << 5,
	kAllowTesterMode = 1 << 6,
	kSkillTester = 1 << 7
};

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
const int kNpcBoss = 200;
const int kNpcUnique = 201;

/**
 * Mobile flags: used by char_data.char_specials.act
 */
enum EMobFlag : Bitvector {
	kSpec = 1 << 0,            // Mob has a callable spec-proc  //
	kSentinel = 1 << 1,        // Mob should not move     //
	kScavenger = 1 << 2,    // Mob picks up stuff on the ground //
	kNpc = 1 << 3,            // (R) Automatically set on all Mobs   //
	kAware = 1 << 4,            // Mob can't be backstabbed      //
	kAgressive = 1 << 5,    // Mob hits players in the room  //
	kStayZone = 1 << 6,    // Mob shouldn't wander out of zone //
	kWimpy = 1 << 7,            // Mob flees if severely injured //
	kAgressiveDay = 1 << 8,        // //
	kAggressiveNight = 1 << 9,    // //
	kAgressiveFullmoon = 1 << 10,  // //
	kMemory = 1 << 11,            // remember attackers if attacked   //
	kHelper = 1 << 12,            // attack PCs fighting other NPCs   //
	kNoCharm = 1 << 13,        // Mob can't be charmed    //
	kNoSummon = 1 << 14,        // Mob can't be summoned      //
	kNoSleep = 1 << 15,        // Mob can't be slept      //
	kNoBash = 1 << 16,            // Mob can't be bashed (e.g. trees) //
	kNoBlind = 1 << 17,        // Mob can't be blinded    //
	kMounting = 1 << 18,
	kNoHold = 1 << 19,
	kNoSilence = 1 << 20,
	kAgressiveMono = 1 << 21,
	kAgressivePoly = 1 << 22,
	kNoFear = 1 << 23,
	kNoGroup = 1 << 24,
	kCorpse = 1 << 25,
	kLooter = 1 << 26,
	kProtect = 1 << 27,
	kMobDeleted = 1 << 28,            // RESERVED - ONLY INTERNALLY //
	kMobFreed = 1 << 29,            // RESERVED - ONLY INTERBALLY //

	kSwimming = kIntOne | (1 << 0),
	kFlying = kIntOne | (1 << 1),
	kOnlySwimming = kIntOne | (1 << 2),
	kAgressiveWinter = kIntOne | (1 << 3),
	kAgressiveSpring = kIntOne | (1 << 4),
	kAgressiveSummer = kIntOne | (1 << 5),
	kAgressiveAutumn = kIntOne | (1 << 6),
	kAppearsDay = kIntOne | (1 << 7),
	kAppearsNight = kIntOne | (1 << 8),
	kAppearsFullmoon = kIntOne | (1 << 9),
	kAppearsWinter = kIntOne | (1 << 10),
	kAppearsSpring = kIntOne | (1 << 11),
	kAppearsSummer = kIntOne | (1 << 12),
	kAppearsAutumn = kIntOne | (1 << 13),
	kNoFight = kIntOne | (1 << 14),
	kDecreaseAttack = kIntOne | (1 << 15), // понижает количество своих атак по мере убывания тек.хп
	kHorde = kIntOne | (1 << 16),
	kClone = kIntOne | (1 << 17),
	kNotKillPunctual = kIntOne | (1 << 18),
	kNoUndercut = kIntOne | (1 << 19),
	kTutelar = kIntOne | (1 << 20),	// ангел-хранитель
	kCityGuardian = kIntOne | (1 << 21), //моб-стражник, ставится программно из файла guards.xml
	kIgnoreForbidden = kIntOne | (1 << 22), // игнорирует печать
	kNoBattleExp = kIntOne | (1 << 23), // не дает экспу за удары
	kNoHammer = kIntOne | (1 << 24), // нельзя оглушить богатырским молотом
	kMentalShadow = kIntOne | (1 << 25), // Используется для ментальной тени
	kSummoned = kIntOne | (1 << 26), // (ангел, тень, храны, трупы, умки)

	kFireBreath = kIntTwo | (1 << 0),
	kGasBreath = kIntTwo | (1 << 1),
	kFrostBreath = kIntTwo | (1 << 2),
	kAcidBreath = kIntTwo | (1 << 3),
	kLightingBreath = kIntTwo | (1 << 4),
	kNoSkillTrain = kIntTwo | (1 << 5),
	kNoRest = kIntTwo | (1 << 6),
	kAreaAttack = kIntTwo | (1 << 7),
	kNoOverwhelm = kIntTwo | (1 << 8),
	kNoHelp = kIntTwo | (1 << 9),
	kOpensDoor = kIntTwo | (1 << 10),
	kIgnoresNoMob = kIntTwo | (1 << 11),
	kIgnoresPeaceRoom = kIntTwo | (1 << 12),
	kResurrected = kIntTwo | (1 << 13), // !поднять труп! или !оживить труп! только програмно//
// свободно
	kNoResurrection = kIntTwo | (1 << 20),
	kMobAwake = kIntTwo | (1 << 21),
	kIgnoresFormation = kIntTwo | (1 << 22)
};

/**
 * NPC's flags used by CharData.mob_specials.npc_flags
 */
enum ENpcFlag : Bitvector {
	kBlockNorth = 1 << 0,
	kBlockEast = 1 << 1,
	kBlockSouth = 1 << 2,
	kBlockWest = 1 << 3,
	kBlockUp = 1 << 4,
	kBlockDown = 1 << 5,
	kToxic = 1 << 6,
	kInvis = 1 << 7,
	kSneaking = 1 << 8,
	kDisguising = 1 << 9,
	kMoveFly = 1 << 11,
	kMoveCreep = 1 << 12,
	kMoveJump = 1 << 13,
	kMoveSwim = 1 << 14,
	kMoveRun = 1 << 15,
	kAirCreature = 1 << 20,
	kWaterCreature = 1 << 21,
	kEarthCreature = 1 << 22,
	kFireCreature = 1 << 23,
	kHelped = 1 << 24,
	kFreeDrop = 1 << 25,
	kNoIngrDrop = 1 << 26,
	kNoMercList = 1 << 27,
	kStealing = kIntOne | (1 << 0),
	kWielding = kIntOne | (1 << 1),
	kArmoring = kIntOne | (1 << 2),
	kUsingLight = kIntOne | (1 << 3),
	kNoTakeItems = kIntOne | (1 << 4),
	kIgnoreRareKill = kIntOne | (1 << 5)
};

/*
 * ========================================================================================
 *  								Room's constants
 * ========================================================================================
 */

extern std::unordered_map<int, std::string> SECTOR_TYPE_BY_VALUE;

/**
 * The cardinal directions: used as index to room_data.dir_option[]
 */
enum EDirection : int {
	kUndefinedDir = -1,
	kNorth = 0,
	kEast = 1,
	kSouth = 2,
	kWest = 3,
	kUp = 4,
	kDown = 5,
	kFirstDir = kNorth,
	kLastDir = kDown,
	kMaxDirNum = 6        // number of directions in a room (nsewud) //
};

EDirection &operator++(EDirection &d);

/**
 * Room flags: used in room_data.room_flags
 * WARNING: In the world files, NEVER set the bits marked "R" ("Reserved")
 */
 enum ERoomFlag : Bitvector {
	kDarked = 1 << 0,
	kDeathTrap =  1 << 1,    // Death trap      //
	kNoEntryMob = 1 << 2,
	kIndoors = 1 << 3,
	kPeaceful = 1 << 4,
	kSoundproof = 1 << 5,
	kNoTrack = 1 << 6,
	kNoMagic = 1 << 7,
	kTunnel = 1 << 8,
	kNoTeleportIn = 1 << 9,
	kGodsRoom = 1 << 10,    // kLevelGod+ only allowed //
	kHouse = 1 << 11,    // (R) Room is a house  //
	kHouseCrash = 1 << 12,    // (R) House needs saving   //
	kHouseEntry = 1 << 13,    // (R) The door to a house //
	//kOlc = 1 << 14,    // Не используется //
	kBfsMark = 1 << 15,    // (R) breath-first srch mrk   //
	kForMages = 1 << 16,
	kForSorcerers = 1 << 17,
	kForThieves = 1 << 18,
	kForWarriors = 1 << 19,
	kForAssasines = 1 << 20,
	kForGuards = 1 << 21,
	kForPaladines = 1 << 22,
	kForRangers = 1 << 23,
	kForPoly = 1 << 24,
	kForMono = 1 << 25,
	kForge = 1 << 26,
	kForMerchants = 1 << 27,
	kForMaguses = 1 << 28,
	kArena = 1 << 29,

	kNoSummonOut = kIntOne | (1 << 0),
	kNoTeleportOut = kIntOne | (1 << 1),
	kNohorse = kIntOne | (1 << 2),
	kNoWeather = kIntOne | (1 << 3),
	kSlowDeathTrap = kIntOne | (1 << 4),
	kIceTrap = kIntOne | (1 << 5),
	kNoRelocateIn = kIntOne | (1 << 6),
	kTribune = kIntOne | (1 << 7),  // комната в которой слышно сообщения арены
	kArenaSend = kIntOne | (1 << 8),   // комната из которой отправляются сообщения арены
	kNoBattle = kIntOne | (1 << 9),
	//ROOM_QUEST = kIntOne | (1 << 10),	// не используется //
	kAlwaysLit = kIntOne | (1 << 11),
	kMoMapper = kIntOne | (1 << 12),  //нет внумов комнат

	kNoItem = kIntTwo | (1 << 0),    // Передача вещей в комнате запрещена
	kDominationArena = kIntTwo | (1 << 1) // комната арены доминирования
 };

/**
 * Exit info: used in room_data.dir_option.exit_info
 */
 enum EExitFlag : Bitvector {
	kHasDoor = 1 << 0,		// Exit has a door     //
	kClosed = 1 << 1,
	kLocked = 1 << 2,
	kPickroof = 1 << 3,		// Lock can't be picked  //
	kHidden = 1 << 4,
	kBrokenLock = 1 << 5,	// замок двери сломан
	kDungeonEntry = 1 << 6	// When character goes through this door then he will get into a copy of the zone behind the door.
 };

/**
 * Sector types: used in room_data.sector_type
 */
 enum ESector : int {
	kInside = 0,
	kCity = 1,
	kField = 2,
	kForest = 3,
	kHills = 4,
	kMountain = 5,
	kWaterSwim = 6,		// Swimmable water      //
	kWaterNoswim = 7,	// Water - need a boat  //
	kOnlyFlying = 8,	// Wheee!         //
	kUnderwater = 9,
	kSecret = 10,
	kStoneroad = 11,
	kRoad = 12,
	kWildroad = 13,
// надо не забывать менять NUM_ROOM_SECTORS в olc.h
// Values for weather changes //
	kFieldSnow = 20,
	kFieldRain = 21,
	kForestSnow = 22,
	kForestRain = 23,
	kHillsSnow = 24,
	kHillsRain = 25,
	kMountainSnow = 26,
	kThinIce = 27,
	kNormalIce = 28,
	kThickIce = 29
 };

/*
 * ========================================================================================
 *  								Object's constants
 * ========================================================================================
 */

/**
 * Object types.
 */
enum EObjType {
	kItemUndefined = 0,
	kLightSource = 1,		// Item is a light source  //
	kScroll = 2,			// Item is a scroll     //
	kWand = 3,				// Item is a wand    //
	kStaff = 4,				// Item is a staff      //
	kWeapon = 5,			// Item is a weapon     //
	kElementWeapon = 6,		// Unimplemented     //
	kMissile = 7,			// Unimplemented     //
	kTreasure = 8,			// Item is a treasure, not gold  //
	kArmor = 9,				// Item is armor     //
	kPotion = 10,			// Item is a potion     //
	kWorm = 11,				// Unimplemented     //
	kOther = 12,			// Misc object       //
	kTrash = 13,			// Trash - shopkeeps won't buy   //
	kTrap = 14,				// Unimplemented     //
	kContainer = 15,		// Item is a container     //
	kNote = 16,				// Item is note      //
	kLiquidContainer = 17,	// Item is a drink container  //
	kKey = 18,				// Item is a key     //
	kFood = 19,				// Item is food         //
	kMoney = 20,			// Item is money (gold)    //
	kPen = 21,				// Item is a pen     //
	kBoat = 22,				// Item is a boat    //
	kFountain = 23,			// Item is a fountain      //
	kBook = 24,				// Item is book //
	kIngredient = 25,		// Item is magical ingradient //
	kMagicIngredient = 26,	// Магический ингредиент //
	kCraftMaterial = 27,	// Материал для крафтовых умений //
	kBandage = 28,			// бинты для перевязки
	kLightArmor = 29,		// легкий тип брони
	kMediumArmor = 30,		// средний тип брони
	kHeavyArmor = 31,		// тяжелый тип брони
	kEnchant = 32,			// зачарование предмета
	kMagicMaterial = 33,	// Item is a material related to crafts system
	kMagicArrow = 34,		// Item is a material related to crafts system
	kMagicContaner = 35,	// Item is a material related to crafts system
	kCraftMaterial2 = 36,	// Item is a material related to crafts system
};

template<>
const std::string &NAME_BY_ITEM<EObjType>(const EObjType item);
template<>
EObjType ITEM_BY_NAME<EObjType>(const std::string &name);

enum EObjMaterial {
	kMaterialUndefined = 0,
	kBulat = 1,
	kBronze = 2,
	kIron = 3,
	kSteel = 4,
	kForgedSteel = 5,
	kPreciousMetel = 6,
	kCrystal = 7,
	kWood = 8,
	kHardWood = 9,
	kCeramic = 10,
	kGlass = 11,
	kStone = 12,
	kBone = 13,
	kCloth = 14,
	kSkin = 15,
	kOrganic = 16,
	kPaper = 17,
	kDiamond = 18
};

template<>
const std::string &NAME_BY_ITEM<EObjMaterial>(const EObjMaterial item);
template<>
EObjMaterial ITEM_BY_NAME<EObjMaterial>(const std::string &name);

/**
 * Magic books types.
 */
 enum EBook {
	kSpell = 0,
	kSkill = 1,
	kSkillUpgrade = 2,
	kReceipt = 3,
	kFeat = 4
};

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
	kZonedecay = 1 << 11,
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
	kQuestItem = kIntOne | (1 << 10),						//< TODO: не используется, см ConvertObjValues()
	k2inlaid = kIntOne | (1 << 11),
	k3inlaid = kIntOne | (1 << 12),
	kNopour = kIntOne | (1 << 13),						//< нельзя перелить
	kUnique = kIntOne | (1 << 14),						// объект уникальный, т.е. только один в экипировке
	kTransformed = kIntOne | (1 << 15),					// Наложено заклинание заколдовать оружие
	kNoRentTimer = kIntOne | (1 << 16),					// пока свободно, можно использовать
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
	kWizard = 1 << 16,
	kNecromancer = 1 << 17,
	kFighter = 1 << 18,
	kKiller = kIntOne | (1 << 0),
	kColored = kIntOne | (1 << 1),    // нельзя цветным //
	kBattle = kIntOne | (1 << 2),
	kFree1 = kIntTwo | 1 << 0,  // флаги удалены, можно (но не нужно) использовать
	kFree2 = kIntTwo | 1 << 1,
	kFree3 = kIntTwo | 1 << 2,
	kFree4 = kIntTwo | 1 << 3,
	kFree5 = kIntTwo | 1 << 4,
	kFree6 = kIntTwo | 1 << 5,
	kMale = kIntTwo | 1 << 6,
	kFemale = kIntTwo | 1 << 7,
	kCharmice = kIntTwo | 1 << 8,
	kFree7 = kIntTwo | 1 << 9,
	kFree8 = kIntTwo | 1 << 10,
	kFree9 = kIntTwo | 1 << 11,
	kFree10 = kIntTwo | 1 << 12,
	kFree11 = kIntTwo | 1 << 13,
	kFree12 = kIntTwo | 1 << 14,
	kFree13 = kIntTwo | 1 << 15,
	kFree14 = kIntTwo | 1 << 16,
	kFree15 = kIntTwo | 1 << 17,
	kFree16 = kIntTwo | 1 << 18,
	kFree17 = kIntTwo | 1 << 19,
	kFree18 = kIntTwo | 1 << 20,
	kFree19 = kIntThree | 1 << 0,
	kFree20 = kIntThree | 1 << 1,
	kFree21 = kIntThree | 1 << 2,
	kNoPkClan = kIntThree | (1 << 3)      // не может быть взята !пк кланом
};

template<>
const std::string &NAME_BY_ITEM<EAntiFlag>(EAntiFlag item);
template<>
EAntiFlag ITEM_BY_NAME<EAntiFlag>(const std::string &name);

/**
 * Container flags - value[1]
 */
enum EContainerFlag {
	kCloseable = 1 << 0,
	kUncrackable = 1 << 1,
	kShutted = 1 << 2,
	kLockedUp = 1 << 3,
	kLockIsBroken = 1 << 4
};

#endif //BYLINS_SRC_ENTITY_ROOMS_ROOM_CONSTANTS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :