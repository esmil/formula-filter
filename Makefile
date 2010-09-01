CC	= gcc
CFLAGS  = -O2 -pipe -Wall -Wextra -Wno-variadic-macros
STRIP   = strip
INSTALL = install

DESTDIR =
PREFIX = $(HOME)
PLUGINDIR = .purple/plugins

plugins = formula-filter simple

ifdef NDEBUG
DEFINES+=-DNDEBUG
endif

PIDGIN_CFLAGS = $(shell pkg-config pidgin --cflags)
PIDGIN_LIBS   = $(shell pkg-config pidgin --libs)

.PHONY: all strip install_plugindir install clean
.PRECIOUS: %.pic.o

all: $(plugins:%=%.so)

%.pic.o: %.c
	@echo '  CC $@'
	@$(CC) -fPIC -nostartfiles $(PIDGIN_CFLAGS) $(CFLAGS) $(DEFINES) -c $< -o $@

%.so: %.pic.o
	@echo '  LD $@'
	@$(CC) -shared $(PIDGIN_LIBS) $(LDFLAGS) $^ -o $@

strip_%: %.so
	@echo '  STRIP $<'
	@$(STRIP) $<

strip: $(plugins:%=strip_%)

install_plugindir:
	@echo '  INSTALL -d $(DESTDIR)$(PREFIX)/$(PLUGINDIR)'
	@$(INSTALL) -d $(DESTDIR)$(PREFIX)/$(PLUGINDIR)

install_%: %.so | install_plugindir
	@echo '  INSTALL $<'
	@$(INSTALL) $< $(DESTDIR)$(PREFIX)/$(PLUGINDIR)/$<

install: $(plugins:%=install_%)

clean:
	rm -f $(plugins:%=%.so) *.o *.c~ *.h~
