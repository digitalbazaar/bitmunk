/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include <iostream>
#include <sstream>

#include "bitmunk/node/Node.h"
#include "bitmunk/test/Tester.h"
#include "monarch/rt/Exception.h"
#include "monarch/test/Test.h"
#include "monarch/test/TestRunner.h"
#include "monarch/rt/DynamicObject.h"
#include "monarch/data/json/JsonWriter.h"
#include "monarch/io/File.h"
#include "monarch/util/Timer.h"

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::protocol;
using namespace bitmunk::node;
using namespace monarch::config;
using namespace monarch::data::json;
using namespace monarch::io;
using namespace monarch::net;
using namespace monarch::rt;
using namespace monarch::test;
using namespace monarch::util;

void mediaGetTest(Node& node, TestRunner& tr)
{
   tr.group("media");
   
   Messenger* messenger = node.getMessenger();
   
   tr.test("get");
   {
      // create url for obtaining media
      Url url("/api/3.0/media/2");
      
      // get media
      Media media;
      messenger->getFromBitmunk(&url, media);
      assertNoException();
      
      cout << endl << "Media:" << endl;
      dumpDynamicObject(media, false);
   }
   tr.passIfNoException();
   
   tr.test("invalid get");
   {
      // create url for obtaining media
      Url url("/api/3.0/media/invalidMediaId");

      // get media
      Media media;
      messenger->getFromBitmunk(&url, media);
   }
   tr.passIfException();

   tr.test("all");
   {
      // create url for getting media
      Url url("/api/3.0/media/?start=0&num=5");
      
      // get results
      DynamicObject results;
      //messenger->getFromBitmunk(&url, results);
      assertNoException();
      
      cout << endl << "DISABLED FOR PERFORMANCE" << endl;
      /*
      cout << endl << "Getting 5 media starting at #0" << endl;
      cout << "(of " <<
         results["total"]->getUInt32() << " found)" << endl;
      dumpDynamicObject(results, false);
      */
   }
   tr.passIfNoException();
   
   tr.test("owned");
   {
      // create url for searching media
      Url url("/api/3.0/media?owner=900");
      
      // get results
      DynamicObject results;
      messenger->getFromBitmunk(&url, results);
      assertNoException();
      
      cout << endl << "Media owned by user 900" << endl;
      cout << "Results " << results["start"]->getUInt32() << "-" <<
         results["start"]->getUInt32() + results["num"]->getUInt32() <<
         " of " << results["total"]->getUInt32() << endl;
      dumpDynamicObject(results, false);
   }
   tr.passIfNoException();
   
   tr.test("search");
   {
      // create url for searching media
      Url url("/api/3.0/media/?query=test&start=0&num=10");
      
      // get results
      DynamicObject results;
      messenger->getFromBitmunk(&url, results);
      assertNoException();
      
      cout << endl << "Searching media & contributors for " << "'test'" << endl;
      cout << "Results " << results["start"]->getUInt32() << "-" <<
         results["start"]->getUInt32() + results["num"]->getUInt32() <<
         " of " << results["total"]->getUInt32() << endl;
      dumpDynamicObject(results, false);
   }
   tr.passIfNoException();
   
   tr.test("genre media");
   {
      // create url for searching media
      Url url("/api/3.0/media?type=audio&genre=165&start=4&num=5");
      
      // get results
      DynamicObject results;
      messenger->getFromBitmunk(&url, results);
      assertNoException();
      
      cout << endl << "Audio from genre 165" << endl;
      cout << "Results " << results["start"]->getUInt32() << "-" <<
         results["start"]->getUInt32() + results["num"]->getUInt32() <<
         " of " << results["total"]->getUInt32() << endl;
      dumpDynamicObject(results, false);
   }
   tr.passIfNoException();
   
   tr.test("media list");
   {
      // create url for searching media
      Url url("/api/3.0/media?owner=1&list=1");
      
      // get results
      DynamicObject results;
      messenger->getFromBitmunk(&url, results);
      assertNoException();
      
      cout << endl << "Media list " << results["name"]->getString() << endl;
      cout << "\"" << results["description"]->getString() << "\"" << endl;
      dumpDynamicObject(results, false);
   }
   tr.passIfNoException();
   
   tr.ungroup();
}

void userGetTest(Node& node, TestRunner& tr)
{
   tr.group("user");
   
   Messenger* messenger = node.getMessenger();
   
   tr.test("get self (valid)");
   {
      // create url for obtaining users
      Url url("/api/3.0/users/900");
      
      // get user
      User user;
      messenger->getSecureFromBitmunk(&url, user, node.getDefaultUserId());
      assertNoException();
      
      cout << endl << "User:" << endl;
      dumpDynamicObject(user, false);
   }
   tr.passIfNoException();
   
   /*
   tr.test("get non-self (valid)");
   {
      // create url for obtaining users
      Url url("/api/3.0/users/900");
      
      // get user
      User user;
      messenger->getSecureFromBitmunk(&url, user, node.getDefaultUserId());
      assertNoException();
      
      cout << endl << "User:" << endl;
      dumpDynamicObject(user, false);
   }
   tr.passIfNoException();
   */
   
   tr.test("get identity (valid)");
   {
      // create url for obtaining identity
      Url url("/api/3.0/users/identity/900");
      
      // get identity
      Identity identity;
      messenger->getSecureFromBitmunk(
         &url, identity, node.getDefaultUserId());
      assertNoException();
      
      cout << endl << "Identity:" << endl;
      dumpDynamicObject(identity, false);
   }
   tr.passIfNoException();
   
   /*
   tr.test("get identity (invalid)");
   {
      // create url for obtaining identity
      Url url("/api/3.0/users/identity/69766");
      
      // get identity
      Identity identity;
      messenger->getSecureFromBitmunk((
         &url, identity, node.getDefaultUserId());
      assertException();
      
      cout << endl << "Identity:" << endl;
      dumpDynamicObject(identity, false);
   }
   tr.passIfException();
   */
   
   /*
   tr.test("sellerKey");
   {
      // create url for obtaining users
      Url url("/api/3.0/users/keys/seller/900");
      
      // get user
      DynamicObject sellerKey;
      messenger->getFromBitmunk(&url, sellerKey);
      assertNoException();
      
      cout << endl << "Seller Key:" << endl;
      dumpDynamicObject(sellerKey, false);
   }
   tr.passIfNoException();
   
   tr.test("friends");
   {
      Url url("/api/3.0/friends/?user=900");
      
      // get friends
      User user;
      messenger->getSecureFromBitmunk(&url, user, node.getDefaultUserId());
      assertNoException();
      
      cout << endl << "User Friends:" << endl;
      dumpDynamicObject(user, false);
   }
   tr.passIfNoException();
   */
   
   tr.ungroup();
}

void userAddTest(Node& node, TestRunner& tr)
{
   tr.group("user add");
   
   Messenger* messenger = node.getMessenger();
   
   tr.test("add (valid)");
   {
      Url url("/api/3.0/users");
      
      // password update
      User user;
      user["email"] = "testuser1@bitmunk.com";
      user["username"] = "testuser1";
      user["password"] = "password";
      user["confirm"] = "password";
      user["tosAgree"] = "agree";
      
      messenger->postSecureToBitmunk(
         &url, &user, NULL, node.getDefaultUserId());
      assertNoException();
      
      cout << endl << "User added." << endl;
   }
   tr.passIfNoException();
   
   tr.ungroup();
}

void userUpdateTest(Node& node, TestRunner& tr)
{
   tr.group("user update");
   
   /*
   Messenger* messenger = node.getMessenger();
   
   tr.test("password (valid)");
   {
      Url url("/api/3.0/users/password");
      
      // password update
      User user;
      user["id"] = 70238;
      user["oldPassword"] = "password";
      user["password"] = "omgpants";
      user["confirm"] = "omgpants";
      
      // change password
      messenger->postSecureToBitmunk(
         &url, &user, NULL, BM_USER_ID(user["id"]));
      assertNoException();
      
      cout << endl << "User password updated 1st time." << endl;
      
      user["oldPassword"] = "omgpants";
      user["password"] = "password";
      user["confirm"] = "password";
      
      // change password back
      messenger->postSecureToBitmunk(
         &url, &user, NULL, BM_USER_ID(user["id"]));
      assertNoException();
      
      cout << "User password updated 2nd time." << endl;
   }
   tr.passIfNoException();
   */
   
   /*
   tr.test("password code");
   {
      Url url("/api/3.0/users/password/69766");
      
      messenger->deleteSecureFromBitmunk(
         &url, NULL, node.getDefaultUserId());
      assertNoException();
      
      cout << endl << "Password reset code emailed." << endl;
   }
   tr.passIfNoException();
   */
   
   /*
   tr.test("post email code");
   {
      Url url("/api/3.0/users/emailcode");
      
      // output
      User user;
      user["email"] = "testuser1@bitmunk.com";
      
      messenger->postToBitmunk(&url, &user);
      assertNoException();
      
      cout << endl << "Code emailed." << endl;
   }
   tr.passIfNoException();
   */
   
   /*
   tr.test("put email code");
   {
      Url url("/api/3.0/users/emailcode/900");
      
      // output
      User user;
      user["emailCode"] = "1234567";
      
      cout << "user id=" << node.getDefaultUserId() << endl;
      
      messenger->putSecureToBitmunk(&url, user, NULL, node.getDefaultUserId());
      assertNoException();
      
      cout << endl << "Email verified." << endl;
   }
   tr.passIfNoException();
   */
   
   /*
   tr.test("password - email code (valid)");
   {
      Url url("/api/3.0/users/password");
      
      // input
      DynamicObject in;
      
      // password update
      User user;
      user["id"] = 900;
      user["emailCode"] = "1234567";
      user["password"] = "password";
      user["confirm"] = "password";
      
      messenger->postSecureToBitmunk(&url, &user, &in, node.getDefaultUserId());
      assertNoException();
      
      cout << endl << "User Password:" << endl;
      dumpDynamicObject(in, false);
   }
   tr.passIfNoException();
   */
   
   /*
   tr.test("put identity (valid)");
   {
      // create url for obtaining users
      Url url("/api/3.0/users/identity/900");
      
      // post identity form
      DynamicObject form;
      form["signedStatement"]["statement"] =
         "This is the statement that must be signed.";
      form["signedStatement"]["signature"] =
         "/DEV USER/";
      
      // create identity
      Identity& identity = form["identity"];
      identity["userId"] = 900;
      identity["firstName"] = "DEV";
      identity["lastName"] = "USER";
      Address& address = identity["address"];
      address["street"] = "1700 Kraft Dr. Suite 2408";
      address["locality"] = "Blacksburg";
      address["region"] = "Virginia";
      address["postalCode"] = "24060";
      address["countryCode"] = "US";
      
      Identity in;
      messenger->putSecureToBitmunk(&url, form, &in, node.getDefaultUserId());
      assertNoException();
      
      cout << endl << "Identity:" << endl;
      dumpDynamicObject(in, false);
   }
   tr.passIfNoException();
   */
   
   /*
   tr.test("put identity (invalid)");
   {
      // create url for obtaining users
      Url url("/api/3.0/users/identity/69776");
      
      // post identity form
      DynamicObject form;
      form["signedStatement"]["statement"] =
         "This is the statement that must be signed.";
      form["signedStatement"]["signature"] =
         "/Dev User/";
      
      // create identity
      Identity& identity = form["identity"];
      identity["userId"] = 900;
      identity["firstName"] = "Dev";
      identity["lastName"] = "User";
      Address& address = identity["address"];
      address["street"] = "1700 Kraft Dr. Suite 2408";
      address["locality"] = "Blacksburg";
      address["region"] = "Virginia";
      address["postalCode"] = "24060";
      address["countryCode"] = "US";
      
      Identity in;
      messenger->putSecureToBitmunk(&url, form, &in, node.getDefaultUserId());
      assertException();
      
      cout << endl << "Exception:" << endl;
      dumpException();
      Exception::clearLast();
   }
   tr.passIfNoException();
   */
   
   tr.ungroup();
}

void accountGetTest(Node& node, TestRunner& tr)
{
   tr.group("accounts");
   
   Messenger* messenger = node.getMessenger();
   
   tr.test("get");
   {
      Url url("/api/3.0/accounts/?owner=900");
      
      // get all accounts
      User user;
      messenger->getSecureFromBitmunk(&url, user, node.getDefaultUserId());
      
      string userstr = JsonWriter::writeToString(user);
      
      cout << endl << "Accounts:" << endl << userstr << endl;
   }
   tr.passIfNoException();
   
   tr.ungroup();
   
   tr.group("account");
   
   tr.test("get");
   {
      // create url for obtaining users
      Url url("/api/3.0/accounts/9000");
      
      // get account
      Account account;
      messenger->getSecureFromBitmunk(&url, account, node.getDefaultUserId());
      assertNoException();
      
      cout << endl << "Account:" << endl;
      dumpDynamicObject(account, false);
   }
   tr.passIfNoException();
   
   tr.ungroup();
}

void contributorGetTest(Node& node, TestRunner& tr)
{
   tr.group("contributors");
   
   Messenger* messenger = node.getMessenger();
   
   tr.test("id");
   {
      Url url("/api/3.0/contributors/1");
      
      // get contributor
      DynamicObject contributor;
      messenger->getFromBitmunk(&url, contributor);
      assertNoException();
      
      cout << endl << "Contributor:" << endl;
      dumpDynamicObject(contributor, false);
   }
   tr.passIfNoException();
   
   tr.test("owner");
   {
      Url url("/api/3.0/contributors/?owner=900");
      
      // get contributor
      DynamicObject contributors;
      messenger->getFromBitmunk(&url, contributors);
      assertNoException();
      
      cout << endl << "Contributors:" << endl;
      dumpDynamicObject(contributors, false);
   }
   tr.passIfNoException();
   
   tr.test("media");
   {
      Url url("/api/3.0/contributors/?media=1");
      
      // get contributor
      DynamicObject contributors;
      messenger->getFromBitmunk(&url, contributors);
      assertNoException();
      
      cout << endl << "Contributors:" << endl;
      dumpDynamicObject(contributors, false);
   }
   tr.passIfNoException();
   
   tr.ungroup();
}

void permissionGetTest(Node& node, TestRunner& tr)
{
   tr.group("permissions");
   
   Messenger* messenger = node.getMessenger();
   
   tr.test("group get");
   {
      // create url for obtaining permission group information
      Url url("/api/3.0/permissions/100");
      
      // get permission group
      PermissionGroup group;
      messenger->getFromBitmunk(&url, group);
      assertNoException();
      
      cout << endl << "Group:" << endl;
      dumpDynamicObject(group, false);
   }
   tr.passIfNoException();
   
   tr.ungroup();
}

void reviewGetTest(Node& node, TestRunner& tr)
{
   tr.group("reviews");
   
   Messenger* messenger = node.getMessenger();
   
   tr.test("user get");
   {
      // create url for obtaining user reviews
      Url url("/api/3.0/reviews/user/900");
      
      // get account
      DynamicObject reviews;
      messenger->getSecureFromBitmunk(&url, reviews, node.getDefaultUserId());
      assertNoException();
      
      cout << endl << "Reviews:" << endl;
      dumpDynamicObject(reviews, false);
   }
   tr.passIfNoException();
   
   tr.test("media get");
   {
      // create url for obtaining media reviews
      Url url("/api/3.0/reviews/media/1");
      
      // get account
      DynamicObject reviews;
      messenger->getSecureFromBitmunk(&url, reviews, node.getDefaultUserId());
      assertNoException();
      
      cout << endl << "Reviews:" << endl;
      dumpDynamicObject(reviews, false);
   }
   tr.passIfNoException();
   
   tr.ungroup();
}

void acquireLicenseTest(Node& node, TestRunner& tr)
{
   tr.group("media license");
   
   Messenger* messenger = node.getMessenger();
   
   tr.test("acquire signed media");
   {
      // create url for obtaining media license
      Url url("/api/3.0/sva/contracts/media/2");
      
      // get signed media
      Media media;
      messenger->postSecureToBitmunk(
         &url, NULL, &media, node.getDefaultUserId());
      assertNoException();
      
      cout << endl << "Signed Media:" << endl;
      dumpDynamicObject(media, false);
   }
   tr.passIfNoException();
   
   tr.ungroup();   
}

void pingPerfTest(Node& node, TestRunner& tr, bitmunk::test::Tester* tester)
{
   tr.group("ping perf");
   
   Messenger* messenger = node.getMessenger();
   
   // number of loops for each test
   Config cfg = node.getConfigManager()->getConfig(
      tester->getApp()->getMetaConfig()["groups"]["main"]->getString());
   int n = cfg->hasMember("loops") ? cfg["loops"]->getInt32() : 50;
   
   tr.test("insecure get");
   {
      Url url("/api/3.0/system/test/ping");
      
      DynamicObject dummy;
      uint64_t startTime = Timer::startTiming();
      for(int i = 0; i < n; ++i)
      {
         messenger->getFromBitmunk(&url, dummy);
      }
      double dt = Timer::getSeconds(startTime);
      printf("t=%g ms, r=%g ms, r/s=%g",  dt * 1000.0, dt/n * 1000.0, n/dt);
   }
   tr.passIfNoException();
   
   tr.test("secure get");
   {
      Url url("/api/3.0/system/test/ping");
      
      DynamicObject dummy;
      uint64_t startTime = Timer::startTiming();
      for(int i = 0; i < n; ++i)
      {
         messenger->getSecureFromBitmunk(&url, dummy, node.getDefaultUserId());
      }
      double dt = Timer::getSeconds(startTime);
      printf("t=%g ms, r=%g ms, r/s=%g",  dt * 1000.0, dt/n * 1000.0, n/dt);
   }
   tr.passIfNoException();
   
   tr.ungroup();
}

void accountGetPerfTest(Node& node, TestRunner& tr)
{
   tr.group("accounts perf");
   
   Messenger* messenger = node.getMessenger();
   
   // number of loops
   int n = 25;
   
   tr.test("insecure get");
   {
      Url url("/api/3.0/accounts/?owner=900");
      
      uint64_t startTime = Timer::startTiming();
      for(int i = 0; i < n; ++i)
      {
         User user;
         messenger->getFromBitmunk(&url, user);
      }
      double dt = Timer::getSeconds(startTime);
      printf("t=%g ms, r=%g ms, r/s=%g",  dt * 1000.0, dt/n * 1000.0, n/dt);
   }
   tr.passIfNoException();
   
   tr.test("secure get");
   {
      Url url("/api/3.0/accounts/?owner=900");
      
      uint64_t startTime = Timer::startTiming();
      for(int i = 0; i < n; ++i)
      {
         User user;
         messenger->getFromBitmunk(&url, user, node.getDefaultUserId());
      }
      double dt = Timer::getSeconds(startTime);
      printf("t=%g ms, r=%g ms, r/s=%g",  dt * 1000.0, dt/n * 1000.0, n/dt);
   }
   tr.passIfNoException();
   
   tr.ungroup();
}

class BmNodeServiceTester : public bitmunk::test::Tester
{
public:
   BmNodeServiceTester()
   {
      setName("Node Services");
   }

   /**
    * Run automatic unit tests.
    */
   virtual int runAutomaticTests(TestRunner& tr)
   {
      // FIXME:
#if 0
      // create a client node for communicating
      Node node;
      {
         bool success;
         success = setupNode(&node);
         assertNoException();
         assert(success);
         success = setupPeerNode(&node);
         assertNoException();
         assert(success);

         // Note: always print this out to avoid confusion
         const char* profileFile = "devuser.profile";
         const char* profileDir =
            getApp()->getConfig()["node"]["profilePath"]->getString();
         string prof = File::join(profileDir, profileFile);
         File file(prof.c_str());
         cout << "You must copy " << profileFile << " from pki to " <<
            profileDir << " to run this. If you're seeing security breaches, " <<
            "your copy may be out of date." << endl;
         cout << "You may need to remove testuser5.profile from /tmp/ to "
            "run the password update test." << endl;
         if(!file->exists())
         {
            exit(1);
         }
      }
      if(!node.start())
      {
         dumpException();
         exit(1);
      }
      
      // login the devuser
      node.login("devuser", "password");
      //node.login("testuser5", "password");
      assertNoException();
      
      // run tests
      //mediaGetTest(node, tr);
      //userGetTest(node, tr);
      //userAddTest(node, tr);
      //userUpdateTest(node, tr);
      //accountGetTest(node, tr);
      //contributorGetTest(node, tr);
      //permissionGetTest(node, tr);
      //reviewGetTest(node, tr);
      //acquireLicenseTest(node, tr);
      
      // performance tests
      //accountGetPerfTest(node, tr);
      
      // logout of client node
      node.logout(0);
      node.stop();
#endif
      return 0;
   }

   /**
    * Runs interactive unit tests.
    */
   virtual int runInteractiveTests(TestRunner& tr)
   {
      // FIXME:
#if 0
      Node node;
      {
         bool success;
         success = setupNode(&node);
         assertNoException();
         assert(success);
         success = setupPeerNode(&node);
         assertNoException();
         assert(success);
      }
      if(!node.start())
      {
         dumpException();
         exit(1);
      }
      
      // login the devuser
      node.login("devuser", "password");
      //node.login("testuser5", "password");
      assertNoException();
      
      Config cfg = getApp()->getConfig();
      const char* test = cfg["monarch.test.Tester"]["test"]->getString();
      bool all = (strcmp(test, "all") == 0);
      
      if(all || (strcmp(test, "ping") == 0))
      {
         pingPerfTest(node, tr, this);
      }
      
      // logout of client node
      node.logout(0);
      node.stop();
#endif
      return 0;
   }
};

#ifndef MO_TEST_NO_MAIN
BM_TEST_MAIN(BmNodeServiceTester)
#endif
