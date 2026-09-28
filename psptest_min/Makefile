TARGET = hitr_min
OBJS   = min.o

CFLAGS  = -O2 -G0 -Wall -DPSP
CXXFLAGS = $(CFLAGS) -fno-exceptions

LIBS = -Wl,--start-group -lstdc++ -lm -lpthreadglue -Wl,--end-group \
       -lpspgu -lpspctrl -lpspdisplay -lpspkernel -lpspdebug -lpsprtc

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = HitR Min Test

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
