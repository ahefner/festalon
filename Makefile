
FESTALON_VERSION="0.5.5"

OBJS	=	src/festalon.o src/filter.o src/cputest.o src/nes/x6502.o src/nes/sound.o src/nes/nsf.o src/nes/cart.o src/nes/nsfe.o

OBJS	+=	src/fidlib/fidlib.o
OBJS	+=	src/nes/ext/vrc6.o src/nes/ext/vrc7.o src/nes/ext/emu2413.o src/nes/ext/ay.o src/nes/ext/mmc5.o src/nes/ext/n106.o src/nes/ext/fds.o

OBJS	+=	src/pce/hes.o src/pce/vdc.o src/pce/system.o src/pce/psg.o src/pce/pce.o src/pce/h6280.o

OBJS	+=	src/sexyal/sexyal.o src/sexyal/convert.o src/sexyal/md5.o src/sexyal/smallc.o
OBJS	+=	src/drivers/wave.o

OBJS	+=	src/drivers/cli.o src/drivers/args.o

# Enable output drivers by including them:
include driver_ao.mk
include driver_oss.mk
include driver_jack.mk

BASE=`pwd`

CFLAGS += -Isrc/ -I$(BASE)/src/fidlib/ -DFIDLIB_LINUX -DFESTALON_VERSION=\"$(FESTALON_VERSION)\"
LDFLAGS += -lm -lsamplerate

festalon: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o festalon


clean:
	rm -f festalon $(OBJS)

