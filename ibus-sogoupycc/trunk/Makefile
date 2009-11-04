# IBUSXMLDIR: should look like: gconf.xml gtkpanel.xml pinyin.xml ...
# PKGDATADIR: anywhere should be ok
# PKGBINDIR:  anywhere should be ok
IBUSXMLDIR = /usr/share/ibus/component
PKGBINDIR  = /usr/lib/ibus
PKGDATADIR = /usr/share/ibus-sogoupycc

CFLAGS	=	-Wall -c -O3 `pkg-config --cflags ibus-1.0` -DPKGDATADIR='"$(PKGDATADIR)"' 
# comment above and uncomment following if you want to use doublepinyin all the time:
# CFLAGS	=	-Wall -c -O3 -DALWAYS_USE_DOUBLEPINYIN `pkg-config --cflags ibus-1.0` -DPKGDATADIR='"$(PKGDATADIR)"'

APP		=	ibus-sogoupycc
CC		=	gcc
LDFLAGS	=	`pkg-config --libs ibus-1.0` -lpthread
SRCS	=	doublepinyin_scheme.c engine.c sogoupycc.c main.c
OBJS	=	$(SRCS:.c=.o)
INSTALL = /bin/install -c

all: $(APP) sogoupycc.xml

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(APP):	$(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $(APP)

sogoupycc.xml:
	sed 's#_PKGBINDIR#$(PKGBINDIR)#g;s#_PKGDATADIR#$(PKGDATADIR)#g' sogoupycc.xml.template > sogoupycc.xml

clean:
	rm *.o $(APP) sogoupycc.xml

install:
	if which js 1>/dev/null 2>/dev/null && which wget 1>/dev/null 2>/dev/null; then \
	if ! [ -d $(PKGDATADIR) ]; then mkdir -p $(PKGDATADIR); fi; \
	if ! [ -d $(PKGBINDIR) ]; then mkdir -p $(PKGBINDIR); fi; \
	if ! [ -d $(IBUSXMLDIR) ]; then mkdir -p $(IBUSXMLDIR); fi; \
	$(INSTALL) -m 644 sogoupycc.xml $(IBUSXMLDIR)/sogoupycc.xml && \
	$(INSTALL) -m 644 ibus-sogoupycc.png $(PKGDATADIR)/ibus-sogoupycc.png && \
	$(INSTALL) -m 755 sgccget.sh $(PKGDATADIR)/sgccget.sh && \
	$(INSTALL) -m 755 ibus-sogoupycc $(PKGBINDIR)/ibus-sogoupycc; \
	else echo '>>> please make sure js and wget are available first.'; false; fi

uninstall:
	rm $(IBUSXMLDIR)/sogoupycc.xml; \
	rm $(PKGDATADIR)/ibus-sogoupycc.png; \
	rm $(PKGDATADIR)/sgccget.sh; \
	rm $(PKGBINDIR)/ibus-sogoupycc; \
	true
