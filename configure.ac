AC_INIT([src/festalon.c])

FESTALON_VERSION='0.5.5'

AC_CANONICAL_HOST
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([festalon], $FESTALON_VERSION)

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AM_PROG_LIBTOOL

AC_DEFINE(_GNU_SOURCE)
AC_CHECK_FUNC(asprintf, AC_DEFINE([HAVE_ASPRINTF]), [])
AC_CHECK_FUNC(posix_memalign, AC_DEFINE([HAVE_POSIX_MEMALIGN]), [])

PKG_CHECK_MODULES(SAMPLERATE, samplerate >= 0.0.15, [], AC_MSG_ERROR([*** libsamplerate >= 0.0.15 not found!]))

dnl AC_SUBST(SAMPLERATE_CFLAGS)
dnl AC_SUBST(SAMPLERATE_LIBS)

LIBS="$LIBS $SAMPLERATE_LIBS"

AC_ARG_ENABLE([interface], [  --enable-interface=interface	bmp, xmms or console],
	[
		interface=$enableval
	]
	,
	[	echo '*** No interface specified, defaulting to console';
		interface=console;
	]
	)

AM_CONDITIONAL(HAVE_OSSDSP, false)
AM_CONDITIONAL(HAVE_DIRECTSOUND, false)
AM_CONDITIONAL(HAVE_ESOUND, false)
AM_CONDITIONAL(HAVE_JACK, false)
AM_CONDITIONAL(HAVE_SDL, false)

AM_CONDITIONAL(INTERFACE_CONSOLE, false)
AM_CONDITIONAL(INTERFACE_XMMS, false)
AM_CONDITIONAL(INTERFACE_BMP, false)

if test x$interface = xconsole
then
	AM_CONDITIONAL(INTERFACE_CONSOLE, true)
	AC_SUBST(binlibtarget, "bin_PROGRAMS	=	festalon")

	if expr x"$target" : 'x.*mingw' > /dev/null; then
		AC_CHECK_HEADER([dsound.h],HAVE_DIRECTSOUND=true, HAVE_DIRECTSOUND=false)

		if $HAVE_DIRECTSOUND
		then
			AM_CONDITIONAL(HAVE_DIRECTSOUND,true)
			AC_DEFINE([HAVE_DIRECTSOUND])
			LIBS="-mwindows -ldxguid -lwinmm -ldsound $LIBS"
		fi
	else
	AC_CHECK_HEADER([sys/soundcard.h],[AM_CONDITIONAL(HAVE_OSSDSP,true)
                                   AC_DEFINE([HAVE_OSSDSP])],)
	fi

	AC_CHECK_PROG(has_esd_config, esd-config, yes)
	if test x$has_esd_config = xyes
	then
		AM_CONDITIONAL(HAVE_ESOUND, true)
		AC_DEFINE([HAVE_ESOUND])
		LIBS="$LIBS `esd-config --libs`"
		CFLAGS="$CFLAGS `esd-config --cflags`"
	fi

	PKG_CHECK_MODULES(JACK, jack, HAVE_JACK=true, HAVE_JACK=false)

	if $HAVE_JACK
	then
		AM_CONDITIONAL(HAVE_JACK, true)
		AC_DEFINE([HAVE_JACK])
		LIBS="$LIBS $JACK_LIBS"
		CFLAGS="$CFLAGS $JACK_CFLAGS"
	fi


	 SDL_VERSION=1.2.0
	 AM_PATH_SDL($SDL_VERSION, HAVE_SDL=true, HAVE_SDL=false)
	 if $HAVE_SDL
	 then
	 	AM_CONDITIONAL(HAVE_SDL, true)
		AC_DEFINE([HAVE_SDL])
		LIBS="$LIBS $SDL_LIBS"
		CFLAGS="$CFLAGS $SDL_CFLAGS"
	fi
elif test x$interface = xbmp
then
	AM_CONDITIONAL(INTERFACE_BMP, true)
	AC_DEFINE([INTERFACE_BMP])
	PKG_CHECK_MODULES(BMP, bmp >= 0.0.1, [], AC_MSG_ERROR([*** beep media player >= 0.0.1 not found!]))
	CFLAGS="$CFLAGS $BMP_CFLAGS"
	LIBS="$LIBS $BMP_LIBS"

        AC_PREFIX_DEFAULT("`pkg-config bmp --variable prefix`")
        libdir="`pkg-config bmp --variable input_plugin_dir`"
else
	AM_CONDITIONAL(INTERFACE_XMMS, true)
	AC_CHECK_PROG(has_xmms_config, xmms-config, yes)
	AC_CHECK_PROG(has_gtk_config, gtk-config, yes)
	AC_CHECK_PROG(has_glib_config, glib-config, yes)

	if test x$has_xmms_config = xyes; then
        	if test x$has_gtk_config = xyes; then
                	if test x$has_glib_config = xyes; then
                        	CFLAGS="$CFLAGS `gtk-config --cflags` `xmms-config --cflags` `glib-config --cflags`"
	                        LIBS="$LIBS `gtk-config --libs` `xmms-config --libs` `glib-config --libs`"
        	                can_build_xmms_plugin=yes
	                fi
	        fi
	fi

	if test x$can_build_xmms_plugin != xyes
	then
		AC_MSG_ERROR([*** Cannot build xmms plugin!])
	fi
	AC_PREFIX_DEFAULT("`xmms-config --prefix`")
	libdir="`xmms-config --input-plugin-dir`"
	dnl bindir="~/.xmms/Plugins"
fi

CFLAGS="-O2 -Ifidlib -Wall -fomit-frame-pointer $CFLAGS"

if expr x"$target" : 'x.*mingw' > /dev/null; then
	AC_DEFINE([FIDLIB_MINGW])
else
	AC_DEFINE([FIDLIB_LINUX])
fi

AM_CONDITIONAL(ARCH_X86, false)
AM_CONDITIONAL(ARCH_POWERPC, false)
if expr x"$target_cpu" : 'xi.86' > /dev/null; then
	AC_DEFINE([ARCH_X86])
	AM_CONDITIONAL(ARCH_X86, true)
	dnl CFLAGS="-mcpu=i686 $CFLAGS"
elif expr x"$target_cpu" : 'xx86*' > /dev/null; then
        AC_DEFINE([ARCH_X86])
        AM_CONDITIONAL(ARCH_X86, true)
elif expr x"$target_cpu" : 'xpowerpc' > /dev/null; then
	CFLAGS="-faltivec $CFLAGS"
	AC_DEFINE([ARCH_POWERPC])
	AM_CONDITIONAL(ARCH_POWERPC, true)
fi


LIBS="$LIBS -lm"

AC_C_BIGENDIAN(AC_DEFINE([MSB_FIRST]), AC_DEFINE([LSB_FIRST]))
AC_DEFINE_UNQUOTED([FESTALON_VERSION],"$FESTALON_VERSION")

dnl Output Makefiles
AC_OUTPUT([Makefile src/Makefile])
