// Copyright (c) 2011 Posvist
// Part of Bylins http://www.mud.ru

/* ���� ����� �����, ������ ���:
����� ������ ���� � ������ ��������
builtin_converters.cpp - PyUnicode_FromString �� PyUnicode_DecodeLocale, PyUnicode_FromStringAndSize �� PyUnicode_DecodeLocaleAndSize
builtin_converters.hpp - PyUnicode_FromStringAndSize �� PyUnicode_DecodeLocaleAndSize
str.cpp - PyUnicode_FromString �� PyUnicode_DecodeLocale, PyUnicode_FromStringAndSize �� PyUnicode_DecodeLocaleAndSize
� ���-�� ��� � ������ ���� ������� _PyUnicode_AsString( obj ), �� ������ �� PyBytes_AsString( PyUnicode_EncodeLocale(obj) )
�.�. ������ ��� ��� ��, ��� � ����� http://habrahabr.ru/post/161931/
*/
#include "scripting.hpp"

#include "object.prototypes.hpp"
#include "logger.hpp"
#include "utils.h"
#include "comm.h"
#include "char.hpp"
#include "interpreter.h"
#include "obj.hpp"
#include "db.h"
#include "cache.hpp"
#include "spell_parser.hpp"
#include "spells.h"
#include "handler.h"
#include "constants.h"
#include "modify.h"

// Required because pyconfig.h defines ssize_t by himself
#if defined(ssize_t)
#undef ssize_t
#endif
#include <boost/python.hpp>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/locale.hpp>

#include <string>
#include <vector>

#define DEFINE_CONSTANT(X) scope().attr(#X) = static_cast<int>(X);
#define DEFINE_ENUM_CONSTANT(X) scope().attr(NAME_BY_ITEM(X).c_str()) = to_underlying(X)

using namespace boost::python;
namespace py=boost::python;
using namespace scripting;
extern room_rnum find_target_room(CHAR_DATA * ch, char *rawroomstr, int trig);
namespace
{
	std::list<object> objs_to_call_in_main_thread;
	object events;
	PyThreadState *_save = NULL;
}

class GILAcquirer
{
	PyGILState_STATE _state;
	public:
		GILAcquirer():
			_state(PyGILState_Ensure()) { };
		~GILAcquirer()
		{
			PyGILState_Release(_state);
		}
};

std::string parse_python_exception();
void register_global_command(const std::string& command, const std::string& command_koi8r, object callable, sh_int minimum_position, sh_int minimum_level, int unhide_percent);
void unregister_global_command(const std::string& command);

template <class t>
inline t pass_through(const t& o) { return o; }

//�����-������� ������ �������� ������� ������� �����
template <class t, class nextfunc, class getfunc, class result_t=t>
class CIterator
{
public:
	typedef t object_t;
	CIterator(const object_t& o, const nextfunc nfunc, const getfunc gfunc):
		_object(o), _next_func(nfunc), _get_func(gfunc) { }
	result_t next();
	private:
	object_t _object;
	const nextfunc _next_func;
	const getfunc _get_func;
};

py::handle<PyObject> ObjectDoesNotExist;

template <class t>
class Wrapper
{
public:
	typedef t wrapped_t;
	typedef caching::Cache<wrapped_t*> cache_t;
	Wrapper(t* obj, cache_t& cache_):
		cache(cache_), id(cache_.get_id(obj))
	{
	}

	Wrapper(const Wrapper& other):
		cache(other.cache), id(other.id) { }

	Wrapper& operator=(const Wrapper& other)
	{
		id = other.id;
		return *this;
	}

	operator t*() const
	{
		t* r = cache.get_obj(id);
		if (!r)
			raise_error();
		return r;
	}

	operator bool() { return id!=0; }

	void raise_error() const
	{
		PyErr_SetString(ObjectDoesNotExist.get(), "Object you are referencing doesn't exist anymore");
		throw_error_already_set();
	}

	struct Ensurer
	{
		Ensurer(const Wrapper& w): ptr(w) { }

		t* operator->() { return ptr; }
		operator t*() { return ptr; }
		t* ptr;
	};

private:
	cache_t& cache;
	caching::id_t id;
};

class CharacterWrapper: public Wrapper<CHAR_DATA>
{
public:
CharacterWrapper(CHAR_DATA* ch):Wrapper<CHAR_DATA>(ch, caching::character_cache) { }
const char* get_name() const
{
	Ensurer ch(*this);
	return ch->get_name().c_str();
}

void set_name(const char* name)
{
	Ensurer ch(*this);
	ch->set_name(name);
}

void send(const std::string& msg)
{
	Ensurer ch(*this);
	send_to_char(msg, (CHAR_DATA*)ch);
}

void _page_string(const std::string& msg)
{
	Ensurer ch(*this);
	page_string(ch->desc, msg);
}

void act_on_char(const char *str, bool hide_invisible, const OBJ_DATA * obj, const CharacterWrapper& victim, int type)
{
	Ensurer ch(*this);
	Ensurer v(victim);
	act(str, hide_invisible, ch, obj, (CHAR_DATA*)victim, type);
}

void act_on_obj(const char *str, bool hide_invisible, const OBJ_DATA * obj, const OBJ_DATA *victim, int type)
{
	Ensurer ch(*this);
	act(str, hide_invisible, ch, obj, victim, type);
}

py::list get_followers()
{
	Ensurer ch(*this);
	py::list result;
	for (follow_type* i=ch->followers; i; i=i->next)
		result.append(CharacterWrapper(i->follower));
	return result;
}

bool is_immortal()
{
	Ensurer ch(*this);
	return IS_IMMORTAL(ch);
}

bool is_impl()
{
	Ensurer ch(*this);
	return IS_IMPL(ch);
}

bool is_NPC()
{
	Ensurer ch(*this);
	return IS_NPC(ch);
}

const char* get_long_descr() const
{
	Ensurer ch(*this);
	return ch->get_long_descr();
}

void set_long_descr(const char* v)
{
	Ensurer ch(*this);
	ch->set_long_descr(v);
}

const char* get_description() const
{
	Ensurer ch(*this);
	return ch->get_description();
}

void set_description(const char* v)
{
	Ensurer ch(*this);
	ch->set_description(v);
}

short get_class() const
{
	Ensurer ch(*this);
	return ch->get_class();
}

void set_class(const short v)
{
	Ensurer ch(*this);
	ch->set_class(v);
}

short get_level() const
{
	Ensurer ch(*this);
	return ch->get_level();
}

void set_level(const short v)
{
	Ensurer ch(*this);
	ch->set_level(v);
}

long get_exp() const
{
	Ensurer ch(*this);
	return ch->get_exp();
}

void set_exp(const long v)
{
	Ensurer ch(*this);
	gain_exp(ch, v-ch->get_exp());
}

long get_gold() const
{
	Ensurer ch(*this);
	return ch->get_gold();
}

void set_gold(const long v)
{
	Ensurer ch(*this);
	ch->set_gold(v);
}

long get_bank() const
{
	Ensurer ch(*this);
	return ch->get_bank();
}

void set_bank(const long v)
{
	Ensurer ch(*this);
	ch->set_bank(v);
}

int get_str() const
{
	Ensurer ch(*this);
	return ch->get_str();
}

void set_str(const int v)
{
	Ensurer ch(*this);
	ch->set_str(v);
}

int get_dex() const
{
	Ensurer ch(*this);
	return ch->get_dex();
}

void set_dex(const int v)
{
	Ensurer ch(*this);
	ch->set_dex(v);
}

int get_con() const
{
	Ensurer ch(*this);
	return ch->get_con();
}

void set_con(const int v)
{
	Ensurer ch(*this);
	ch->set_con(v);
}

int get_wis() const
{
	Ensurer ch(*this);
	return ch->get_wis();
}

void set_wis(const int v)
{
	Ensurer ch(*this);
	ch->set_wis(v);
}

int get_int() const
{
	Ensurer ch(*this);
	return ch->get_int();
}

void set_int(const int v)
{
	Ensurer ch(*this);
	ch->set_int(v);
}

int get_cha() const
{
	Ensurer ch(*this);
	return ch->get_cha();
}

void set_cha(const int v)
{
	Ensurer ch(*this);
	ch->set_cha(v);
}

byte get_sex() const
{
	Ensurer ch(*this);
	return to_underlying(ch->get_sex());
}

void set_sex(const byte v)
{
	Ensurer ch(*this);
	ch->set_sex(static_cast<ESex>(v));
}

ubyte get_weight() const
{
	Ensurer ch(*this);
	return ch->get_weight();
}

void set_weight(const ubyte v)
{
	Ensurer ch(*this);
	ch->set_weight(v);
}

ubyte get_height() const
{
	Ensurer ch(*this);
	return ch->get_height();
}

void set_height(const ubyte v)
{
	Ensurer ch(*this);
	ch->set_height(v);
}

ubyte get_religion() const
{
	Ensurer ch(*this);
	return ch->get_religion();
}

void set_religion(const ubyte v)
{
	Ensurer ch(*this);
	ch->set_religion(v);
}

ubyte get_kin() const
{
	Ensurer ch(*this);
	return ch->get_kin();
}

void set_kin(const ubyte v)
{
	Ensurer ch(*this);
	ch->set_kin(v);
}

ubyte get_race() const
{
	Ensurer ch(*this);
	return ch->get_race();
}

void set_race(const ubyte v)
{
	Ensurer ch(*this);
	ch->set_race(v);
}

int get_hit() const
{
	Ensurer ch(*this);
	return ch->get_hit();
}

void set_hit(const int v)
{
	Ensurer ch(*this);
	ch->set_hit(v);
}

int get_max_hit() const
{
	Ensurer ch(*this);
	return ch->get_max_hit();
}

void set_max_hit(const int v)
{
	Ensurer ch(*this);
	ch->set_max_hit(v);
}

sh_int get_move() const
{
	Ensurer ch(*this);
	return ch->get_move();
}

void set_move(const sh_int v)
{
	Ensurer ch(*this);
	ch->set_move(v);
}

sh_int get_max_move() const
{
	Ensurer ch(*this);
	return ch->get_max_move();
}

void set_max_move(const sh_int v)
{
	Ensurer ch(*this);
	ch->set_max_move(v);
}

const char* get_email() const
{
	Ensurer ch(*this);
	return GET_EMAIL(ch);
}

const object get_pad(const int v) const
{
	Ensurer ch(*this);
	const char* val = ch->get_pad(v);
	size_t size = strlen(val);
	PyObject* py_buf = PyBytes_FromStringAndSize(val, size);
	object retval = object(handle<>(py_buf));
	return retval;
}

void set_pad(const int pad, const char* v)
{
	Ensurer ch(*this);
	ch->set_pad(pad, v);
}

void remove_gold(const long num, const bool log=true)
{
	Ensurer ch(*this);
	ch->remove_gold(num, log);
}

void remove_bank(const long num, const bool log=true)
{
	Ensurer ch(*this);
	ch->remove_bank(num, log);
}

void remove_both_gold(const long num, const bool log=true)
{
	Ensurer ch(*this);
	ch->remove_both_gold(num, log);
}

void add_gold(const long num, const bool log=true)
{
	Ensurer ch(*this);
	ch->add_gold(num, log);
}

void add_bank(const long num, const bool log=true)
{
	Ensurer ch(*this);
	ch->add_bank(num, log);
}

long get_total_gold() const
{
	Ensurer ch(*this);
	return ch->get_total_gold();
	}

int get_uid() const
{
	Ensurer ch(*this);
	return ch->get_uid();
}

short get_remort() const
{
	Ensurer ch(*this);
	return ch->get_remort();
}

int get_skill(int skill_num) const
{
	Ensurer ch(*this);
	return ch->get_skill(static_cast<ESkill>(skill_num));
}

void set_skill(int skill_num, int percent)
{
	Ensurer ch(*this);
	ch->set_skill(static_cast<ESkill>(skill_num), percent);
}

void clear_skills()
{
	Ensurer ch(*this);
	ch->clear_skills();
}

int get_skills_count() const
{
	Ensurer ch(*this);
	return ch->get_skills_count();
}

int get_equipped_skill(int skill_num) const
{
	Ensurer ch(*this);
	return ch->get_equipped_skill(static_cast<ESkill>(skill_num));
}

int get_trained_skill(int skill_num) const
{
	Ensurer ch(*this);
	return ch->get_trained_skill(static_cast<ESkill>(skill_num));
}

ubyte get_spell(int spell_num) const
{
	Ensurer ch(*this);
	return GET_SPELL_TYPE(ch, spell_num);
}

void set_spell(int spell_num, ubyte value)
{
	Ensurer ch(*this);
	GET_SPELL_TYPE(ch, spell_num) = value;
}

void interpret(char* command)
{
	Ensurer ch(*this);
	command_interpreter(ch, command);
}

mob_rnum get_nr() const
{
	Ensurer ch(*this);
	return ch->nr;
}

void set_nr(const mob_rnum nr)
{
	Ensurer ch(*this);
	ch->nr = nr;
}

room_rnum get_in_room() const
{
	Ensurer ch(*this);
	return ch->in_room;
}

void set_in_room(room_rnum in_room)
{
	Ensurer ch(*this);
	char_from_room(ch);
	char_to_room(ch, in_room);
	check_horse(ch);
}

bool is_affected_by_spell(int spell_num) const
{
	Ensurer ch(*this);
	return affected_by_spell(ch, spell_num);
}

void add_affect(const AFFECT_DATA<EApplyLocation>& af)
{
	Ensurer ch(*this);
	affect_to_char(ch, af);
}

CharacterWrapper get_vis(const char* name, int where) const
{
	Ensurer ch(*this);
	CHAR_DATA* r = get_char_vis(ch, name, where);
	if (!r)
	{
		PyErr_SetString(PyExc_ValueError, "Character not found");
		throw_error_already_set();
	}
	return r;
}

void restore()
{
	Ensurer vict(*this);
	GET_HIT(vict) = GET_REAL_MAX_HIT(vict);
	GET_MOVE(vict) = GET_REAL_MAX_MOVE(vict);
	if (IS_MANA_CASTER(vict))
	{
		GET_MANA_STORED(vict) = GET_MAX_MANA(vict);
	}
	else
	{
		GET_MEM_COMPLETED(vict) = GET_MEM_TOTAL(vict);
	}
}

void quested_add(const CharacterWrapper& ch, int vnum, char *text)
{
	Ensurer self(*this);
	self->quested_add((CHAR_DATA*)Ensurer(ch), vnum, text);
}

bool quested_remove(int vnum)
{
	Ensurer ch(*this);
	return ch->quested_remove(vnum);
}

std::string quested_get_text(int vnum)
{
	Ensurer ch(*this);
	return ch->quested_get_text(vnum);
}

std::string quested_print() const
{
	Ensurer ch(*this);
	return ch->quested_print();
}

unsigned get_wait() const
{
	Ensurer ch(*this);
	return ch->get_wait();
}

void set_wait(const unsigned v)
{
	Ensurer ch(*this);
	ch->set_wait(v);
}

std::string clan_status()
{
	Ensurer ch(*this);
	return GET_CLAN_STATUS(ch);
}

bool set_password_wrapped(const std::string&/* name*/, const std::string&/* password*/)
{
	// ��������
	return true;
}


};

CharacterWrapper create_mob_from_proto(mob_rnum proto_rnum, bool is_virtual=true)
{
	return read_mobile(proto_rnum, is_virtual);
}

BOOST_PYTHON_FUNCTION_OVERLOADS(create_mob_from_proto_overloads, create_mob_from_proto, 1, 2)

struct CharacterListWrapper
{
typedef CharacterWrapper (*func_type)(const CharacterWrapper& );
typedef CIterator<CharacterWrapper, func_type, func_type> iterator;

static CharacterWrapper my_next_func(const CharacterWrapper& w)
{
	CharacterWrapper::Ensurer ch(w);
	if (ch->get_next())
	{
		return ch->get_next();
	}
	PyErr_SetString(PyExc_StopIteration, "End of list.");
	throw_error_already_set();
	return CharacterWrapper(NULL); //to prevent compiler warning
}

static iterator* iter() { return new iterator(character_list, my_next_func, pass_through); }
};

CharacterWrapper get_mob_proto(const mob_rnum rnum)
{
	if (rnum>=0 && rnum <= top_of_mobt)
		return &mob_proto[rnum];
		PyErr_SetString(PyExc_ValueError, "mob rnum is out of range");
	throw_error_already_set();
	return CharacterWrapper(NULL);
	}

CHAR_DATA* character_get_master(CHAR_DATA* ch)
{
	return ch->get_master();
}

void character_set_master(CHAR_DATA* ch, CHAR_DATA* master)
{
	ch->set_master(master);
}

std::string get_spell_type_str(const AFFECT_DATA<EApplyLocation>& af)
{
	return spell_info[af.type].name;
}

std::string get_location_str(const AFFECT_DATA<EApplyLocation>& af)
{
	char buf[MAX_STRING_LENGTH];
	sprinttype(af.location, apply_types, buf);
	return buf;
}

std::string get_bitvector_str(const AFFECT_DATA<EApplyLocation>& af)
{
	char buf[MAX_STRING_LENGTH];
	sprintbitwd(af.bitvector, affected_bits, buf, ", ");
	return buf;
}

typedef std::array<obj_affected_type, MAX_OBJ_AFFECT> affected_t;

template<class T, int N>
struct _arrayN
{
    typedef boost::array<T,N> arrayN;

    static T get(arrayN const& self, int idx)
    {
      if( !(0<=idx && idx<N ))
	  {
      PyErr_SetString(PyExc_KeyError,"index out of range");
      throw_error_already_set();
	  }
	  return self[idx];
    }

    static boost::python::list getslice(arrayN const& self, int a,int b)
    {
      if( !(a>=0 && a<N && b>0 && b<=N) )
      {
       PyErr_SetString(PyExc_KeyError,"index out of range");
       throw_error_already_set();
      }
      if(b>N){b=N;}
      if(a<0){a=0;}
      boost::python::list t;
      for(int i=a;i<b;++i)
          t.append(self[i]);
       return t;
    }
    static void setslice(arrayN& self, int a,int b,boost::python::object& v)
    {
      if( !(a>=0 && a<N && b>0 && b<=N) )
      {
       PyErr_SetString(PyExc_KeyError,"index out of range");
       throw_error_already_set();
      }
      if(b>N){b=N;}
      if(a<0){a=0;}

      for(int i=a;i<b;++i)
          self[i]=extract<T>(v[i]);
    }
        static void set(arrayN& self, int idx, const T val) { self[idx]=val; }

    static boost::python::list values(arrayN const& self)
    {
        boost::python::list t;
        for(int i=0;i<N;++i)
            t.append(self[i]);
        return t;
    }
    static int size(arrayN const &/* self*/)
    {
		return N;
    }
};

class ObjWrapper: private std::shared_ptr<OBJ_DATA>, public Wrapper<OBJ_DATA>
{
public:
	ObjWrapper(OBJ_DATA* obj) : Wrapper<OBJ_DATA>(obj, caching::obj_cache)
	{
	}

	ObjWrapper(const CObjectPrototype::shared_ptr& obj):
		std::shared_ptr<OBJ_DATA>(new OBJ_DATA(*obj)),
		Wrapper<OBJ_DATA>(get(), caching::obj_cache)
	{
	}

std::string get_aliases() const
{
	Ensurer obj(*this);
	return obj->get_aliases();
}

void set_aliases(const char* aliases)
{
	Ensurer obj(*this);
	obj->set_aliases(aliases);
}

std::string get_description() const
{
	Ensurer obj(*this);
	return obj->get_description();
}

void set_description(const char* description)
{
	Ensurer obj(*this);
	obj->set_description(description);
}
std::string get_short_description() const
{
	Ensurer obj(*this);
	return obj->get_short_description();
}

void set_short_description(const char* short_description)
{
	Ensurer obj(*this);
	obj->set_short_description(short_description);
}
std::string get_action_description() const
{
	Ensurer obj(*this);
	return obj->get_action_description();
}

void set_action_description(const char* action_description)
{
	Ensurer obj(*this);
	obj->set_action_description(action_description);
}
std::string get_pad(const unsigned pad) const
{
	Ensurer obj(*this);
	if (pad < 6)
	{
		return obj->get_PName(pad);
	}

	return "";
}

void set_pad(const unsigned pad, const char* s)
{
	if (pad >= 6)
	{
		return;
	}
	Ensurer obj(*this);
	obj->set_PName(pad, s);
}

int get_value(const unsigned i) const
{
	if (i >= OBJ_DATA::VALS_COUNT)
	{
		PyErr_SetString(PyExc_ValueError, "argument out of range");
		throw_error_already_set();
	}
	Ensurer obj(*this);
	return obj->get_val(i);
}

void set_value(const int i, const int v)
{
	if (i >= OBJ_DATA::VALS_COUNT)
	{
		PyErr_SetString(PyExc_ValueError, "argument out of range");
		throw_error_already_set();
	}
	Ensurer obj(*this);
	obj->set_val(i, v);
}

int get_obj_type() const
{
	Ensurer obj(*this);
	return obj->get_type();
}

void set_obj_type(const int v)
{
	Ensurer obj(*this);
	obj->set_type(static_cast<OBJ_DATA::EObjectType>(v));
}

int get_wear_flags() const
{
	Ensurer obj(*this);
	return obj->get_wear_flags();
}

void set_wear_flags(const int v)
{
	Ensurer obj(*this);
	obj->set_wear_flags(v);
}

unsigned get_weight() const
{
	Ensurer obj(*this);
	return obj->get_weight();
}

void set_weight(const unsigned v)
{
	Ensurer obj(*this);
	obj->set_weight(v);
}
unsigned get_cost() const
{
	Ensurer obj(*this);
	return obj->get_cost();
}

void set_cost(const unsigned v)
{
	Ensurer obj(*this);
	obj->set_cost(v);
}
unsigned get_cost_per_day_on() const
{
	Ensurer obj(*this);
	return obj->get_rent_on();
}

void set_cost_per_day_on(const unsigned v)
{
	Ensurer obj(*this);
	obj->set_rent_on(v);
}
unsigned get_cost_per_day_off() const
{
	Ensurer obj(*this);
	return obj->get_rent_off();
}

void set_cost_per_day_off(const unsigned v)
{
	Ensurer obj(*this);
	obj->set_rent_off(v);
}
int get_sex() const
{
	Ensurer obj(*this);
	return to_underlying(obj->get_sex());
}

int get_timer() const
{
	Ensurer obj(*this);
	return obj->get_timer();
}

void set_timer(const int timer) const
{
	Ensurer obj(*this);
	obj->set_timer(timer);
}

void set_sex(const int v)
{
	Ensurer obj(*this);
	obj->set_sex(static_cast<ESex>(v));
}
int get_spell() const
{
	Ensurer obj(*this);
	return obj->get_spell();
}

void set_spell(const int v)
{
	Ensurer obj(*this);
	obj->set_spell(v);
}
int get_level() const
{
	Ensurer obj(*this);
	return obj->get_level();
}

void set_level(const int v)
{
	Ensurer obj(*this);
	obj->set_level(v);
}
int get_skill() const
{
	Ensurer obj(*this);
	return obj->get_skill();
}

void set_skill(const int v)
{
	Ensurer obj(*this);
	obj->set_skill(v);
}
int get_max() const
{
	Ensurer obj(*this);
	return obj->get_maximum_durability();
}

void set_max(const int v)
{
	Ensurer obj(*this);
	obj->set_maximum_durability(v);
}
int get_cur() const
{
	Ensurer obj(*this);
	return obj->get_current_durability();
}

void set_cur(const int v)
{
	Ensurer obj(*this);
	obj->set_current_durability(v);
}

int get_mater() const
{
	Ensurer obj(*this);
	return obj->get_material();
}

void set_mater(const int v)
{
	Ensurer obj(*this);
	obj->set_material(static_cast<OBJ_DATA::EObjectMaterial>(v));
}
int get_owner() const
{
	Ensurer obj(*this);
	return obj->get_owner();
}

void set_owner(const int v)
{
	Ensurer obj(*this);
	obj->set_owner(v);
}

int get_maker() const
{
	Ensurer obj(*this);
	return obj->get_crafter_uid();
}

void set_maker(const int v)
{
	Ensurer obj(*this);
	obj->set_crafter_uid(v);
}

int get_destroyer() const
{
	Ensurer obj(*this);
	return obj->get_destroyer();
}

void set_destroyer(const int v)
{
	Ensurer obj(*this);
	obj->set_destroyer(v);
}

int get_zone() const
{
	Ensurer obj(*this);
	return obj->get_zone();
}

void set_zone(const int v)
{
	Ensurer obj(*this);
	obj->set_zone(v);
}

FLAG_DATA get_affects() const
{
	Ensurer obj(*this);
	return obj->get_affect_flags();
}

void set_affects(const FLAG_DATA& f)
{
	Ensurer obj(*this);
	obj->set_affect_flags(f);
}

FLAG_DATA get_anti_flag() const
{
	Ensurer obj(*this);
	return obj->get_anti_flags();
}

void set_anti_flag(const FLAG_DATA& f)
{
	Ensurer obj(*this);
	obj->set_anti_flags(f);
}

FLAG_DATA get_no_flag() const
{
	Ensurer obj(*this);
	return obj->get_no_flags();
}

void set_no_flag(const FLAG_DATA& f)
{
	Ensurer obj(*this);
	obj->set_no_flags(f);
}

FLAG_DATA get_extra_flags() const
{
	Ensurer obj(*this);
	return obj->get_extra_flags();
}

void set_extra_flags(const FLAG_DATA& f)
{
	Ensurer obj(*this);
	obj->set_extra_flags(f);
}

const affected_t& get_affected()
{
	Ensurer obj(*this);
	return obj->get_all_affected();
}

object get_carried_by() const
{
	Ensurer obj(*this);
	if (obj->get_carried_by())
	{
		return object(CharacterWrapper(obj->get_carried_by()));
	}
	return object();
}

object get_worn_by() const
{
	Ensurer obj(*this);
	if (obj->get_worn_by())
	{
		return object(CharacterWrapper(obj->get_worn_by()));
	}
	return object();
}

obj_rnum get_item_number() const
{
	Ensurer obj(*this);
	return obj->get_rnum();
}

void set_item_number(const obj_rnum n)
{
	Ensurer obj(*this);
	obj->set_rnum(n);
}

obj_vnum get_vnum() const {
	Ensurer obj(*this);
	return GET_OBJ_VNUM(obj);
}
};

ObjWrapper get_obj_proto(const obj_rnum rnum)
{
	if (rnum >= 0 && rnum < static_cast<int>(obj_proto.size()))
	{
		return obj_proto[rnum];
	}
	PyErr_SetString(PyExc_ValueError, "obj rnum is out of range");
	throw_error_already_set();
	return ObjWrapper(static_cast<OBJ_DATA *>(nullptr));
}

object get_char_equipment(const CharacterWrapper& c, const unsigned num)
{
	if (num >= NUM_WEARS)
	{
			PyErr_SetString(PyExc_ValueError, "invalid wear slot");
		throw_error_already_set();
	}
	CharacterWrapper::Ensurer ch(c);
	OBJ_DATA* r = ch->equipment[num];
	if (!r)
		return object();
	else
		return object(ObjWrapper(r));
}



void obj_to_char_wrap(const CharacterWrapper& c, ObjWrapper& o)
{
	CharacterWrapper::Ensurer ch(c);
	ObjWrapper::Ensurer obj(o);
	obj_to_char(obj, ch);
}

bool flag_is_set(const FLAG_DATA& flag, const unsigned f)
{
	return flag.get(f);
}

void flag_set(FLAG_DATA& flag, const unsigned f)
{
	flag.set(f);
}

void flag_remove(FLAG_DATA& flag, const unsigned f)
{
	flag.unset(f);
}

void flag_toggle(FLAG_DATA& flag, const unsigned f)
{
	flag.toggle(f);
}

str flag_str(const FLAG_DATA& flag)
{
	char buf[MAX_STRING_LENGTH];
	*buf='\0';
	flag.tascii(4, buf);
	return str(buf);
}

object obj_affected_type_str(const obj_affected_type& affect)
{
	char buf[MAX_STRING_LENGTH];
	sprinttype(affect.location, apply_types, buf);
	bool negative = false;
	for (int j = 0; *apply_negative[j] != '\n'; j++)
	{
		if (!str_cmp(buf2, apply_negative[j]))
		{
			negative = true;
			break;
		}
	}
	if (!negative && affect.modifier < 0)
	{
		negative = true;
	}
	else if (negative && affect.modifier < 0)
	{
		negative = false;
	}
	return str("%s%s%d") %
	make_tuple(buf,
	negative ? str(" �������� �� ") : str(" �������� �� "),
	affect.modifier>=0 ? affect.modifier : -affect.modifier);
}

void call_later(object callable)
{
	objs_to_call_in_main_thread.push_back(callable);
}

boost::python::list get_players()
{
	boost::python::list tmp_list;
	DESCRIPTOR_DATA *d;
	for (d = descriptor_list; d; d = d->next)
	{
		if (STATE(d) == CON_PLAYING)
		{
			tmp_list.append(CharacterWrapper(d->character));
		}
	}
	return tmp_list;
}

bool check_ingame(std::string name)
{
	DESCRIPTOR_DATA *d;
	for (d = descriptor_list; d; d = d->next)
	{
		if (STATE(d) == CON_PLAYING)
		{
			if (d->character->get_name_str() == name)
			{
				return true;
			}
		}
	}
	return false;
}

void char_to_room_wrap(CharacterWrapper& c, int vnum)
{
	CharacterWrapper::Ensurer ch(c);
	room_rnum location;
	if (((location = real_room(vnum)) == NOWHERE))
	{
		log("[PythonError] Error in char_to_room_wrap. %d vnum invalid.", vnum);
		return;
	}
	char_from_room(ch);
	char_to_room(ch, location);
	check_horse(ch);
	look_at_room(ch, 0);

}

BOOST_PYTHON_MODULE(mud)
{
	def("get_players", get_players, "return players online");
	def("check_ingame", check_ingame, "check player in game");
	def("log", mudlog_python, ( py::arg("msg"), py::arg("msg_type")=DEF, py::arg("level")=LVL_IMMORT, py::arg("channel")=static_cast<int>(SYSLOG), py::arg("to_file")=TRUE ) ,
	"Записывает сообщение msg типа msg_type в канал лога channel для уровня level.\n"
	"\n"
	"msg_type принимает значения констант из utils.h, defines for mudlog.\n"
	"channel  канал, в который будет записано сообщение (comm.h). в настоящее время может принимать значения constants.SYSLOG, constants.ERRLOG и constants.IMLOG.\n"
	"to_file  Записывать ли сообщение так же в файл, помимо вывода его иммам");
	def("send_all", send_to_all, (py::arg("msg")),
"Шлет сообщение msg всем игрокам.");
	def("find_skill_num", find_skill_num, "Возвращает номер скила по его названию.");
	def("find_spell_num", find_spell_num, "Возвращает номер спелла по его названию.");
	def("get_mob_proto", get_mob_proto, "Возвращает моба из базы прототипов с заданым rnum.");
	def("get_obj_proto", get_obj_proto, "Возвращает объект из базы прототипов с заданым rnum.");
	def("get_mob_rnum", real_mobile, "Возвращает rnum моба по vnum.\n"
	"\n"
	"rnum - это индекс в базе прототипов.\n"
	"vnum - это виртуальный номер моба, который задается билдером.");
	def("get_obj_rnum", real_object, "Возвращает rnum объекта по vnum."
	"\n"
	"См. комментарии к get_mob_rnum.");
	def("get_room_rnum", real_room, "Возвращает rnum комнаты по vnum."
	"\n"
	"См. комментарии к get_mob_rnum.");
	def("create_mob", create_mob_from_proto, create_mob_from_proto_overloads((py::args("proto_num"), py::args("virtual")=true), "Создает экземпляр моба из заданного прототипа.\n"
	"\n"
	"num  виртуальный (vnum) или реальный (rnum) прототипа.\n"
	"virtual  определяет смысл первого параметра. True - задан vnum, False - задан rnum.\n"
	"После создания, моба нужно поместить куданибудь (например установкой поля in_room)."));
	def("register_global_command", register_global_command, (py::arg("command"), py::arg("command_koi8-r"), py::arg("func"), py::arg("minimum_position"), py::arg("minimum_level"), py::arg("unhide_percent")=-1), "Регистрирует игровую команду, реализованную на питоне.\n"
	"\n"
	"command - полное сллово команды\n"
	"func - питоновская функция или любой вызываемый объект. Вызывается с тремя параметрами: чар, строка команды, строка аргументов.\n"
	"minimum_position - позиция, начиная с которой команду можно выполнять. constants.POS_XXX\n"
	"minimum_level - мин. уровень игрока для выполнения команды\n"
	"unhide_percent - вероятность разхайдиться");
	def("unregister_global_command", unregister_global_command, "Отменяет регистрацию игровой команды, реализованной на питоне.");
	def("call_later", call_later, py::arg("callable"),
		"Сохраняет переданую функцию для выполнения в в основном цикле сервера позже.");
	ObjectDoesNotExist = handle<>(PyErr_NewException((char*)"mud.ObjectDoesNotExist", PyExc_RuntimeError, NULL));
	scope().attr("ObjectDoesNotExist") = ObjectDoesNotExist;
	class_<CharacterWrapper>("Character", "Игровой персонаж.", no_init)
		.def("char_from_room", char_to_room_wrap, "")
		.def("obj_to_char", obj_to_char_wrap, "передает объект чару.")
		.def("send", &CharacterWrapper::send, "Посылает персонажу заданную строку.")
		.def("page_string", &CharacterWrapper::_page_string, "Отправляет строку персонажу с возможностью постраничного просмотра.")
		.def("act", &CharacterWrapper::act_on_char, (py::arg("msg"), py::arg("hide_invisible")=false, py::arg("obj")=NULL, py::arg("victim")=NULL, py::arg("act_type")=TO_CHAR), "Классический act (см. comm.cpp).")
		.def("act", &CharacterWrapper::act_on_obj)
		.def("interpret", &CharacterWrapper::interpret, "Персонаж выполняет заданную игровую команду.")
		.add_property("name", &CharacterWrapper::get_name, &CharacterWrapper::set_name)
		.def("get_pad", &CharacterWrapper::get_pad, "Получить имя в указанном падеже (0-5).")
		.def("set_pad", &CharacterWrapper::set_pad, "Установить указанный падеж (0-5).")
		.add_property("long_descr", &CharacterWrapper::get_long_descr, &CharacterWrapper::set_long_descr, "Описание моба, отображаемое в комнате.")
		.add_property("description", &CharacterWrapper::get_description, &CharacterWrapper::set_description, "Описание моба, видное по команде 'см моб'.")
		.add_property("class", &CharacterWrapper::get_class, &CharacterWrapper::set_class)
		.add_property("level", &CharacterWrapper::get_level, &CharacterWrapper::set_level)
		.add_property("UID", &CharacterWrapper::get_uid)
		.add_property("exp", &CharacterWrapper::get_exp, &CharacterWrapper::set_exp)
		.add_property("remort", &CharacterWrapper::get_remort)
		.add_property("gold", &CharacterWrapper::get_gold, &CharacterWrapper::set_gold)
		.add_property("bank", &CharacterWrapper::get_bank, &CharacterWrapper::set_bank)
		.def("remove_gold", &CharacterWrapper::remove_gold)
		.def("remove_bank", &CharacterWrapper::remove_bank)
		.def("remove_both_gold", &CharacterWrapper::remove_both_gold, "Пытается снять указанную суму у персонажа с рук, и, в случае нехватки наличности, с банковского счета.")
		.def("add_gold", &CharacterWrapper::add_gold)
		.def("add_bank", &CharacterWrapper::add_bank)
		.add_property("total_gold", &CharacterWrapper::get_total_gold, "Суммарное количество денег у персонажа (банк + наличка).")
		.add_property("str", &CharacterWrapper::get_str, &CharacterWrapper::set_str)
		.add_property("dex", &CharacterWrapper::get_dex, &CharacterWrapper::set_dex)
		.add_property("con", &CharacterWrapper::get_con, &CharacterWrapper::set_con)
		.add_property("wis", &CharacterWrapper::get_wis, &CharacterWrapper::set_wis)
		.add_property("int", &CharacterWrapper::get_int, &CharacterWrapper::set_int)
		.add_property("cha", &CharacterWrapper::get_cha, &CharacterWrapper::set_cha)
		.add_property("email", &CharacterWrapper::get_email)
		.add_property("sex", &CharacterWrapper::get_sex, &CharacterWrapper::set_sex, "Пол персонажа. значения из constants.SEX_XXX")
		.add_property("weight", &CharacterWrapper::get_weight, &CharacterWrapper::set_weight, "Вес")
		.add_property("height", &CharacterWrapper::get_height, &CharacterWrapper::set_height, "рост")
		.add_property("religion", &CharacterWrapper::get_religion, &CharacterWrapper::set_religion, "Религиозная направленность. 0 - политеизм, 1 - монотеизм.")
		.add_property("kin", &CharacterWrapper::get_kin, &CharacterWrapper::set_kin, "племя")
		.add_property("race", &CharacterWrapper::get_race, &CharacterWrapper::set_race, "род")
		.add_property("hit", &CharacterWrapper::get_hit, &CharacterWrapper::set_hit, "Текущее количество единиц жизни")
		.add_property("max_hit", &CharacterWrapper::get_max_hit, &CharacterWrapper::set_max_hit, "Максимальное количество единиц жизни")
		.add_property("move", &CharacterWrapper::get_move, &CharacterWrapper::set_move, "Текущее количество единиц движения")
		.add_property("max_move", &CharacterWrapper::get_max_move, &CharacterWrapper::set_max_move, "Максимальное колличество единиц энергии")
		//.add_property("master", make_getter(&CharacterWrapper::master, return_value_policy<reference_existing_object>()), make_setter(Character::master, ))
		.def("clan_status", &CharacterWrapper::clan_status, "get clan status")
		.def("followers", &CharacterWrapper::get_followers, "Возвращает список последователей персонажа.")
		.add_property("is_immortal", &CharacterWrapper::is_immortal)
		.add_property("is_impl", &CharacterWrapper::is_impl)
		.add_property("is_NPC", &CharacterWrapper::is_NPC)
		.def("get_skill", &CharacterWrapper::get_skill, "Возвращает уровень владения заданым умением.\n"
		"\n"
		"Скилл с учетом всех плюсов и минусов от шмоток/яда.\n"
		"Для получения номера умения по названию см. get_skill_num.")
		.def("set_skill", &CharacterWrapper::set_skill, "Устанавливает уровень владения заданным умением.")
		.def("clear_skills", &CharacterWrapper::clear_skills, "Очищает список  умений.")
		.add_property("skills_count", &CharacterWrapper::get_skills_count, "Содержит число умений, которыми владеет персонаж.")
		.def("get_equipped_skill", &CharacterWrapper::get_equipped_skill, "Скилл со шмоток.")
		.def("get_trained_skill", &CharacterWrapper::get_trained_skill, "Родной тренированный скилл чара.")
		.def("get_spell", &CharacterWrapper::get_spell, "Владеет ли персонаж указанным заклинанием.\n"
		"\n"
		"Для получения номера заклинания по названию см. get_spell_num.")
		.def("set_spell", &CharacterWrapper::set_spell, "Устанавливает изученность заклинания у персонажа.")
		.add_property("rnum", &CharacterWrapper::get_nr, &CharacterWrapper::set_nr, "Реальный номер персонажа в таблице прототипов.")
		.add_property("in_room", &CharacterWrapper::get_in_room, &CharacterWrapper::set_in_room, "rnum комнаты, в которой находится персонаж.\n"
		"\n"
		"При изменении этого свойства персонаж переносится в заданную комнату.")
		.def("affected_by_spell", &CharacterWrapper::is_affected_by_spell, "Подвержен ли персонаж действию указанного заклинания.")
		.def("add_affect", &CharacterWrapper::add_affect, "Наложить на персонажа заданный аффект.")
		.def("get_char_vis", &CharacterWrapper::get_vis, "Ищет указанного персонажа с учетом видимости.\n"
		"\n"
		"Последний параметр принимает значения из constants.FIND_XXX.")
		.def("get_equipment", get_char_equipment, "Возвращает объект из заданного слота экипировки персонажа.")
		.def("restore", &CharacterWrapper::restore, "Восттанавливает очки жизни, энергии и ману/мем.")
		.def("quested_add", &CharacterWrapper::quested_add, "Добавление выполненного квеста номер/строка данных (128 символов).")
		.def("quested_remove", &CharacterWrapper::quested_remove, "Удаление информации о квесте.")
		.def("quested_get", &CharacterWrapper::quested_get_text, "Возвращает строку квестовой информации, сохраненной под заданым номером vnum.")
		.add_property("quested_text", &CharacterWrapper::quested_print, "Вся информация по квестам в текстовом виде.")
		.add_property("wait", &CharacterWrapper::get_wait, &CharacterWrapper::set_wait, "Сколько циклов ждать")

	;

	class_<affected_t>("ObjAffectedArray", "Массив из шести модификаторов объекта.", no_init)
		.def("__len__", &_arrayN<obj_affected_type, MAX_OBJ_AFFECT>::size)
		.def("__getitem__",&_arrayN<obj_affected_type, MAX_OBJ_AFFECT>::get)
		.def("__getslice__",&_arrayN<obj_affected_type, MAX_OBJ_AFFECT>::getslice)
		.def("__setslice__",&_arrayN<obj_affected_type, MAX_OBJ_AFFECT>::setslice)
		.def("__setitem__",&_arrayN<obj_affected_type, MAX_OBJ_AFFECT>::set)
		.def("values", &_arrayN<obj_affected_type, MAX_OBJ_AFFECT>::values)
	;

	//wraps obj_data (see obj.hpp)
	class_<ObjWrapper>("Object", "Игровой объект (вещь).", no_init)
	.add_property("name", &ObjWrapper::get_aliases, &ObjWrapper::set_aliases, "Алиасы объекта")
	.add_property("description", &ObjWrapper::get_description, &ObjWrapper::set_description, "Описание объекта когда лежит.")
	.add_property("short_description", &ObjWrapper::get_short_description, &ObjWrapper::set_short_description, "название объекта (именительный падеж)")
	.add_property("action_description", &ObjWrapper::get_action_description, &ObjWrapper::set_action_description, "Описание при действии (палочки и т.п.)")
	.def("get_pad", &ObjWrapper::get_pad, "Получает название предмета в заданном падеже.")
	.def("set_pad", &ObjWrapper::set_pad, "Устанавливает заданный падеж предмета в заданное значение.")
	.add_property("item_type", &ObjWrapper::get_obj_type, &ObjWrapper::set_obj_type, "Тип предмета (значения из constants.ITEM_XXX)")
	.def("get_value", &ObjWrapper::get_value, "Получить поле из значений предмета (0-3).\n"
	"\n"
	"Поля значений имеют разное использование для разных типов предметов.\n")
	.def("set_value", &ObjWrapper::set_value, "Установить поле значения предмета (поля 0-3)")
	.add_property("wear_flags", &ObjWrapper::get_wear_flags, &ObjWrapper::set_wear_flags, "Маска флагов одевания (куда предмет может быт одет?)")
	.add_property("weight", &ObjWrapper::get_weight, &ObjWrapper::set_weight, "Вес")
	.add_property("cost", &ObjWrapper::get_cost, &ObjWrapper::set_cost, "Цена при продаже в магазине")
	.add_property("cost_per_day_on", &ObjWrapper::get_cost_per_day_on, &ObjWrapper::set_cost_per_day_on, "Цена ренты, когда предмет одет в экипировке")
	.add_property("cost_per_day_off", &ObjWrapper::get_cost_per_day_off, &ObjWrapper::set_cost_per_day_off, "Цена ренты, когда предмет не в экипировке")
		.add_property("sex", &ObjWrapper::get_sex, &ObjWrapper::set_sex, "Пол предмета")
		.add_property("spell", &ObjWrapper::get_spell, &ObjWrapper::set_spell)
		.add_property("level", &ObjWrapper::get_level, &ObjWrapper::set_level)
		.add_property("skill", &ObjWrapper::get_skill, &ObjWrapper::set_skill)
		.add_property("timer", &ObjWrapper::get_timer, &ObjWrapper::set_timer, "Таймер объекта")
		.add_property("max", &ObjWrapper::get_max, &ObjWrapper::set_max, "Максимальная прочность")
		.add_property("cur", &ObjWrapper::get_cur, &ObjWrapper::set_cur, "Текущая прочность")
		.add_property("material", &ObjWrapper::get_mater, &ObjWrapper::set_mater, "Материал")
		.add_property("owner", &ObjWrapper::get_owner, &ObjWrapper::set_owner, "ID владельца (для ковки?)")
		.add_property("destroyer", &ObjWrapper::get_destroyer, &ObjWrapper::set_destroyer)
		.add_property("maker", &ObjWrapper::get_maker, &ObjWrapper::set_maker, "ID крафтера (?)")
		.add_property("zone", &ObjWrapper::get_zone, &ObjWrapper::set_zone, "rnum зоны, из которой предмет родом")
		.add_property("rnum", &ObjWrapper::get_item_number, &ObjWrapper::set_item_number, "Реальный номер объекта, являющийся индексом в таблице прототипов.")
		.def("vnum", &ObjWrapper::get_vnum, "виртуальный номер объекта-прототипа.")
		.add_property("affects", &ObjWrapper::get_affects, &ObjWrapper::set_affects, "Накладываемые аффекты")
		.add_property("extra_flags", &ObjWrapper::get_extra_flags, &ObjWrapper::set_extra_flags, "Экстрафлаги (шумит, горит и т.п.)")
		.add_property("no_flags", &ObjWrapper::get_no_flag, &ObjWrapper::set_no_flag)
		.add_property("anti_flags", &ObjWrapper::get_anti_flag, &ObjWrapper::set_anti_flag)
		.add_property("modifiers", make_function(&ObjWrapper::get_affected, return_internal_reference<>()), "Массив модификаторов (XXX улучшает на YYY)")
		.add_property("carried_by", &ObjWrapper::get_carried_by, "У кого предмет в инвентаре")
		.add_property("worn_by", &ObjWrapper::get_worn_by, "На ком предмет одет в экипировке")
	;

	//implicitly_convertible<Character*, CharacterWrapper>();
	implicitly_convertible<CharacterWrapper, CHAR_DATA* >();

	class_<CharacterListWrapper::iterator>("CharacterListIterator", "Итератор по списку mud.character_list", no_init)
		.def("next", &CharacterListWrapper::iterator::next);
	scope().attr("character_list") = class_<CharacterListWrapper>("CharacterListWrapper", "Список всех персонажей в игре.")
		.def("__iter__", &CharacterListWrapper::iter, return_value_policy<manage_new_object>())
		.staticmethod("__iter__")
	();

	class_<AFFECT_DATA<EApplyLocation>, std::auto_ptr<AFFECT_DATA<EApplyLocation>> >("Affect", "Игровой аффект.")
		.def_readwrite("spell_type", &AFFECT_DATA<EApplyLocation>::type, "Номер заклинания, которое наложило этот аффект")
		.add_property("spell_type_str", get_spell_type_str, "Название заклинания (только для чтения)")
		.add_property("location_str", get_location_str, "Название изменяемого параметра (только для чтения)")
		.add_property("bitvector_str", get_bitvector_str, "Какие аффекты накладываем (в текстовом виде, только для чтения)")
		.def_readwrite("duration", &AFFECT_DATA<EApplyLocation>::duration, "Продолжительность в секундах")
		.def_readwrite("modifier", &AFFECT_DATA<EApplyLocation>::modifier, "Величина изменения параметра")
		.def_readwrite("location", &AFFECT_DATA<EApplyLocation>::location, "Изменяемый параметр (constants.APLY_XXX)")
		.def_readwrite("battleflag", &AFFECT_DATA<EApplyLocation>::battleflag, "Флаг для боя (холд, и т.п.)")
		.def_readwrite("bitvector", &AFFECT_DATA<EApplyLocation>::bitvector, "Накладываемые аффекты")
		.def_readwrite("caster_id", &AFFECT_DATA<EApplyLocation>::caster_id, "ID скастовавшего")
		.def_readwrite("apply_time", &AFFECT_DATA<EApplyLocation>::apply_time, "Указывает сколько аффект висит (пока используется только в комнатах)")
	;

	class_<FLAG_DATA>("FlagData", "Флаги чего-нибудь.")
		.def("__contains__", flag_is_set, "Содержится ли флаг в этом поле?")
		.def("set", flag_set, "Установить указанный флаг")
		.def("remove", flag_remove, "Убрать указанный флаг")
		.def("toggle", flag_toggle, "Переключить указанный флаг")
		.def("__str__", flag_str)
	;

	class_<obj_affected_type>("ObjectModifier", "Модификатор персонажа, накладываемый объектом.")
	.def(py::init<EApplyLocation, int>(py::args("location", "modifier")))
		.def_readwrite("location", &obj_affected_type::location, "Атрибут, который изменяем")
		.def_readwrite("modifier", &obj_affected_type::modifier, "Величина изменения")
		.def("__str__", obj_affected_type_str, "Переводит модификатор в строку вида (XXX улучшает на YYY)")
	;
}


BOOST_PYTHON_MODULE(constants)
{
	//log channels
	DEFINE_ENUM_CONSTANT(SYSLOG);
	DEFINE_ENUM_CONSTANT(ERRLOG);
	DEFINE_ENUM_CONSTANT(IMLOG);

	//act
	DEFINE_CONSTANT(TO_ROOM);
	DEFINE_CONSTANT(TO_VICT);
	DEFINE_CONSTANT(TO_NOTVICT);
	DEFINE_CONSTANT(TO_CHAR);
	DEFINE_CONSTANT(TO_ROOM_HIDE);
	DEFINE_CONSTANT(CHECK_NODEAF);
	DEFINE_CONSTANT(CHECK_DEAF);
	DEFINE_CONSTANT(TO_SLEEP);

		DEFINE_CONSTANT(NOHOUSE);
    DEFINE_CONSTANT(NOWHERE);
    DEFINE_CONSTANT(NOTHING);
    DEFINE_CONSTANT(NOBODY);
    DEFINE_CONSTANT(FORMAT_INDENT);
    DEFINE_CONSTANT(KT_ALT);
    DEFINE_CONSTANT(KT_WIN);
    DEFINE_CONSTANT(KT_WINZ);
    DEFINE_CONSTANT(KT_WINZ6);
    DEFINE_CONSTANT(KT_UTF8);
    DEFINE_CONSTANT(KT_LAST);
    DEFINE_CONSTANT(KT_SELECTMENU);
    DEFINE_CONSTANT(HOLES_TIME);
    DEFINE_CONSTANT(NORTH);
    DEFINE_CONSTANT(EAST);
    DEFINE_CONSTANT(SOUTH);
    DEFINE_CONSTANT(WEST);
    DEFINE_CONSTANT(UP);
    DEFINE_CONSTANT(DOWN);
    DEFINE_CONSTANT(ROOM_DARK);
    DEFINE_CONSTANT(ROOM_DEATH);
    DEFINE_CONSTANT(ROOM_NOMOB);
    DEFINE_CONSTANT(ROOM_INDOORS);
    DEFINE_CONSTANT(ROOM_PEACEFUL);
    DEFINE_CONSTANT(ROOM_SOUNDPROOF);
    DEFINE_CONSTANT(ROOM_NOTRACK);
    DEFINE_CONSTANT(ROOM_NOMAGIC);
    DEFINE_CONSTANT(ROOM_TUNNEL);
    DEFINE_CONSTANT(ROOM_NOTELEPORTIN);
    DEFINE_CONSTANT(ROOM_GODROOM);
    DEFINE_CONSTANT(ROOM_HOUSE);
    DEFINE_CONSTANT(ROOM_HOUSE_CRASH);
    DEFINE_CONSTANT(ROOM_ATRIUM);
    DEFINE_CONSTANT(ROOM_OLC);
    DEFINE_CONSTANT(ROOM_BFS_MARK);
    DEFINE_CONSTANT(ROOM_MAGE);
    DEFINE_CONSTANT(ROOM_CLERIC);
    DEFINE_CONSTANT(ROOM_THIEF);
    DEFINE_CONSTANT(ROOM_WARRIOR);
    DEFINE_CONSTANT(ROOM_ASSASINE);
    DEFINE_CONSTANT(ROOM_GUARD);
    DEFINE_CONSTANT(ROOM_PALADINE);
    DEFINE_CONSTANT(ROOM_RANGER);
    DEFINE_CONSTANT(ROOM_POLY);
    DEFINE_CONSTANT(ROOM_MONO);
    DEFINE_CONSTANT(ROOM_SMITH);
    DEFINE_CONSTANT(ROOM_MERCHANT);
    DEFINE_CONSTANT(ROOM_DRUID);
    DEFINE_CONSTANT(ROOM_ARENA);
    DEFINE_CONSTANT(ROOM_NOSUMMON);
    DEFINE_CONSTANT(ROOM_NOTELEPORTOUT);
    DEFINE_CONSTANT(ROOM_NOHORSE);
    DEFINE_CONSTANT(ROOM_NOWEATHER);
    DEFINE_CONSTANT(ROOM_SLOWDEATH);
    DEFINE_CONSTANT(ROOM_ICEDEATH);
    DEFINE_CONSTANT(ROOM_NORELOCATEIN);
    DEFINE_CONSTANT(ROOM_ARENARECV);
    DEFINE_CONSTANT(ROOM_ARENASEND);
    DEFINE_CONSTANT(ROOM_NOBATTLE);
    DEFINE_CONSTANT(ROOM_NOITEM);
    DEFINE_CONSTANT(ROOM_RUSICHI);
    DEFINE_CONSTANT(ROOM_VIKINGI);
    DEFINE_CONSTANT(ROOM_STEPNYAKI);
    DEFINE_CONSTANT(AFF_ROOM_LIGHT);
    DEFINE_CONSTANT(AFF_ROOM_FOG);
    DEFINE_CONSTANT(AFF_ROOM_RUNE_LABEL);
    DEFINE_CONSTANT(AFF_ROOM_FORBIDDEN);
    DEFINE_CONSTANT(AFF_ROOM_HYPNOTIC_PATTERN);
    DEFINE_CONSTANT(AFF_ROOM_EVARDS_BLACK_TENTACLES);
    DEFINE_CONSTANT(EX_ISDOOR);
    DEFINE_CONSTANT(EX_CLOSED);
    DEFINE_CONSTANT(EX_LOCKED);
    DEFINE_CONSTANT(EX_PICKPROOF);
    DEFINE_CONSTANT(EX_HIDDEN);
    DEFINE_CONSTANT(EX_BROKEN);
    DEFINE_CONSTANT(AF_BATTLEDEC);
    DEFINE_CONSTANT(AF_DEADKEEP);
    DEFINE_CONSTANT(AF_PULSEDEC);
    DEFINE_CONSTANT(AF_SAME_TIME);
    DEFINE_CONSTANT(SECT_INSIDE);
    DEFINE_CONSTANT(SECT_CITY);
    DEFINE_CONSTANT(SECT_FIELD);
    DEFINE_CONSTANT(SECT_FOREST);
    DEFINE_CONSTANT(SECT_HILLS);
    DEFINE_CONSTANT(SECT_MOUNTAIN);
    DEFINE_CONSTANT(SECT_WATER_SWIM);
    DEFINE_CONSTANT(SECT_WATER_NOSWIM);
    DEFINE_CONSTANT(SECT_FLYING);
    DEFINE_CONSTANT(SECT_UNDERWATER);
    DEFINE_CONSTANT(SECT_SECRET);
    DEFINE_CONSTANT(SECT_STONEROAD);
    DEFINE_CONSTANT(SECT_ROAD);
    DEFINE_CONSTANT(SECT_WILDROAD);
    DEFINE_CONSTANT(SECT_FIELD_SNOW);
    DEFINE_CONSTANT(SECT_FIELD_RAIN);
    DEFINE_CONSTANT(SECT_FOREST_SNOW);
    DEFINE_CONSTANT(SECT_FOREST_RAIN);
    DEFINE_CONSTANT(SECT_HILLS_SNOW);
    DEFINE_CONSTANT(SECT_HILLS_RAIN);
    DEFINE_CONSTANT(SECT_MOUNTAIN_SNOW);
    DEFINE_CONSTANT(SECT_THIN_ICE);
    DEFINE_CONSTANT(SECT_NORMAL_ICE);
    DEFINE_CONSTANT(SECT_THICK_ICE);
    DEFINE_CONSTANT(WEATHER_QUICKCOOL);
    DEFINE_CONSTANT(WEATHER_QUICKHOT);
    DEFINE_CONSTANT(WEATHER_LIGHTRAIN);
    DEFINE_CONSTANT(WEATHER_MEDIUMRAIN);
    DEFINE_CONSTANT(WEATHER_BIGRAIN);
    DEFINE_CONSTANT(WEATHER_GRAD);
    DEFINE_CONSTANT(WEATHER_LIGHTSNOW);
    DEFINE_CONSTANT(WEATHER_MEDIUMSNOW);
    DEFINE_CONSTANT(WEATHER_BIGSNOW);
    DEFINE_CONSTANT(WEATHER_LIGHTWIND);
    DEFINE_CONSTANT(WEATHER_MEDIUMWIND);
    DEFINE_CONSTANT(WEATHER_BIGWIND);
    DEFINE_CONSTANT(MAX_REMORT);
    DEFINE_CONSTANT(CLASS_MOB);
    DEFINE_CONSTANT(MASK_BATTLEMAGE);
    DEFINE_CONSTANT(MASK_CLERIC);
    DEFINE_CONSTANT(MASK_THIEF);
    DEFINE_CONSTANT(MASK_WARRIOR);
    DEFINE_CONSTANT(MASK_ASSASINE);
    DEFINE_CONSTANT(MASK_GUARD);
    DEFINE_CONSTANT(MASK_DEFENDERMAGE);
    DEFINE_CONSTANT(MASK_CHARMMAGE);
    DEFINE_CONSTANT(MASK_NECROMANCER);
    DEFINE_CONSTANT(MASK_PALADINE);
    DEFINE_CONSTANT(MASK_RANGER);
    DEFINE_CONSTANT(MASK_SMITH);
    DEFINE_CONSTANT(MASK_MERCHANT);
    DEFINE_CONSTANT(MASK_DRUID);
    DEFINE_CONSTANT(MASK_MAGES);
    DEFINE_CONSTANT(MASK_CASTER);
    DEFINE_CONSTANT(RELIGION_POLY);
    DEFINE_CONSTANT(RELIGION_MONO);
    DEFINE_CONSTANT(MASK_RELIGION_POLY);
    DEFINE_CONSTANT(MASK_RELIGION_MONO);
    DEFINE_CONSTANT(NUM_KIN);
    DEFINE_CONSTANT(NPC_CLASS_BASE);
    DEFINE_CONSTANT(PLAYER_CLASS_NEXT);
    DEFINE_CONSTANT(NPC_RACE_BASIC);
    DEFINE_CONSTANT(NPC_RACE_HUMAN);
    DEFINE_CONSTANT(NPC_RACE_HUMAN_ANIMAL);
    DEFINE_CONSTANT(NPC_RACE_BIRD);
    DEFINE_CONSTANT(NPC_RACE_ANIMAL);
    DEFINE_CONSTANT(NPC_RACE_REPTILE);
    DEFINE_CONSTANT(NPC_RACE_FISH);
    DEFINE_CONSTANT(NPC_RACE_INSECT);
    DEFINE_CONSTANT(NPC_RACE_PLANT);
    DEFINE_CONSTANT(NPC_RACE_THING);
    DEFINE_CONSTANT(NPC_RACE_ZOMBIE);
    DEFINE_CONSTANT(NPC_RACE_GHOST);
    DEFINE_CONSTANT(NPC_RACE_EVIL_SPIRIT);
    DEFINE_CONSTANT(NPC_RACE_SPIRIT);
    DEFINE_CONSTANT(NPC_RACE_MAGIC_CREATURE);
    DEFINE_CONSTANT(NPC_RACE_NEXT);
    DEFINE_ENUM_CONSTANT(ESex::SEX_NEUTRAL);
	DEFINE_ENUM_CONSTANT(ESex::SEX_MALE);
	DEFINE_ENUM_CONSTANT(ESex::SEX_FEMALE);
	DEFINE_ENUM_CONSTANT(ESex::SEX_POLY);
    DEFINE_CONSTANT(NUM_SEXES);
    DEFINE_CONSTANT(MASK_SEX_NEUTRAL);
    DEFINE_CONSTANT(MASK_SEX_MALE);
    DEFINE_CONSTANT(MASK_SEX_FEMALE);
    DEFINE_CONSTANT(MASK_SEX_POLY);
    DEFINE_CONSTANT(GF_GODSLIKE);
    DEFINE_CONSTANT(GF_GODSCURSE);
    DEFINE_CONSTANT(GF_HIGHGOD);
    DEFINE_CONSTANT(GF_REMORT);
    DEFINE_CONSTANT(GF_DEMIGOD);
    DEFINE_CONSTANT(GF_PERSLOG);
    DEFINE_CONSTANT(POS_DEAD);
    DEFINE_CONSTANT(POS_MORTALLYW);
    DEFINE_CONSTANT(POS_INCAP);
    DEFINE_CONSTANT(POS_STUNNED);
    DEFINE_CONSTANT(POS_SLEEPING);
    DEFINE_CONSTANT(POS_RESTING);
    DEFINE_CONSTANT(POS_SITTING);
    DEFINE_CONSTANT(POS_FIGHTING);
    DEFINE_CONSTANT(POS_STANDING);
    DEFINE_CONSTANT(PLR_KILLER);
    DEFINE_CONSTANT(PLR_THIEF);
    DEFINE_CONSTANT(PLR_FROZEN);
    DEFINE_CONSTANT(PLR_DONTSET);
    DEFINE_CONSTANT(PLR_WRITING);
    DEFINE_CONSTANT(PLR_MAILING);
    DEFINE_CONSTANT(PLR_CRASH);
    DEFINE_CONSTANT(PLR_SITEOK);
    DEFINE_CONSTANT(PLR_MUTE);
    DEFINE_CONSTANT(PLR_NOTITLE);
    DEFINE_CONSTANT(PLR_DELETED);
    DEFINE_CONSTANT(PLR_LOADROOM);
    DEFINE_CONSTANT(PLR_NODELETE);
    DEFINE_CONSTANT(PLR_INVSTART);
    DEFINE_CONSTANT(PLR_CRYO);
    DEFINE_CONSTANT(PLR_HELLED);
    DEFINE_CONSTANT(PLR_NAMED);
    DEFINE_CONSTANT(PLR_REGISTERED);
    DEFINE_CONSTANT(PLR_DUMB);
    DEFINE_CONSTANT(PLR_DELETE);
    DEFINE_CONSTANT(PLR_FREE);
    DEFINE_CONSTANT(MOB_SPEC);
    DEFINE_CONSTANT(MOB_SENTINEL);
    DEFINE_CONSTANT(MOB_SCAVENGER);
    DEFINE_CONSTANT(MOB_ISNPC);
    DEFINE_CONSTANT(MOB_AWARE);
    DEFINE_CONSTANT(MOB_AGGRESSIVE);
    DEFINE_CONSTANT(MOB_STAY_ZONE);
    DEFINE_CONSTANT(MOB_WIMPY);
    DEFINE_CONSTANT(MOB_AGGR_DAY);
    DEFINE_CONSTANT(MOB_AGGR_NIGHT);
    DEFINE_CONSTANT(MOB_AGGR_FULLMOON);
    DEFINE_CONSTANT(MOB_MEMORY);
    DEFINE_CONSTANT(MOB_HELPER);
    DEFINE_CONSTANT(MOB_NOCHARM);
    DEFINE_CONSTANT(MOB_NOSUMMON);
    DEFINE_CONSTANT(MOB_NOSLEEP);
    DEFINE_CONSTANT(MOB_NOBASH);
    DEFINE_CONSTANT(MOB_NOBLIND);
    DEFINE_CONSTANT(MOB_MOUNTING);
    DEFINE_CONSTANT(MOB_NOHOLD);
    DEFINE_CONSTANT(MOB_NOSIELENCE);
    DEFINE_CONSTANT(MOB_AGGRMONO);
    DEFINE_CONSTANT(MOB_AGGRPOLY);
    DEFINE_CONSTANT(MOB_NOFEAR);
    DEFINE_CONSTANT(MOB_NOGROUP);
    DEFINE_CONSTANT(MOB_CORPSE);
    DEFINE_CONSTANT(MOB_LOOTER);
    DEFINE_CONSTANT(MOB_PROTECT);
    DEFINE_CONSTANT(MOB_DELETE);
    DEFINE_CONSTANT(MOB_FREE);
    DEFINE_CONSTANT(MOB_SWIMMING);
    DEFINE_CONSTANT(MOB_FLYING);
    DEFINE_CONSTANT(MOB_ONLYSWIMMING);
    DEFINE_CONSTANT(MOB_AGGR_WINTER);
    DEFINE_CONSTANT(MOB_AGGR_SPRING);
    DEFINE_CONSTANT(MOB_AGGR_SUMMER);
    DEFINE_CONSTANT(MOB_AGGR_AUTUMN);
    DEFINE_CONSTANT(MOB_LIKE_DAY);
    DEFINE_CONSTANT(MOB_LIKE_NIGHT);
    DEFINE_CONSTANT(MOB_LIKE_FULLMOON);
    DEFINE_CONSTANT(MOB_LIKE_WINTER);
    DEFINE_CONSTANT(MOB_LIKE_SPRING);
    DEFINE_CONSTANT(MOB_LIKE_SUMMER);
    DEFINE_CONSTANT(MOB_LIKE_AUTUMN);
    DEFINE_CONSTANT(MOB_NOFIGHT);
    DEFINE_CONSTANT(MOB_EADECREASE);
    DEFINE_CONSTANT(MOB_HORDE);
    DEFINE_CONSTANT(MOB_CLONE);
    DEFINE_CONSTANT(MOB_NOTKILLPUNCTUAL);
    DEFINE_CONSTANT(MOB_NOTRIP);
    DEFINE_CONSTANT(MOB_ANGEL);
    DEFINE_CONSTANT(MOB_GUARDIAN);
    DEFINE_CONSTANT(MOB_IGNORE_FORBIDDEN);
    DEFINE_CONSTANT(MOB_NO_BATTLE_EXP);
    DEFINE_CONSTANT(MOB_NOHAMER);
    DEFINE_CONSTANT(MOB_GHOST);
    DEFINE_CONSTANT(MOB_FIREBREATH);
    DEFINE_CONSTANT(MOB_GASBREATH);
    DEFINE_CONSTANT(MOB_FROSTBREATH);
    DEFINE_CONSTANT(MOB_ACIDBREATH);
    DEFINE_CONSTANT(MOB_LIGHTBREATH);
    DEFINE_CONSTANT(MOB_NOTRAIN);
    DEFINE_CONSTANT(MOB_NOREST);
    DEFINE_CONSTANT(MOB_AREA_ATTACK);
    DEFINE_CONSTANT(MOB_NOSTUPOR);
    DEFINE_CONSTANT(MOB_NOHELPS);
    DEFINE_CONSTANT(MOB_OPENDOOR);
    DEFINE_CONSTANT(MOB_IGNORNOMOB);
    DEFINE_CONSTANT(MOB_IGNORPEACE);
    DEFINE_CONSTANT(MOB_RESURRECTED);
    DEFINE_CONSTANT(MOB_RUSICH);
    DEFINE_CONSTANT(MOB_VIKING);
    DEFINE_CONSTANT(MOB_STEPNYAK);
    DEFINE_CONSTANT(MOB_AGGR_RUSICHI);
    DEFINE_CONSTANT(MOB_AGGR_VIKINGI);
    DEFINE_CONSTANT(MOB_AGGR_STEPNYAKI);
    DEFINE_CONSTANT(MOB_NORESURRECTION);
    DEFINE_CONSTANT(NPC_NORTH);
    DEFINE_CONSTANT(NPC_EAST);
    DEFINE_CONSTANT(NPC_SOUTH);
    DEFINE_CONSTANT(NPC_WEST);
    DEFINE_CONSTANT(NPC_UP);
    DEFINE_CONSTANT(NPC_DOWN);
    DEFINE_CONSTANT(NPC_POISON);
    DEFINE_CONSTANT(NPC_INVIS);
    DEFINE_CONSTANT(NPC_SNEAK);
    DEFINE_CONSTANT(NPC_CAMOUFLAGE);
    DEFINE_CONSTANT(NPC_MOVEFLY);
    DEFINE_CONSTANT(NPC_MOVECREEP);
    DEFINE_CONSTANT(NPC_MOVEJUMP);
    DEFINE_CONSTANT(NPC_MOVESWIM);
    DEFINE_CONSTANT(NPC_MOVERUN);
    DEFINE_CONSTANT(NPC_AIRCREATURE);
    DEFINE_CONSTANT(NPC_WATERCREATURE);
    DEFINE_CONSTANT(NPC_EARTHCREATURE);
    DEFINE_CONSTANT(NPC_FIRECREATURE);
    DEFINE_CONSTANT(NPC_HELPED);
    DEFINE_CONSTANT(NPC_STEALING);
    DEFINE_CONSTANT(NPC_WIELDING);
    DEFINE_CONSTANT(NPC_ARMORING);
    DEFINE_CONSTANT(NPC_USELIGHT);
    DEFINE_CONSTANT(DESC_CANZLIB);
    DEFINE_CONSTANT(PRF_BRIEF);
    DEFINE_CONSTANT(PRF_COMPACT);
    DEFINE_CONSTANT(PRF_NOHOLLER);
    DEFINE_CONSTANT(PRF_NOTELL);
    DEFINE_CONSTANT(PRF_DISPHP);
    DEFINE_CONSTANT(PRF_DISPMANA);
    DEFINE_CONSTANT(PRF_DISPMOVE);
    DEFINE_CONSTANT(PRF_AUTOEXIT);
    DEFINE_CONSTANT(PRF_NOHASSLE);
    DEFINE_CONSTANT(PRF_SUMMONABLE);
    DEFINE_CONSTANT(PRF_QUEST);
    DEFINE_CONSTANT(PRF_NOREPEAT);
    DEFINE_CONSTANT(PRF_HOLYLIGHT);
    DEFINE_CONSTANT(PRF_COLOR_1);
    DEFINE_CONSTANT(PRF_COLOR_2);
    DEFINE_CONSTANT(PRF_NOWIZ);
    DEFINE_CONSTANT(PRF_LOG1);
    DEFINE_CONSTANT(PRF_LOG2);
    DEFINE_CONSTANT(PRF_NOAUCT);
    DEFINE_CONSTANT(PRF_NOGOSS);
    DEFINE_CONSTANT(PRF_DISPFIGHT);
    DEFINE_CONSTANT(PRF_ROOMFLAGS);
    DEFINE_CONSTANT(PRF_DISPEXP);
    DEFINE_CONSTANT(PRF_DISPEXITS);
    DEFINE_CONSTANT(PRF_DISPLEVEL);
    DEFINE_CONSTANT(PRF_DISPGOLD);
    DEFINE_CONSTANT(PRF_DISPTICK);
    DEFINE_CONSTANT(PRF_PUNCTUAL);
    DEFINE_CONSTANT(PRF_AWAKE);
    DEFINE_CONSTANT(PRF_CODERINFO);
    DEFINE_CONSTANT(PRF_AUTOMEM);
    DEFINE_CONSTANT(PRF_NOSHOUT);
    DEFINE_CONSTANT(PRF_GOAHEAD);
    DEFINE_CONSTANT(PRF_SHOWGROUP);
    DEFINE_CONSTANT(PRF_AUTOASSIST);
    DEFINE_CONSTANT(PRF_AUTOLOOT);
    DEFINE_CONSTANT(PRF_AUTOSPLIT);
    DEFINE_CONSTANT(PRF_AUTOMONEY);
    DEFINE_CONSTANT(PRF_NOARENA);
    DEFINE_CONSTANT(PRF_NOEXCHANGE);
    DEFINE_CONSTANT(PRF_NOCLONES);
    DEFINE_CONSTANT(PRF_NOINVISTELL);
    DEFINE_CONSTANT(PRF_POWERATTACK);
    DEFINE_CONSTANT(PRF_GREATPOWERATTACK);
    DEFINE_CONSTANT(PRF_AIMINGATTACK);
    DEFINE_CONSTANT(PRF_GREATAIMINGATTACK);
    DEFINE_CONSTANT(PRF_NEWS_MODE);
    DEFINE_CONSTANT(PRF_BOARD_MODE);
    DEFINE_CONSTANT(PRF_DECAY_MODE);
    DEFINE_CONSTANT(PRF_TAKE_MODE);
    DEFINE_CONSTANT(PRF_PKL_MODE);
    DEFINE_CONSTANT(PRF_POLIT_MODE);
    DEFINE_CONSTANT(PRF_IRON_WIND);
    DEFINE_CONSTANT(PRF_PKFORMAT_MODE);
    DEFINE_CONSTANT(PRF_WORKMATE_MODE);
    DEFINE_CONSTANT(PRF_OFFTOP_MODE);
    DEFINE_CONSTANT(PRF_ANTIDC_MODE);
    DEFINE_CONSTANT(PRF_NOINGR_MODE);
    DEFINE_CONSTANT(PRF_NOINGR_LOOT);
    DEFINE_CONSTANT(PRF_DISP_TIMED);
    DEFINE_CONSTANT(PRF_IGVA_PRONA);
    DEFINE_CONSTANT(PRF_EXECUTOR);
    DEFINE_CONSTANT(PRF_DRAW_MAP);
    DEFINE_CONSTANT(PRF_CAN_REMORT);
    DEFINE_CONSTANT(PRF_ENTER_ZONE);
    DEFINE_CONSTANT(PRF_MISPRINT);
    DEFINE_CONSTANT(PRF_BRIEF_SHIELDS);
    DEFINE_CONSTANT(PRF_AUTO_NOSUMMON);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_BLIND);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_INVISIBLE);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_DETECT_ALIGN);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_DETECT_INVIS);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_DETECT_MAGIC);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_SENSE_LIFE);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_WATERWALK);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_SANCTUARY);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_GROUP);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_CURSE);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_INFRAVISION);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_POISON);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_PROTECT_EVIL);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_PROTECT_GOOD);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_SLEEP);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_NOTRACK);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_TETHERED);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_BLESS);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_SNEAK);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_HIDE);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_COURAGE);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_CHARM);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_HOLD);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_FLY);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_SILENCE);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_AWARNESS);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_BLINK);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_HORSE);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_NOFLEE);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_SINGLELIGHT);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_HOLYLIGHT);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_HOLYDARK);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_DETECT_POISON);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_DRUNKED);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_ABSTINENT);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_STOPRIGHT);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_STOPLEFT);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_STOPFIGHT);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_HAEMORRAGIA);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_CAMOUFLAGE);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_WATERBREATH);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_SLOW);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_HASTE);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_SHIELD);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_AIRSHIELD);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_FIRESHIELD);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_ICESHIELD);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_MAGICGLASS);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_STAIRS);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_STONEHAND);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_PRISMATICAURA);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_HELPER);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_EVILESS);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_AIRAURA);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_FIREAURA);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_ICEAURA);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_DEAFNESS);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_CRYING);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_PEACEFUL);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_MAGICSTOPFIGHT);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_BERSERK);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_LIGHT_WALK);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_BROKEN_CHAINS);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_CLOUD_OF_ARROWS);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_SHADOW_CLOAK);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_GLITTERDUST);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_AFFRIGHT);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_SCOPOLIA_POISON);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_DATURA_POISON);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_SKILLS_REDUCE);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_NOT_SWITCH);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_BELENA_POISON);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_NOTELEPORT);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_LACKY);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_BANDAGE);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_NO_BANDAGE);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_MORPH);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_STRANGLED);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_RECALL_SPELLS);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_NOOB_REGEN);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_VAMPIRE);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_EXPEDIENT);
    DEFINE_ENUM_CONSTANT(EAffectFlag::AFF_COMMANDER);
    DEFINE_CONSTANT(IGNORE_TELL);
    DEFINE_CONSTANT(IGNORE_SAY);
    DEFINE_CONSTANT(IGNORE_CLAN);
    DEFINE_CONSTANT(IGNORE_ALLIANCE);
    DEFINE_CONSTANT(IGNORE_GOSSIP);
    DEFINE_CONSTANT(IGNORE_SHOUT);
    DEFINE_CONSTANT(IGNORE_HOLLER);
    DEFINE_CONSTANT(IGNORE_GROUP);
    DEFINE_CONSTANT(IGNORE_WHISPER);
    DEFINE_CONSTANT(IGNORE_ASK);
    DEFINE_CONSTANT(IGNORE_EMOTE);
    DEFINE_CONSTANT(IGNORE_OFFTOP);
    DEFINE_CONSTANT(CON_PLAYING);
    DEFINE_CONSTANT(CON_CLOSE);
    DEFINE_CONSTANT(CON_GET_NAME);
    DEFINE_CONSTANT(CON_NAME_CNFRM);
    DEFINE_CONSTANT(CON_PASSWORD);
    DEFINE_CONSTANT(CON_NEWPASSWD);
    DEFINE_CONSTANT(CON_CNFPASSWD);
    DEFINE_CONSTANT(CON_QSEX);
    DEFINE_CONSTANT(CON_QCLASS);
    DEFINE_CONSTANT(CON_RMOTD);
    DEFINE_CONSTANT(CON_MENU);
    DEFINE_CONSTANT(CON_EXDESC);
    DEFINE_CONSTANT(CON_CHPWD_GETOLD);
    DEFINE_CONSTANT(CON_CHPWD_GETNEW);
    DEFINE_CONSTANT(CON_CHPWD_VRFY);
    DEFINE_CONSTANT(CON_DELCNF1);
    DEFINE_CONSTANT(CON_DELCNF2);
    DEFINE_CONSTANT(CON_DISCONNECT);
    DEFINE_CONSTANT(CON_OEDIT);
    DEFINE_CONSTANT(CON_REDIT);
    DEFINE_CONSTANT(CON_ZEDIT);
    DEFINE_CONSTANT(CON_MEDIT);
    DEFINE_CONSTANT(CON_TRIGEDIT);
    DEFINE_CONSTANT(CON_NAME2);
    DEFINE_CONSTANT(CON_NAME3);
    DEFINE_CONSTANT(CON_NAME4);
    DEFINE_CONSTANT(CON_NAME5);
    DEFINE_CONSTANT(CON_NAME6);
    DEFINE_CONSTANT(CON_RELIGION);
    DEFINE_CONSTANT(CON_RACE);
    DEFINE_CONSTANT(CON_LOWS);
    DEFINE_CONSTANT(CON_GET_KEYTABLE);
    DEFINE_CONSTANT(CON_GET_EMAIL);
    DEFINE_CONSTANT(CON_ROLL_STATS);
    DEFINE_CONSTANT(CON_MREDIT);
    DEFINE_CONSTANT(CON_QKIN);
    DEFINE_CONSTANT(CON_QCLASSV);
    DEFINE_CONSTANT(CON_QCLASSS);
    DEFINE_CONSTANT(CON_MAP_MENU);
    DEFINE_CONSTANT(CON_COLOR);
    DEFINE_CONSTANT(CON_WRITEBOARD);
    DEFINE_CONSTANT(CON_CLANEDIT);
    DEFINE_CONSTANT(CON_NEW_CHAR);
    DEFINE_CONSTANT(CON_SPEND_GLORY);
    DEFINE_CONSTANT(CON_RESET_STATS);
    DEFINE_CONSTANT(CON_BIRTHPLACE);
    DEFINE_CONSTANT(CON_WRITE_MOD);
    DEFINE_CONSTANT(CON_GLORY_CONST);
    DEFINE_CONSTANT(CON_NAMED_STUFF);
    DEFINE_CONSTANT(CON_RESET_KIN);
    DEFINE_CONSTANT(CON_RESET_RACE);
    DEFINE_CONSTANT(CON_CONSOLE);
    DEFINE_CONSTANT(CON_TORC_EXCH);
    DEFINE_CONSTANT(CON_MENU_STATS);
    DEFINE_CONSTANT(CON_SEDIT);
    DEFINE_CONSTANT(CON_RESET_RELIGION);
    DEFINE_CONSTANT(WEAR_LIGHT);
    DEFINE_CONSTANT(WEAR_FINGER_R);
    DEFINE_CONSTANT(WEAR_FINGER_L);
    DEFINE_CONSTANT(WEAR_NECK_1);
    DEFINE_CONSTANT(WEAR_NECK_2);
    DEFINE_CONSTANT(WEAR_BODY);
    DEFINE_CONSTANT(WEAR_HEAD);
    DEFINE_CONSTANT(WEAR_LEGS);
    DEFINE_CONSTANT(WEAR_FEET);
    DEFINE_CONSTANT(WEAR_HANDS);
    DEFINE_CONSTANT(WEAR_ARMS);
    DEFINE_CONSTANT(WEAR_SHIELD);
    DEFINE_CONSTANT(WEAR_ABOUT);
    DEFINE_CONSTANT(WEAR_WAIST);
    DEFINE_CONSTANT(WEAR_WRIST_R);
    DEFINE_CONSTANT(WEAR_WRIST_L);
    DEFINE_CONSTANT(WEAR_WIELD);
    DEFINE_CONSTANT(WEAR_HOLD);
    DEFINE_CONSTANT(WEAR_BOTHS);
    DEFINE_CONSTANT(NUM_WEARS);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::ITEM_LIGHT);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::ITEM_SCROLL);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::ITEM_WAND);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::ITEM_STAFF);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::ITEM_WEAPON);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::ITEM_FIREWEAPON);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::ITEM_MISSILE);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::ITEM_TREASURE);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::ITEM_ARMOR);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::ITEM_POTION);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::ITEM_WORN);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::ITEM_OTHER);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::ITEM_TRASH);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::ITEM_TRAP);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::ITEM_CONTAINER);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::ITEM_NOTE);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::ITEM_DRINKCON);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::ITEM_KEY);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::ITEM_FOOD);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::ITEM_MONEY);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::ITEM_PEN);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::ITEM_BOAT);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::ITEM_FOUNTAIN);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::ITEM_BOOK);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::ITEM_INGREDIENT);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::ITEM_MING);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::ITEM_MATERIAL);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::ITEM_BANDAGE);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::ITEM_ARMOR_LIGHT);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::ITEM_ARMOR_MEDIAN);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::ITEM_ARMOR_HEAVY);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::ITEM_ENCHANT);
    DEFINE_CONSTANT(BOOK_SPELL);
    DEFINE_CONSTANT(BOOK_SKILL);
    DEFINE_CONSTANT(BOOK_UPGRD);
    DEFINE_CONSTANT(BOOK_RECPT);
    DEFINE_CONSTANT(BOOK_FEAT);
    DEFINE_ENUM_CONSTANT(EWearFlag::ITEM_WEAR_TAKE);
    DEFINE_ENUM_CONSTANT(EWearFlag::ITEM_WEAR_FINGER);
    DEFINE_ENUM_CONSTANT(EWearFlag::ITEM_WEAR_NECK);
	DEFINE_ENUM_CONSTANT(EWearFlag::ITEM_WEAR_BODY);
	DEFINE_ENUM_CONSTANT(EWearFlag::ITEM_WEAR_HEAD);
	DEFINE_ENUM_CONSTANT(EWearFlag::ITEM_WEAR_LEGS);
	DEFINE_ENUM_CONSTANT(EWearFlag::ITEM_WEAR_FEET);
	DEFINE_ENUM_CONSTANT(EWearFlag::ITEM_WEAR_HANDS);
	DEFINE_ENUM_CONSTANT(EWearFlag::ITEM_WEAR_ARMS);
	DEFINE_ENUM_CONSTANT(EWearFlag::ITEM_WEAR_SHIELD);
	DEFINE_ENUM_CONSTANT(EWearFlag::ITEM_WEAR_ABOUT);
	DEFINE_ENUM_CONSTANT(EWearFlag::ITEM_WEAR_WAIST);
	DEFINE_ENUM_CONSTANT(EWearFlag::ITEM_WEAR_WRIST);
	DEFINE_ENUM_CONSTANT(EWearFlag::ITEM_WEAR_WIELD);
	DEFINE_ENUM_CONSTANT(EWearFlag::ITEM_WEAR_HOLD);
	DEFINE_ENUM_CONSTANT(EWearFlag::ITEM_WEAR_BOTHS);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_GLOW);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_HUM);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_NORENT);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_NODONATE);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_NOINVIS);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_INVISIBLE);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_MAGIC);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_NODROP);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_BLESS);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_NOSELL);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_DECAY);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_ZONEDECAY);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_NODISARM);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_NODECAY);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_POISONED);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_SHARPEN);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_ARMORED);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_DAY);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_NIGHT);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_FULLMOON);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_WINTER);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_SPRING);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_SUMMER);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_AUTUMN);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_SWIMMING);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_FLYING);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_THROWING);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_TICKTIMER);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_FIRE);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_REPOP_DECAY);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_NOLOCATE);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_TIMEDLVL);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_NOALTER);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_WITH1SLOT);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_WITH2SLOTS);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_WITH3SLOTS);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_SETSTUFF);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_NO_FAIL);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_NAMED);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_BLOODY);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_1INLAID);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_2INLAID);
	DEFINE_ENUM_CONSTANT(EExtraFlag::ITEM_3INLAID);
    DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_MONO);
	DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_POLY);
	DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_NEUTRAL);
    DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_MAGIC_USER);
	DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_CLERIC);
	DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_THIEF);
    DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_WARRIOR);
	DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_ASSASINE);
	DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_GUARD);
    DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_PALADINE);
	DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_RANGER);
	DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_SMITH);
    DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_MERCHANT);
	DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_DRUID);
	DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_BATTLEMAGE);
    DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_CHARMMAGE);
	DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_DEFENDERMAGE);
	DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_NECROMANCER);
    DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_KILLER);
	DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_COLORED);
	DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_MALE);
	DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_FEMALE);
	DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_CHARMICE);
	DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_POLOVCI);
	DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_PECHENEGI);
	DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_MONGOLI);
    DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_YIGURI);
	DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_KANGARI);
	DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_XAZARI);
	DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_SVEI);
	DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_DATCHANE);
	DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_GETTI);
	DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_UTTI);
	DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_XALEIGI);
    DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_NORVEZCI);
    DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_RUSICHI);
    DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_STEPNYAKI);
    DEFINE_ENUM_CONSTANT(ENoFlag::ITEM_NO_VIKINGI);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_MONO);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_POLY);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_NEUTRAL);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_MAGIC_USER);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_CLERIC);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_THIEF);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_WARRIOR);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_ASSASINE);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_GUARD);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_PALADINE);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_RANGER);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_SMITH);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_MERCHANT);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_DRUID);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_BATTLEMAGE);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_CHARMMAGE);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_DEFENDERMAGE);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_NECROMANCER);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_KILLER);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_COLORED);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_MALE);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_FEMALE);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_CHARMICE);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_POLOVCI);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_PECHENEGI);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_MONGOLI);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_YIGURI);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_KANGARI);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_XAZARI);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_SVEI);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_DATCHANE);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_GETTI);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_UTTI);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_XALEIGI);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_NORVEZCI);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_RUSICHI);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_STEPNYAKI);
    DEFINE_ENUM_CONSTANT(EAntiFlag::ITEM_AN_VIKINGI);
    DEFINE_CONSTANT(APPLY_NONE);
    DEFINE_CONSTANT(APPLY_STR);
    DEFINE_CONSTANT(APPLY_DEX);
    DEFINE_CONSTANT(APPLY_INT);
    DEFINE_CONSTANT(APPLY_WIS);
    DEFINE_CONSTANT(APPLY_CON);
    DEFINE_CONSTANT(APPLY_CHA);
    DEFINE_CONSTANT(APPLY_CLASS);
    DEFINE_CONSTANT(APPLY_LEVEL);
    DEFINE_CONSTANT(APPLY_AGE);
    DEFINE_CONSTANT(APPLY_CHAR_WEIGHT);
    DEFINE_CONSTANT(APPLY_CHAR_HEIGHT);
    DEFINE_CONSTANT(APPLY_MANAREG);
    DEFINE_CONSTANT(APPLY_HIT);
    DEFINE_CONSTANT(APPLY_MOVE);
    DEFINE_CONSTANT(APPLY_GOLD);
    DEFINE_CONSTANT(APPLY_EXP);
    DEFINE_CONSTANT(APPLY_AC);
    DEFINE_CONSTANT(APPLY_HITROLL);
    DEFINE_CONSTANT(APPLY_DAMROLL);
    DEFINE_CONSTANT(APPLY_SAVING_WILL);
    DEFINE_CONSTANT(APPLY_RESIST_FIRE);
    DEFINE_CONSTANT(APPLY_RESIST_AIR);
    DEFINE_CONSTANT(APPLY_SAVING_CRITICAL);
    DEFINE_CONSTANT(APPLY_SAVING_STABILITY);
    DEFINE_CONSTANT(APPLY_HITREG);
    DEFINE_CONSTANT(APPLY_MOVEREG);
    DEFINE_CONSTANT(APPLY_C1);
    DEFINE_CONSTANT(APPLY_C2);
    DEFINE_CONSTANT(APPLY_C3);
    DEFINE_CONSTANT(APPLY_C4);
    DEFINE_CONSTANT(APPLY_C5);
    DEFINE_CONSTANT(APPLY_C6);
    DEFINE_CONSTANT(APPLY_C7);
    DEFINE_CONSTANT(APPLY_C8);
    DEFINE_CONSTANT(APPLY_C9);
    DEFINE_CONSTANT(APPLY_SIZE);
    DEFINE_CONSTANT(APPLY_ARMOUR);
    DEFINE_CONSTANT(APPLY_POISON);
    DEFINE_CONSTANT(APPLY_SAVING_REFLEX);
    DEFINE_CONSTANT(APPLY_CAST_SUCCESS);
    DEFINE_CONSTANT(APPLY_MORALE);
    DEFINE_CONSTANT(APPLY_INITIATIVE);
    DEFINE_CONSTANT(APPLY_RELIGION);
    DEFINE_CONSTANT(APPLY_ABSORBE);
    DEFINE_CONSTANT(APPLY_LIKES	);
    DEFINE_CONSTANT(APPLY_RESIST_WATER);
    DEFINE_CONSTANT(APPLY_RESIST_EARTH);
    DEFINE_CONSTANT(APPLY_RESIST_VITALITY);
    DEFINE_CONSTANT(APPLY_RESIST_MIND);
    DEFINE_CONSTANT(APPLY_RESIST_IMMUNITY);
    DEFINE_CONSTANT(APPLY_AR	);
    DEFINE_CONSTANT(APPLY_MR	);
    DEFINE_CONSTANT(APPLY_ACONITUM_POISON);
    DEFINE_CONSTANT(APPLY_SCOPOLIA_POISON);
    DEFINE_CONSTANT(APPLY_BELENA_POISON);
    DEFINE_CONSTANT(APPLY_DATURA_POISON);
    DEFINE_CONSTANT(APPLY_HIT_GLORY);
    DEFINE_CONSTANT(APPLY_PR);
    DEFINE_CONSTANT(NUM_APPLIES	);
    DEFINE_CONSTANT(APPLY_ROOM_NONE);
    DEFINE_CONSTANT(APPLY_ROOM_POISON);
    DEFINE_CONSTANT(APPLY_ROOM_FLAME);
    DEFINE_CONSTANT(NUM_ROOM_APPLIES);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::EObjectMaterial::MAT_NONE);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::EObjectMaterial::MAT_BULAT);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::EObjectMaterial::MAT_BRONZE);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::EObjectMaterial::MAT_IRON);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::EObjectMaterial::MAT_STEEL);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::EObjectMaterial::MAT_SWORDSSTEEL);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::EObjectMaterial::MAT_COLOR);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::EObjectMaterial::MAT_CRYSTALL);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::EObjectMaterial::MAT_WOOD);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::EObjectMaterial::MAT_SUPERWOOD);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::EObjectMaterial::MAT_FARFOR);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::EObjectMaterial::MAT_GLASS);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::EObjectMaterial::MAT_ROCK);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::EObjectMaterial::MAT_BONE);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::EObjectMaterial::MAT_MATERIA);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::EObjectMaterial::MAT_SKIN);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::EObjectMaterial::MAT_ORGANIC);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::EObjectMaterial::MAT_PAPER);
    DEFINE_ENUM_CONSTANT(OBJ_DATA::EObjectMaterial::MAT_DIAMOND);
    DEFINE_CONSTANT(TRACK_NPC);
    DEFINE_CONSTANT(TRACK_HIDE);
    DEFINE_CONSTANT(CONT_CLOSEABLE);
    DEFINE_CONSTANT(CONT_PICKPROOF);
    DEFINE_CONSTANT(CONT_CLOSED);
    DEFINE_CONSTANT(CONT_LOCKED);
    DEFINE_CONSTANT(CONT_BROKEN);
    DEFINE_CONSTANT(SUN_DARK);
    DEFINE_CONSTANT(SUN_RISE);
    DEFINE_CONSTANT(SUN_LIGHT);
    DEFINE_CONSTANT(SUN_SET);
    DEFINE_CONSTANT(MOON_INCREASE);
    DEFINE_CONSTANT(MOON_DECREASE);
    DEFINE_CONSTANT(SKY_CLOUDLESS);
    DEFINE_CONSTANT(SKY_CLOUDY);
    DEFINE_CONSTANT(SKY_RAINING);
    DEFINE_CONSTANT(SKY_LIGHTNING);
    DEFINE_CONSTANT(EXTRA_FAILHIDE);
    DEFINE_CONSTANT(EXTRA_FAILSNEAK);
    DEFINE_CONSTANT(EXTRA_FAILCAMOUFLAGE);
    DEFINE_CONSTANT(EXTRA_GRP_KILL_COUNT);
    DEFINE_CONSTANT(LVL_FREEZE);
    DEFINE_CONSTANT(NUM_OF_DIRS);
    DEFINE_CONSTANT(MAGIC_NUMBER);
    DEFINE_CONSTANT(OPT_USEC);
    DEFINE_CONSTANT(PASSES_PER_SEC);
    DEFINE_CONSTANT(PULSE_ZONE);
    DEFINE_CONSTANT(PULSE_MOBILE);
    DEFINE_CONSTANT(PULSE_VIOLENCE);
    DEFINE_CONSTANT(ZONES_RESET);
    DEFINE_CONSTANT(PULSE_LOGROTATE);
    DEFINE_CONSTANT(MAX_SOCK_BUF);
    DEFINE_CONSTANT(MAX_PROMPT_LENGTH);
    DEFINE_CONSTANT(GARBAGE_SPACE);
    DEFINE_CONSTANT(SMALL_BUFSIZE);
    DEFINE_CONSTANT(LARGE_BUFSIZE);
    DEFINE_CONSTANT(MAX_STRING_LENGTH);
    DEFINE_CONSTANT(MAX_EXTEND_LENGTH);
    DEFINE_CONSTANT(MAX_INPUT_LENGTH);
    DEFINE_CONSTANT(MAX_RAW_INPUT_LENGTH);
    DEFINE_CONSTANT(MAX_MESSAGES);
    DEFINE_CONSTANT(MAX_NAME_LENGTH);
    DEFINE_CONSTANT(MIN_NAME_LENGTH);
    DEFINE_CONSTANT(HOST_LENGTH);
    DEFINE_CONSTANT(EXDSCR_LENGTH);
    DEFINE_CONSTANT(MAX_SPELLS);
    DEFINE_CONSTANT(MAX_AFFECT);
    DEFINE_CONSTANT(MAX_OBJ_AFFECT);
    DEFINE_CONSTANT(MAX_TIMED_SKILLS);
    DEFINE_CONSTANT(MAX_FEATS);
    DEFINE_CONSTANT(MAX_TIMED_FEATS);
    DEFINE_CONSTANT(MAX_HITS);
    DEFINE_CONSTANT(MAX_REMEMBER_PRAY);
    DEFINE_CONSTANT(MAX_REMEMBER_GOSSIP);
}

void scripting::init()
{

	PyImport_AppendInittab ("mud", PyInit_mud );
	PyImport_AppendInittab ("constants", PyInit_constants );
	Py_InitializeEx(0); //pass 0 to skip initialization registration of signal handlers
	PyEval_InitThreads();
	log("Using python version %s", Py_GetVersion());
	try
	{
	import("sys").attr("path").attr("insert")(0, "scripts");
	import("cmd").attr("initialize")();
	} catch(error_already_set const &)
	{
		log("%s",parse_python_exception().c_str());
		//����� �� ���� ���� ����������� � ����������� �������, ��� ����� ������������ ��� ��������� ����� �� ����
		puts("SYSERR: error initializing Python");
		exit(1);
	}
	Py_UNBLOCK_THREADS
}


/*void scripting::terminate()
{
	Py_BLOCK_THREADS
	try
	{
	import("pluginhandler").attr("terminate")();
	} catch(error_already_set const &)
	{
		log("%s",parse_python_exception().c_str());
	}
	Py_Finalize();
}*/

//Converts last python exception into string for pretty printing
//Precondition: catch(boost::python::error_already_set const &)
std::string parse_python_exception()
{
	PyObject *type_ptr = NULL, *value_ptr = NULL, *traceback_ptr = NULL;
	PyErr_Fetch(&type_ptr, &value_ptr, &traceback_ptr);
	std::string ret("Unfetchable Python error");
	if (type_ptr != NULL)
	try
	{
		handle<> h_type(type_ptr);
		str type_pstr(extract<object>(h_type.get())().attr("__name__"));
		ret = extract<std::string>(type_pstr);
	} catch (error_already_set const &)
	{
		ret = "Internal error getting exception type";
		PyErr_Print();
	}
	if (value_ptr != NULL)
	{
		handle<> h_val(value_ptr);
		str val_pstr(h_val);
		extract<std::string> e_val_pstr(val_pstr);
		if (e_val_pstr.check())
			ret += ": " + e_val_pstr();
		else
			ret += ": internal error getting exception value";
	}
	if (traceback_ptr != NULL)
	try
	{
		handle<> h_tb(traceback_ptr);
		object tb_list = import("traceback").attr("format_tb")(h_tb);
		object tb_str(str("\n").join(tb_list));
		extract<std::string> e_tb_pstr(tb_str);
		if (e_tb_pstr.check())
			ret = "Traceback (most recent call last):\n" + e_tb_pstr() + ret;
		else
			ret += "\nInternal error while formatting traceback";
	} catch (error_already_set const &)
	{
		ret += "\nInternal error while formatting traceback";
		PyErr_Print();
	}

	return ret;
}

void scripting::heartbeat()
{
	GILAcquirer acquire_gil;
	// execute callables passed to call_later
	std::list<object>::iterator i = objs_to_call_in_main_thread.begin();
	while (i != objs_to_call_in_main_thread.end())
	{
		std::list<object>::iterator cur = i++;
		try
		{
			(*cur)();
				} catch(error_already_set const &)
		{
			std::string err = "Error in callable submitted to call_later: " + parse_python_exception();
			mudlog(err.c_str(), DEF, LVL_BUILDER, ERRLOG, TRUE);
		}
		objs_to_call_in_main_thread.erase(cur);
	}
}

void publish_event(const char* event_name, dict kwargs)
{
	try
	{
		events.attr("publish")(*make_tuple(event_name), **kwargs);
		} catch(error_already_set const &)
		{
			mudlog((std::string("Error executing Python event ") + event_name + std::string(": " + parse_python_exception())).c_str(), CMP, LVL_IMMORT, ERRLOG, true);
	}
}



template <class t, class nextfunc, class getfunc, class result_t>
result_t CIterator<t, nextfunc, getfunc, result_t>::next()
{
	object_t result = _object;
	if (_object)
		_object = _next_func(_object);
	return _get_func(result);
}

struct PythonUserCommand
{
	std::string command;
	std::string command_koi8r;
	object callable;
	byte minimum_position;
	sh_int minimum_level;
	int unhide_percent;
	PythonUserCommand(const std::string& command_, const std::string& command____, const object& callable_, byte minimum_position_, sh_int minimum_level_, int unhide_percent_):
		command(command_), command_koi8r(command____), callable(callable_), minimum_position(minimum_position_), minimum_level(minimum_level_), unhide_percent(unhide_percent_) { }
};
typedef std::vector<PythonUserCommand> python_command_list_t;
 python_command_list_t global_commands;

 extern void check_hiding_cmd(CHAR_DATA * ch, int percent);

 bool check_command_on_list(const python_command_list_t& lst, CHAR_DATA* ch, const std::string& command, const std::string& args)
{
	for (python_command_list_t::const_iterator i = lst.begin(); i != lst.end(); ++i)
	{
		// ����������, ������� ��, �������������� i->command � koi8-r, �� � �� ����, ����� ����� ���
		if (!boost::starts_with(i->command_koi8r, command)) continue;

		//Copied from interpreter.cpp
		if (IS_NPC(ch) && i->minimum_level >= LVL_IMMORT)
		{
			send_to_char("�� ��� �� ���, ����� ������ ���.\r\n", ch);
			return true;
		}
		if (GET_POS(ch) < i->minimum_position)
		{
			switch (GET_POS(ch))
			{
			case POS_DEAD:
				send_to_char("����� ���� - �� ������ !!! :-(\r\n", ch);
				break;
			case POS_INCAP:
			case POS_MORTALLYW:
				send_to_char("�� � ����������� ��������� � �� ������ ������ ������!\r\n", ch);
				break;
			case POS_STUNNED:
				send_to_char("�� ������� �����, ����� ������� ���!\r\n", ch);
				break;
			case POS_SLEEPING:
				send_to_char("������� ��� � ����� ����?\r\n", ch);
				break;
			case POS_RESTING:
				send_to_char("���... �� ������� �����������..\r\n", ch);
				break;
			case POS_SITTING:
				send_to_char("�������, ��� ����� ������ �� ����.\r\n", ch);
				break;
			case POS_FIGHTING:
				send_to_char("�� �� ���! �� ���������� �� ���� �����!\r\n", ch);
				break;
			}
		return true;
		}
		check_hiding_cmd(ch, i->unhide_percent);
		try
		{
			// send command as bytes
			PyObject* buffer = PyBytes_FromStringAndSize(command.c_str(), command.length());
			object cmd = object(handle<>(buffer));

			// send arguments as bytes
			buffer = PyBytes_FromStringAndSize(args.c_str(), args.length());
			object arguments = object(handle<>(buffer));

			i->callable(CharacterWrapper(ch), cmd, arguments);
			return true;
		} catch(error_already_set const &)
		{
			std::string msg = std::string("Error executing Python command: " + parse_python_exception());
			mudlog(msg.c_str(), BRF, LVL_IMPL, SYSLOG, true);
			return false;
		}
	}
	return false;
}

void register_global_command(const std::string& command, const std::string& command_koi8r, object callable, sh_int minimum_position, sh_int minimum_level, int unhide_percent)
{
	global_commands.push_back(PythonUserCommand(command, command_koi8r, callable, minimum_position, minimum_level, unhide_percent));
}

void unregister_global_command(const std::string& command)
{
	python_command_list_t::iterator found = global_commands.end();
	for (python_command_list_t::iterator i = global_commands.begin(); i != global_commands.end() && found == global_commands.end(); ++i)
		if (i->command == command)
			found = i;
	if (found != global_commands.end())
		global_commands.erase(found);
	else {
		PyErr_SetString(PyExc_ValueError, "Command not found");
		throw_error_already_set();
	}
}

// returns true if command is found & dispatched
bool scripting::execute_player_command(CHAR_DATA* ch, const char* command, const char* args)
{
	GILAcquirer acquire_gil;
	return check_command_on_list(global_commands, ch, command, args);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
