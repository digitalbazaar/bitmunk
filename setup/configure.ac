# Configure script for Digital Bazaar Bitmunk product line
# Usage: Run ./configure once 
# Author: Manu Sporny

AC_PREREQ([2.60])
AC_INIT([bitmunk],[3.2.5],[support@digitalbazaar.com])
AC_CONFIG_AUX_DIR(setup)
AC_CONFIG_HEADERS([setup/config.h])

# Versioning
BITMUNK_VERSION_MAJOR=3
BITMUNK_VERSION_MINOR=2
BITMUNK_MAJORMINOR=$BITMUNK_VERSION_MAJOR.$BITMUNK_VERSION_MINOR
AC_SUBST(BITMUNK_VERSION_MAJOR)
AC_SUBST(BITMUNK_VERSION_MINOR)
AC_SUBST(BITMUNK_MAJORMINOR)

# Setup standard build environment variables
# FIXME: allow changing via configure option
MONARCH_DIR=`(cd ${srcdir}/../monarch && pwd)`
BITMUNK_DIR=`(cd ${srcdir} && pwd)`
AC_SUBST(MONARCH_DIR)
AC_SUBST(BITMUNK_DIR)
# Note: 'date -R' not portable to MacOS
DATE_RFC_2822=`date +"%a, %d %b %Y %H:%M:%S %z"`
DATE_YMD=`date +%Y%m%d`
FULL_DOTTED_VERSION=${PACKAGE_VERSION}
FULL_VERSION=${PACKAGE_VERSION}-${DATE_YMD}
AC_SUBST(DATE_RFC_2822)
AC_SUBST(FULL_VERSION)
AC_SUBST(FULL_DOTTED_VERSION)

dnl ----------------- platform -----------------
AC_ARG_ENABLE([linux],
   AS_HELP_STRING([--enable-linux], [compile for linux [[yes]]]),
   [
      case "${enableval}" in
         yes) BUILD_FOR_LINUX=yes 
              BUILD_FOR_WINDOWS=no
              BUILD_FOR_MACOS=no
              ;;
         no) BUILD_FOR_LINUX=no ;;
         *) AC_MSG_ERROR(bad value ${enableval} for --enable-linux) ;;
      esac
   ], [BUILD_FOR_LINUX=yes])  dnl Default value
if test "x$BUILD_FOR_LINUX" = xyes; then
   BITMUNK_PLATFORM=linux
fi

AC_ARG_ENABLE([windows],
   AS_HELP_STRING([--enable-windows], [compile for windows [[no]]]),
   [
      case "${enableval}" in
         yes) BUILD_FOR_WINDOWS=yes 
              BUILD_FOR_LINUX=no
              BUILD_FOR_MACOS=no
              ;;
         no) BUILD_FOR_WINDOWS=no ;;
         *) AC_MSG_ERROR(bad value ${enableval} for --enable-windows) ;;
      esac
   ], [BUILD_FOR_WINDOWS=no])  dnl Default value
if test "x$BUILD_FOR_WINDOWS" = xyes; then
   BITMUNK_PLATFORM=windows
fi

AC_ARG_ENABLE([macos],
   AS_HELP_STRING([--enable-macos], [compile for MacOS [[no]]]),
   [
      case "${enableval}" in
         yes) BUILD_FOR_MACOS=yes 
              BUILD_FOR_LINUX=no
              BUILD_FOR_WINDOWS=no
              ;;
         no) BUILD_FOR_MACOS=no ;;
         *) AC_MSG_ERROR(bad value ${enableval} for --enable-macos) ;;
      esac
   ], [BUILD_FOR_MACOS=no])  dnl Default value
if test "x$BUILD_FOR_MACOS" = xyes; then
   BITMUNK_PLATFORM=macos
fi

# Check for Firefox plugin build
AC_ARG_ENABLE([firefox-plugin],
   AS_HELP_STRING([--enable-firefox-plugin], [compile firefox plugin [[no]]]),
   [
      case "${enableval}" in
         yes) BUILD_FIREFOX_PLUGIN=yes ;;
         no) BUILD_FIREFOX_PLUGIN=no ;;
         *) AC_MSG_ERROR(bad value ${enableval} for --enable-firefox-plugin) ;;
      esac
   ], 
   [
      # By default, build the Firefox plugin
      if test "x$BUILD_FOR_MACOS" = "xyes" -o \
              "x$BUILD_FOR_WINDOWS" = "xyes"; then
          BUILD_FIREFOX_PLUGIN=yes
      else
          BUILD_FIREFOX_PLUGIN=no
      fi
   ])  dnl Default value
AC_SUBST(BUILD_FIREFOX_PLUGIN)

dnl --- linux target cpu ---
if test "x$BUILD_FOR_LINUX" = xyes; then
   case "`uname -m`" in
      i686) TARGET_CPU=x86 ;;
      *) AC_MSG_ERROR(unknown target cpu type) ;;
   esac
   AC_SUBST(TARGET_CPU)
fi
dnl --- windows target cpu ---
if test "x$BUILD_FOR_WINDOWS" = xyes; then
   TARGET_CPU=x86
   AC_SUBST(TARGET_CPU)
fi
dnl --- macos target cpu ---
if test "x$BUILD_FOR_MACOS" = xyes; then
   case "`uname -p`" in
      i386) TARGET_CPU=x86 ;;
      powerpc) TARGET_CPU=ppc ;;
      dnl powerpc64) TARGET_CPU=ppc64 ;;
      *) AC_MSG_ERROR(unknown target cpu type) ;;
   esac
   AC_SUBST(TARGET_CPU)
fi

AC_SUBST(BUILD_FOR_LINUX)
AC_SUBST(BUILD_FOR_WINDOWS)
AC_SUBST(BUILD_FOR_MACOS)

dnl ----------------- release mode -----------------
BITMUNK_RELEASE_MODE=dev

AC_ARG_ENABLE([release],
   AC_HELP_STRING(
      [--enable-release],
      [set the release mode to 'official' [[no]]]),
   [
   case "${enableval}" in
      yes) BITMUNK_RELEASE_MODE=official ;;
      no)  BITMUNK_RELEASE_MODE=$BITMUNK_RELEASE_MODE ;;
      *)   AC_MSG_ERROR(bad value ${enableval} for --enable-release) ;;
   esac
   ])

AC_SUBST(BITMUNK_RELEASE_MODE)

dnl ----------------- Bitmunk share directory -----------------

BITMUNK_SHARE_DIR="$datadir/bitmunk$BITMUNK_VERSION_MAJOR"
AC_SUBST(BITMUNK_SHARE_DIR)

dnl ----------------- System install directories ----------------
# If we're releasing the software, ensure that at least the prefix,
# sysconfdir, datadir and localstatedir variables are defined.
if test "$prefix" = "NONE"; then
   prefix="/usr"
fi
if test "$sysconfdir" = "\${prefix}/etc"; then
   sysconfdir="/etc"
fi
if test "$datadir" = "\${prefix}/usr/share"; then
   datadir="/usr/share"
fi
if test "$localstatedir" = "\${prefix}/var"; then
   localstatedir="/var"
fi

dnl ------------- Bitmunk API port numbers -------------------

BITMUNK_LOCAL_API_HOST="0.0.0.0"
BITMUNK_API_HOST=dev.bitmunk.com
BITMUNK_API_PORT=19100
BITMUNK_LOCAL_API_PORT=19100
BITMUNK_FLASH_POLICY_PORT=19843

dnl ---------- Linux configuration settings -----------------

if test "x$BUILD_FOR_LINUX" = "xyes"; then
   dnl === Linux === Development ===
   BITMUNK_MODULES_DIR="$BITMUNK_DIR/dist/modules/bpe"
   BITMUNK_WEBUI_PLUGINS_DIR="$BITMUNK_DIR/cpp/apps/bitmunk/plugins"
   BITMUNK_PURCHASE_TEMP_DIR="tmp"
   BITMUNK_PURCHASE_DOWNLOAD_DIR="downloads"
   BITMUNK_SYSTEM_CONFIG_DIR="$BITMUNK_DIR/configs"
   BITMUNK_SYSTEM_CONFIG="$BITMUNK_DIR/configs/dev.config"
   BITMUNK_SYSTEM_USER_CONFIG="{BITMUNK_HOME}/dev.config"
   if test "x$BITMUNK_RELEASE_MODE" = "xofficial"; then
      BITMUNK_MODULES_DIR="/usr/lib/bitmunk$BITMUNK_VERSION_MAJOR"
      BITMUNK_SYSTEM_CONFIG_DIR="/etc/bitmunk3/conf.d"
   fi
fi

dnl ---------- Mac OS X configuration settings -----------------

if test "x$BUILD_FOR_MACOS" = "xyes"; then
   dnl === Mac OS === Development ===
   BITMUNK_MODULES_DIR="$BITMUNK_DIR/modules/macos"
   BITMUNK_WEBUI_PLUGINS_DIR="$BITMUNK_DIR/cpp/apps/bitmunk/plugins"
   BITMUNK_PURCHASE_TEMP_DIR="/tmp/dev/tmp"
   BITMUNK_PURCHASE_DOWNLOAD_DIR="/tmp/dev/downloads"
   BITMUNK_SYSTEM_CONFIG_DIR="$BITMUNK_DIR/configs"
   BITMUNK_SYSTEM_CONFIG="$BITMUNK_DIR/configs/dev.config"
   BITMUNK_SYSTEM_USER_CONFIG="{BITMUNK_HOME}/dev.config"
fi

dnl ---------- Windows configuration settings -----------------

if test "x$BUILD_FOR_WINDOWS" = "xyes"; then
   dnl === Windows === Development ===
   BITMUNK_MODULES_DIR="../../bpe"
   BITMUNK_WEBUI_PLUGINS_DIR="../../plugins"
   BITMUNK_PURCHASE_TEMP_DIR="../../tmp"
   BITMUNK_PURCHASE_DOWNLOAD_DIR="../../downloads"
   BITMUNK_SYSTEM_CONFIG="NOT_USED"
   BITMUNK_SYSTEM_USER_CONFIG="NOT_USED"
fi

dnl ---------- Firefox plugin configuration settings ----------

if test "x$BUILD_FIREFOX_PLUGIN" = xyes; then
   dnl === Firefox Plugin ===
   BITMUNK_LOCAL_API_PORT=19200
   BITMUNK_FLASH_POLICY_PORT=19943
   BITMUNK_MODULES_DIR="{RESOURCE_PATH}/modules"
   BITMUNK_WEBUI_PLUGINS_DIR="{RESOURCE_PATH}/webuiplugins"
   BITMUNK_PURCHASE_TEMP_DIR="tmp"
   BITMUNK_PURCHASE_DOWNLOAD_DIR="downloads"
   BITMUNK_SYSTEM_CONFIG_DIR="$BITMUNK_DIR/configs"
   BITMUNK_SYSTEM_CONFIG="$BITMUNK_DIR/configs/dev.config"
   BITMUNK_SYSTEM_USER_CONFIG="{BITMUNK_HOME}/dev.config"

   NODE_SSL_GENERATE=true
   NODE_SSL_CERTIFICATE="{RESOURCE_PATH}/pki/nodessl.crt"
   NODE_SSL_KEY="{RESOURCE_PATH}/pki/nodessl.key"
   NODE_SSL_CA_FILE="{RESOURCE_PATH}/pki/test-bitmunk-ca-file.pem"
else
   NODE_SSL_GENERATE=true
   NODE_SSL_CERTIFICATE="$BITMUNK_DIR/pki/nodessl.crt"
   NODE_SSL_KEY="$BITMUNK_DIR/pki/nodessl.key"
   NODE_SSL_CA_FILE="$BITMUNK_DIR/pki/test-bitmunk-ca-file.pem"
fi

AC_SUBST(NODE_SSL_GENERATE)
AC_SUBST(NODE_SSL_CERTIFICATE)
AC_SUBST(NODE_SSL_KEY)
AC_SUBST(NODE_SSL_CA_FILE)

AC_SUBST(BITMUNK_API_HOST)
AC_SUBST(BITMUNK_API_PORT)
AC_SUBST(BITMUNK_LOCAL_API_HOST)
AC_SUBST(BITMUNK_LOCAL_API_PORT)
AC_SUBST(BITMUNK_FLASH_POLICY_PORT)

dnl ----------------- modules dir -----------------

AC_ARG_WITH(modules-dir,
  AC_HELP_STRING([--with-modules-dir=DIR],
    [use DIR for Bitmunk modules [[LIBDIR/bitmunkBITMUNK_VERSION_MAJOR]]]),
  [
    case "${withval}" in
      yes) AC_MSG_ERROR(bad value ${withval} for --with-modules-dir) ;;
      no)  AC_MSG_ERROR(bad value ${withval} for --with-modules-dir) ;;
      *)   BITMUNK_MODULES_DIR="${withval}" ;;
    esac
  ],
  [:]) dnl Default value

dnl ----------------- docs -----------------

AC_ARG_ENABLE([docs],
   AS_HELP_STRING([--enable-docs], [build documentation [[no]]]),
   [ 
      case "${enableval}" in
         yes) BUILD_DOCS=yes ;;
         no) BUILD_DOCS=no ;;
         *) AC_MSG_ERROR(bad value ${enableval} for --enable-docs) ;;
      esac
   ], [BUILD_DOCS=no])  dnl Default value

dnl ----------------- only enable tests in pure dev mode -----------------

if test "x$BUILD_FIREFOX_PLUGIN" = "xyes" ; then
   BUILD_TESTS=no
elif test "x$BITMUNK_RELEASE_MODE" = "xofficial"; then
   BUILD_TESTS=no
else
   BUILD_TESTS=yes
fi

AC_ARG_ENABLE([tests],
   AC_HELP_STRING([--disable-tests], [disable building test apps [[no]]]),
   [
   case "${enableval}" in
      yes) BUILD_TESTS=yes ;;
      no)  BUILD_TESTS=no ;;
      *)   AC_MSG_ERROR(bad value ${enableval} for --disable-tests) ;;
   esac
   ])

AC_SUBST(BUILD_TESTS)

dnl ----------------- disable log line numbers -----------------

AC_ARG_ENABLE([log-line-numbers],
   AC_HELP_STRING(
      [--disable-log-line-numbers],
      [disable log line numbers [[no]]]),
   [
   case "${enableval}" in
      yes) MO_DISABLE_LOG_LINE_NUMBERS=no ;;
      no)  MO_DISABLE_LOG_LINE_NUMBERS=yes ;;
      *)   AC_MSG_ERROR(bad value ${enableval} for --enable-log-line-numbers) ;;
   esac
   ],
   [MO_DISABLE_LOG_LINE_NUMBERS=no]) dnl Default value
if test "x$BUILD_FIREFOX_PLUGIN" = "xyes"; then
   if test "x$BITMUNK_RELEASE_MODE" = "xofficial"; then
      MO_DISABLE_LOG_LINE_NUMBERS=yes
   fi
fi
if test "x$MO_DISABLE_LOG_LINE_NUMBERS" = "xyes"; then
   AC_DEFINE([MO_DISABLE_LOG_LINE_NUMBERS], [1],
      [Enable log line numbers.])
fi

dnl ----------------- system config -----------------

AC_ARG_WITH(system-config,
  AC_HELP_STRING([--with-system-config=FILE],
    [use FILE for Bitmunk system config (default depends on mode and platform)]),
  [
    case "${withval}" in
      yes) AC_MSG_ERROR(bad value ${withval} for --with-system-config) ;;
      no)  AC_MSG_ERROR(bad value ${withval} for --with-system-config) ;;
      *)   BITMUNK_SYSTEM_CONFIG="${withval}" ;;
    esac
  ],
  [:]) dnl Default value

dnl ----------------- system user home dir -----------------

dnl default system user bitmunk home dir 
if test "x$BITMUNK_RELEASE_MODE" = "xofficial"; then
   case "${BITMUNK_PLATFORM}" in
      linux) BITMUNK_HOME_DIR="~/.bitmunk3" ;;
      windows) BITMUNK_HOME_DIR="~/Application Data/Bitmunk3" ;;
      macos) BITMUNK_HOME_DIR="~/Library/Application Support/Bitmunk3" ;;
      *) AC_MSG_ERROR(home dir unknown for platform ${BITMUNK_PLATFORM}) ;;
   esac
else
   case "${BITMUNK_PLATFORM}" in
      linux) BITMUNK_HOME_DIR="/tmp/dev" ;;
      windows) BITMUNK_HOME_DIR="~/Application Data/Bitmunk3/dev" ;;
      macos) BITMUNK_HOME_DIR="~/Library/Application Support/Bitmunk3/dev" ;;
      *) AC_MSG_ERROR(home dir unknown for platform ${BITMUNK_PLATFORM}) ;;
   esac
fi

AC_ARG_WITH(home-dir,
  AC_HELP_STRING([--with-home-dir=DIR],
    [use DIR for Bitmunk home (default depends on platform and mode)]),
  [
    case "${withval}" in
      yes) AC_MSG_ERROR(bad value ${withval} for --with-home-dir) ;;
      no)  AC_MSG_ERROR(bad value ${withval} for --with-home-dir) ;;
      *)   BITMUNK_HOME_DIR="${withval}" ;;
    esac
  ],
  [:]) dnl Default value

dnl ----------------- system user config -----------------

AC_ARG_WITH(config,
  AC_HELP_STRING([--with-config=FILE],
    [use FILE for Bitmunk system user config (absolute or relative to BITMUNK_HOME_DIR) [[BITMUNK_HOME_DIR/system.config]]]),
  [
    case "${withval}" in
      yes) AC_MSG_ERROR(bad value ${withval} for --with-config) ;;
      no)  AC_MSG_ERROR(bad value ${withval} for --with-config) ;;
      *)   BITMUNK_SYSTEM_USER_CONFIG="${withval}" ;;
    esac
  ],
  [:]) dnl Default value

dnl ----------------- profiles dir -----------------

if test -d "${BITMUNK_DIR}/pki"; then
   BITMUNK_PROFILES_DIR="$BITMUNK_DIR/pki"
else
   AC_MSG_NOTICE(pki profiles dir not found)
   BITMUNK_PROFILES_DIR="profiles"
fi

AC_ARG_WITH(profiles-dir,
  AC_HELP_STRING([--with-profiles-dir=DIR],
    [use DIR for Bitmunk profiles (absolute or relative to BITMUNK_HOME_DIR) (default depends on mode and platform)]),
  [
    case "${withval}" in
      yes) AC_MSG_ERROR(bad value ${withval} for --with-profiles-dir) ;;
      no)  AC_MSG_ERROR(bad value ${withval} for --with-profiles-dir) ;;
      *)   BITMUNK_PROFILES_DIR="${withval}" ;;
    esac
  ],
  [:]) dnl Default value

dnl ----------------- users dir -----------------

dnl default location of profiles.
BITMUNK_USERS_DIR="users"

AC_ARG_WITH(users-dir,
  AC_HELP_STRING([--with-users-dir=DIR],
    [use DIR for Bitmunk users (absolute or relative to BITMUNK_HOME_DIR) [[BITMUNK_HOME_DIR/users]]]),
  [
    case "${withval}" in
      yes) AC_MSG_ERROR(bad value ${withval} for --with-users-dir) ;;
      no)  AC_MSG_ERROR(bad value ${withval} for --with-users-dir) ;;
      *)   BITMUNK_USERS_DIR="${withval}" ;;
    esac
  ],
  [:]) dnl Default value

dnl ----------------- BFP config -----------------

dnl === BFP API version ===

# use simple constant in development mode
BITMUNK_BFP_API_VERSION=${PACKAGE_VERSION}

AC_ARG_WITH(bfp-api-version,
  AC_HELP_STRING([--with-bfp-api-version=VERSION],
    [use VERSION for Bitmunk BFP API version]),
  [
    case "${withval}" in
      yes) AC_MSG_ERROR(bad value ${withval} for --with-bfp-api-version) ;;
      no)  AC_MSG_ERROR(bad value ${withval} for --with-bfp-api-version) ;;
      *)   BITMUNK_BFP_API_VERSION="${withval}" ;;
    esac
  ],
  [:]) dnl Default set above

AC_DEFINE_UNQUOTED([BITMUNK_BFP_API_VERSION], ["$BITMUNK_BFP_API_VERSION"],
   [Bitmunk BFP API Version])

dnl ----------------- build flash -----------------

AC_ARG_ENABLE([flash],
   AC_HELP_STRING([--enable-flash], [enable building flash [[yes]]]),
   [
   case "${enableval}" in
      yes) BUILD_FLASH=yes ;;
      no)  BUILD_FLASH=no ;;
      *)   AC_MSG_ERROR(bad value ${enableval} for --disable-flash) ;;
   esac
   ], [BUILD_FLASH=yes])  dnl Default value

AC_SUBST(BUILD_FLASH)

dnl ----------------- build gtk-ui -----------------

AC_ARG_ENABLE([gtk-ui],
   AC_HELP_STRING([--enable-gtk-ui], [enable building gtk-ui [[yes]]]),
   [
   case "${enableval}" in
      yes) BUILD_GTK_UI=yes
           GTK_INCLUDES=`pkg-config --cflags gtk+-2.0 gthread-2.0`
           GTK_LIBS=`pkg-config --libs gtk+-2.0 gthread-2.0` ;;
      no) BUILD_GTK_UI=no ;;
      *)   AC_MSG_ERROR(bad value ${enableval} for --disable-gtk-ui) ;;
   esac
   ], [BUILD_GTK_UI=no BUILD_GTK_INCLUDES= BUILD_GTK_LIBS= ])  dnl Default value

AC_SUBST(BUILD_GTK_UI)
AC_SUBST(GTK_INCLUDES)
AC_SUBST(GTK_LIBS)

dnl ----------------- Forge dir -----------------

dnl Location of Forge dir.
FORGE_DIR="$BITMUNK_DIR/../forge/dist/forge"

AC_ARG_WITH(forge-dir,
   AC_HELP_STRING([--with-forge-dir=DIR],
      [use DIR for Forge JavaScript [BITMUNK_DIR/../forge/dist/forge]]),
   [
      case "${withval}" in
         yes) AC_MSG_ERROR(bad value ${withval} for --with-forge-dir) ;;
         no)  AC_MSG_ERROR(bad value ${withval} for --with-forge-dir) ;;
         *)   FORGE_DIR="${withval}" ;;
      esac
   ],
   [:]) dnl Default value

dnl Check Forge available.
if test ! -d "$FORGE_DIR"; then
   AC_MSG_ERROR(Forge dir "$FORGE_DIR" not found.  Use --with-forge-dir.)
fi

AC_SUBST(FORGE_DIR)

dnl ----------------- mxmlc debug -----------------

if test "$BUILD_FIREFOX_PLUGIN" = "yes" ; then
   MXMLC_DEBUG_MODE=false
elif test "x$BITMUNK_RELEASE_MODE" = "xofficial"; then
   MXMLC_DEBUG_MODE=false
else
   MXMLC_DEBUG_MODE=true
fi

AC_ARG_ENABLE([mxmlc-debug],
   AC_HELP_STRING([--disable-mxmlc-debug], [disable mxmlc debug mode [[false]]]),
   [
   case "${enableval}" in
      yes) MXMLC_DEBUG_MODE=true ;;
      no)  MXMLC_DEBUG_MODE=false ;;
      *)   AC_MSG_ERROR(bad value ${enableval} for --disable-mxmlc-debug) ;;
   esac
   ])

AC_SUBST(MXMLC_DEBUG_MODE)

dnl ----------------- end of options -----------------

echo -e "\n--------- Configuring Bitmunk Build Environment -----------"

PKG_PROG_PKG_CONFIG

# Check the system type
AC_CANONICAL_BUILD()

# Checking for standard build tools
#AC_LANG(C++)
AC_PROG_INSTALL()

# Common part of XUL config
if test "$BUILD_FIREFOX_PLUGIN" = "yes" ; then
   if test "x$PKG_CONFIG" != "x" && $PKG_CONFIG --exists libxul; then
      XULRUNNER_TYPE=["`$PKG_CONFIG --variable=includetype libxul`"]
      XULRUNNER_SDK_DIR=["`$PKG_CONFIG --variable=sdkdir libxul`"]
      XULRUNNER_IDL_DIR=["`$PKG_CONFIG --variable=idldir libxul`"]
      AC_PATH_PROG([XPIDL], [xpidl],, [$XULRUNNER_SDK_DIR/bin])
      XULRUNNER_CXX_FLAGS=["`$PKG_CONFIG --cflags libxul`"]
      XULRUNNER_LD_FLAGS=["`$PKG_CONFIG --libs-only-L libxul`"]
      XULRUNNER_IDL_FLAGS=["-I $XULRUNNER_IDL_DIR/$XULRUNNER_TYPE"]
   fi
   # Platform specific sections below and in Makefiles will setup XUL config
   # if not setup here.
fi

# Conditional setup for Linux build
if test "$BUILD_FOR_LINUX" = "yes" ; then
   AC_PROG_CXX()
   AC_CHECK_PROG(AR, ar, ar, not found)
   AC_CHECK_PROG(AS, as, as, not found)
   AC_CHECK_PROG(STRIP, strip, strip, not found)
   LIB_PREFIX=lib
   DYNAMIC_LIB_EXT=so
   STATIC_LIB_EXT=a
   EXECUTABLE_EXT=

   if test "$BUILD_FIREFOX_PLUGIN" = "yes" ; then
      if test "x$XPIDL" = "x" ; then
         # non-pkg-config defaults
         AC_PATH_PROG([XPIDL], xpidl /usr/lib/xulrunner-1.9/xpidl,, $PATH /)
         XULRUNNER_CXX_FLAGS="-I/usr/include/xulrunner-1.9/stable"
         XULRUNNER_LD_FLAGS="-L/usr/lib/xulrunner-devel-1.9/sdk/lib"
         XULRUNNER_IDL_FLAGS="-I /usr/share/idl/xulrunner-1.9/stable"
      fi
      if test "x$XPIDL" = "x" ; then
         AC_MSG_ERROR(xpidl not found)
      fi
      FIREFOX_PLATFORM="Linux_x86-gcc3"
   fi

   if test "$BUILD_FLASH" = "yes" ; then
      AC_CHECK_PROGS(MXMLC, mxmlc /usr/lib/flex3/bin/mxmlc, not found, $PATH /)
   fi
fi

# Conditional setup for Windows build
if test "$BUILD_FOR_WINDOWS" = "yes" ; then

   AC_CHECK_PROG(CXX, i586-mingw32msvc-g++,
                 i586-mingw32msvc-g++, not found)
   AC_CHECK_PROG(AR, i586-mingw32msvc-ar,
                 i586-mingw32msvc-ar, not found)
   AC_CHECK_PROG(AS, i586-mingw32msvc-as,
                 i586-mingw32msvc-as, not found)
   AC_CHECK_PROG(STRIP, i586-mingw32msvc-strip,
                 i586-mingw32msvc-strip, not found)
   STRIP_FLAGS="--strip-debug"
   LIB_PREFIX=
   DYNAMIC_LIB_EXT=dll
   STATIC_LIB_EXT=a
   EXECUTABLE_EXT=.exe

   # If the mingw32 compiler wasn't found, use the system default
   if test "$WINDOWS_CXX" = "not found" ; then
      AC_PROG_CXX()
   fi

   if test "$BUILD_FIREFOX_PLUGIN" = "yes" ; then
      # Setup the xulrunner variables
      if test "x$XPIDL" = "x" ; then
         # non-pkg-config defaults
         AC_CHECK_PROGS(XPIDL, xpidl)
         # Note: setup in the vcproj config
         #XULRUNNER_CXX_FLAGS=
         #XULRUNNER_LD_FLAGS=
         XULRUNNER_IDL_FLAGS="-I $BITMUNK_DIR/../gecko-sdk/idl"
      fi
      if test "x$XPIDL" = "x"; then
         AC_MSG_ERROR(xpidl not found)
      fi
      FIREFOX_PLATFORM="WINNT_x86-msvc"
   fi

   if test "$BUILD_FLASH" = "yes" ; then
      AC_CHECK_PROGS(MXMLC, mxmlc /usr/lib/flex3/bin/mxmlc, not found, $PATH /)
   fi
fi

# Conditional setup for MacOS build
if test "$BUILD_FOR_MACOS" = "yes" ; then
   AC_PROG_CXX(g++-4 g++)
   AC_CHECK_PROG(AR, ar, ar, not found)
   AC_CHECK_PROG(AS, as, as, not found)
   AC_CHECK_PROG(STRIP, strip, strip, not found)
   STRIP_FLAGS=-S
   LIB_PREFIX=lib
   DYNAMIC_LIB_EXT=dylib
   STATIC_LIB_EXT=a
   EXECUTABLE_EXT=

   if test "$BUILD_FIREFOX_PLUGIN" = "yes" ; then
      if test "x$XPIDL" = "x" ; then
         # non-pkg-config defaults
         AC_CHECK_PROGS(XPIDL, /usr/local/gecko-sdk/bin/xpidl xpidl)
         # Note: hard-coded values used in the Makefiles
         #XULRUNNER_CXX_FLAGS=
         #XULRUNNER_LD_FLAGS=
         XULRUNNER_IDL_FLAGS="-I /usr/local/gecko-sdk/idl"
      fi
      if test "x$XPIDL" = "x"; then
         AC_MSG_ERROR(xpidl not found)
      fi
      FIREFOX_PLATFORM="Darwin_x86-gcc3"
   fi

   if test "$BUILD_FLASH" = "yes" ; then
      AC_CHECK_PROGS(MXMLC, mxmlc /usr/lib/flex3/bin/mxmlc, not found, $PATH /)
   fi
fi

# Xulrunner is used to build the Firefox plugin
AC_SUBST(XPIDL)
AC_SUBST(XULRUNNER_CXX_FLAGS)
AC_SUBST(XULRUNNER_LD_FLAGS)
AC_SUBST(XULRUNNER_IDL_FLAGS)

# Platform variables
AC_SUBST(BITMUNK_PLATFORM)
AC_SUBST(FIREFOX_PLATFORM)
AC_SUBST(LIB_PREFIX)
AC_SUBST(DYNAMIC_LIB_EXT)
AC_SUBST(STATIC_LIB_EXT)
AC_SUBST(EXECUTABLE_EXT)

# Library symbol stripping variables
AC_SUBST(STRIP)
AC_SUBST(STRIP_FLAGS)

# Directory location variables
AC_SUBST(BITMUNK_SYSTEM_CONFIG_DIR)
AC_SUBST(BITMUNK_SYSTEM_CONFIG)
AC_SUBST(BITMUNK_SYSTEM_USER_CONFIG)
AC_SUBST(BITMUNK_HOME_DIR)
AC_SUBST(BITMUNK_PROFILES_DIR)
AC_SUBST(BITMUNK_USERS_DIR)
AC_SUBST(BITMUNK_WEBUI_PLUGINS_DIR)
AC_SUBST(BITMUNK_PURCHASE_DOWNLOAD_DIR)
AC_SUBST(BITMUNK_PURCHASE_TEMP_DIR)
AC_SUBST(BITMUNK_MODULES_DIR)
AC_SUBST(BITMUNK_BFP_API_VERSION)

dnl ----------------- optimization -----------------

dnl option to override default c++ optimization (often -g -O2)
AC_ARG_WITH(opt-flags,
  AC_HELP_STRING([--with-opt-flags=FLAGS],
    [use FLAGS for optimization (-O -fno-exceptions)]),
  [BITMUNK_CXX_OPT_FLAGS="${withval}"],
  [BITMUNK_CXX_OPT_FLAGS="-O -fno-exceptions"]) dnl Default value

CXXFLAGS="$CXXFLAGS $BITMUNK_CXX_OPT_FLAGS"

dnl ----------------------------------

# Generating files
AC_CONFIG_FILES([
   .gitignore
   Makefile
   setup/Makefile.base
   setup/docs.doxygen
   cpp/app/Makefile
   cpp/apps/bitmunk/Makefile
   cpp/apps/bfp-file-id/Makefile
   cpp/apps/format-detect/Makefile
   cpp/apps/tester/Makefile
   cpp/apps/firefox/Makefile
   cpp/apps/firefox/Makefile.common
   cpp/apps/firefox/components/Makefile
   cpp/apps/firefox/extension/chrome/bitmunk/content/bitmunk-urls.js
   cpp/apps/firefox/extension/install.rdf
   cpp/apps/firefox/plugins/Makefile
   cpp/bfp/BfpApiVersion.h
   cpp/bfp/Makefile
   cpp/common/Makefile
   cpp/customcatalog/Makefile
   cpp/data/Makefile
   cpp/eventreactor-downloadstate/Makefile
   cpp/eventreactor/Makefile
   cpp/filebrowser/Makefile
   cpp/gtkui/Makefile
   cpp/medialibrary/Makefile
   cpp/negotiate/Makefile
   cpp/node/Makefile
   cpp/nodemodule/Makefile
   cpp/peruserdb/Makefile
   cpp/portmapper/Makefile
   cpp/protocol/Makefile
   cpp/purchase/Makefile
   cpp/sell/Makefile
   cpp/system/Makefile
   cpp/test/Makefile
   cpp/tests/Makefile
   cpp/webui/Makefile
])

if test "x$BITMUNK_RELEASE_MODE" = "xdev"; then
AC_CONFIG_FILES([
	configs/apps/bpe.config
	configs/apps/bfp-file-id.config
	configs/apps/format-detect.config
	configs/apps/test.config
	configs/dev.d/bitmunk.bfp.Bfp.config
	configs/dev.d/bitmunk.catalog.CustomCatalog.config
	configs/dev.d/bitmunk.medialibrary.MediaLibrary.config
	configs/dev.d/bitmunk.purchase.Purchase.config
	configs/dev.d/bitmunk.sell.Sell.config
	configs/dev.d/bitmunk.system.System.config
	configs/dev.d/bitmunk.webui.WebUi.config
	configs/dev.d/node.config
	configs/dev-bpe.config
	configs/test.d/node.config
	configs/test.d/test.config
	configs/tests/bpe.config
	configs/tests/common.config
	configs/tests/test-customcatalog.config
	configs/tests/test-peerbuy.config
])
fi

if test "x$BUILD_FIREFOX_PLUGIN" = "xyes"; then
AC_CONFIG_FILES([
    configs/firefox-plugin.config
])
fi

# Generate scripts and configuration files for release
AC_CONFIG_FILES([
   installers/debian/changelog
])
AC_CONFIG_FILES([bitmunk-run], [chmod +x bitmunk-run])
AC_CONFIG_FILES([installers/debian/rules], [chmod +x installers/debian/rules])

# add newlines to internal output file list
CONFIGURE_GENERATED_FILES="`echo $ac_config_files | tr ' ' '\n'`"
AC_SUBST(CONFIGURE_GENERATED_FILES)

AC_OUTPUT

# Dump the path configuration
echo -e "\n--------- Bitmunk Path Configuration -----------"
echo "Modules dir       : $BITMUNK_MODULES_DIR"
echo "System config     : $BITMUNK_SYSTEM_CONFIG"
echo "System user config: $BITMUNK_SYSTEM_USER_CONFIG"
echo "Home dir          : $BITMUNK_HOME_DIR"
echo "Profiles dir      : $BITMUNK_PROFILES_DIR"
echo "Users dir         : $BITMUNK_USERS_DIR"
echo "SSL certificate   : $NODE_SSL_CERTIFICATE"
echo "SSL keyfiles dir  : $NODE_SSL_KEY"
echo "SSL CA file       : $NODE_SSL_CA_FILE"
echo "Web UIs dir       : $BITMUNK_WEBUI_PLUGINS_DIR"
echo "Purchase tmp dir  : $BITMUNK_PURCHASE_TEMP_DIR"
echo "Purchase d/l dir  : $BITMUNK_PURCHASE_DOWNLOAD_DIR"

# Dump the build configuration
echo -e "\n--------- Bitmunk Build Environment -----------"
echo "System            : $PACKAGE_NAME $FULL_VERSION"
echo "Build system      : $build_cpu-$build_vendor-$build_os"
echo "Build platform    : $BITMUNK_PLATFORM"

if test "x$BUILD_FOR_LINUX" = "xyes"; then
   echo "Linux             : enabled"
else
   echo "Linux             : disabled (use --enable-linux to enable)"
fi
if test "x$BUILD_FOR_WINDOWS" = "xyes"; then
   echo "Win32             : enabled"
else
   echo "Win32             : disabled (use --enable-windows to enable)"
fi
if test "x$BUILD_FOR_MACOS" = "xyes"; then
   echo "MacOS             : enabled"
else
   echo "MacOS             : disabled (use --enable-macos to enable)"
fi
echo "C++ compiler      : $CXX"
echo "C++ archiver      : $AR"
echo "Assembler         : $AS"
echo "Target CPU        : $TARGET_CPU"

echo "Optimization Flags: $BITMUNK_CXX_OPT_FLAGS"

echo "Firefox Platform  : $FIREFOX_PLATFORM"

if test "$BUILD_FLASH" = "yes" ; then
   echo "Adobe Flash Apps  : enabled"
else
   echo "Adobe Flash Apps  : disabled"
fi

if test "x$BUILD_DOCS" = "xyes"; then
   echo "Documentation     : enabled"
else
   echo "Documentation     : disabled (use --enable-docs to enable)"
fi

if test "x$BUILD_TESTS" = "xyes"; then
   echo "Tests             : enabled"
else
   echo "Tests             : disabled (use --enable-tests to enable)"
fi

if test "x$MO_DISABLE_LOG_LINE_NUMBERS" = "xyes"; then
   echo "Log line numbers  : disabled (use --enable-log-line-numbers to enable)"
else
   echo "Log line numbers  : enabled"
fi

echo "BFP API Version   : $BITMUNK_BFP_API_VERSION"
echo "GTK UI enabled    : $BUILD_GTK_UI"
echo "Forge dir         : $FORGE_DIR"
echo "Release mode      : $BITMUNK_RELEASE_MODE"

