/*
 * Copyright (c) 2010 Digital Bazaar, Inc. All rights reserved.
 */

/*
 * This file contains the implementation of the Bitmunk MIME Type Plugin
 * for Firefox.
 */
#include <sys/types.h>
#include <sys/stat.h>
#ifdef WIN32
#include <io.h>

#define open _open
#define read _read
#define ssize_t unsigned int
#define OSCALL WINAPI
#else
#include <unistd.h>
#define OSCALL
#endif
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include "npapi.h"

// check if using old style NewNPP_* API or newer NPP_* API
#if (((NP_VERSION_MAJOR << 8) + NP_VERSION_MINOR) < 20)
#include "npupp.h"
#else
#define USENPFUNCTIONS 1
#include "npfunctions.h"
#endif

#include "nsCOMPtr.h"
#include "nsIObserverService.h"
#include "nsXPCOM.h"
#include "nsIServiceManager.h"
#include "nsStringAPI.h"
#include "nsEmbedString.h"

// global static gecko engine function table
static NPNetscapeFuncs gGeckoFunctionTable;

/**
 * Define the MIME-type and plugin name description strings.
 */
#define MIME_TYPES_HANDLED      "application/x-bitmunk-directive"
#define PLUGIN_NAME             "Bitmunk Directive Processor"
#define MIME_TYPES_DESCRIPTION  MIME_TYPES_HANDLED ":bmd:" PLUGIN_NAME
#define PLUGIN_DESCRIPTION      "Bitmunk BMD file handler"

// maximum size of a JSON bitmunk directive (including null-terminator)
#define MAX_BMD_SIZE    16384

//#define WINDOWS_DEBUG_MODE  1
//#define LINUX_DEBUG_MODE    1
#define SEND_BMD_TO_BITMUNK 1

// clear NPAPI functions (use C style symbols) for windows and linux, do not
// do this for mac OS, it crashes on NP_EntryPoints ... it works on mac OS if
// you do C++ mangling
#if !defined(__APPLE__)
extern "C"
{
#if defined(WIN32)
NPError OSCALL NP_Initialize(NPNetscapeFuncs* geckoFunctions);
#else
NPError OSCALL NP_Initialize(
   NPNetscapeFuncs* geckoFunctions, NPPluginFuncs* pFuncs);
#endif
NPError OSCALL NP_GetEntryPoints(NPPluginFuncs* pFuncs);
NPError OSCALL NP_Shutdown();
char* NP_GetMIMEDescription();
NPError OSCALL NP_GetValue(void* future, NPPVariable valueType, void* value);
}
#endif

// a buffer for storing streaming BMD data
struct BmdBuffer
{
   int32 capacity;
   int32 length;
   char* data;
};

/**
 * Creates a new instance of the plugin.
 *
 * @param mimeType the mimetype that was detected in the browser.
 * @param instance the instance that received the event, should be NULL.
 * @param mode the mode that was used to open the plugin (ie: embedded,
 *             full-screen, or background)
 * @param argc the argument count for the instance creation.
 * @param argn the argument names associated with the object/embed.
 * @param argv the argument values associated with the argument names.
 * @param saved the saved state that is passed in from the last destruction of
 *              the plugin -- where it was created.
 *
 * @return an NP error code.
 */
static NPError BmpNewInstance(
   NPMIMEType mime_type, NPP instance,
   uint16 mode, uint16 argc, char* argn[], char* argv[], NPSavedData* saved)
{
#ifdef WINDOWS_DEBUG_MODE
   FILE* fp = fopen("C:\\temp\\bitmunk-firefox-plugin.log", "a");
   fprintf(fp, "BmpNewInstance\n");
   fclose(fp);
#endif

#ifdef LINUX_DEBUG_MODE
   printf("BmpNewInstance\n");
#endif

#ifdef SEND_BMD_TO_BITMUNK
   // get the service manager
   nsCOMPtr<nsIServiceManager> svcMgr;
   nsresult rv = NS_GetServiceManager(getter_AddRefs(svcMgr));
   if(!NS_FAILED(rv))
   {
      // get the observer service
      nsCOMPtr<nsIObserverService> observerService;
      rv = svcMgr->GetServiceByContractID(
         "@mozilla.org/observer-service;1",
         NS_GET_IID(nsIObserverService),
         getter_AddRefs(observerService));
      if(!NS_FAILED(rv))
      {
         // notify observers that a bitmunk directive has started
         observerService->NotifyObservers(
            NULL, "bitmunk-directive-started", NULL);
      }
   }
#endif

   // ensure that the instance passed is valid
   return (instance == NULL) ? NPERR_INVALID_INSTANCE_ERROR : NPERR_NO_ERROR;
}

/**
 * Destroys a previously created instance of the plugin.
 *
 * @param instance the instance that received the event.
 * @param save a pointer that can be set to state information that can be
 *             allocated in this method -- it will be passed to the next
 *             call to create a new instance.
 *
 * @return an NP error code.
 */
static NPError BmpDestroyInstance(NPP instance, NPSavedData** save)
{
#ifdef WINDOWS_DEBUG_MODE
   FILE* fp = fopen("C:\\temp\\bitmunk-firefox-plugin.log", "a");
   fprintf(fp, "BmpDestroyInstance\n");
   fclose(fp);
#endif

#ifdef LINUX_DEBUG_MODE
   printf("BmpDestroyInstance\n");
#endif

   // ensure that the instance passed is valid
   return (instance == NULL) ? NPERR_INVALID_INSTANCE_ERROR : NPERR_NO_ERROR;
}

/**
 * Allocates enough space in the passed buffer to write "amount" bytes and
 * leave room for the null-terminator.
 *
 * @param buffer the buffer to resize.
 * @param amount the amount to be written into the buffer.
 *
 * @return true if successful, false if there was not enough memory.
 */
static bool allocateBmdBufferSpace(BmdBuffer* buffer, int amount)
{
   bool rval = true;

   // ensure there is enough space, including the null-terminator
   amount++;
   int32 remaining = buffer->capacity - buffer->length;
   if(remaining > amount)
   {
      // enough space, so nothing needs to be done
#ifdef LINUX_DEBUG_MODE
      printf("allocateBmdBufferSpace: enough space for %d bytes\n", amount);
#endif
   }
   else
   {
#ifdef LINUX_DEBUG_MODE
      printf("allocateBmdBufferSpace: allocating space for %d bytes\n", amount);
#endif
      // not enough space, save old data
      char* oldData = buffer->data;

      // try to allocate space (enough for old data, new data, and 1 null)
      int32 newCapacity = buffer->length + amount + 1;
      buffer->data = (char*)gGeckoFunctionTable.memalloc(newCapacity);
      if(buffer->data == NULL)
      {
#ifdef LINUX_DEBUG_MODE
         printf("allocateBmdBufferSpace: out of memory\n");
#endif
         // error, out of memory, restore old data
         rval = false;
         buffer->data = oldData;
      }
      else
      {
#ifdef LINUX_DEBUG_MODE
         printf("allocateBmdBufferSpace: space allocated\n");
#endif
         // set new capacity and clear new memory
         buffer->capacity = newCapacity;
         memset(
            buffer->data + buffer->length, 0,
            buffer->capacity - buffer->length);
         if(oldData != NULL)
         {
            // copy old data and free then it
            memcpy(buffer->data, oldData, buffer->length);
            gGeckoFunctionTable.memfree(oldData);
         }
      }
   }

   return rval;
}

/**
 * Called by the application whenever a new stream matching the MIME type of
 * the plugin is detected on a page.
 *
 * @param instance the instance that received the event.
 * @param type the MIME-type that was specified for the embedded object or file.
 * @param stream the stream associated with the MIME-type.
 * @param seekable true if the stream is seek-able, false otherwise.
 * @param stype to be set to the type of streaming to use:
 *    NP_NORMAL (Default): Delivers stream data to the instance in a series
 *    of calls to NPP_WriteReady and NPP_Write.
 *    NP_ASFILEONLY: Saves stream data to a file in the local cache.
 *    NP_ASFILE: File download. Like NP_ASFILEONLY except that data is
 *    delivered to the plug-in as it is saved to the file
 *    (as in mode NP_NORMAL).
 *    NP_SEEK: Stream data randomly accessible by the plug-in as needed,
 *    through calls to NPN_RequestRead.
 *
 * @return an NP error code.
 */
static NPError BmpNewStream(
   NPP instance, NPMIMEType type, NPStream* stream,
   NPBool seekable, uint16* stype)
{
   NPError rval = NPERR_NO_ERROR;

#ifdef WINDOWS_DEBUG_MODE
   FILE* fp = fopen("C:\\temp\\bitmunk-firefox-plugin.log", "a");
   fprintf(fp, "BmpNewStream\n");
   fclose(fp);
#endif

#ifdef LINUX_DEBUG_MODE
   printf("BmpNewStream\n");
#endif

   // use normal streaming
   *stype = (uint16)NP_NORMAL;

   // allocate bmd buffer
   stream->pdata = gGeckoFunctionTable.memalloc(sizeof(BmdBuffer));
   if(stream->pdata == NULL)
   {
      rval = NPERR_OUT_OF_MEMORY_ERROR;
   }
   else
   {
      // initialize bmd buffer
      BmdBuffer* buffer = (BmdBuffer*)stream->pdata;
      buffer->capacity = 0;
      buffer->length = 0;
      buffer->data = NULL;
   }

   return rval;
}

/**
 * Called by the application whenever a stream is destroyed for a particular
 * MIME type.
 *
 * @param instance the instance that received the event.
 * @param stream the stream that was destroyed.
 * @param reason the reason the stream was destroyed.
 *    NPRES_DONE (Most common): Completed normally; all data was sent to the
 *    instance.
 *    NPRES_USER_BREAK: User canceled stream directly by clicking the Stop
 *    button or indirectly by some action such as deleting the instance or
 *    initiating higher-priority network operations.
 *    NPRES_NETWORK_ERR: Stream failed due to problems with network, disk I/O,
 *    lack of memory, or other problems.
 *
 * @return an NP error code.
 */
static NPError BmpDestroyStream(NPP instance, NPStream* stream, NPReason reason)
{
   NPError rval = NPERR_NO_ERROR;

#ifdef WINDOWS_DEBUG_MODE
   FILE* fp = fopen("C:\\temp\\bitmunk-firefox-plugin.log", "a");
   fprintf(fp, "BmpDestroyStream\n");
   fclose(fp);
#endif

#ifdef LINUX_DEBUG_MODE
   printf("BmpDestroyStream\n");
#endif

   // get bmd buffer
   BmdBuffer* buffer = (BmdBuffer*)stream->pdata;

#ifdef LINUX_DEBUG_MODE
   if(reason == NPRES_DONE && buffer->length > 0)
   {
      printf("Read BMD data\n%s\n", buffer->data);
   }
#endif

#ifdef SEND_BMD_TO_BITMUNK
   // get the service manager
   nsCOMPtr<nsIServiceManager> svcMgr;
   nsresult rv = NS_GetServiceManager(getter_AddRefs(svcMgr));
   if(!NS_FAILED(rv))
   {
      // get the observer service
      nsCOMPtr<nsIObserverService> observerService;
      rv = svcMgr->GetServiceByContractID(
         "@mozilla.org/observer-service;1",
         NS_GET_IID(nsIObserverService),
         getter_AddRefs(observerService));
      if(!NS_FAILED(rv))
      {
         nsEmbedString utf16;

         // stream completed normally and there is data to send
         if(reason == NPRES_DONE && buffer->length > 0)
         {
            // write the bmdData into a UTF-16 string
            NS_CStringToUTF16(
               nsEmbedCString(buffer->data),
               NS_CSTRING_ENCODING_ASCII, utf16);

            // send the UTF-16 data to observers
            observerService->NotifyObservers(
               NULL, "bitmunk-directive", utf16.get());
         }
         // user canceled
         else if(reason == NPRES_USER_BREAK)
         {
            // set error message
            NS_CStringToUTF16(
               nsEmbedCString("The directive was canceled."),
               NS_CSTRING_ENCODING_ASCII, utf16);

            // send the UTF-16 data to observers
            observerService->NotifyObservers(
               NULL, "bitmunk-directive-error", utf16.get());
         }
         // network error
         else if(reason == NPRES_NETWORK_ERR)
         {
            // set error message
            NS_CStringToUTF16(
               nsEmbedCString("There was a network error."),
               NS_CSTRING_ENCODING_ASCII, utf16);

            // send the UTF-16 data to observers
            observerService->NotifyObservers(
               NULL, "bitmunk-directive-error", utf16.get());
         }
         // no data
         else
         {
            // set error message
            NS_CStringToUTF16(
               nsEmbedCString("The directive was empty."),
               NS_CSTRING_ENCODING_ASCII, utf16);

            // send the UTF-16 data to observers
            observerService->NotifyObservers(
               NULL, "bitmunk-directive-error", utf16.get());
         }
      }
   }
#endif

   // free bmd buffer
   if(buffer != NULL)
   {
      // free buffer data
      if(buffer->data != NULL)
      {
         gGeckoFunctionTable.memfree(buffer->data);
      }
      // free buffer itself
      gGeckoFunctionTable.memfree(stream->pdata);
      stream->pdata = NULL;
   }

   return rval;
}

/**
 * Called by the application to determine if the plugin is ready to have
 * data written to it and the maximum number of bytes it can accept.
 *
 * @param instance the instance that received the event.
 * @param stream the associated stream.
 *
 * @return the maximum number of bytes the plugin can accept.
 */
static int32 BmpWriteReady(NPP instance, NPStream* stream)
{
   int32 rval = 0;

   // return maximum bmd size - bytes already read - 1 for null-terminator
   BmdBuffer* buffer = (BmdBuffer*)stream->pdata;
   rval = MAX_BMD_SIZE - buffer->length - 1;

#ifdef LINUX_DEBUG_MODE
   printf("BmpWriteReady: %d\n", rval);
#endif

   return rval;
}

/**
 * Called by the application to write data to the plugin.
 *
 * @param instance the instance that received the event.
 * @param stream the associated stream.
 * @param offset the offset into the entire stream, this is not the offset
 *               that valid bytes begin in the passed buffer.
 * @param len the length of the data.
 * @param buffer the buffer with the data.
 *
 * @return the number of bytes written or -1 on error.
 */
static int32 BmpWrite(
   NPP instance, NPStream* stream, int32 offset, int32 len, void* buffer)
{
   int32 rval = -1;

#ifdef LINUX_DEBUG_MODE
   printf("BmpWrite: %d bytes\n", len);
#endif

   // allocate enough space in the bmd buffer
   BmdBuffer* b = (BmdBuffer*)stream->pdata;
   if(allocateBmdBufferSpace(b, len))
   {
      // copy data into bmd buffer
      memcpy(b->data + b->length, buffer, len);
      b->length += len;
      rval = len;
   }

   return rval;
}

// Function called by application to get the MIME-type for this plugin.
char* NP_GetMIMEDescription()
{
#ifdef WINDOWS_DEBUG_MODE
   FILE* fp = fopen("C:\\temp\\bitmunk-firefox-plugin.log", "a");
   fprintf(fp, "NP_GetMIMEDescription: %s\n", MIME_TYPES_DESCRIPTION);
   fclose(fp);
#endif

#ifdef LINUX_DEBUG_MODE
   printf("NP_GetMIMEDescription: %s\n", MIME_TYPES_DESCRIPTION);
#endif

   return (char*)(MIME_TYPES_DESCRIPTION);
}

// Function called by application to get values per plugin
NPError OSCALL NP_GetValue(void* future, NPPVariable valueType, void* value)
{
#ifdef WINDOWS_DEBUG_MODE
   FILE* fp = fopen("C:\\temp\\bitmunk-firefox-plugin.log", "a");
   fprintf(fp, "NP_GetValue\n");
   fclose(fp);
#endif

#ifdef LINUX_DEBUG_MODE
   printf("NP_GetValue\n");
#endif

   NPError err = NPERR_NO_ERROR;

   // check the type of the value and return the associated value
   switch(valueType)
   {
      case NPPVpluginNameString:
         *((const char**)value) = PLUGIN_NAME;
         break;
      case NPPVpluginDescriptionString:
         *((const char**)value) = PLUGIN_DESCRIPTION;
         break;
      default:
         err = NPERR_INVALID_PARAM;
         break;
   }

   return err;
}

// called by the browser in windows to initialize entry points
NPError OSCALL NP_GetEntryPoints(NPPluginFuncs* pFuncs)
{
   NPError rval = NPERR_INVALID_FUNCTABLE_ERROR;

#ifdef WINDOWS_DEBUG_MODE
   FILE* fp = fopen("C:\\temp\\bitmunk-firefox-plugin.log", "a");
   fprintf(fp, "NP_GetEntryPoints\n");
#endif

#ifdef LINUX_DEBUG_MODE
   printf("NP_GetEntryPoints\n");
#endif

   // make sure the table function pointers are valid and that the
   // plugin function table size is the same as what the plugin was
   // compiled against
   if(pFuncs != NULL && pFuncs->size >= sizeof(NPPluginFuncs))
   {
#if WINDOWS_DEBUG_MODE
      fprintf(fp, "NP_GetEntryPoints pFuncs is correct\n");
#endif

      // initialize the plugin function table
      pFuncs->version = (NP_VERSION_MAJOR << 8) + NP_VERSION_MINOR;
#ifdef USENPFUNCTIONS
      pFuncs->newp = NPP_NewProcPtr(BmpNewInstance);
      pFuncs->destroy = NPP_DestroyProcPtr(BmpDestroyInstance);
      pFuncs->newstream = NPP_NewStreamProcPtr(BmpNewStream);
      pFuncs->destroystream = NPP_DestroyStreamProcPtr(BmpDestroyStream);
      pFuncs->asfile = NULL;
      pFuncs->writeready = NPP_WriteReadyProcPtr(BmpWriteReady);
      pFuncs->write = NPP_WriteProcPtr(BmpWrite);
      pFuncs->print = NULL;
      pFuncs->setwindow = NULL;
      pFuncs->urlnotify = NULL;
      pFuncs->event = NULL;
#else
      pFuncs->newp = NewNPP_NewProc(BmpNewInstance);
      pFuncs->destroy = NewNPP_DestroyProc(BmpDestroyInstance);
      pFuncs->newstream = NewNPP_NewStreamProc(BmpNewStream);
      pFuncs->destroystream = NewNPP_DestroyStreamProc(BmpDestroyStream);
      pFuncs->asfile = NULL;
      pFuncs->writeready = NewNPP_WriteReadyProc(BmpWriteReady);
      pFuncs->write = NewNPP_WriteProc(BmpWrite);
      pFuncs->print = NULL;
      pFuncs->setwindow = NULL;
      pFuncs->urlnotify = NULL;
      pFuncs->event = NULL;
#endif

#ifdef OJI
      // if Java is enabled, set the Java handler to NULL
      pFuncs->javaClass = NULL;
#endif

      rval = NPERR_NO_ERROR;
   }
#ifdef WINDOWS_DEBUG_MODE
   else
   {
      fprintf(fp, "NP_GetEntryPoints pFuncs is NOT correct\n");
   }

   fprintf(fp, "NP_GetEntryPoints done\n");
   fclose(fp);
#endif

   return rval;
}

// Called by the browser in linux to initialize entry points.
#if defined(WIN32)
NPError OSCALL NP_Initialize(NPNetscapeFuncs* geckoFunctions)
#else
NPError OSCALL NP_Initialize(
   NPNetscapeFuncs* geckoFunctions, NPPluginFuncs* pFuncs)
#endif
{
   NPError rval = NPERR_INVALID_FUNCTABLE_ERROR;

#ifdef WINDOWS_DEBUG_MODE
   FILE* fp = fopen("C:\\temp\\bitmunk-firefox-plugin.log", "a");
   fprintf(fp, "NP_Initialize\n");
#endif

#ifdef LINUX_DEBUG_MODE
   printf("NP_Initialize\n");
#endif

   // make sure the table function pointers are valid.
   if(geckoFunctions == NULL)
   {
      rval = NPERR_INVALID_FUNCTABLE_ERROR;
   }
   // Make sure that the browser version and the plugin version are compatible
   else if((geckoFunctions->version >> 8) > NP_VERSION_MAJOR)
   {
      rval = NPERR_INCOMPATIBLE_VERSION_ERROR;
   }
   // make sure the function table size is the same as what the plugin was
   // compiled against.
   else if(geckoFunctions->size < sizeof(NPNetscapeFuncs))
   {
      rval = NPERR_INVALID_FUNCTABLE_ERROR;
   }
   else
   {
#if defined(__linux__) || defined(__APPLE__)
      rval = NP_GetEntryPoints(pFuncs);
#else
      rval = NPERR_NO_ERROR;
#endif
      if(rval == NPERR_NO_ERROR)
      {
         // make a local copy of the gecko function table
         memcpy(&gGeckoFunctionTable, geckoFunctions, sizeof(NPNetscapeFuncs));
      }
   }

#ifdef WINDOWS_DEBUG_MODE
   fprintf(fp, "NP_Initialize complete. rval=%i\n", rval);
   fclose(fp);
#endif

#ifdef LINUX_DEBUG_MODE
   printf("NP_Initialize complete. rval=%i\n", rval);
#endif

   return rval;
}

// mac os X has its own special entry point
#if defined(__APPLE__)

int main(
   NPNetscapeFuncs* nsTable,
   NPPluginFuncs* pluginFuncs,
   NPP_ShutdownUPP* unloadUpp);

DEFINE_API_C(int) main(
   NPNetscapeFuncs* nsTable,
   NPPluginFuncs* pluginFuncs,
   NPP_ShutdownUPP* unloadUpp)
{
   NPError rval = NP_Initialize(nsTable, pluginFuncs);
   if(rval == NPERR_NO_ERROR)
   {
#ifdef USENPFUNCTIONS
      *unloadUpp = NPP_ShutdownProcPtr(NP_Shutdown);
#else
      *unloadUpp = NewNPP_ShutdownProc(NP_Shutdown);
#endif
   }

   return rval;
}

// end defined __APPLE__
#endif

// called by all browsers when shutting down
NPError OSCALL NP_Shutdown()
{
#ifdef WINDOWS_DEBUG_MODE
   FILE* fp = fopen("C:\\temp\\bitmunk-firefox-plugin.log", "a");
   fprintf(fp, "NP_Shutdown\n");
   fclose(fp);
#endif

#ifdef LINUX_DEBUG_MODE
   printf("NP_Shutdown\n");
#endif

   return NPERR_NO_ERROR;
}

// This code is needed by Windows to specify an entry-point for the
// DLL
#ifdef WIN32
#include "windows.h"

BOOL APIENTRY DllMain(
   HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
#ifdef WINDOWS_DEBUG_MODE
   FILE* fp = fopen("C:\\temp\\bitmunk-firefox-plugin.log", "a");
   fprintf(fp, "BOOL APIENTRY DllMain()\n");
   fclose(fp);
#endif

   return TRUE;
}
#endif
