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
#define BITMUNK_BIN      "libs\\bitmunk.exe"
#define PLATFORM_DIR     "WINNT_x86-msvc"

#ifdef WINDOWS_DEBUG_MODE
#define WINDOWS_FIREFOX_LOG "C:\\temp\\bitmunk-firefox.log"
#endif

#ifdef WINDOWS_BITMUNK_LOG_FILE
#define WINDOWS_BITMUNK_LOG "C:\\temp\\bitmunk-output.log"
#endif

/* We need to keep track of an environment that can start the
   windows process properly. Seriously. */
static LPCH gEnvironment = NULL;

// Windows doesn't have a good string replace function. Seriously.
static string& stringReplaceAll(
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

// Windows has to clean up its globally created environment.
static void _cleanup()
{
   // release global environment handle
   if(gEnvironment != NULL)
   {
      FreeEnvironmentStrings(gEnvironment);
   }
}

#endif

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
   const char* nodePortFileKey = "bitmunk\\.system\\.System.nodePortFile";

   // linux/apple specific implementation
#if defined(__linux__) || defined(__APPLE__)
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

   pid_t pid = fork();
   if(pid == 0)
   {
      // build command line
      char* cmd[] =
      {
         (char*)"bitmunk",
         (char*)"--resource-path", (char*)resourcePath.c_str(),
         (char*)"--package-config", (char*)packageConfigFile.c_str(),
         (char*)"--option", (char*)nodePortFileKey,
         (char*)nodePortFile.c_str(),
         (char*)"--log", (char*)logFile.c_str(),
         (NULL)
      };

      // build environment
      char* env[] =
      {
         (char*)libraryPath.c_str(),
         (char*)homePath.c_str(),
         (NULL)
      };

      // the child should overlay the Bitmunk application
      execve(bitmunkApp.c_str(), cmd, env);
      perror("execve");   /* execve() only returns on error */
      exit(EXIT_FAILURE);
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

   // windows specific implementation
#elif defined(WIN32)

#ifdef WINDOWS_DEBUG_MODE
   FILE* fp = fopen(WINDOWS_FIREFOX_LOG, "a");
   fprintf(fp, "Launching bitmunk app...\n");
#endif

   // normalize resource path to '/' (it is passed to the Bitmunk app, and
   // it understands '/' to be a valid file separator on any OS)
   stringReplaceAll(resourcePath, "\\", "/");

   // escape bitmunk application path (it must be escaped for CreateProcess,
   // quoting it will cause CreateProcess to fail)
   stringReplaceAll(bitmunkApp, "\\", "\\\\");

   // build the parameter list
   // the first parameter needs to be the exe itself, windows wants that
   string parameters;
   parameters.append("bitmunk.exe");
   // resource path
   parameters.append(" --resource-path ");
   parameters.append("\"");
   parameters.append(resourcePath.c_str());
   parameters.append("\"");
   // package config file
   parameters.append(" --package-config ");
   parameters.append("\"");
   parameters.append(packageConfigFile.c_str());
   parameters.append("\"");
   // set the node port file
   parameters.append(" --option ");
   parameters.append("\"");
   parameters.append(nodePortFileKey);
   parameters.append("\" \"");
   parameters.append(nodePortFile.c_str());
   parameters.append("\"");
   // set log file
   parameters.append(" --log ");
   parameters.append("\"");
   parameters.append(logFile.c_str());
   parameters.append("\"");

   // we only need to create the global environment once, thereafter it will
   // be cloned via CreateProcess
   if(gEnvironment == NULL)
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
#ifdef WINDOWS_DEBUG_MODE
      fprintf(fp, "PATH: %s\n\n", (const char*)newPath);
#endif
      // set new path value and then free new and old paths
      SetEnvironmentVariable("PATH", newPath);
      free(newPath);
      if(oldPath != NULL)
      {
         free(oldPath);
      }

      // *NOW* get the current environment since our PATH has been updated
      gEnvironment = GetEnvironmentStrings();
   }

#ifdef WINDOWS_DEBUG_MODE
   fprintf(fp, "CreateProcess() params:\n\n");
   fprintf(fp, "lpApplicationName: %s\n\n", bitmunkApp.c_str());
   fprintf(fp, "lpCommandLine: %s\n\n", parameters.c_str());
#endif

   /* CreateProcess API initialization */
   STARTUPINFO siStartupInfo;
   PROCESS_INFORMATION piProcessInfo;
   memset(&siStartupInfo, 0, sizeof(siStartupInfo));
   memset(&piProcessInfo, 0, sizeof(piProcessInfo));
   siStartupInfo.cb = sizeof(siStartupInfo);

   // do not inherit handles unless in debug mode
   BOOL inheritHandles = TRUE;

#ifdef WINDOWS_BITMUNK_LOG_FILE
   // only for debugging
   inheritHandles = FALSE;
   SECURITY_ATTRIBUTES securityAttr;
   securityAttr.nLength = sizeof(securityAttr);
   securityAttr.bInheritHandle = TRUE;
   securityAttr.lpSecurityDescriptor = NULL;
   HANDLE hFile = CreateFile(
      WINDOWS_BITMUNK_LOG,
      GENERIC_WRITE, FILE_SHARE_WRITE, &securityAttr,
      OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
   if(hFile == INVALID_HANDLE_VALUE)
   {
      fprintf(fp, "Could not create file handle for debug output.\n");
   }
   siStartupInfo.dwFlags    = STARTF_USESTDHANDLES;
   siStartupInfo.hStdInput  = INVALID_HANDLE_VALUE;
   siStartupInfo.hStdError  = hFile;
   siStartupInfo.hStdOutput = hFile;
#endif

   if(CreateProcess(const_cast<LPSTR>(bitmunkApp.c_str()),
                    const_cast<LPSTR>(parameters.c_str()), 0, 0,
                    inheritHandles,
                    CREATE_NO_WINDOW, /* create no console window */
                    gEnvironment, /* use saved global environment */
                    NULL, /* current directory not needed */
                    &siStartupInfo, &piProcessInfo) != 0)
   {
#ifdef WINDOWS_DEBUG_MODE
      fprintf(fp, "Process created.\n");
#endif
      /* Wait to see if process immediately terminates */
      DWORD waitRc = WaitForSingleObject(piProcessInfo.hProcess, (1 * 1000));
      if(waitRc == 0)
      {
#ifdef WINDOWS_DEBUG_MODE
         fprintf(fp, "Process terminated.\n");
         DWORD exitCode = 0;
         if(GetExitCodeProcess(piProcessInfo.hProcess, &exitCode))
         {
            fprintf(fp, "Process exit code: (%d)\n", exitCode);
         }
#endif
      }
      else
      {
         // return windows process ID
         rval = GetProcessId(piProcessInfo.hProcess);
      }
   }
#ifdef WINDOWS_DEBUG_MODE
   else
   {
      /* CreateProcess failed */
      DWORD dwLastError = GetLastError();
      fprintf(fp, "Process creation failed: (%d)\n", dwLastError);

      // get last error message
      if(dwLastError != 0)
      {
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
            fprintf(fp, "Error message: %s\n", errorString);
         }

         // free lpBuffer
         LocalFree(lpBuffer);
      }
   }
#endif

   /* Release handles */
#ifdef WINDOWS_BITMUNK_LOG_FILE
   CloseHandle(hFile);
#endif
   CloseHandle(piProcessInfo.hProcess);
   CloseHandle(piProcessInfo.hThread);

#ifdef WINDOWS_DEBUG_MODE
   if(rval > 0)
   {
      fprintf(fp, "Bitmunk launch succeeded. PID: %d\n", rval);
   }
   else
   {
      fprintf(fp, "Bitmunk launch failed.\n");
   }

   // close log file
   fclose(fp);
#endif

#endif // end of OS specific implementations

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
      // FIXME: set rval to something useful, now it returns not implemented
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
      // FIXME: set rval to something useful, not it returns not implemented

#ifdef WINDOWS_DEBUG_MODE
      DWORD dwLastError = GetLastError();
      FILE* fp = fopen(WINDOWS_FIREFOX_LOG, "a");
      fprintf(fp, "Process termination failed: (%d)\n", dwLastError);

      // get last error message
      if(dwLastError != 0)
      {
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
            fprintf(fp, "Error message: %s\n", errorString);
         }

         // free lpBuffer
         LocalFree(lpBuffer);
      }

      // close log file
      fclose(fp);
#endif // windows debug mode
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
   bitmunkApp.append(FILE_SEPARATOR BITMUNK_BIN);

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

   if(rval == NS_OK)
   {
      (*_retval) = PR_TRUE;
   }
   else
   {
      (*_retval) = PR_FALSE;
   }

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
