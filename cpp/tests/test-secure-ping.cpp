/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include <iostream>

#include "bitmunk/bfp/BfpFactory.h"
#include "bitmunk/common/TypeDefinitions.h"
#include "bitmunk/protocol/BtpClient.h"
#include "bitmunk/protocol/BtpMessage.h"
#include "bitmunk/node/App.h"
#include "monarch/app/App.h"
#include "monarch/data/json/JsonWriter.h"
#include "monarch/io/File.h"
#include "monarch/io/FileInputStream.h"
#include "monarch/io/FileList.h"
#include "monarch/modest/Kernel.h"
#include "monarch/net/Url.h"
#include "monarch/test/Test.h"

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::bfp;
using namespace bitmunk::protocol;
using namespace monarch::app;
using namespace monarch::data::json;
using namespace monarch::io;
using namespace monarch::modest;
using namespace monarch::net;
using namespace monarch::rt;
using namespace monarch::test;

class SecurePingApp :
   public monarch::app::AppPlugin
{
protected:
   /**
    * Options from the command line.
    */
   DynamicObject mOptions;

   /**
    * BitmunkCustomerSupport's profile.
    */
   ProfileRef mProfile;

public:
   SecurePingApp() :
      mProfile(NULL)
   {
      mInfo["id"] = "bitmunk.test.SecurePing";
      mInfo["dependencies"]->append() = "bitmunk.app.App";

      mOptions["profile"] = "/tmp/devuser.profile";
      mOptions["userId"] = 900;
      mOptions["bitmunkHost"] = "localhost:19100";
   };

   virtual ~SecurePingApp() {};

   virtual DynamicObject getCommandLineSpecs()
   {
      DynamicObject spec;
      spec["help"] =
"SecurePing options\n"
"      --profile       The full path to the Bitmunk profile.\n"
"      --user-id       The user ID associated with the profile.\n"
"      --bitmunk-host  The hostname and port to use for the webservices.\n"
"\n";

      DynamicObject opt(NULL);

      // create option to set the profile directory
      opt = spec["options"]->append();
      opt["long"] = "--profile";
      opt["argError"] = "Profile filename not specified.";
      opt["arg"]["target"] = mOptions["profile"];

      // create option to set the user id
      opt = spec["options"]->append();
      opt["long"] = "--user-id";
      opt["argError"] = "User ID not specified.";
      opt["arg"]["target"] = mOptions["userId"];

      // create option to set bitmunk host+port
      opt = spec["options"]->append();
      opt["long"] = "--bitmunk-host";
      opt["argError"] = "No bitmunk host specified.";
      opt["arg"]["target"] = mOptions["bitmunkHost"];

      DynamicObject specs = AppPlugin::getCommandLineSpecs();
      specs->append(spec);
      return specs;
   };

   virtual bool doSecurePing()
   {
      bool rval = false;

      // format the service URL
      Url url;
      url.format("https://%s/api/3.0/system/test/ping",
         mOptions["bitmunkHost"]->getString());

      printf("Pinging service at %s ...", url.toString().c_str());

      // setup messages, do communication on behalf of the profile
      DynamicObject dyno;
      BtpMessage in;
      BtpMessage out;
      dyno["empty"] = true;
      out.setType(BtpMessage::Get);
      out.setDynamicObject(dyno);
      out.setUserId(mProfile->getUserId());
      out.setAgentProfile(mProfile);

      // FIXME: do not bother with SSL verification for now
      BtpClient btp;
      btp.getSslContext()->setPeerAuthentication(false);
      if((rval = btp.exchange(0, &url, &out, &in)))
      {
         printf("PASS.\n");
         rval = true;
      }
      else
      {
         printf("FAIL.\n");
         App::printException();
      }

      return rval;
   }

   virtual bool run()
   {
      // get profile filename
      string profile = mOptions["profile"]->getString();
      UserId userId = mOptions["userId"]->getUInt64();

      printf("Using the following options, Hit Ctrl+C if incorrect:\n\n");
      printf("Profile:                 %s\n", profile.c_str());
      printf("User ID:                 %" PRIu64 "\n", userId);
      printf("Bitmunk host to contact: https://%s\n",
         mOptions["bitmunkHost"]->getString());

      // request password to continue
      printf("\nEnter %s password to continue: ", profile.c_str());
      char input[100];
      cin.getline(input, 100);
      string password = input;
      printf("Unlocking %s...", profile.c_str());

      // load BitmunkCustomerSupport's profile
      mProfile = new Profile();
      mProfile->setUserId(userId);
      File file(profile.c_str());
      FileInputStream fis(file);
      if(mProfile->load(password.c_str(), &fis))
      {
         printf("unlocked.\n");
         doSecurePing();
      }

      return !Exception::isSet();
   };
};

/**
 * Create main() for the SecurePing app
 */
BM_APP_PLUGIN_MAIN(SecurePingApp);
