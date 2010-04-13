/*
 * Implementation of the BitmunkExtension.h and IBitmunkExtension.h
 * headers. This class is the implementation for the XPCOM interfaces
 * which are called via Javascript to control the Bitmunk components.
 */
#if defined(__linux__) || defined(__APPLE__)
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#elif defined(WIN32)
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#endif

#include <string>
#include <cstdio>
#include "nsXPCOM.h"
#include "nsCOMPtr.h"
#include "nsIProperties.h"
#include "nsIServiceManager.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIGenericFactory.h"
#include "nsIFile.h"
#include "nsILocalFile.h"
#include "BitmunkExtension.h"

using namespace std;

// uncomment to enable windows debugging
//#define WINDOWS_DEBUG_MODE 1
//#define WINDOWS_BITMUNK_LOG_FILE 1

/*
 * Magic Firefox macro that creates a default factory constructor for
 * the Bitmunk extension.
 */
NS_GENERIC_FACTORY_CONSTRUCTOR(nsBitmunkExtension)

/*
 * Magic Firefox macro that states that the nsBitmunkExtension class
 * supports the IDL defined in IBitmunkExtension.
 */
NS_IMPL_ISUPPORTS1(nsBitmunkExtension, nsIBitmunkExtension)

// the default node port to use if a node port file cannot be found
#define DEFAULT_NODE_PORT 8200
#define NODE_PORT_FILE    "node-port-file.txt"

// define system-dependent constants:

// linux
#if defined(__linux__)
#define ENV_LIBRARY_PATH "LD_LIBRARY_PATH"
#define ENV_HOME         "HOME"
#define PATH_SEPARATOR   ":"
#define FILE_SEPARATOR   "/"
#define BITMUNK_BIN      "bitmunk"
#define BITMUNK_BIN_PATH BITMUNK_BIN
#if defined(__i386__)
#define PLATFORM_DIR     "Linux_x86-gcc3"
#endif

static void _cleanup() {};

// apple
#elif defined(__APPLE__)
#define ENV_LIBRARY_PATH "DYLD_LIBRARY_PATH"
#define ENV_HOME         "HOME"
#define PATH_SEPARATOR   ":"
#define FILE_SEPARATOR   "/"
#define BITMUNK_BIN      "bitmunk"
#define BITMUNK_BIN_PATH BITMUNK_BIN
#if defined(__i386__)
#define PLATFORM_DIR     "Darwin_x86-gcc3"
#endif

static void _cleanup() {};

// windows
#elif defined(WIN32)
#define ENV_LIBRARY_PATH ""
#define ENV_HOME         "USERPROFILE"
#define PATH_SEPARATOR   ";"
#define FILE_SEPARATOR   "\\"
#define BITMUNK_BIN      "bitmunk.exe"
#define BITMUNK_BIN_PATH "libs\\" BITMUNK_BIN
#define PLATFORM_DIR     "WINNT_x86-msvc"

#ifdef WINDOWS_DEBUG_MODE
// log file
#define WINDOWS_FIREFOX_LOG "C:\\temp\\bitmunk-firefox.log"
// logging macros
#define _logOpen() \
   return fopen(WINDOWS_FIREFOX_LOG, "a")
#define _logClose(fp) \
   fclose(fp)
#define _log(fp, args...) \
   fprintf(fp, ##args)
#else
// empty logging macros
#define _logOpen() return NULL
#define _log(fp, args...)
#define _logClose(fp)
#define _logLastError(fp)
#endif

#ifdef WINDOWS_BITMUNK_LOG_FILE
// log file for bitmunk process output
#define WINDOWS_BITMUNK_LOG "C:\\temp\\bitmunk-output.log"
#endif

/* We need to keep track of an environment that can start the
   windows process properly. Seriously. */
static LPCH gEnvironment = NULL;

// Windows has to create a globally-used environment once and then it will
// be cloned on each CreateProcess call
inline static void _createWindowsEnvironment(string& path)
{
   /* For windows, we must get the old PATH environment variable,
      append our library directory to it and save a handle to the current
      environment strings after doing so. Then we pass that environment
      to CreateProcess(). Once we're finished with that, we restore the
      other PATH value and free the handle to the current environment.

      We cannot simply call SetEnvironmentVariable("PATH") and then pass
      NULL to CreateProcess. This does not work... and we're not sure
      why. All we get is an Access Violation exit code of 0xc0000005
      from the process.
    */

   // get the old path value
   LPTSTR newPath = NULL;
   LPTSTR oldPath = NULL;
   DWORD newPathSize = 0;
   DWORD oldPathSize = GetEnvironmentVariable("PATH", oldPath, 0);
   if(oldPathSize == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND)
   {
      // no PATH in current environment, just create new path and
      // append library path
      DWORD length = libraryPath.length() + 1;
      newPath = (LPTSTR)malloc(length);
      StringCchCopy(newPath, length, const_cast<LPSTR>(libraryPath.c_str()));
   }
   else
   {
      // allocate enough space for the old path and get it
      LPTSTR oldPath = (LPTSTR)malloc(oldPathSize * sizeof(TCHAR));
      GetEnvironmentVariable("PATH", oldPath, oldPathSize);

      // allocate enough space for the old path AND the new path
      libraryPath.insert(0, PATH_SEPARATOR);

      newPathSize = oldPathSize + libraryPath.length();
      newPath = (LPTSTR)malloc(newPathSize * sizeof(TCHAR));

      // copy old path into new path and append library directory
      StringCchCopy(newPath, oldPathSize, oldPath);
      StringCchCat(
         newPath, oldPathSize + libraryPath.length(),
         const_cast<LPSTR>(libraryPath.c_str()));
   }

   /* Note: We no longer save the old path and reset it after
      calling create process because we have to save the environment
      for use later. No one knows why. Any calls to start the bitmunk
      app after the first call will have no usable environment that
      we can append our path to. And you can't just add the typical
      windows paths (for system32, etc)... that doesn't work. We clean
      up the global environment variable when we destroy the plugin.
    */
   // set new path value and then free new and old paths
   path = newPath;
   SetEnvironmentVariable("PATH", newPath);
   free(newPath);
   if(oldPath != NULL)
   {
      free(oldPath);
   }

   // *NOW* get the current environment since our PATH has been updated
   gEnvironment = GetEnvironmentStrings();
}

// Windows has to clean up its globally created environment.
static void _cleanup()
{
   // release global environment handle
   if(gEnvironment != NULL)
   {
      FreeEnvironmentStrings(gEnvironment);
   }
}

// Windows doesn't have a good string replace function. Seriously.
static string& _stringReplaceAll(
   string& str, const string& find, const string& replace)
{
   string::size_type found = str.find(find);
   while(found != string::npos)
   {
      str.replace(found, find.length(), replace);
      found = str.find(find, found + replace.length());
   }

   return str;
}

// Windows must quote command line parameters
inline static void _quote(string& str)
{
   str.insert(0, "\"");
   str.push_back('"');
}

#ifdef WINDOWS_DEBUG_MODE
// Gets an error message on Windows
static string _getLastErrorMsg(DWORD error)
{
   string rval;

   // get the last error in an allocated string buffer
   //
   // Note: A LPTSTR is a pointer to an UTF-8 string (TSTR). If any
   // characters in the error message are not ASCII, then they will
   // get munged unless the bytes in the returned string are converted
   // back to wide characters for display/other use
   LPVOID lpBuffer;
   char errorString[100];
   memset(errorString, 0, 100);
   unsigned int size = FormatMessage(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
      FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dwLastError, 0,
      (LPTSTR)&lpBuffer, 0, NULL);
   if(size > 0)
   {
      // copy into error string
      size = (size > 99) ? 99 : size;
      memcpy(errorString, lpBuffer, size);
      rval = errorString;
   }

   // free lpBuffer
   LocalFree(lpBuffer);

   return rval;
}

// logs the last error message on Windows
#define _logLastError(fp, str) \
DWORD error = GetLastError(); \
if(error != 0) \
{ \
   FILE* tmp = fp; \
   if(fp == NULL) \
   { \
      tmp = _logOpen(); \
   } \
   string msg = _getLastErrorMsg(error); \
   _log(tmp, "%s: (%d)\nError message: %s\n", error, msg.c_str()); \
   if(fp == NULL) \
   { \
      _logClose(tmp); \
   } \
}
#endif

// Creates a log file for bitmunk process std output on Windows
inline static void _createBitmunkLogFile(
   STARTUPINFO& siStartupInfo, BOOL& inheritHandles, HANDLE& hFile, FILE* fp)
{
#ifdef WINDOWS_BITMUNK_LOG_FILE
   // cannot inherit handles when piping output to a file
   inheritHandles = FALSE;
   SECURITY_ATTRIBUTES securityAttr;
   securityAttr.nLength = sizeof(securityAttr);
   securityAttr.bInheritHandle = TRUE;
   securityAttr.lpSecurityDescriptor = NULL;
   hFile = CreateFile(
      WINDOWS_BITMUNK_LOG,
      GENERIC_WRITE, FILE_SHARE_WRITE, &securityAttr,
      OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
   siStartupInfo.dwFlags    = STARTF_USESTDHANDLES;
   siStartupInfo.hStdInput  = INVALID_HANDLE_VALUE;
   if(hFile != INVALID_HANDLE_VALUE)
   {
      siStartupInfo.hStdError  = hFile;
      siStartupInfo.hStdOutput = hFile;
   }
   else
   {
      _log(fp, "Could not get file handle for debug output.\n");
   }
#endif
}

#endif

/**
 * Starts the bitmunk process.
 *
 * @param bitmunkApp the full path to the bitmunk executable.
 * @param argv the arguments to the bitmunk app.
 * @param libraryPath the library path environment variable.
 * @param homePath the home environment variable.
 *
 * @return on linux/macos: does not return on success and returns 0 on
 *         failure, on windows: pid on success, 0 on failure.
 */
inline static PRInt32 _execve(
   string& bitmunkApp, char* argv[], string& libraryPath, string& homePath)
{
   PRInt32 rval = 0;

// linux/apple specific implementation
#if defined(__linux__) || defined(__APPLE__)
   // build environment
   char* envp[] =
   {
      (char*)libraryPath.c_str(),
      (char*)homePath.c_str(),
      (NULL)
   };

   // the child should overlay the Bitmunk application
   execve(bitmunkApp.c_str(), argv, envp);
   perror("execve");
   exit(EXIT_FAILURE);

// windows specific implementation
#elif defined(WIN32)
   // build the parameter list
   string params;
   for(int i = 0; argv[i] != NULL; i++)
   {
      if(i > 0)
      {
         params.push_back(' ');
      }
      params.append(argv[i]);
   }

   // create windows environment to be cloned, we only need to do this once
   if(gEnvironment == NULL)
   {
      _createWindowsEnvironment(libraryPath);
   }

   // log information about starting bitmunk
   FILE* fp = _logOpen();
   _log(fp, "Launching bitmunk app...\n");
   _log(fp, "PATH: %s\n\n", libraryPath.c_str());
   _log(fp, "CreateProcess() params:\n\n");
   _log(fp, "lpApplicationName: %s\n\n", bitmunkApp.c_str());
   _log(fp, "lpCommandLine: %s\n\n", params.c_str());

   /* CreateProcess API initialization */
   STARTUPINFO siStartupInfo;
   PROCESS_INFORMATION piProcessInfo;
   memset(&siStartupInfo, 0, sizeof(siStartupInfo));
   memset(&piProcessInfo, 0, sizeof(piProcessInfo));
   siStartupInfo.cb = sizeof(siStartupInfo);
   BOOL inheritHandles = TRUE;

   // open bitmunk process log file
   HANDLE hFile = INVALID_HANDLE_VALUE;
   _createBitmunkLogFile(siStartupInfo, inheritHandles, hFile, fp);

   if(CreateProcess(
      const_cast<LPSTR>(bitmunkApp.c_str()),
      const_cast<LPSTR>(params.c_str()), 0, 0,
      inheritHandles,
      CREATE_NO_WINDOW, /* create no console window */
      gEnvironment, /* use saved global environment */
      NULL, /* current directory not needed */
      &siStartupInfo, &piProcessInfo) != 0)
   {
      _log(fp, "Process created.\n");

      /* Wait to see if process immediately terminates */
      DWORD waitRc = WaitForSingleObject(piProcessInfo.hProcess, 1000);
      if(waitRc == 0)
      {
         // process returned immediately (some error occurred)
         _log(fp, "Process terminated.\n");
         DWORD exitCode = 0;
         if(GetExitCodeProcess(piProcessInfo.hProcess, &exitCode))
         {
            _log(fp, "Process exit code: (%d)\n", exitCode);
         }
      }
      else
      {
         // success, return windows process ID
         rval = GetProcessId(piProcessInfo.hProcess);
      }
   }
   else
   {
      /* CreateProcess failed */
      _logLastError(fp, "Process creation failed");
   }

   /* Release handles */
   if(hFile != INVALID_HANDLE_VALUE)
   {
      CloseHandle(hFile);
   }
   CloseHandle(piProcessInfo.hProcess);
   CloseHandle(piProcessInfo.hThread);

   if(rval > 0)
   {
      _log(fp, "Bitmunk launch succeeded. PID: %d\n", rval);
   }
   else
   {
      _log(fp, "Bitmunk launch failed.\n");
   }

   // close log file
   _logClose(fp);

#endif // end OS-specific implementations

   return rval;
}

/**
 * Starts the bitmunk application using the given parameters, with a
 * specific implementation for each supported OS.
 *
 * @param bitmunkApp the full path to the bitmunk binary.
 * @param libraryPath the environment library path.
 * @param homePath the environment home path.
 * @param resourcePath the config system resource path option.
 * @param packageConfigFile the config system package config option.
 * @param nodePortFile the config system node port file option.
 * @param logFile the config system log file option.
 *
 * @return the pid of the new bitmunk process or 0 on error.
 */
static PRInt32 _startBitmunk(
   string& bitmunkApp, string& libraryPath, string& homePath,
   string& resourcePath, string& packageConfigFile, string& nodePortFile,
   string& logFile)
{
   PRInt32 rval = 0;

   /* This is the path to the node port file config key, a '.' is a
      separator between levels in our config tree, and since this value
      resides at:

      "bitmunk.system.System" : {
         "nodePortFile" : "<insert value here>"
      }

      We must escape the first 2 dots, but not the last one.
   */
   string nodePortFileKey = "bitmunk\\.system\\.System.nodePortFile";

// linux/apple specific implementation
#if defined(__linux__) || defined(__APPLE__)
   pid_t pid = fork();
// windows specific implementation
#elif defined(WIN32)
   int pid = 0;

   // normalize resource path to '/' (it is passed to the Bitmunk app, and
   // it understands '/' to be a valid file separator on any OS)
   _stringReplaceAll(resourcePath, "\\", "/");

   // escape bitmunk application path (it must be escaped for CreateProcess,
   // quoting it will cause CreateProcess to fail)
   _stringReplaceAll(bitmunkApp, "\\", "\\\\");

   // quote parameter values
   _quote(resourcePath);
   _quote(packageConfigFile);
   _quote(nodePortFileKey);
   _quote(nodePortFile);
   _quote(logFile);
#endif
   if(pid == 0)
   {
      // build command line arguments
      char* argv[] =
      {
         (char*)BITMUNK_BIN,
         (char*)"--resource-path", (char*)resourcePath.c_str(),
         (char*)"--config", (char*)packageConfigFile.c_str(),
         (char*)"--package-config", (char*)packageConfigFile.c_str(),
         (char*)"--option", (char*)nodePortFileKey.c_str(),
         (char*)nodePortFile.c_str(),
         (char*)"--log", (char*)logFile.c_str(),
         (NULL)
      };

      // execute bitmunk application
      rval = _execve(bitmunkApp, argv, libraryPath, homePath);
   }
   else if(pid == -1)
   {
      //printf("Failed to launch Bitmunk Application\n");
      rval = pid;
   }
   else
   {
      //printf("Attempting to launch Bitmunk...\n");
      rval = pid;
   }

   return rval;
}

/**
 * Stops a running bitmunk application, with a specific implementation for
 * each supported OS.
 *
 * @param pid the process ID for the running bitmunk application.
 *
 * @return NS_OK on success, something else on failure.
 */
nsresult _stopBitmunk(PRInt32 pid)
{
   nsresult rval = NS_ERROR_NOT_IMPLEMENTED;

// linux/apple specific implementation
#if defined(__linux__) || defined(__APPLE__)
   if(kill(pid, SIGTERM) == 0)
   {
      rval = NS_OK;
   }
   else
   {
      // FIXME: set rval to something useful, not "NS_ERROR_NOT_IMPLEMENTED"
   }
// windows specific implementation
#elif defined(WIN32)
   HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
   if(hProcess != NULL)
   {
      // second parameter is exit code
      TerminateProcess(hProcess, 0);
      CloseHandle(hProcess);
      rval = NS_OK;
   }
   else
   {
      // FIXME: set rval to something useful, not "NS_ERROR_NOT_IMPLEMENTED"
      _logLastError(NULL, "Process termination failed");
   }
#endif // end OS-specific implementations

   return rval;
}

/* Definition for the extension API */

nsBitmunkExtension::nsBitmunkExtension()
{
   /* member initializers and constructor code */
}

nsBitmunkExtension::~nsBitmunkExtension()
{
   /* destructor code */

   // system-dependent cleanup
   _cleanup();
}

/**
 * Gets the port for the local background Bitmunk application.
 *
 * @param pluginDir the plugin directory that contains the Bitmunk application
 *                  as well as all libraries, modules and content.
 *
 * @return the port for the local background Bitmunk application.
 */
PRInt32 nsBitmunkExtension::getBitmunkNodePort(nsCString pluginDir)
{
   // the default port value
   PRInt32 rval = DEFAULT_NODE_PORT;

   // get the path to the node port file and open it
   string path(pluginDir.get());
   path.append(FILE_SEPARATOR NODE_PORT_FILE);
   FILE* fp = fopen(path.c_str(), "r");

   // check to make sure the file exists
   if(fp != NULL)
   {
      // a port number is at most 5 bytes long (max port=65535)
      char port[6];
      memset(port, 0, 6);
      bool error = false;
      int count = fread(port, 1, 5, fp);
      if(count != 5)
      {
         // check for an error (eof will not be an error, and we might
         // get that if the port is fewer than 5 bytes long which is fine)
         if(ferror(fp) != 0)
         {
            // there was an error reading the file
            error = true;
         }
      }

      // close the file
      fclose(fp);

      if(!error)
      {
         // we read the port successfully, so use it instead of the default
         // if it parses as something other than 0
         PRInt32 tmp = (PRInt32)(strtol(port, NULL, 10) & 0xFFFFFFFF);
         if(tmp != 0)
         {
            rval = tmp;
         }
      }
   }

   return rval;
}

/**
 * Launches the Bitmunk application from the plugin directory.
 *
 * @param pluginDir the plugin directory that contains the Bitmunk application
 *                  as well as all libraries, modules and content.
 *
 * @return the PID of the started process or 0 if already started or error.
 */
PRInt32 nsBitmunkExtension::launchBitmunkApplication(nsCString pluginDir)
{
   PRInt32 rval = 0;

   // create a global to prevent double-startup (auto-initialized to 0)
   static bool gStartupInProcess;
   if(gStartupInProcess)
   {
	   return rval;
   }
   gStartupInProcess = true;

   //printf("launchBitmunkApplication()\n");

   /* Setup all of the paths that are needed by the OS-dependency call to
      start the Bitmunk application. These paths need to have system-dependent
      slashes. */
   string bitmunkApp(pluginDir.get());
   bitmunkApp.append(FILE_SEPARATOR BITMUNK_BIN_PATH);

   // libs path must contain *BOTH* libs and modules because currently
   // some modules link to other modules
   string libraryPath(ENV_LIBRARY_PATH "=");
   libraryPath.append(pluginDir.get());
   libraryPath.append(FILE_SEPARATOR "libs" PATH_SEPARATOR);
   libraryPath.append(pluginDir.get());
   libraryPath.append(FILE_SEPARATOR "modules");

   // all OS's set the HOME environment var, but where they get the value
   // is OS-dependent (ENV_HOME)
   string homePath("HOME=");
   homePath.append(getenv(ENV_HOME));

   /* Setup the command line options to be passed into the Bitmunk app. These
      can have all of their file separators normalized to '/'.
    */
   // set the resource path
   string resourcePath(pluginDir.get());
   string packageConfigFile("{RESOURCE_PATH}/configs/default.config");
   string nodePortFile("{RESOURCE_PATH}/" NODE_PORT_FILE);
   string logFile("{BITMUNK_HOME}/bitmunk.log");

   // make bitmunk app executable (fix firefox 3.6.0 bug)
   nsCOMPtr<nsILocalFile> file;
   nsDependentCString cstr(bitmunkApp.c_str());
   nsDependentString astr;
   NS_CStringToUTF16(cstr, NS_CSTRING_ENCODING_UTF8, astr);
   nsresult rv = NS_NewLocalFile(astr, PR_FALSE, getter_AddRefs(file));
   if(rv != NS_OK)
   {
      // could not set permissions
      return 0;
   }
   file->SetPermissions(0755);

   // start bitmunk application
   rval = _startBitmunk(
      bitmunkApp, libraryPath, homePath,
      resourcePath, packageConfigFile, nodePortFile, logFile);

   gStartupInProcess = false;

   return rval;
}

/**
 * Gets the current plugin directory.
 *
 * @param path the pathname of the plugin directory will be set to this value.
 * @return NS_OK on success, NS_ERROR_* on failure.
 */
nsresult nsBitmunkExtension::getPlatformPluginDirectory(nsString& path)
{
   nsresult rv;

   // Get the Firefox service manager
   nsCOMPtr<nsIServiceManager> svcMgr;
   rv = NS_GetServiceManager(getter_AddRefs(svcMgr));
   if(NS_FAILED(rv))
   {
      return rv;
   }

   // Get the directory service
   nsCOMPtr<nsIProperties> directory;
   rv = svcMgr->GetServiceByContractID(
      "@mozilla.org/file/directory_service;1",
      NS_GET_IID(nsIProperties),
      getter_AddRefs(directory));
   if(NS_FAILED(rv))
   {
      return rv;
   }

   // Get the component registry file
   nsIFile* aFile;
   rv = directory->Get(
      NS_XPCOM_COMPONENT_REGISTRY_FILE, NS_GET_IID(nsIFile), (void**)&aFile);

   // Get the parent directory of the component registry file, this is the
   // user's base directory for extensions and plugins.
   nsIFile* tFile;
   nsIFile* pFile;
   aFile->GetParent(&tFile);
   tFile->Clone(&pFile);

   // append the extensions/uuid/platform directory
   pFile->AppendNative(nsEmbedCString("extensions"));
   pFile->AppendNative(nsEmbedCString("bitmunk@digitalbazaar.com"));
   pFile->AppendNative(nsEmbedCString("platform"));
   pFile->AppendNative(nsEmbedCString(PLATFORM_DIR));

   nsString data;
   pFile->GetPath(data);
   //printf("data.Length(): %d\n", data.Length());

   // for debugging only:
   //nsCString pdata;
   //NS_UTF16ToCString(data, NS_CSTRING_ENCODING_ASCII, pdata);
   //printf("pdata.get(): %s\n", (const char*)pdata.get());

   path = data;

   return rv;
}

/* boolean launchBitmunk(); */
NS_IMETHODIMP nsBitmunkExtension::LaunchBitmunk(PRInt32 *pid, PRBool *_retval)
{
   nsresult rval = NS_ERROR_FAILURE;

   nsString path;
   rval = getPlatformPluginDirectory(path);

   if(!NS_FAILED(rval))
   {
      nsCString pdata;
      NS_UTF16ToCString(path, NS_CSTRING_ENCODING_ASCII, pdata);

      *pid = launchBitmunkApplication(pdata);
      if(*pid > 0)
      {
         rval = NS_OK;
         *_retval = PR_TRUE;
      }
      else
      {
         rval = NS_ERROR_FAILURE;
         *_retval = PR_FALSE;
      }
   }

   return rval;
}

/* boolean launchBitmunk (); */
NS_IMETHODIMP nsBitmunkExtension::GetBitmunkPort(PRInt32 *port, PRBool *_retval)
{
   nsresult rval = NS_ERROR_FAILURE;
   (*_retval) = PR_FALSE;

   // get the platform plugin directory
   nsString path;
   rval = getPlatformPluginDirectory(path);

   if(!NS_FAILED(rval))
   {
      // convert the platform plugin directory to a Netscape C String
      nsCString pdata;
      NS_UTF16ToCString(path, NS_CSTRING_ENCODING_ASCII, pdata);

      *port = getBitmunkNodePort(pdata);
      rval = NS_OK;
      *_retval = PR_TRUE;
   }

   return rval;
}

/* boolean terminateBitmunk(); */
NS_IMETHODIMP nsBitmunkExtension::TerminateBitmunk(PRInt32 pid, PRBool* _retval)
{
   //printf("nsBitmunkExtension::TerminateBitmunk\n");
   nsresult rval = _stopBitmunk(pid);
   (*_retval) = (rval == NS_OK) ? PR_TRUE : PR_FALSE;
   return rval;
}

/*
 * All of the Bitmunk components that are a part of the Bitmunk
 * extension. This is how the various services are registered in
 * Firefox for the Bitmunk Extension.
 */
static const nsModuleComponentInfo gBitmunkComponents[] =
{
   {
      BITMUNK_CLASSNAME,
      BITMUNK_CID,
      BITMUNK_CONTRACTID,
      nsBitmunkExtensionConstructor
	}
};

// Create the method that will be called to register the extension.
NS_IMPL_NSGETMODULE(nsBitmunkExtension, gBitmunkComponents)

// This code is needed by Windows to specify an entry-point for the DLL
#ifdef WIN32
#include "windows.h"

BOOL APIENTRY DllMain(
   HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
   return TRUE;
}
#endif
