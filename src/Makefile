# для сборки на платформах, не поддерживающих epoll (отличных от linux 2.6+),
# используйте параметр NO_EPOLL:
#
# make [options] [target] NO_EPOLL=1
#
# это выключает устанавливаемый по умолчанию флаг -DHAS_EPOLL
#

# C compiler to use
CC = g++
TESTCC = g++

MKDIR = mkdir -p

# Path to cxref utility
CXREF = cxref

##############################################################################
# Do Not Modify Anything Below This Line (unless you know what you're doing) #
##############################################################################

BINDIR = ../bin

ifdef NO_EPOLL
EPOLL_FLAG = 
else
EPOLL_FLAG = -DHAS_EPOLL
endif

CFLAGS = -std=gnu++0x -m32 -ggdb3 -O2 -Wall -Wextra -Wno-unused-parameter -DBOOST_ENABLE_ASSERT_HANDLER -DHAVE_ICONV $(EPOLL_FLAG) $(EXTRA_CFLAGS)

# use 'make test' for faster compile w/o optimizations, crypt and 34lvl god for first character
TESTCFLAGS = -std=gnu++0x -m32 -Wall -Wextra -Wno-unused-parameter -ggdb3 -DNOCRYPT -DTEST_BUILD -DLOG_AUTOFLUSH $(EXTRA_CFLAGS)
DEBUGFLAGS = -m32 -D_GLIBCXX_DEBUG -D_GLIBXX_DEBUG_PEDANTIC $(EPOLL_FLAG) $(EXTRA_CFLAGS)
FASTTEST   = -m32 -ggdb3 -Wall -DNOCRYPT -DTEST_BUILD -O3 $(EPOLL_FLAG) $(EXTRA_CFLAGS)
SEARCHPATH = -I /usr/include/python2.6

LIBS =  -m32 -lz -lpython2.6 -lboost_python-mt

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
# needed for clang
EXTRA_CFLAGS += -Wno-invalid-source-encoding -Wno-out-of-line-declaration
else
# there is no -lcrypt on darwin
LIBS +=  -lcrypt
endif

OBJFILES = act.comm.o act.informative.o act.item.o act.movement.o \
	act.offensive.o act.other.o act.social.o act.wizard.o alias.o ban.o \
	boards.o class.o comm.o config.o constants.o features.o db.o fight.o \
	graph.o handler.o house.o interpreter.o limits.o magic.o mail.o \
	mobact.o modify.o objsave.o olc.o random.o spec_assign.o \
	spec_procs.o spell_parser.o spells.o utils.o weather.o \
	redit.o oedit.o zedit.o medit.o \
	dg_comm.o dg_db_scripts.o dg_event.o dg_handler.o dg_mobcmd.o \
	dg_misc.o dg_objcmd.o dg_scripts.o dg_triggers.o dg_wldcmd.o dg_olc.o \
	color.o pk.o genchar.o diskio.o im.o names.o auction.o \
	item.creation.o skills.o exchange.o top.o stuff.o description.o \
	deathtrap.o title.o password.o privilege.o depot.o glory.o file_crc.o \
	char.o char_player.o quested.o mobmax.o parcel.o liquid.o poison.o \
	corpse.o house_exp.o remember.o name_list.o dps.o obj.spell.o obj.o room.o \
	spam.o AffectHandler.o glory_const.o pugixml.o morph.o glory_misc.o \
	shop_ext.o named_stuff.o birth_places.o player_races.o celebrates.o cache.o \
	sets_drop.o fight_hit.o fight_stuff.o objsave_ext.o dictionary.o help.o \
	map.o ext_money.o noob.o parse.o reset_stats.o mob_stat.o obj_sets.o \
	obj_sets_olc.o obj_enchant.o scripting.o version.o
	

CXREF_FILES = act.comm.cpp act.informative.cpp act.item.cpp act.movement.cpp \
	act.offensive.cpp act.other.cpp act.social.cpp act.wizard.cpp alias.cpp ban.cpp \
	boards.cpp class.cpp comm.cpp config.cpp constants.cpp features.cpp db.cpp fight.cpp \
	graph.cpp handler.cpp house.cpp interpreter.cpp limits.cpp magic.cpp mail.cpp \
	mobact.cpp modify.cpp objsave.cpp olc.cpp random.cpp spec_assign.cpp\
	spec_procs.cpp spell_parser.cpp spells.cpp utils.cpp weather.cpp \
	redit.cpp oedit.cpp zedit.cpp medit.cpp \
	dg_comm.cpp dg_db_scripts.cpp dg_event.cpp dg_handler.cpp dg_mobcmd.cpp \
	dg_misc.cpp dg_objcmd.cpp dg_scripts.cpp dg_triggers.cpp dg_wldcmd.cpp dg_olc.cpp \
	color.cpp pk.cpp genchar.cpp diskio.cpp im.cpp names.cpp \
	auction.cpp item.creation.cpp skills.cpp exchange.cpp top.cpp stuff.cpp \
	description.cpp deathtrap.cpp title.cpp password.cpp privilege.cpp \
	depot.cpp glory.cpp file_crc.cpp char.cpp char_player.cpp quested.cpp mobmax.cpp \
	parcel.cpp liquid.cpp poison.cpp corpse.cpp house_exp.cpp remember.cpp \
	name_list.cpp dps.cpp obj.spell.cpp obj.cpp room.cpp spam.cpp AffectHandler.cpp \
	glory_const.cpp pugixml.cpp glory_misc.cpp shop_ext.cpp named_stuff.cpp \
	celebrates.cpp birth_places.cpp player_races.cpp morph.cpp cache.cpp \
	sets_drop.cpp fight_hit.cpp fight_stuff.cpp objsave_ext.cpp dictionary.cpp \
	help.cpp map.cpp ext_money.cpp noob.cpp parse.cpp reset_stats.cpp \
	mob_stat.cpp obj_sets.cpp obj_sets_olc.cpp obj_enchant.cpp scripting.cpp version.cpp

default: circle

#all: .accepted
#	$(MAKE) $(BINDIR)/circle

#accepted:
#	@./licheck less

circle:
	$(MAKE) $(BINDIR)/circle
	HGENCODING=utf-8 hg log -v -M > $(BINDIR)/changelog

$(BINDIR)/circle : $(OBJFILES)
	$(MKDIR) $(BINDIR)
	$(CC) -o $(BINDIR)/circle $(OBJFILES) $(LIBS)
	rm -f version.o
	@echo "Done building Bylins."

clean:
	rm -f *.o

depend:
	$(CC) $(SEARCHPATH) -std=gnu++0x -MM $(CXREF_FILES) > .depend
	@echo "Done building dependencies."

testdepend:
	$(TESTCC) -MM $(CXREF_FILES) > .depend
	@echo "Done building dependencies."

test:
	$(MAKE) CFLAGS="$(TESTCFLAGS)" CC="$(TESTCC)" $(BINDIR)/circle

debug:
	$(MAKE) CFLAGS="$(TESTCFLAGS) $(DEBUGFLAGS)" CC="$(TESTCC)" $(BINDIR)/circle

fasttest:
	$(MAKE) CFLAGS="$(FASTTEST)" CC="$(TESTCC)" $(BINDIR)/circle

# Dependencies for the object files (automagically generated with
# gcc -MM)

.cpp.o:
	$(CC) $(SEARCHPATH) -c $(CFLAGS) -o $@ $<

include .depend
