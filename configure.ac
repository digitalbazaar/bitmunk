# Configure script for Digital Bazaar Bitmunk product line
# Usage: Run ./configure once 
# Author: Manu Sporny

AC_INIT([bitmunk],[3.2.2],[support@digitalbazaar.com])
AC_CONFIG_AUX_DIR(setup)

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

# Default BFP constants
BITMUNK_DEFAULT_BFP_ID=1
BITMUNK_DEFAULT_BFP_API_VERSION=${PACKAGE_VERSION}-`date +%Y%m%d%H%M`

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
      # By default, build the Firefox plugin in Windows and Mac OS X only
      if test "x$BUILD_FOR_WINDOWS" = xyes; then
         BUILD_FIREFOX_PLUGIN=yes
      elif test "x$BUILD_FOR_MACOS" = xyes; then
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

AC_ARG_ENABLE([release],
   AC_HELP_STRING(
      [--enable-release],
      [enable release build [[no]]]),
   [
   case "${enableval}" in
      yes) BITMUNK_RELEASE_MODE=yes ;;
      no)  BITMUNK_RELEASE_MODE=no ;;
      *)   AC_MSG_ERROR(bad value ${enableval} for --enable-release) ;;
   esac
   ],
   [BITMUNK_RELEASE_MODE=no]) dnl Default value
if test "x$BITMUNK_RELEASE_MODE" = xyes; then
   AC_DEFINE(BITMUNK_RELEASE_MODE, 1,
      [Build in release mode.])
else
   AC_DEFINE(BITMUNK_RELEASE_MODE, 0,
      [Do not build in release mode.])
fi
AC_SUBST(BITMUNK_RELEASE_MODE)

dnl ------------- Bitmunk SSL information --------------------
if test "x$BITMUNK_RELEASE_MODE" = xyes; then
   if test "x$BUILD_FIREFOX_PLUGIN" = xyes; then
      NODE_SSL_GENERATE=true
      NODE_SSL_CERTIFICATE="{RESOURCE_PATH}/pki/nodessl.crt"
      NODE_SSL_KEY="{RESOURCE_PATH}/pki/nodessl.key"
      NODE_SSL_CA_FILE="{RESOURCE_PATH}/pki/bitmunk-ca-file.pem"
   else
      # Often setup in a config file.
      NODE_SSL_GENERATE=false
      NODE_SSL_CERTIFICATE="/etc/bitmunk3/mysite.com.crt"
      NODE_SSL_KEY="/etc/bitmunk3/mysite.com.key"
      NODE_SSL_CA_FILE="/etc/bitmunk3/mysite-ca-file.pem"
   fi
else
   if test "x$BUILD_FIREFOX_PLUGIN" = xyes; then
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
fi


dnl ------------- Bitmunk API port numbers -------------------
BITMUNK_LOCAL_API_HOST=localhost
if test "x$BITMUNK_RELEASE_MODE" = xyes; then
   BITMUNK_API_HOST=bitmunk.com
   BITMUNK_API_PORT=8100
   BITMUNK_LOCAL_API_PORT=8100
   BITMUNK_SAMPLE_API_PORT=8100
   BITMUNK_FLASH_POLICY_PORT=8843
else
   BITMUNK_API_HOST=dev.bitmunk.com
   BITMUNK_API_PORT=19100
   BITMUNK_LOCAL_API_PORT=19100
   BITMUNK_SAMPLE_API_PORT=19100
   BITMUNK_FLASH_POLICY_PORT=19843
fi


dnl ---------- Linux configuration settings -----------------

if test "x$BUILD_FOR_LINUX" = "xyes" -a "x$BITMUNK_RELEASE_MODE" = xno; then
   dnl === Linux === Development ===
   BITMUNK_MODULES_DIR="$BITMUNK_DIR/dist/modules/bpe"
   BITMUNK_WEBUI_PLUGINS_DIR="$BITMUNK_DIR/cpp/apps/bitmunk/plugins"
   BITMUNK_PURCHASE_TEMP_DIR="tmp"
   BITMUNK_PURCHASE_DOWNLOAD_DIR="downloads"
   BITMUNK_PACKAGE_CONFIG=""
   BITMUNK_SYSTEM_CONFIG_DIR="$BITMUNK_DIR/configs"
   BITMUNK_SYSTEM_CONFIG="$BITMUNK_DIR/configs/dev.config"
   BITMUNK_SYSTEM_USER_CONFIG="dev.config"
if test "x$BUILD_FIREFOX_PLUGIN" = xyes; then
   dnl === Linux === Development === Firefox Plugin ===
   BITMUNK_LOCAL_API_PORT=19200
   BITMUNK_FLASH_POLICY_PORT=19943
   BITMUNK_MODULES_DIR="{RESOURCE_PATH}/modules"
   BITMUNK_WEBUI_PLUGINS_DIR="{RESOURCE_PATH}/webuiplugins"
   BITMUNK_PURCHASE_TEMP_DIR="tmp"
   BITMUNK_PURCHASE_DOWNLOAD_DIR="downloads"
   BITMUNK_PACKAGE_CONFIG=""
   BITMUNK_SYSTEM_CONFIG_DIR="$BITMUNK_DIR/configs"
   BITMUNK_SYSTEM_CONFIG="$BITMUNK_DIR/configs/dev.config"
   BITMUNK_SYSTEM_USER_CONFIG="dev.config"
fi
fi
if test "x$BUILD_FOR_LINUX" = "xyes" -a "x$BITMUNK_RELEASE_MODE" = xyes; then
   dnl === Linux === Release ===
   # If we're releasing the software, ensure that at least the prefix,
   # sysconfdir, and localstatedir variables are defined.
   if test "$prefix" = "NONE"; then
      prefix="/usr"
   fi
   if test "$sysconfdir" = "\${prefix}/etc"; then
      sysconfdir="/etc"
   fi
   if test "$localstatedir" = "\${prefix}/var"; then
      localstatedir="/var"
   fi

   BITMUNK_LOCAL_API_HOST=bitmunk.com
   BITMUNK_MODULES_DIR="/usr/lib/bitmunk$BITMUNK_VERSION_MAJOR"
   BITMUNK_WEBUI_PLUGINS_DIR="NOT_USED"
   BITMUNK_PURCHASE_TEMP_DIR="NOT_USED"
   BITMUNK_PURCHASE_DOWNLOAD_DIR="NOT_USED"
   BITMUNK_PACKAGE_CONFIG=""
   BITMUNK_SYSTEM_CONFIG_DIR="$sysconfdir/bitmunk3/conf.d"
   BITMUNK_SYSTEM_CONFIG="$sysconfdir/bitmunk3/system.config"
   BITMUNK_SYSTEM_USER_CONFIG="default.config"
if test "x$BUILD_FIREFOX_PLUGIN" = xyes; then
   dnl === Linux === Release === Firefox Plugin ===
   BITMUNK_LOCAL_API_HOST="0.0.0.0"
   BITMUNK_LOCAL_API_PORT=8200
   BITMUNK_SAMPLE_API_PORT=8100
   BITMUNK_MODULES_DIR="{RESOURCE_PATH}/modules"
   BITMUNK_WEBUI_PLUGINS_DIR="{RESOURCE_PATH}/webuiplugins"
   BITMUNK_PURCHASE_TEMP_DIR="tmp"
   BITMUNK_PURCHASE_DOWNLOAD_DIR="~/Music/Bitmunk"
   BITMUNK_PACKAGE_CONFIG="{RESOURCE_PATH}/configs/default.config"
   BITMUNK_SYSTEM_CONFIG_DIR="/etc/bitmunk3/conf.d"
   BITMUNK_SYSTEM_CONFIG="/etc/bitmunk3/system.config"
   BITMUNK_SYSTEM_USER_CONFIG="default.config"
fi
fi

dnl ---------- Mac OS X configuration settings -----------------

if test "x$BUILD_FOR_MACOS" = "xyes" -a "x$BITMUNK_RELEASE_MODE" = xno; then
   dnl === Mac OS === Development ===
   BITMUNK_MODULES_DIR="$BITMUNK_DIR/modules/macos"
   BITMUNK_WEBUI_PLUGINS_DIR="$BITMUNK_DIR/cpp/apps/bitmunk/plugins"
   BITMUNK_PURCHASE_TEMP_DIR="/tmp/dev/tmp"
   BITMUNK_PURCHASE_DOWNLOAD_DIR="/tmp/dev/downloads"
   BITMUNK_PACKAGE_CONFIG=""
   BITMUNK_SYSTEM_CONFIG_DIR="$BITMUNK_DIR/configs"
   BITMUNK_SYSTEM_CONFIG="$BITMUNK_DIR/configs/dev.config"
   BITMUNK_SYSTEM_USER_CONFIG="dev.config"
if test "x$BUILD_FIREFOX_PLUGIN" = xyes; then
   dnl === Mac OS === Development === Firefox Plugin ===
   BITMUNK_LOCAL_API_PORT=19200
   BITMUNK_FLASH_POLICY_PORT=19943
   BITMUNK_MODULES_DIR="{RESOURCE_PATH}/modules"
   BITMUNK_WEBUI_PLUGINS_DIR="{RESOURCE_PATH}/webuiplugins"
   BITMUNK_PURCHASE_TEMP_DIR="tmp"
   BITMUNK_PURCHASE_DOWNLOAD_DIR="downloads"
   BITMUNK_PACKAGE_CONFIG=""
   BITMUNK_SYSTEM_CONFIG_DIR="$BITMUNK_DIR/configs"
   BITMUNK_SYSTEM_CONFIG="$BITMUNK_DIR/configs/dev.config"
   BITMUNK_SYSTEM_USER_CONFIG="dev.config"
fi
fi
if test "x$BUILD_FOR_MACOS" = "xyes" -a "x$BITMUNK_RELEASE_MODE" = xyes; then
   dnl === Mac OS === Release ===
   BITMUNK_MODULES_DIR="${libdir}/bitmunk$BITMUNK_VERSION_MAJOR"
   BITMUNK_WEBUI_PLUGINS_DIR="NOT_USED"
   BITMUNK_PURCHASE_TEMP_DIR="NOT_USED"
   BITMUNK_PURCHASE_DOWNLOAD_DIR="NOT_USED"
   BITMUNK_PACKAGE_CONFIG=""
   BITMUNK_SYSTEM_CONFIG_DIR="/etc/bitmunk3/conf.d"
   BITMUNK_SYSTEM_CONFIG="/etc/bitmunk3/system.config"
   BITMUNK_SYSTEM_USER_CONFIG="default.config"
if test "x$BUILD_FIREFOX_PLUGIN" = xyes; then
   dnl === Mac OS === Release === Firefox Plugin ===
   BITMUNK_LOCAL_API_HOST="0.0.0.0"
   BITMUNK_LOCAL_API_PORT=8200
   BITMUNK_SAMPLE_API_PORT=8100
   BITMUNK_MODULES_DIR="{RESOURCE_PATH}/modules"
   BITMUNK_WEBUI_PLUGINS_DIR="{RESOURCE_PATH}/webuiplugins"
   BITMUNK_PURCHASE_TEMP_DIR="tmp"
   BITMUNK_PURCHASE_DOWNLOAD_DIR="~/Music/Bitmunk"
   BITMUNK_PACKAGE_CONFIG="{RESOURCE_PATH}/configs/default.config"
   BITMUNK_SYSTEM_CONFIG_DIR="/etc/bitmunk3/conf.d"
   BITMUNK_SYSTEM_CONFIG="/etc/bitmunk3/system.config"
   BITMUNK_SYSTEM_USER_CONFIG="default.config"
fi
fi

dnl ---------- Windows configuration settings -----------------

if test "x$BUILD_FOR_WINDOWS" = "xyes" -a "x$BITMUNK_RELEASE_MODE" = xno; then
   dnl === Windows === Development ===
   BITMUNK_MODULES_DIR="../../bpe"
   BITMUNK_WEBUI_PLUGINS_DIR="../../plugins"
   BITMUNK_PURCHASE_TEMP_DIR="../../tmp"
   BITMUNK_PURCHASE_DOWNLOAD_DIR="../../downloads"
   BITMUNK_PACKAGE_CONFIG=""
   BITMUNK_SYSTEM_CONFIG="NOT_USED"
   BITMUNK_SYSTEM_USER_CONFIG="NOT_USED"
if test "x$BUILD_FIREFOX_PLUGIN" = xyes; then
   dnl === Windows === Development === Firefox Plugin ===
   BITMUNK_LOCAL_API_PORT=19200
   BITMUNK_FLASH_POLICY_PORT=19943
   BITMUNK_MODULES_DIR="{RESOURCE_PATH}/modules"
   BITMUNK_WEBUI_PLUGINS_DIR="{RESOURCE_PATH}/webuiplugins"
   BITMUNK_PURCHASE_TEMP_DIR="tmp"
   BITMUNK_PURCHASE_DOWNLOAD_DIR="downloads"
   BITMUNK_PACKAGE_CONFIG="configs/dev.config"
   # NOTE: See note about this var in Win/Rel/FF section below.
   #BITMUNK_SYSTEM_CONFIG="C:/Program Files/Bitmunk3/configs/system.config"
   BITMUNK_SYSTEM_CONFIG=""
   BITMUNK_SYSTEM_USER_CONFIG="dev.config"
fi
fi
if test "x$BUILD_FOR_WINDOWS" = "xyes" -a "x$BITMUNK_RELEASE_MODE" = xyes; then
   dnl === Windows === Release ===
   BITMUNK_MODULES_DIR="NOT_USED"
   BITMUNK_WEBUI_PLUGINS_DIR="NOT_USED"
   BITMUNK_PURCHASE_TEMP_DIR="NOT_USED"
   BITMUNK_PURCHASE_DOWNLOAD_DIR="NOT_USED"
   BITMUNK_PACKAGE_CONFIG="NOT_USED"
   BITMUNK_SYSTEM_CONFIG="NOT_USED"
   BITMUNK_SYSTEM_USER_CONFIG="NOT_USED"
if test "x$BUILD_FIREFOX_PLUGIN" = xyes; then
   dnl === Windows === Release === Firefox Plugin ===
   BITMUNK_LOCAL_API_HOST="0.0.0.0"
   BITMUNK_LOCAL_API_PORT=8200
   BITMUNK_SAMPLE_API_PORT=8100
   BITMUNK_MODULES_DIR="{RESOURCE_PATH}/modules"
   BITMUNK_WEBUI_PLUGINS_DIR="{RESOURCE_PATH}/webuiplugins"
   BITMUNK_PURCHASE_TEMP_DIR="tmp"
   BITMUNK_PURCHASE_DOWNLOAD_DIR="~/My Documents/My Music/Bitmunk"
   BITMUNK_PACKAGE_CONFIG="{RESOURCE_PATH}/configs/default.config"
   # NOTE: Not loading a system default file on Windows.
   # In the future, change "C:" to a magic %%-type of var.
   #BITMUNK_SYSTEM_CONFIG="C:/Program Files/Bitmunk3/configs/system.config"
   BITMUNK_SYSTEM_CONFIG=""
   BITMUNK_SYSTEM_USER_CONFIG="default.config"
fi
fi

AC_SUBST(NODE_SSL_GENERATE)
AC_SUBST(NODE_SSL_CERTIFICATE)
AC_SUBST(NODE_SSL_KEY)
AC_SUBST(NODE_SSL_CA_FILE)

AC_SUBST(BITMUNK_API_HOST)
AC_SUBST(BITMUNK_API_PORT)
AC_SUBST(BITMUNK_LOCAL_API_HOST)
AC_SUBST(BITMUNK_LOCAL_API_PORT)
AC_SUBST(BITMUNK_SAMPLE_API_PORT)
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

dnl ----------------- tests -----------------

if test "x$BUILD_FIREFOX_PLUGIN" = "xyes" ; then
   BUILD_TESTS=no
elif test "x$BITMUNK_RELEASE_MODE" = "xyes"; then
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
   if test "x$BITMUNK_RELEASE_MODE" = "xyes"; then
      MO_DISABLE_LOG_LINE_NUMBERS=yes
   fi
fi
if test "x$MO_DISABLE_LOG_LINE_NUMBERS" = "xyes"; then
   AC_DEFINE(MO_DISABLE_LOG_LINE_NUMBERS, 1,
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
if test "x$BITMUNK_RELEASE_MODE" = xyes; then
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

dnl default location of profiles.
if test "x$BITMUNK_RELEASE_MODE" = xyes \
   -o "x$BUILD_FIREFOX_PLUGIN" = "xyes" ; then
   BITMUNK_PROFILES_DIR="profiles"
else
   if test -d "${BITMUNK_DIR}/pki"; then
      BITMUNK_PROFILES_DIR="$BITMUNK_DIR/pki"
   else
      AC_MSG_NOTICE(pki profiles dir not found)
      BITMUNK_PROFILES_DIR="profiles"
   fi
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
if test "x$BITMUNK_RELEASE_MODE" = xno; then
   BITMUNK_BFP_API_VERSION=${PACKAGE_VERSION}-1
else
   BITMUNK_BFP_API_VERSION=${BITMUNK_DEFAULT_BFP_API_VERSION}
fi

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

dnl ----------------- mxmlc debug -----------------

if test "$BUILD_FIREFOX_PLUGIN" = "yes" ; then
   MXMLC_DEBUG_MODE=false
elif test "$BITMUNK_RELEASE_MODE" = "yes"; then
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

# Check the system type
AC_CANONICAL_BUILD()

# Checking for standard build tools
#AC_LANG(C++)
AC_PROG_INSTALL()

AC_PATH_PROG([PKGCONFIG], [pkg-config])

# Common part of XUL config
if test "$BUILD_FIREFOX_PLUGIN" = "yes" ; then
   if test "x$PKGCONFIG" != "x" && $PKGCONFIG --exists libxul; then
      XULRUNNER_TYPE=["`$PKGCONFIG --variable=includetype libxul`"]
      XULRUNNER_SDK_DIR=["`$PKGCONFIG --variable=sdkdir libxul`"]
      XULRUNNER_IDL_DIR=["`$PKGCONFIG --variable=idldir libxul`"]
      AC_PATH_PROG([XPIDL], [xpidl],, [$XULRUNNER_SDK_DIR/bin])
      XULRUNNER_CXX_FLAGS=["`$PKGCONFIG --cflags libxul`"]
      XULRUNNER_LD_FLAGS=["`$PKGCONFIG --libs-only-L libxul`"]
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
   STRIP_FLAGS=

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
      LIB_PREFIX="lib"
      SHARED_LIB_EXTENSION="so"
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
      LIB_PREFIX=
      SHARED_LIB_EXTENSION="dll"
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
   MACOS_CXX="$CXX"

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
      LIB_PREFIX=lib
      SHARED_LIB_EXTENSION="dylib"
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
AC_SUBST(SHARED_LIB_EXTENSION)

# Library symbol stripping variables
AC_SUBST(STRIP)
AC_SUBST(STRIP_FLAGS)

# Directory location variables
AC_SUBST(BITMUNK_PACKAGE_CONFIG)
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
AC_SUBST(BITMUNK_BFP_PEM_DIR)
AC_SUBST(BITMUNK_BFP_ID)
AC_SUBST(BITMUNK_BFP_PRIVATE_KEY_PEM)
AC_SUBST(BITMUNK_BFP_PRIVATE_KEY_PW)
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
   Makefile
   setup/Makefile.base
   setup/docs.doxygen
   cpp/app/Makefile
   cpp/apps/bitmunk/Makefile
   cpp/apps/bitmunk/config.h
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
   cpp/peruserdb/Makefile
   cpp/portmapper/Makefile
   cpp/protocol/Makefile
   cpp/purchase/Makefile
   cpp/sell/Makefile
   cpp/system/Makefile
   cpp/system/config.h
   cpp/test/Makefile
   cpp/tests/Makefile
   cpp/tests/bitmunk-unit-tests.h
   cpp/webui/Makefile
])

if test "x$BITMUNK_RELEASE_MODE" = "xyes"; then
AC_CONFIG_FILES([
	configs/live.d/bitmunk.bfp.Bfp.config
	configs/live.d/bitmunk.catalog.CustomCatalog.config
	configs/live.d/bitmunk.medialibrary.MediaLibrary.config
	configs/live.d/bitmunk.purchase.Purchase.config
	configs/live.d/bitmunk.sell.Sell.config
	configs/live.d/bitmunk.system.System.config
	configs/live.d/bitmunk.webui.WebUi.config
	configs/live.d/node.config
])
else
AC_CONFIG_FILES([
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
])
fi

# Generate scripts and configuration files for release
AC_CONFIG_FILES([
   installers/debian/changelog
])
AC_CONFIG_FILES([bitmunk-bpe], [chmod +x bitmunk-bpe])
AC_CONFIG_FILES([bitmunk-bpe-gdb], [chmod +x bitmunk-bpe-gdb])
AC_CONFIG_FILES([bitmunk-bpe-valgrind], [chmod +x bitmunk-bpe-valgrind])
AC_CONFIG_FILES([bitmunk-app], [chmod +x bitmunk-app])
AC_CONFIG_FILES([bitmunk-app-gdb], [chmod +x bitmunk-app-gdb])
AC_CONFIG_FILES([bitmunk-app-valgrind], [chmod +x bitmunk-app-valgrind])
AC_CONFIG_FILES([bitmunk-run], [chmod +x bitmunk-run])
AC_CONFIG_FILES([bitmunk-run-gdb], [chmod +x bitmunk-run-gdb])
AC_CONFIG_FILES([installers/debian/rules], [chmod +x installers/debian/rules])

AC_OUTPUT

# Write a proper .gitignore file based on files that are generated
GIT_IGNORE_FILES=".gitignore build dist autom4te.cache config.log config.status 
configure"
GIT_IGNORE_FILES="$GIT_IGNORE_FILES $ac_config_files"
echo $GIT_IGNORE_FILES | tr ' ' '\n' > .gitignore

# Dump the path configuration
echo -e "\n--------- Bitmunk Path Configuration -----------"
echo "Modules dir       : $BITMUNK_MODULES_DIR"
if test "x$BITMUNK_PACKAGE_CONFIG" = "x"; then
echo "Package config    : (empty)"
else
echo "Package config    : $BITMUNK_PACKAGE_CONFIG"
fi
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
echo "Release build     : $BITMUNK_RELEASE_MODE"
