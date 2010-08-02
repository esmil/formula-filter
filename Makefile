CC	= gcc
CFLAGS  ?= -O2 -pipe -Wall -Wextra -Wno-variadic-macros
STRIP   = strip
INSTALL = install
INDENT  = uncrustify -l C --replace --no-backup

DESTDIR =
PREFIX = $(HOME)
PLUGINDIR = .purple/plugins

plugins = formula-filter simple

ifdef NDEBUG
DEFINES+=-DNDEBUG
endif

PIDGIN_CFLAGS = $(shell pkg-config pidgin --cflags)
PIDGIN_LIBS   = $(shell pkg-config pidgin --libs)

c_files = $(wildcard *.c)
h_files = $(wildcard *.h)

.PHONY: pidgin_plugin_% all allinone strip install_plugindir install indent clean
.PRECIOUS: %.pic.o %.o %.so

all: $(plugins:%=pidgin_plugin_%)

%.pic.o: %.c
	@echo '  CC $@'
	@$(CC) -fPIC $(CFLAGS) $(DEFINES) -c $< -o $@

%.o: %.c
	@echo '  CC $@'
	@$(CC) $(CFLAGS) $(DEFINES) -c $<

%.so: %.pic.o
	@echo '  LD $@'
	@$(CC) -shared $(LDFLAGS) $^ -o $@

pidgin_plugin_%: override CFLAGS += $(PIDGIN_CFLAGS)
pidgin_plugin_%: override LDFLAGS += $(PIDGIN_LIBS)
pidgin_plugin_%: %.so
	

allinone:
	@echo 'Compiling ff.so..'
	@$(CC) $(CFLAGS) -fPIC $(PIDGIN_CFLAGS) $(DEFINES) -DALLINONE \
		$(LDFLAGS) -shared $(PIDGIN_LIBS) ff.c -o ff.so \
		&& echo '  done'
	@echo 'Compiling simple.so..'
	@$(CC) $(CFLAGS) -fPIC $(PIDGIN_CFLAGS) $(DEFINES) -DALLINONE \
		$(LDFLAGS) -shared $(PIDGIN_LIBS) simple.c -o simple.so \
		&& echo '  done'

strip_pidgin_plugin_%: pidgin_plugin_%
	@echo '  STRIP $(<:pidgin_plugin_%=%.so)'
	@$(STRIP) $(<:pidgin_plugin_%=%.so)

strip: $(plugins:%=strip_pidgin_plugin_%)

install_plugindir:
	@echo '  INSTALL -d $(DESTDIR)$(PREFIX)/$(PLUGINDIR)'
	@$(INSTALL) -d $(DESTDIR)$(PREFIX)/$(PLUGINDIR)

install_%_plugin: %.so | install_plugindir
	@echo '  INSTALL $<'
	@$(INSTALL) $< $(DESTDIR)$(PREFIX)/$(PLUGINDIR)/$<

install: $(plugins:%=install_%_plugin)

indent_%_h: %.h
	@echo '  INDENT $<'
	@$(INDENT) $<

indent_%_c: %.c
	@echo '  INDENT $<'
	@$(INDENT) $<

indent: $(h_files:%.h=indent_%_h) $(c_files:%.c=indent_%_c)

clean:
	rm -f $(plugins:%=%.so) *.o *.c~ *.h~
