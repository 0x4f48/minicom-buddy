CC               :=gcc
INCLUDE          :=-I.
CFLAGS           :=-Wall $(INCLUDE) `pkg-config gtk+-2.0  gmodule-2.0 libxml-2.0 --cflags`
LDFLAGS          := `pkg-config gtk+-2.0  gmodule-2.0 libxml-2.0 --libs`
INSTALL_BIN      :=/usr/local/bin
INSTALL_RESOURCE :=$(HOME)/.mbuddy

SHORTCUT_INFO =\
"[Desktop Entry]\n"\
"Name=mbuddy\n"\
"Exec=$(INSTALL_BIN)/mbuddy\n"\
"Terminal=false\n"\
"Type=Application\n"\
"Icon=$(INSTALL_RESOURCE)/images/mbuddy_128x128.png\n"\
"Categories=GTK;Development;"

#
# list of objects to build
#
OBJS += \
mbuddy.o \
xmlio.o \
util.o \
proclist.o

#
# default target
#
all: mbuddy

gladeconvert:
	xxd -i mbuddy-gui.glade >mbuddy-gui.h
	
#
# objects to build
#
%.o: %.c
	@echo 'Building file: $<'
	$(CC) $(CFLAGS) -c $< 

#
# building binary
#
mbuddy: gladeconvert $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

#
# install
#
install:
	mkdir -p $(INSTALL_RESOURCE)
	cp -ar ./images $(INSTALL_RESOURCE)
	cp ./mbuddy $(INSTALL_BIN)
	@echo $(SHORTCUT_INFO)>/usr/share/applications/mbuddy.desktop


clean:
	rm -rf *.o mbuddy mbuddy-gui.h
