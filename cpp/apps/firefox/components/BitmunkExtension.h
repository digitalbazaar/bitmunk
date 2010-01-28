/**
 * Copyright 2007-2008 Digital Bazaar, Inc.
 */
#ifndef _NS_BITMUNK_EXTENSION_H
#define _NS_BITMUNK_EXTENSION_H
#include "nsStringAPI.h"
#include "nsEmbedString.h"
#include "IBitmunkExtension.h"

// f1dab993-22bb-4416-9471-afd19dc9dd86
#define BITMUNK_CID {0xf1dab993, 0x22bb, 0x4416, \
    { 0x94, 0x71, 0xaf, 0xd1, 0x9d, 0xc9, 0xdd, 0x86 }}
#define BITMUNK_CONTRACTID "@bitmunk.com/xpcom;1"
#define BITMUNK_CLASSNAME "Bitmunk Extension"

/**
 * The Bitmunk Extension class implements the public XPCOM Bitmunk
 * Extension interface for the Mozilla class of web browsers.
 *
 * @author Manu Sporny
 */
class nsBitmunkExtension : public nsIBitmunkExtension
{
public:
   NS_DECL_ISUPPORTS
   NS_DECL_NSIBITMUNKEXTENSION

   nsBitmunkExtension();

private:
   ~nsBitmunkExtension();

protected:
   /**
    * Gets the current plugin directory.
    *
    * @param path the pathname of the plugin directory will be set to
    *             this value.
    * @return NS_OK on success, NS_ERROR_* on failure.
    */
   nsresult getPlatformPluginDirectory(nsString& path);

   /**
    * Gets the Bitmunk node port that should be used to communicate with
    * BPE.
    *
    * @param path the path to the plugin directory.
    * @return The port number that can be used to communicate with the BPE.
    */
   PRInt32 getBitmunkNodePort(nsCString path);

   /**
    * Launches the Bitmunk application from the plugin directory.
    *
    * @param pluginDir the plugin directory that contains the Bitmunk
    *                  application as well as all libraries, modules
    *                  and content.
    * @return The process ID of the launched application.
    */
   PRInt32 launchBitmunkApplication(nsCString pluginDir);
};

#endif
