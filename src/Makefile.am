AUTOMAKE_OPTIONS	=	subdir-objects

#bin_PROGRAMS		=	festalon
#lib_LTLIBRARIES	=	libfestalon-xmms.la

festalon_CFLAGS         =       $(AM_CFLAGS)

if INTERFACE_CONSOLE
bin_PROGRAMS		=	festalon
else
if INTERFACE_XMMS
lib_LTLIBRARIES 	= 	libfestalon_xmms.la
else
lib_LTLIBRARIES		=	libfestalon_bmp.la
endif
endif

festalon_SOURCES	=	festalon.c filter.c cputest.c nes/x6502.c nes/sound.c nes/nsf.c nes/cart.c nes/nsfe.c

festalon_SOURCES	+=	fidlib/fidlib.c
festalon_SOURCES	+=	nes/ext/vrc6.c nes/ext/vrc7.c nes/ext/emu2413.c nes/ext/ay.c nes/ext/mmc5.c nes/ext/n106.c nes/ext/fds.c

festalon_SOURCES	+=	pce/hes.c pce/vdc.c pce/system.c pce/psg.c pce/pce.c pce/h6280.c

if INTERFACE_XMMS
interface_SOURCES	=	drivers/xmms.c
endif

if INTERFACE_BMP
interface_SOURCES	=	drivers/xmms.c
endif

if INTERFACE_CONSOLE
include sexyal/Makefile.am.inc
interface_SOURCES	=       drivers/cli.c drivers/wave.c drivers/args.c $(sexyal_SOURCES)
endif

festalon_SOURCES	+=	$(interface_SOURCES)

libfestalon_xmms_la_SOURCES	=	$(festalon_SOURCES)
libfestalon_bmp_la_SOURCES	=       $(festalon_SOURCES)
