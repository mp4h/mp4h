## ------------------------------- ##
## Check for function prototypes.  ##
## From Franc,ois Pinard           ##
## ------------------------------- ##

# serial 1

AC_DEFUN(AM_C_PROTOTYPES,
[AC_REQUIRE([AM_PROG_CC_STDC])
AC_REQUIRE([AC_PROG_CPP])
AC_MSG_CHECKING([for function prototypes])
if test "$am_cv_prog_cc_stdc" != no; then
  AC_MSG_RESULT(yes)
  AC_DEFINE(PROTOTYPES,1,[Define if compiler has function prototypes])
  U= ANSI2KNR=
else
  AC_MSG_RESULT(no)
  U=_ ANSI2KNR=./ansi2knr
  # Ensure some checks needed by ansi2knr itself.
  AC_HEADER_STDC
  AC_CHECK_HEADERS(string.h)
fi
AC_SUBST(U)dnl
AC_SUBST(ANSI2KNR)dnl
])
## ----------------------------------------- ##
## ANSIfy the C compiler whenever possible.  ##
## From Franc,ois Pinard                     ##
## ----------------------------------------- ##

# serial 1

# @defmac AC_PROG_CC_STDC
# @maindex PROG_CC_STDC
# @ovindex CC
# If the C compiler in not in ANSI C mode by default, try to add an option
# to output variable @code{CC} to make it so.  This macro tries various
# options that select ANSI C on some system or another.  It considers the
# compiler to be in ANSI C mode if it handles function prototypes correctly.
#
# If you use this macro, you should check after calling it whether the C
# compiler has been set to accept ANSI C; if not, the shell variable
# @code{am_cv_prog_cc_stdc} is set to @samp{no}.  If you wrote your source
# code in ANSI C, you can make an un-ANSIfied copy of it by using the
# program @code{ansi2knr}, which comes with Ghostscript.
# @end defmac

AC_DEFUN(AM_PROG_CC_STDC,
[AC_REQUIRE([AC_PROG_CC])
AC_BEFORE([$0], [AC_C_INLINE])
AC_BEFORE([$0], [AC_C_CONST])
dnl Force this before AC_PROG_CPP.  Some cpp's, eg on HPUX, require
dnl a magic option to avoid problems with ANSI preprocessor commands
dnl like #elif.
dnl FIXME: can't do this because then AC_AIX won't work due to a
dnl circular dependency.
dnl AC_BEFORE([$0], [AC_PROG_CPP])
AC_MSG_CHECKING(for ${CC-cc} option to accept ANSI C)
AC_CACHE_VAL(am_cv_prog_cc_stdc,
[am_cv_prog_cc_stdc=no
ac_save_CC="$CC"
# Don't try gcc -ansi; that turns off useful extensions and
# breaks some systems' header files.
# AIX			-qlanglvl=ansi
# Ultrix and OSF/1	-std1
# HP-UX			-Aa -D_HPUX_SOURCE
# SVR4			-Xc -D__EXTENSIONS__
for ac_arg in "" -qlanglvl=ansi -std1 "-Aa -D_HPUX_SOURCE" "-Xc -D__EXTENSIONS__"
do
  CC="$ac_save_CC $ac_arg"
  AC_TRY_COMPILE(
[#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
/* Most of the following tests are stolen from RCS 5.7's src/conf.sh.  */
struct buf { int x; };
FILE * (*rcsopen) (struct buf *, struct stat *, int);
static char *e (p, i)
     char **p;
     int i;
{
  return p[i];
}
static char *f (char * (*g) (char **, int), char **p, ...)
{
  char *s;
  va_list v;
  va_start (v,p);
  s = g (p, va_arg (v,int));
  va_end (v);
  return s;
}
int test (int i, double x);
struct s1 {int (*f) (int a);};
struct s2 {int (*f) (double a);};
int pairnames (int, char **, FILE *(*)(struct buf *, struct stat *, int), int, int);
int argc;
char **argv;
], [
return f (e, argv, 0) != argv[0]  ||  f (e, argv, 1) != argv[1];
],
[am_cv_prog_cc_stdc="$ac_arg"; break])
done
CC="$ac_save_CC"
])
if test -z "$am_cv_prog_cc_stdc"; then
  AC_MSG_RESULT([none needed])
else
  AC_MSG_RESULT($am_cv_prog_cc_stdc)
fi
case "x$am_cv_prog_cc_stdc" in
  x|xno) ;;
  *) CC="$CC $am_cv_prog_cc_stdc" ;;
esac
])

## ----------------------------------- ##
## Check if --with-dmalloc was given.  ##
## From Franc,ois Pinard               ##
## ----------------------------------- ##

# serial 1

AC_DEFUN(AM_WITH_DMALLOC,
[AC_MSG_CHECKING(if malloc debugging is wanted)
AC_ARG_WITH(dmalloc,
[  --with-dmalloc          use dmalloc, as in http://www.dmalloc.com],
[if test "$withval" = yes; then
  AC_MSG_RESULT(yes)
  AC_DEFINE(WITH_DMALLOC,1,
            [Define if using the dmalloc debugging malloc package])
  LIBS="$LIBS -ldmalloc"
  LDFLAGS="$LDFLAGS -g"
else
  AC_MSG_RESULT(no)
fi], [AC_MSG_RESULT(no)])
])

# Written by Ren� Seindal (rene@seindal.dk)
#
# This file can be copied and used freely without restrictions.  It can
# be used in projects which are not available under the GNU Public License
# but which still want to provide support for the GNU gettext functionality.
# Please note that the actual code is *not* freely available.

# serial 1

AC_DEFUN(AM_WITH_MODULES,
  [AC_MSG_CHECKING(if support for loadable modules is requested)
  AC_ARG_ENABLE(modules,
  [  --disable-modules       disable support for dynamic modules],
  [use_modules=$enableval], [use_modules=yes])
  AC_MSG_RESULT($use_modules)

  if test "$use_modules" = yes; then
    dnl We might no have it anyway, after all.
    with_modules=no

    dnl Test for dlopen in libc
    AC_CHECK_FUNCS(dlopen)
    if test "$ac_cv_func_dlopen" = yes; then
       with_modules=yes
    fi

    dnl Test for dlopen in libdl
    if test "$with_modules" = no; then
      AC_CHECK_LIB(dl, dlopen)
      if test "$ac_cv_lib_dl_dlopen" = yes; then
	with_modules=yes

#	LIBS="$LIBS -ldl"
	AC_DEFINE(HAVE_DLOPEN,1)
      fi
    fi

#    dnl Test for dld_link in libdld
#    if test "$with_modules" = no; then
#      AC_CHECK_LIB(dld, dld_link)
#      if test "$ac_cv_lib_dld_dld_link" = "yes"; then
#	 with_modules=yes
#	 AC_DEFINE(HAVE_DLD,1)
#      fi
#    fi

    dnl Test for shl_load in libdld
    if test "$with_modules" = no; then
       AC_CHECK_LIB(dld, shl_load)
       if test "$ac_cv_lib_dld_shl_load" = yes; then
	  with_modules=yes

#	  LIBS="$LIBS -ldld"
	  AC_DEFINE(HAVE_SHL_LOAD,1)
       fi
    fi

    if test "$with_modules" = yes; then
      dnl This is for libtool
      DLLDFLAGS=-export-dynamic

      AC_DEFINE(WITH_MODULES, 1)
    fi

    if test "$with_modules" = no; then
      AC_MSG_WARN([Loadable modules have not been found on your computer, this feature is disabled])
    fi
    AC_SUBST(DLLDFLAGS)
  fi
  ])
