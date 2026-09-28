TARGET = hitr_poc2
OBJS   = test_main.o

LIBS_DIR = $(CURDIR)/../build/libs
INC_DIRS = \
    -I$(CURDIR)/../code \
    -I$(CURDIR)/../libs/radcore/inc \
    -I$(CURDIR)/../libs/radmath \
    -I$(CURDIR)/../libs/radmath/radmath

STATIC_LIBS = \
    $(LIBS_DIR)/radcore/libradcore.a \
    $(LIBS_DIR)/radmath/libradmath.a

CFLAGS   = -O2 -G0 -Wall -DPSP -DRAD_PSP -DRAD_RELEASE \
           -fno-asynchronous-unwind-tables -fno-unwind-tables \
           $(INC_DIRS)
CXXFLAGS = $(CFLAGS) -fno-exceptions -fsingle-precision-constant

LDFLAGS += -Wl,--allow-multiple-definition

LIBS = -Wl,--start-group $(STATIC_LIBS) -lstdc++ -lm -lpthreadglue -Wl,--end-group \
       -lpspgu -lpspctrl -lpspdisplay -lpspkernel -lpspdebug -lpsprtc

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = HitR PoC v0.2

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
