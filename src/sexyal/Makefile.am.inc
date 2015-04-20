sexyal_SOURCES	=      sexyal/sexyal.c sexyal/md5.c sexyal/smallc.c sexyal/convert.c

if HAVE_DIRECTSOUND
TMP_DIRECTSOUND	=	sexyal/drivers/dsound.c
endif

if HAVE_OSSDSP
TMP_OSSDSP	=	sexyal/drivers/oss.c
endif

if HAVE_ESOUND
TMP_ESOUND	=	sexyal/drivers/esd.c
endif

if HAVE_JACK
TMP_JACK	=	sexyal/drivers/jack.c
endif

if HAVE_SDL
TMP_SDL		=	sexyal/drivers/sdl.c
endif

sexyal_SOURCES	+=	$(TMP_DIRECTSOUND) $(TMP_OSSDSP) $(TMP_ESOUND) $(TMP_JACK) $(TMP_SDL)
