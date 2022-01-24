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
 *  General entities constants
 */

enum class ESex : byte {
	kNeutral = 0,
	kMale = 1,
	kFemale = 2,
	kPoly = 3,
	kLast
};

constexpr ESex kDefaultSex = ESex::kMale;

template<>
ESex ITEM_BY_NAME<ESex>(const std::string &name);
template<>
const std::string &NAME_BY_ITEM(ESex item);

/*
 *  Mutual PC-NPC constants
 */

// Positions

enum class EPosition {
	kIncorrect = -1, // Это неправильно, но за каким-то псом в классах Hit и Damage есть позиция -1, надо переделывать.
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

enum class ESaving : int {
	kWill = 0,
	kCritical = 1,
	kStability = 2,
	kReflex = 3,
	kFirst = kWill,
	kLast = kReflex, // Не забываем менять при добаввлении новых элементов.
};

ESaving& operator++(ESaving &s);

// Character equipment positions: used as index for char_data.equipment[] //
// NOTE: Don't confuse these constants with the ITEM_ bitvectors
//       which control the valid places you can wear a piece of equipment
const __uint8_t WEAR_LIGHT = 0;
const __uint8_t WEAR_FINGER_R = 1;
const __uint8_t WEAR_FINGER_L = 2;
const __uint8_t WEAR_NECK_1 = 3;
const __uint8_t WEAR_NECK_2 = 4;
const __uint8_t WEAR_BODY = 5;
const __uint8_t WEAR_HEAD = 6;
const __uint8_t WEAR_LEGS = 7;
const __uint8_t WEAR_FEET = 8;
const __uint8_t WEAR_HANDS = 9;
const __uint8_t WEAR_ARMS = 10;
const __uint8_t WEAR_SHIELD = 11;
const __uint8_t WEAR_ABOUT = 12;
const __uint8_t WEAR_WAIST = 13;
const __uint8_t WEAR_WRIST_R = 14;
const __uint8_t WEAR_WRIST_L = 15;
const __uint8_t WEAR_WIELD = 16;      // правая рука
const __uint8_t WEAR_HOLD = 17;      // левая рука
const __uint8_t WEAR_BOTHS = 18;      // обе руки
const __uint8_t WEAR_QUIVER = 19;      // под лук (колчан)
const __uint8_t NUM_WEARS = 20;    // This must be the # of eq positions!! //

// Preference flags: used by char_data.player_specials.pref //
constexpr bitvector_t PRF_BRIEF = 1 << 0;        // Room descs won't normally be shown //
constexpr bitvector_t PRF_COMPACT = 1 << 1;    // No extra CRLF pair before prompts  //
constexpr bitvector_t PRF_NOHOLLER = 1 << 2;    // Не слышит команду "орать"   //
constexpr bitvector_t PRF_NOTELL = 1 << 3;        // Не слышит команду "сказать" //
constexpr bitvector_t PRF_DISPHP = 1 << 4;        // Display hit points in prompt   //
constexpr bitvector_t PRF_DISPMANA = 1 << 5;    // Display mana points in prompt   //
constexpr bitvector_t PRF_DISPMOVE = 1 << 6;    // Display move points in prompt   //
constexpr bitvector_t PRF_AUTOEXIT = 1 << 7;    // Display exits in a room      //
constexpr bitvector_t PRF_NOHASSLE = 1 << 8;    // Aggr mobs won't attack    //
constexpr bitvector_t PRF_SUMMONABLE = 1 << 9;  // Can be summoned         //
constexpr bitvector_t PRF_QUEST = 1 << 10;        // On quest                       //
constexpr bitvector_t PRF_NOREPEAT = 1 << 11;   // No repetition of comm commands  //
constexpr bitvector_t PRF_HOLYLIGHT = 1 << 12;  // Can see in dark        //
constexpr bitvector_t PRF_COLOR_1 = 1 << 13;    // Color (low bit)       //
constexpr bitvector_t PRF_COLOR_2 = 1 << 14;    // Color (high bit)         //
constexpr bitvector_t PRF_NOWIZ = 1 << 15;        // Can't hear wizline       //
constexpr bitvector_t PRF_LOG1 = 1 << 16;        // On-line System Log (low bit)   //
constexpr bitvector_t PRF_LOG2 = 1 << 17;        // On-line System Log (high bit)  //
constexpr bitvector_t PRF_NOAUCT = 1 << 18;        // Can't hear auction channel     //
constexpr bitvector_t PRF_NOGOSS = 1 << 19;        // Не слышит команду "болтать" //
constexpr bitvector_t PRF_DISPFIGHT = 1 << 20;  // Видит свое состояние в бою      //
constexpr bitvector_t PRF_ROOMFLAGS = 1 << 21;  // Can see room flags (ROOM_x)  //
constexpr bitvector_t PRF_DISPEXP = 1 << 22;
constexpr bitvector_t PRF_DISPEXITS = 1 << 23;
constexpr bitvector_t PRF_DISPLEVEL = 1 << 24;
constexpr bitvector_t PRF_DISPGOLD = 1 << 25;
constexpr bitvector_t PRF_DISPTICK = 1 << 26;
constexpr bitvector_t PRF_PUNCTUAL = 1 << 27;
constexpr bitvector_t PRF_AWAKE = 1 << 28;
constexpr bitvector_t PRF_CODERINFO = 1 << 29;

constexpr bitvector_t PRF_AUTOMEM = kIntOne | 1 << 0;
constexpr bitvector_t PRF_NOSHOUT = kIntOne | 1 << 1;                // Не слышит команду "кричать"  //
constexpr bitvector_t PRF_GOAHEAD = kIntOne | 1 << 2;                // Добавление IAC GA после промпта //
constexpr bitvector_t PRF_SHOWGROUP = kIntOne | 1 << 3;            // Показ полного состава группы //
constexpr bitvector_t PRF_AUTOASSIST = kIntOne | 1 << 4;            // Автоматическое вступление в бой //
constexpr bitvector_t PRF_AUTOLOOT = kIntOne | 1 << 5;                // Autoloot //
constexpr bitvector_t PRF_AUTOSPLIT = kIntOne | 1 << 6;            // Autosplit //
constexpr bitvector_t PRF_AUTOMONEY = kIntOne | 1 << 7;            // Automoney //
constexpr bitvector_t PRF_NOARENA = kIntOne | 1 << 8;                // Не слышит арену //
constexpr bitvector_t PRF_NOEXCHANGE = kIntOne | 1 << 9;            // Не слышит базар //
constexpr bitvector_t PRF_NOCLONES = kIntOne | 1 << 10;            // Не видит в группе чужих клонов //
constexpr bitvector_t PRF_NOINVISTELL = kIntOne | 1 << 11;            // Не хочет, чтобы телял "кто-то" //
constexpr bitvector_t PRF_POWERATTACK = kIntOne | 1 << 12;            // мощная атака //
constexpr bitvector_t PRF_GREATPOWERATTACK = kIntOne | 1 << 13;    // улучшеная мощная атака //
constexpr bitvector_t PRF_AIMINGATTACK = kIntOne | 1 << 14;        // прицельная атака //
constexpr bitvector_t PRF_GREATAIMINGATTACK = kIntOne | 1 << 15;    // улучшеная прицельная атака //
constexpr bitvector_t PRF_NEWS_MODE = kIntOne | 1 << 16;            // вариант чтения новостей мада и дружины
constexpr bitvector_t PRF_BOARD_MODE = kIntOne | 1 << 17;            // уведомления о новых мессагах на досках
constexpr bitvector_t PRF_DECAY_MODE = kIntOne | 1 << 18;            // канал хранилища, рассыпание шмота
constexpr bitvector_t PRF_TAKE_MODE = kIntOne | 1 << 19;            // канал хранилища, положили/взяли
constexpr bitvector_t PRF_PKL_MODE = kIntOne | 1 << 20;            // уведомления о добавлении/убирании в пкл
constexpr bitvector_t PRF_POLIT_MODE = kIntOne | 1 << 21;            // уведомления об изменении политики, своей и чужой
constexpr bitvector_t PRF_IRON_WIND = kIntOne | 1 << 22;            // включен скилл "железный ветер"
constexpr bitvector_t PRF_PKFORMAT_MODE = kIntOne | 1 << 23;        // формат пкл/дрл
constexpr bitvector_t PRF_WORKMATE_MODE = kIntOne | 1 << 24;        // показ входов/выходов соклановцев
constexpr bitvector_t PRF_OFFTOP_MODE = kIntOne | 1 << 25;        // вкл/выкл канала оффтопа
constexpr bitvector_t PRF_ANTIDC_MODE = kIntOne | 1 << 26;        // режим защиты от дисконекта в бою
constexpr bitvector_t PRF_NOINGR_MODE = kIntOne | 1 << 27;        // не показывать продажу/покупку ингров в канале базара
constexpr bitvector_t PRF_NOINGR_LOOT = kIntOne | 1 << 28;        // не лутить ингры в режиме автограбежа
constexpr bitvector_t PRF_DISP_TIMED = kIntOne | 1 << 29;            // показ задержек для умений и способностей

constexpr bitvector_t PRF_IGVA_PRONA = kIntTwo | 1 << 0;            // для стоп-списка оффтоп
constexpr bitvector_t PRF_EXECUTOR = kIntTwo | 1 << 1;            // палач
constexpr bitvector_t PRF_DRAW_MAP = kIntTwo | 1 << 2;            // отрисовка карты при осмотре клетки
constexpr bitvector_t PRF_CAN_REMORT = kIntTwo | 1 << 3;            // разрешение на реморт через жертвование гривн
constexpr bitvector_t PRF_ENTER_ZONE = kIntTwo | 1 << 4;            // вывод названия/среднего уровня при входе в зону
constexpr bitvector_t PRF_MISPRINT = kIntTwo | 1 << 5;            // показ непрочитанных сообщений на доске опечаток при входе
constexpr bitvector_t PRF_BRIEF_SHIELDS = kIntTwo | 1 << 6;        // краткий режим сообщений при срабатывании маг.щитов
constexpr bitvector_t PRF_AUTO_NOSUMMON = kIntTwo | 1 << 7;        // автоматическое включение режима защиты от призыва ('реж призыв') после удачного суммона/пенты
constexpr bitvector_t PRF_SDEMIGOD = kIntTwo | 1 << 8;            // Для канала демигодов
constexpr bitvector_t PRF_BLIND = kIntTwo | 1 << 9;                // примочки для слепых
constexpr bitvector_t PRF_MAPPER = kIntTwo | 1 << 10;                // Показывает хеши рядом с названием комнаты
constexpr bitvector_t PRF_TESTER = kIntTwo | 1 << 11;                // отображать допинфу при годсфлаге тестер
constexpr bitvector_t PRF_IPCONTROL = kIntTwo | 1 << 12;            // отправлять код на мыло при заходе из новой подсети
constexpr bitvector_t PRF_SKIRMISHER = kIntTwo | 1 << 13;            // персонаж "в строю" в группе
constexpr bitvector_t PRF_DOUBLE_THROW = kIntTwo | 1 << 14;        // готов использовать двойной бросок
constexpr bitvector_t PRF_TRIPLE_THROW = kIntTwo | 1 << 15;        // готов использовать тройной бросок
constexpr bitvector_t PRF_SHADOW_THROW = kIntTwo | 1 << 16;        // применяет "теневой бросок"
constexpr bitvector_t PRF_DISP_COOLDOWNS = kIntTwo | 1 << 17;        // Показывать кулдауны скиллов в промпте
constexpr bitvector_t PRF_TELEGRAM = kIntTwo | 1 << 18;            // Активирует телеграм-канал у персонажа

// при добавлении не забываем про preference_bits[]

/*
 *  PC's constants
 */

// PC religions //
/* \todo Все, связанное с религиями, нужно вынести в отдельный модуль.
*/
const __uint8_t kReligionPoly = 0;
const __uint8_t kReligionMono = 1;

typedef std::array<const char *, static_cast<std::size_t>(ESex::kLast)> religion_genders_t;
typedef std::array<religion_genders_t, 3> religion_names_t;
extern const religion_names_t religion_name;

// Player flags: used by char_data.char_specials.act
constexpr bitvector_t PLR_KILLER = 1 << 0;            // Player is a player-killer     //
constexpr bitvector_t PLR_THIEF = 1 << 1;            // Player is a player-thief      //
constexpr bitvector_t PLR_FROZEN = 1 << 2;            // Player is frozen        //
constexpr bitvector_t PLR_DONTSET = 1 << 3;            // Don't EVER set (ISNPC bit)  //
constexpr bitvector_t PLR_WRITING = 1 << 4;            // Player writing (board/mail/olc)  //
constexpr bitvector_t PLR_MAILING = 1 << 5;            // Player is writing mail     //
constexpr bitvector_t PLR_CRASH = 1 << 6;            // Player needs to be crash-saved   //
constexpr bitvector_t PLR_SITEOK = 1 << 7;            // Player has been site-cleared  //
constexpr bitvector_t PLR_MUTE = 1 << 8;            // Player not allowed to shout/goss/auct  //
constexpr bitvector_t PLR_NOTITLE = 1 << 9;            // Player not allowed to set title  //
constexpr bitvector_t PLR_DELETED = 1 << 10;        // Player deleted - space reusable  //
constexpr bitvector_t PLR_LOADROOM = 1 << 11;        // Player uses nonstandard loadroom  (не используется) //
constexpr bitvector_t PLR_AUTOBOT = 1 << 12;        // Player автоматический игрок //
constexpr bitvector_t PLR_NODELETE = 1 << 13;        // Player shouldn't be deleted //
constexpr bitvector_t PLR_INVSTART = 1 << 14;        // Player should enter game wizinvis //
constexpr bitvector_t PLR_CRYO = 1 << 15;            // Player is cryo-saved (purge prog)   //
constexpr bitvector_t PLR_HELLED = 1 << 16;            // Player is in Hell //
constexpr bitvector_t PLR_NAMED = 1 << 17;            // Player is in Names Room //
constexpr bitvector_t PLR_REGISTERED = 1 << 18;
constexpr bitvector_t PLR_DUMB = 1 << 19;            // Player is not allowed to tell/emote/social //
constexpr bitvector_t PLR_SCRIPTWRITER = 1 << 20;   // скриптер
constexpr bitvector_t PLR_SPAMMER = 1 << 21;        // спаммер
// свободно
constexpr bitvector_t PLR_DELETE = 1 << 28;            // RESERVED - ONLY INTERNALLY (MOB_DELETE) //
constexpr bitvector_t PLR_FREE = 1 << 29;            // RESERVED - ONLY INTERBALLY (MOB_FREE)//

// GODs FLAGS
constexpr bitvector_t GF_GODSLIKE = 1 << 0;
constexpr bitvector_t GF_GODSCURSE = 1 << 1;
constexpr bitvector_t GF_HIGHGOD = 1 << 2;
constexpr bitvector_t GF_REMORT = 1 << 3;
constexpr bitvector_t GF_DEMIGOD = 1 << 4;    // Морталы с привилегиями богов //
constexpr bitvector_t GF_PERSLOG = 1 << 5;
constexpr bitvector_t GF_TESTER = 1 << 6;

// modes of ignoring
constexpr bitvector_t IGNORE_TELL = 1 << 0;
constexpr bitvector_t IGNORE_SAY = 1 << 1;
constexpr bitvector_t IGNORE_CLAN = 1 << 2;
constexpr bitvector_t IGNORE_ALLIANCE = 1 << 3;
constexpr bitvector_t IGNORE_GOSSIP = 1 << 4;
constexpr bitvector_t IGNORE_SHOUT = 1 << 5;
constexpr bitvector_t IGNORE_HOLLER = 1 << 6;
constexpr bitvector_t IGNORE_GROUP = 1 << 7;
constexpr bitvector_t IGNORE_WHISPER = 1 << 8;
constexpr bitvector_t IGNORE_ASK = 1 << 9;
constexpr bitvector_t IGNORE_EMOTE = 1 << 10;
constexpr bitvector_t IGNORE_OFFTOP = 1 << 11;

/*
 *  NPC's constants
 */

// NPC races
const __uint8_t NPC_RACE_BASIC = 100;
const __uint8_t NPC_RACE_HUMAN = 101;
const __uint8_t NPC_RACE_HUMAN_ANIMAL = 102;
const __uint8_t NPC_RACE_BIRD = 103;
const __uint8_t NPC_RACE_ANIMAL = 104;
const __uint8_t NPC_RACE_REPTILE = 105;
const __uint8_t NPC_RACE_FISH = 106;
const __uint8_t NPC_RACE_INSECT = 107;
const __uint8_t NPC_RACE_PLANT = 108;
const __uint8_t NPC_RACE_THING = 109;
const __uint8_t NPC_RACE_ZOMBIE = 110;
const __uint8_t NPC_RACE_GHOST = 111;
const __uint8_t NPC_RACE_EVIL_SPIRIT = 112;
const __uint8_t NPC_RACE_SPIRIT = 113;
const __uint8_t NPC_RACE_MAGIC_CREATURE = 114;
const __uint8_t NPC_RACE_NEXT = 115;

// Virtual NPC races
const __uint8_t NPC_BOSS = 200;
const __uint8_t NPC_UNIQUE = 201;

// Mobile flags: used by char_data.char_specials.act
constexpr bitvector_t MOB_SPEC = 1 << 0;            // Mob has a callable spec-proc  //
constexpr bitvector_t MOB_SENTINEL = 1 << 1;        // Mob should not move     //
constexpr bitvector_t MOB_SCAVENGER = 1 << 2;    // Mob picks up stuff on the ground //
constexpr bitvector_t MOB_ISNPC = 1 << 3;            // (R) Automatically set on all Mobs   //
constexpr bitvector_t MOB_AWARE = 1 << 4;            // Mob can't be backstabbed      //
constexpr bitvector_t MOB_AGGRESSIVE = 1 << 5;    // Mob hits players in the room  //
constexpr bitvector_t MOB_STAY_ZONE = 1 << 6;    // Mob shouldn't wander out of zone //
constexpr bitvector_t MOB_WIMPY = 1 << 7;            // Mob flees if severely injured //
constexpr bitvector_t MOB_AGGR_DAY = 1 << 8;        // //
constexpr bitvector_t MOB_AGGR_NIGHT = 1 << 9;    // //
constexpr bitvector_t MOB_AGGR_FULLMOON = 1 << 10;  // //
constexpr bitvector_t MOB_MEMORY = 1 << 11;            // remember attackers if attacked   //
constexpr bitvector_t MOB_HELPER = 1 << 12;            // attack PCs fighting other NPCs   //
constexpr bitvector_t MOB_NOCHARM = 1 << 13;        // Mob can't be charmed    //
constexpr bitvector_t MOB_NOSUMMON = 1 << 14;        // Mob can't be summoned      //
constexpr bitvector_t MOB_NOSLEEP = 1 << 15;        // Mob can't be slept      //
constexpr bitvector_t MOB_NOBASH = 1 << 16;            // Mob can't be bashed (e.g. trees) //
constexpr bitvector_t MOB_NOBLIND = 1 << 17;        // Mob can't be blinded    //
constexpr bitvector_t MOB_MOUNTING = 1 << 18;
constexpr bitvector_t MOB_NOHOLD = 1 << 19;
constexpr bitvector_t MOB_NOSIELENCE = 1 << 20;
constexpr bitvector_t MOB_AGGRMONO = 1 << 21;
constexpr bitvector_t MOB_AGGRPOLY = 1 << 22;
constexpr bitvector_t MOB_NOFEAR = 1 << 23;
constexpr bitvector_t MOB_NOGROUP = 1 << 24;
constexpr bitvector_t MOB_CORPSE = 1 << 25;
constexpr bitvector_t MOB_LOOTER = 1 << 26;
constexpr bitvector_t MOB_PROTECT = 1 << 27;
constexpr bitvector_t MOB_DELETE = 1 << 28;            // RESERVED - ONLY INTERNALLY //
constexpr bitvector_t MOB_FREE = 1 << 29;            // RESERVED - ONLY INTERBALLY //

constexpr bitvector_t MOB_SWIMMING = kIntOne | (1 << 0);
constexpr bitvector_t MOB_FLYING = kIntOne | (1 << 1);
constexpr bitvector_t MOB_ONLYSWIMMING = kIntOne | (1 << 2);
constexpr bitvector_t MOB_AGGR_WINTER = kIntOne | (1 << 3);
constexpr bitvector_t MOB_AGGR_SPRING = kIntOne | (1 << 4);
constexpr bitvector_t MOB_AGGR_SUMMER = kIntOne | (1 << 5);
constexpr bitvector_t MOB_AGGR_AUTUMN = kIntOne | (1 << 6);
constexpr bitvector_t MOB_LIKE_DAY = kIntOne | (1 << 7);
constexpr bitvector_t MOB_LIKE_NIGHT = kIntOne | (1 << 8);
constexpr bitvector_t MOB_LIKE_FULLMOON = kIntOne | (1 << 9);
constexpr bitvector_t MOB_LIKE_WINTER = kIntOne | (1 << 10);
constexpr bitvector_t MOB_LIKE_SPRING = kIntOne | (1 << 11);
constexpr bitvector_t MOB_LIKE_SUMMER = kIntOne | (1 << 12);
constexpr bitvector_t MOB_LIKE_AUTUMN = kIntOne | (1 << 13);
constexpr bitvector_t MOB_NOFIGHT = kIntOne | (1 << 14);
constexpr bitvector_t MOB_EADECREASE = kIntOne | (1 << 15); // понижает количество своих атак по мере убывания тек.хп
constexpr bitvector_t MOB_HORDE = kIntOne | (1 << 16);
constexpr bitvector_t MOB_CLONE = kIntOne | (1 << 17);
constexpr bitvector_t MOB_NOTKILLPUNCTUAL = kIntOne | (1 << 18);
constexpr bitvector_t MOB_NOTRIP = kIntOne | (1 << 19);
constexpr bitvector_t MOB_ANGEL = kIntOne | (1 << 20);
constexpr bitvector_t MOB_GUARDIAN = kIntOne | (1 << 21); //моб-стражник, ставится программно из файла guards.xml
constexpr bitvector_t MOB_IGNORE_FORBIDDEN = kIntOne | (1 << 22); // игнорирует печать
constexpr bitvector_t MOB_NO_BATTLE_EXP = kIntOne | (1 << 23); // не дает экспу за удары
constexpr bitvector_t MOB_NOMIGHTHIT = kIntOne | (1 << 24); // нельзя оглушить богатырским молотом
constexpr bitvector_t MOB_GHOST = kIntOne | (1 << 25); // Используется для ментальной тени
constexpr bitvector_t MOB_PLAYER_SUMMON = kIntOne | (1 << 26); // (ангел, тень, храны, трупы, умки)

constexpr bitvector_t MOB_FIREBREATH = kIntTwo | (1 << 0);
constexpr bitvector_t MOB_GASBREATH = kIntTwo | (1 << 1);
constexpr bitvector_t MOB_FROSTBREATH = kIntTwo | (1 << 2);
constexpr bitvector_t MOB_ACIDBREATH = kIntTwo | (1 << 3);
constexpr bitvector_t MOB_LIGHTBREATH = kIntTwo | (1 << 4);
constexpr bitvector_t MOB_NOTRAIN = kIntTwo | (1 << 5);
constexpr bitvector_t MOB_NOREST = kIntTwo | (1 << 6);
constexpr bitvector_t MOB_AREA_ATTACK = kIntTwo | (1 << 7);
constexpr bitvector_t MOB_NOSTUPOR = kIntTwo | (1 << 8);
constexpr bitvector_t MOB_NOHELPS = kIntTwo | (1 << 9);
constexpr bitvector_t MOB_OPENDOOR = kIntTwo | (1 << 10);
constexpr bitvector_t MOB_IGNORNOMOB = kIntTwo | (1 << 11);
constexpr bitvector_t MOB_IGNORPEACE = kIntTwo | (1 << 12);
constexpr bitvector_t MOB_RESURRECTED = kIntTwo | (1 << 13); // !поднять труп! или !оживить труп! только програмно//
constexpr bitvector_t MOB_RUSICH = kIntTwo | (1 << 14);
constexpr bitvector_t MOB_VIKING = kIntTwo | (1 << 15);
constexpr bitvector_t MOB_STEPNYAK = kIntTwo | (1 << 16);
constexpr bitvector_t MOB_AGGR_RUSICHI = kIntTwo | (1 << 17);
constexpr bitvector_t MOB_AGGR_VIKINGI = kIntTwo | (1 << 18);
constexpr bitvector_t MOB_AGGR_STEPNYAKI = kIntTwo | (1 << 19);
constexpr bitvector_t MOB_NORESURRECTION = kIntTwo | (1 << 20);
constexpr bitvector_t MOB_AWAKE = kIntTwo | (1 << 21);
constexpr bitvector_t MOB_IGNORE_FORMATION = kIntTwo | (1 << 22);

constexpr bitvector_t NPC_NORTH = 1 << 0;
constexpr bitvector_t NPC_EAST = 1 << 1;
constexpr bitvector_t NPC_SOUTH = 1 << 2;
constexpr bitvector_t NPC_WEST = 1 << 3;
constexpr bitvector_t NPC_UP = 1 << 4;
constexpr bitvector_t NPC_DOWN = 1 << 5;
constexpr bitvector_t NPC_POISON = 1 << 6;
constexpr bitvector_t NPC_INVIS = 1 << 7;
constexpr bitvector_t NPC_SNEAK = 1 << 8;
constexpr bitvector_t NPC_CAMOUFLAGE = 1 << 9;
constexpr bitvector_t NPC_MOVEFLY = 1 << 11;
constexpr bitvector_t NPC_MOVECREEP = 1 << 12;
constexpr bitvector_t NPC_MOVEJUMP = 1 << 13;
constexpr bitvector_t NPC_MOVESWIM = 1 << 14;
constexpr bitvector_t NPC_MOVERUN = 1 << 15;
constexpr bitvector_t NPC_AIRCREATURE = 1 << 20;
constexpr bitvector_t NPC_WATERCREATURE = 1 << 21;
constexpr bitvector_t NPC_EARTHCREATURE = 1 << 22;
constexpr bitvector_t NPC_FIRECREATURE = 1 << 23;
constexpr bitvector_t NPC_HELPED = 1 << 24;
constexpr bitvector_t NPC_FREEDROP = 1 << 25;
constexpr bitvector_t NPC_NOINGRDROP = 1 << 26;

constexpr bitvector_t NPC_STEALING = kIntOne | (1 << 0);
constexpr bitvector_t NPC_WIELDING = kIntOne | (1 << 1);
constexpr bitvector_t NPC_ARMORING = kIntOne | (1 << 2);
constexpr bitvector_t NPC_USELIGHT = kIntOne | (1 << 3);
constexpr bitvector_t NPC_NOTAKEITEMS = kIntOne | (1 << 4);

/*
 *  Room's constants
 */

extern std::unordered_map<int, std::string> SECTOR_TYPE_BY_VALUE;

// The cardinal directions: used as index to room_data.dir_option[]
const __uint8_t kDirNorth = 0;
const __uint8_t kDirEast = 1;
const __uint8_t kDirSouth = 2;
const __uint8_t kDirWest = 3;
const __uint8_t kDirUp = 4;
const __uint8_t kDirDown = 5;
const __uint8_t kDirMaxNumber = 6;        // number of directions in a room (nsewud) //

// Room flags: used in room_data.room_flags //
// WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") //
constexpr bitvector_t ROOM_DARK = 1 << 0;
constexpr bitvector_t ROOM_DEATH =  1 << 1;    // Death trap      //
constexpr bitvector_t ROOM_NOMOB = 1 << 2;
constexpr bitvector_t ROOM_INDOORS = 1 << 3;
constexpr bitvector_t ROOM_PEACEFUL = 1 << 4;
constexpr bitvector_t ROOM_SOUNDPROOF = 1 << 5;
constexpr bitvector_t ROOM_NOTRACK = 1 << 6;
constexpr bitvector_t ROOM_NOMAGIC = 1 << 7;
constexpr bitvector_t ROOM_TUNNEL = 1 << 8;
constexpr bitvector_t ROOM_NOTELEPORTIN = 1 << 9;
constexpr bitvector_t ROOM_GODROOM = 1 << 10;    // kLevelGod+ only allowed //
constexpr bitvector_t ROOM_HOUSE = 1 << 11;    // (R) Room is a house  //
constexpr bitvector_t ROOM_HOUSE_CRASH = 1 << 12;    // (R) House needs saving   //
constexpr bitvector_t ROOM_ATRIUM = 1 << 13;    // (R) The door to a house //
constexpr bitvector_t ROOM_OLC = 1 << 14;    // (R) Modifyable/!compress   //
constexpr bitvector_t ROOM_BFS_MARK = 1 << 15;    // (R) breath-first srch mrk   //
constexpr bitvector_t ROOM_MAGE = 1 << 16;
constexpr bitvector_t ROOM_CLERIC = 1 << 17;
constexpr bitvector_t ROOM_THIEF = 1 << 18;
constexpr bitvector_t ROOM_WARRIOR = 1 << 19;
constexpr bitvector_t ROOM_ASSASINE = 1 << 20;
constexpr bitvector_t ROOM_GUARD = 1 << 21;
constexpr bitvector_t ROOM_PALADINE = 1 << 22;
constexpr bitvector_t ROOM_RANGER = 1 << 23;
constexpr bitvector_t ROOM_POLY = 1 << 24;
constexpr bitvector_t ROOM_MONO = 1 << 25;
constexpr bitvector_t ROOM_SMITH = 1 << 26;
constexpr bitvector_t ROOM_MERCHANT = 1 << 27;
constexpr bitvector_t ROOM_DRUID = 1 << 28;
constexpr bitvector_t ROOM_ARENA = 1 << 29;

constexpr bitvector_t ROOM_NOSUMMON = kIntOne | (1 << 0);
constexpr bitvector_t ROOM_NOTELEPORTOUT = kIntOne | (1 << 1);
constexpr bitvector_t ROOM_NOHORSE = kIntOne | (1 << 2);
constexpr bitvector_t ROOM_NOWEATHER = kIntOne | (1 << 3);
constexpr bitvector_t ROOM_SLOWDEATH = kIntOne | (1 << 4);
constexpr bitvector_t ROOM_ICEDEATH = kIntOne | (1 << 5);
constexpr bitvector_t ROOM_NORELOCATEIN = kIntOne | (1 << 6);
constexpr bitvector_t ROOM_ARENARECV = kIntOne | (1 << 7);  // комната в которой слышно сообщения арены
constexpr bitvector_t ROOM_ARENASEND = kIntOne | (1 << 8);   // комната из которой отправляются сообщения арены
constexpr bitvector_t ROOM_NOBATTLE = kIntOne | (1 << 9);
constexpr bitvector_t ROOM_QUEST = kIntOne | (1 << 10);
constexpr bitvector_t ROOM_LIGHT = kIntOne | (1 << 11);
constexpr bitvector_t ROOM_NOMAPPER = kIntOne | (1 << 12);  //нет внумов комнат

constexpr bitvector_t ROOM_NOITEM = kIntTwo | (1 << 0);    // Передача вещей в комнате запрещена

// Exit info: used in room_data.dir_option.exit_info //
constexpr bitvector_t EX_ISDOOR = 1 << 0;    	// Exit is a door     //
constexpr bitvector_t EX_CLOSED = 1 << 1;   	// The door is closed //
constexpr bitvector_t EX_LOCKED = 1 << 2; 	   	// The door is locked //
constexpr bitvector_t EX_PICKPROOF = 1 << 3;    // Lock can't be picked  //
constexpr bitvector_t EX_HIDDEN = 1 << 4;
constexpr bitvector_t EX_BROKEN = 1 << 5; 		//Polud замок двери сломан
constexpr bitvector_t EX_DUNGEON_ENTRY = 1 << 6;    // When character goes through this door then he will get into a copy of the zone behind the door.

// Sector types: used in room_data.sector_type //
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
 *  Object's constants
 */

// Типы магических книг //
const __uint8_t BOOK_SPELL = 0;    // Книга заклинания //
const __uint8_t BOOK_SKILL = 1;    // Книга умения //
const __uint8_t BOOK_UPGRD = 2;    // Увеличение умения //
const __uint8_t BOOK_RECPT = 3;    // Книга рецепта //
const __uint8_t BOOK_FEAT = 4;        // Книга способности (feats) //

// Take/Wear flags: used by obj_data.obj_flags.wear_flags //
enum class EWearFlag : bitvector_t {
	ITEM_WEAR_UNDEFINED = 0,    // Special value
	ITEM_WEAR_TAKE = 1 << 0,    // Item can be takes      //
	ITEM_WEAR_FINGER = 1 << 1,    // Can be worn on finger  //
	ITEM_WEAR_NECK = 1 << 2,    // Can be worn around neck   //
	ITEM_WEAR_BODY = 1 << 3,    // Can be worn on body    //
	ITEM_WEAR_HEAD = 1 << 4,    // Can be worn on head    //
	ITEM_WEAR_LEGS = 1 << 5,    // Can be worn on legs //
	ITEM_WEAR_FEET = 1 << 6,    // Can be worn on feet //
	ITEM_WEAR_HANDS = 1 << 7,    // Can be worn on hands   //
	ITEM_WEAR_ARMS = 1 << 8,    // Can be worn on arms //
	ITEM_WEAR_SHIELD = 1 << 9,    // Can be used as a shield   //
	ITEM_WEAR_ABOUT = 1 << 10,    // Can be worn about body    //
	ITEM_WEAR_WAIST = 1 << 11,    // Can be worn around waist  //
	ITEM_WEAR_WRIST = 1 << 12,    // Can be worn on wrist   //
	ITEM_WEAR_WIELD = 1 << 13,    // Can be wielded      //
	ITEM_WEAR_HOLD = 1 << 14,    // Can be held      //
	ITEM_WEAR_BOTHS = 1 << 15,
	ITEM_WEAR_QUIVER = 1 << 16      // колчан
};

template<>
const std::string &NAME_BY_ITEM<EWearFlag>(EWearFlag item);
template<>
EWearFlag ITEM_BY_NAME<EWearFlag>(const std::string &name);

// Extra object flags: used by obj_data.obj_flags.extra_flags //
enum class EExtraFlag : bitvector_t {
	ITEM_GLOW = 1 << 0,                        ///< Item is glowing
	ITEM_HUM = 1 << 1,                        ///< Item is humming
	ITEM_NORENT = 1 << 2,                    ///< Item cannot be rented
	ITEM_NODONATE = 1 << 3,                    ///< Item cannot be donated
	ITEM_NOINVIS = 1 << 4,                    ///< Item cannot be made invis
	ITEM_INVISIBLE = 1 << 5,                ///< Item is invisible
	ITEM_MAGIC = 1 << 6,                    ///< Item is magical
	ITEM_NODROP = 1 << 7,                    ///< Item is cursed: can't drop
	ITEM_BLESS = 1 << 8,                    ///< Item is blessed
	ITEM_NOSELL = 1 << 9,                    ///< Not usable by good people
	ITEM_DECAY = 1 << 10,                    ///< Not usable by evil people
	ITEM_ZONEDECAY = 1 << 11,                ///< Not usable by neutral people
	ITEM_NODISARM = 1 << 12,                ///< Not usable by mages
	ITEM_NODECAY = 1 << 13,
	ITEM_POISONED = 1 << 14,
	ITEM_SHARPEN = 1 << 15,
	ITEM_ARMORED = 1 << 16,
	ITEM_DAY = 1 << 17,
	ITEM_NIGHT = 1 << 18,
	ITEM_FULLMOON = 1 << 19,
	ITEM_WINTER = 1 << 20,
	ITEM_SPRING = 1 << 21,
	ITEM_SUMMER = 1 << 22,
	ITEM_AUTUMN = 1 << 23,
	ITEM_SWIMMING = 1 << 24,
	ITEM_FLYING = 1 << 25,
	ITEM_THROWING = 1 << 26,
	ITEM_TICKTIMER = 1 << 27,
	ITEM_FIRE = 1 << 28,                    ///< ...горит
	ITEM_REPOP_DECAY = 1 << 29,                ///< рассыпется при репопе зоны
	ITEM_NOLOCATE = kIntOne | (1 << 0),        ///< нельзя отлокейтить
	ITEM_TIMEDLVL = kIntOne | (1 << 1),        ///< для маг.предметов уровень уменьшается со временем
	ITEM_NOALTER = kIntOne | (1 << 2),        ///< свойства предмета не могут быть изменены магией
	ITEM_WITH1SLOT = kIntOne | (1 << 3),    ///< в предмет можно вплавить 1 камень
	ITEM_WITH2SLOTS = kIntOne | (1 << 4),    ///< в предмет можно вплавить 2 камня
	ITEM_WITH3SLOTS = kIntOne | (1 << 5),    ///< в предмет можно вплавить 3 камня (овер)
	ITEM_SETSTUFF = kIntOne | (1 << 6),        ///< Item is set object
	ITEM_NO_FAIL = kIntOne | (1 << 7),        ///< не фейлится при изучении (в случае книги)
	ITEM_NAMED = kIntOne | (1 << 8),        ///< именной предмет
	ITEM_BLOODY = kIntOne | (1 << 9),        ///< окровавленная вещь (снятая с трупа)
	ITEM_1INLAID = kIntOne | (1 << 10),        ///< TODO: не используется, см convert_obj_values()
	ITEM_2INLAID = kIntOne | (1 << 11),
	ITEM_3INLAID = kIntOne | (1 << 12),
	ITEM_NOPOUR = kIntOne | (1 << 13),        ///< нельзя перелить
	ITEM_UNIQUE = kIntOne | (1
		<< 14),        // объект уникальный, т.е. если у чара есть несколько шмоток с одним внумом, которые одеваются
	// на разные слоты, то чар может одеть на себя только одну шмотку
	ITEM_TRANSFORMED = kIntOne | (1 << 15),        // Наложено заклинание заколдовать оружие
	ITEM_FREE_FOR_USE = kIntOne | (1 << 16),    // пока свободно, можно использовать
	ITEM_NOT_UNLIMIT_TIMER = kIntOne | (1 << 17), // Не может быть нерушимой
	ITEM_UNIQUE_WHEN_PURCHASE = kIntOne | (1 << 18), // станет именной при покупке в магазе
	ITEM_NOT_ONE_CLANCHEST = kIntOne | (1 << 19) //1 штука из набора не лезет в хран
};

template<>
const std::string &NAME_BY_ITEM<EExtraFlag>(EExtraFlag item);
template<>
EExtraFlag ITEM_BY_NAME<EExtraFlag>(const std::string &name);

enum class ENoFlag : bitvector_t {
	ITEM_NO_MONO = 1 << 0,
	ITEM_NO_POLY = 1 << 1,
	ITEM_NO_NEUTRAL = 1 << 2,
	ITEM_NO_MAGIC_USER = 1 << 3,
	ITEM_NO_CLERIC = 1 << 4,
	ITEM_NO_THIEF = 1 << 5,
	ITEM_NO_WARRIOR = 1 << 6,
	ITEM_NO_ASSASINE = 1 << 7,
	ITEM_NO_GUARD = 1 << 8,
	ITEM_NO_PALADINE = 1 << 9,
	ITEM_NO_RANGER = 1 << 10,
	ITEM_NO_SMITH = 1 << 11,
	ITEM_NO_MERCHANT = 1 << 12,
	ITEM_NO_DRUID = 1 << 13,
	ITEM_NO_BATTLEMAGE = 1 << 14,
	ITEM_NO_CHARMMAGE = 1 << 15,
	ITEM_NO_DEFENDERMAGE = 1 << 16,
	ITEM_NO_NECROMANCER = 1 << 17,
	ITEM_NO_FIGHTER_USER = 1 << 18,
	ITEM_NO_KILLER = kIntOne | 1 << 0,
	ITEM_NO_COLORED = kIntOne | 1 << 1,    // нельзя цветным //
	ITEM_NO_BD = kIntOne | 1 << 2,
	ITEM_NO_MALE = kIntTwo | 1 << 6,
	ITEM_NO_FEMALE = kIntTwo | 1 << 7,
	ITEM_NO_CHARMICE = kIntTwo | 1 << 8,
	ITEM_NO_POLOVCI = kIntTwo | 1 << 9,
	ITEM_NO_PECHENEGI = kIntTwo | 1 << 10,
	ITEM_NO_MONGOLI = kIntTwo | 1 << 11,
	ITEM_NO_YIGURI = kIntTwo | 1 << 12,
	ITEM_NO_KANGARI = kIntTwo | 1 << 13,
	ITEM_NO_XAZARI = kIntTwo | 1 << 14,
	ITEM_NO_SVEI = kIntTwo | 1 << 15,
	ITEM_NO_DATCHANE = kIntTwo | 1 << 16,
	ITEM_NO_GETTI = kIntTwo | 1 << 17,
	ITEM_NO_UTTI = kIntTwo | 1 << 18,
	ITEM_NO_XALEIGI = kIntTwo | 1 << 19,
	ITEM_NO_NORVEZCI = kIntTwo | 1 << 20,
	ITEM_NO_RUSICHI = kIntThree | 1 << 0,
	ITEM_NO_STEPNYAKI = kIntThree | 1 << 1,
	ITEM_NO_VIKINGI = kIntThree | 1 << 2,
	ITEM_NOT_FOR_NOPK = kIntThree | (1 << 3)      // не может быть взята !пк кланом
};

template<>
const std::string &NAME_BY_ITEM<ENoFlag>(ENoFlag item);
template<>
ENoFlag ITEM_BY_NAME<ENoFlag>(const std::string &name);

enum class EAntiFlag : bitvector_t {
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
	ITEM_NOT_FOR_NOPK = kIntThree | (1 << 3)      // не может быть взята !пк кланом
};

template<>
const std::string &NAME_BY_ITEM<EAntiFlag>(EAntiFlag item);
template<>
EAntiFlag ITEM_BY_NAME<EAntiFlag>(const std::string &name);

// Container flags - value[1] //
constexpr bitvector_t CONT_CLOSEABLE = 1 << 0;    // Container can be closed //
constexpr bitvector_t CONT_PICKPROOF = 1 << 1;    // Container is pickproof  //
constexpr bitvector_t CONT_CLOSED = 1 << 2;        // Container is closed     //
constexpr bitvector_t CONT_LOCKED = 1 << 3;        // Container is locked     //
constexpr bitvector_t CONT_BROKEN = 1 << 4;        // Container is locked     //

#endif //BYLINS_SRC_ENTITY_ROOMS_ROOM_CONSTANTS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :