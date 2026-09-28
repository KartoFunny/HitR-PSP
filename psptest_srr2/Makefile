TARGET = min
OBJS   = main.o

CFLAGS  = -O2 -G0 -Wall -DPSP
CXXFLAGS = $(CFLAGS) -fno-exceptions

LIBS = -lpspgu -lpspctrl -lpspdisplay -lpspkernel -lpspdebug -lpsprtc

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Min Test

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
