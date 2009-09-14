# C compiler to use
CC = g++
TESTCC = g++
TESTCC4 = g++-4

# Path to cxref utility
CXREF = cxref

##############################################################################
# Do Not Modify Anything Below This Line (unless you know what you're doing) #
##############################################################################

BINDIR = ../bin

CFLAGS = -ggdb3 -O2 -Wall -DBOOST_ENABLE_ASSERT_HANDLER
# use 'make test' for faster compile w/o optimizations, crypt and 34lvl god for first character
TESTCFLAGS = -ggdb3 -Wall -DNOCRYPT -DTEST_BUILD -DLOG_AUTOFLUSH
DEBUGFLAGS = -D_GLIBCXX_DEBUG -D_GLIBXX_DEBUG_PEDANTIC
FASTTEST   = -ggdb3 -Wall -DNOCRYPT -DTEST_BUILD -O3

#LIBS =  -lsocket -lnsl
LIBS =  -lz -lcrypt

OBJFILES = act.comm.o act.informative.o act.item.o act.movement.o \
	act.offensive.o act.other.o act.social.o act.wizard.o alias.o ban.o \
	boards.o class.o comm.o config.o constants.o features.o db.o fight.o \
	graph.o handler.o house.o interpreter.o limits.o magic.o mail.o \
	mobact.o modify.o objsave.o olc.o random.o shop.o spec_assign.o \
	spec_procs.o spell_parser.o spells.o utils.o weather.o \
	redit.o oedit.o zedit.o medit.o sedit.o \
	dg_comm.o dg_db_scripts.o dg_event.o dg_handler.o dg_mobcmd.o \
	dg_misc.o dg_objcmd.o dg_scripts.o dg_triggers.o dg_wldcmd.o dg_olc.o \
	color.o pk.o genchar.o diskio.o im.o names.o auction.o \
	item.creation.o skills.o exchange.o top.o stuff.o description.o \
	deathtrap.o title.o password.o privilege.o version.o depot.o glory.o file_crc.o \
	char.o char_player.o quested.o mobmax.o parcel.o liquid.o poison.o xmlParser.o \
	corpse.o house_exp.o remember.o name_list.o dps.o

CXREF_FILES = act.comm.cpp act.informative.cpp act.item.cpp act.movement.cpp \
	act.offensive.cpp act.other.cpp act.social.cpp act.wizard.cpp alias.cpp ban.cpp \
	boards.cpp class.cpp comm.cpp config.cpp constants.cpp features.cpp db.cpp fight.cpp \
	graph.cpp handler.cpp house.cpp interpreter.cpp limits.cpp magic.cpp mail.cpp \
	mobact.cpp modify.cpp objsave.cpp olc.cpp random.cpp shop.cpp spec_assign.cpp\
	spec_procs.cpp spell_parser.cpp spells.cpp utils.cpp weather.cpp \
	redit.cpp oedit.cpp zedit.cpp medit.cpp sedit.cpp \
	dg_comm.cpp dg_db_scripts.cpp dg_event.cpp dg_handler.cpp dg_mobcmd.cpp \
	dg_misc.cpp dg_objcmd.cpp dg_scripts.cpp dg_triggers.cpp dg_wldcmd.cpp dg_olc.cpp \
	color.cpp pk.cpp genchar.cpp diskio.cpp im.cpp names.cpp \
	auction.cpp item.creation.cpp skills.cpp exchange.cpp top.cpp stuff.cpp \
	description.cpp deathtrap.cpp title.cpp password.cpp privilege.cpp version.cpp \
	depot.cpp glory.cpp file_crc.cpp char.cpp char_player.cpp quested.cpp mobmax.cpp \
	parcel.cpp liquid.cpp poison.cpp xmlParser.cpp corpse.cpp house_exp.cpp remember.cpp \
	name_list.cpp dps.cpp

default: circle

#all: .accepted
#	$(MAKE) $(BINDIR)/circle

#accepted:
#	@./licheck less

circle:
	$(MAKE) $(BINDIR)/circle

$(BINDIR)/circle : $(OBJFILES)
	$(CC) -o $(BINDIR)/circle $(OBJFILES) $(LIBS)
	rm -f version.o
	@echo "Done building Bylins."

clean:
	rm -f *.o

depend:
	$(CC) -MM $(CXREF_FILES) > .depend
	@echo "Done building dependencies."

testdepend:
	$(TESTCC) -MM $(CXREF_FILES) > .depend
	@echo "Done building dependencies."

test:
	$(MAKE) CFLAGS="$(TESTCFLAGS)" CC="$(TESTCC)" $(BINDIR)/circle

test4:
	$(MAKE) CFLAGS="$(TESTCFLAGS)" CC="$(TESTCC4)" $(BINDIR)/circle

debug:
	$(MAKE) CFLAGS="$(TESTCFLAGS) $(DEBUGFLAGS)" CC="$(TESTCC)" $(BINDIR)/circle

fasttest:
	$(MAKE) CFLAGS="$(FASTTEST)" CC="$(TESTCC)" $(BINDIR)/circle

# Dependencies for the object files (automagically generated with
# gcc -MM)

.cpp.o:
	$(CC) -c $(CFLAGS) -o $@ $<

include .depend

