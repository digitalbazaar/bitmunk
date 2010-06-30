/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/test/Tester.h"
#include "monarch/test/Test.h"
#include "monarch/test/TestModule.h"


#include "bitmunk/node/Node.h"
#include "bitmunk/test/Tester.h"
#include "monarch/data/json/JsonWriter.h"
#include "monarch/io/File.h"
#include "monarch/rt/DynamicObject.h"
#include "monarch/rt/Exception.h"
#include "monarch/test/Test.h"
#include "monarch/test/TestModule.h"
#include "monarch/util/Timer.h"

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::protocol;
using namespace bitmunk::node;
using namespace bitmunk::test;
using namespace monarch::config;
using namespace monarch::data::json;
using namespace monarch::io;
using namespace monarch::net;
using namespace monarch::rt;
using namespace monarch::test;
using namespace monarch::util;

namespace bm_tests_node_services
{

static void mediaGetTest(Node& node, TestRunner& tr)
{
   tr.group("media");

   Messenger* messenger = node.getMessenger();

   tr.test("get");
   {
      // create url for obtaining media
      Url url("/api/3.0/media/2");

      // get media
      Media media;
      assertNoException(
         messenger->getFromBitmunk(&url, media));

      printf("\nMedia:\n");
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
      //assertNoException(
      //   messenger->getFromBitmunk(&url, results));

      printf("\nDISABLED FOR PERFORMANCE\n");
      /*
      printf("\nGetting 5 media starting at #0\n(of %" PRIu32 " found)\n",
         results["total"]->getUInt32());
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
      assertNoException(
         messenger->getFromBitmunk(&url, results));

      printf("\nMedia owned by user 900\n");
      printf("Results %" PRIu32 "-%" PRIu32 " of %" PRIu32 "\n",
         results["start"]->getUInt32(),
         results["start"]->getUInt32() + results["num"]->getUInt32(),
         results["total"]->getUInt32());
      dumpDynamicObject(results, false);
   }
   tr.passIfNoException();

   tr.test("search");
   {
      // create url for searching media
      Url url("/api/3.0/media/?query=test&start=0&num=10");

      // get results
      DynamicObject results;
      assertNoException(
         messenger->getFromBitmunk(&url, results));

      printf("\nSearching media & contributors for 'test'\n");
      printf("Results %" PRIu32 "-%" PRIu32 " of %" PRIu32 "\n",
         results["start"]->getUInt32(),
         results["start"]->getUInt32() + results["num"]->getUInt32(),
         results["total"]->getUInt32());
      dumpDynamicObject(results, false);
   }
   tr.passIfNoException();

   tr.test("genre media");
   {
      // create url for searching media
      Url url("/api/3.0/media?type=audio&genre=165&start=4&num=5");

      // get results
      DynamicObject results;
      assertNoException(
         messenger->getFromBitmunk(&url, results));

      printf("\nAudio from genre 165\n");
      printf("Results %" PRIu32 "-%" PRIu32 " of %" PRIu32 "\n",
         results["start"]->getUInt32(),
         results["start"]->getUInt32() + results["num"]->getUInt32(),
         results["total"]->getUInt32());
      dumpDynamicObject(results, false);
   }
   tr.passIfNoException();

   tr.test("media list");
   {
      // create url for searching media
      Url url("/api/3.0/media?owner=1&list=1");

      // get results
      DynamicObject results;
      assertNoException(
         messenger->getFromBitmunk(&url, results));

      printf("\nMedia list %s\n", results["name"]->getString());
      printf("\"%s\"\n", results["description"]->getString());
      dumpDynamicObject(results, false);
   }
   tr.passIfNoException();

   tr.ungroup();
}

static void userGetTest(Node& node, TestRunner& tr)
{
   tr.group("user");

   Messenger* messenger = node.getMessenger();

   tr.test("get self (valid)");
   {
      // create url for obtaining users
      Url url("/api/3.0/users/900");

      // get user
      User user;
      assertNoException(
         messenger->getSecureFromBitmunk(&url, user, node.getDefaultUserId()));

      printf("\nUser:\n");
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

      printf("\nUser:\n");
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
      assertNoException(
         messenger->getSecureFromBitmunk(
            &url, identity, node.getDefaultUserId()));

      printf("\nIdentity:\n");
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

      printf("\nIdentity:\n");
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

      printf("\nSeller Key:\n");
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

      printf("\nUser Friends:\n");
      dumpDynamicObject(user, false);
   }
   tr.passIfNoException();
   */

   tr.ungroup();
}

static void userAddTest(Node& node, TestRunner& tr)
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

      assertNoException(
         messenger->postSecureToBitmunk(
            &url, &user, NULL, node.getDefaultUserId()));

      printf("\nUser added.\n");
   }
   tr.passIfNoException();

   tr.ungroup();
}

static void userUpdateTest(Node& node, TestRunner& tr)
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

      printf("\nUser password updated 1st time.\n");

      user["oldPassword"] = "omgpants";
      user["password"] = "password";
      user["confirm"] = "password";

      // change password back
      messenger->postSecureToBitmunk(
         &url, &user, NULL, BM_USER_ID(user["id"]));
      assertNoException();

      printf("User password updated 2nd time.\n");
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

      printf("\nPassword reset code emailed.\n");
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

      printf("\nCode emailed.\n");
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

      printf("user id=%" PRIu64 "\n", node.getDefaultUserId());

      messenger->putSecureToBitmunk(&url, user, NULL, node.getDefaultUserId());
      assertNoException();

      printf("\nEmail verified.\n");
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

      printf("\nUser Password:\n");
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

      printf("\nIdentity:\n");
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

      printf("\nException:\n");
      dumpException();
      Exception::clearLast();
   }
   tr.passIfNoException();
   */

   tr.ungroup();
}

static void accountGetTest(Node& node, TestRunner& tr)
{
   tr.group("accounts");

   Messenger* messenger = node.getMessenger();

   tr.test("get");
   {
      Url url("/api/3.0/accounts/?owner=900");

      // get all accounts
      User user;
      messenger->getSecureFromBitmunk(&url, user, node.getDefaultUserId());
      printf("\nAccounts:\n");
      dumpDynamicObject(user);
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
      assertNoException(
         messenger->getSecureFromBitmunk(
            &url, account, node.getDefaultUserId()));

      printf("\nAccount:\n");
      dumpDynamicObject(account, false);
   }
   tr.passIfNoException();

   tr.ungroup();
}

static void contributorGetTest(Node& node, TestRunner& tr)
{
   tr.group("contributors");

   Messenger* messenger = node.getMessenger();

   tr.test("id");
   {
      Url url("/api/3.0/contributors/1");

      // get contributor
      DynamicObject contributor;
      assertNoException(
         messenger->getFromBitmunk(&url, contributor));

      printf("\nContributor:\n");
      dumpDynamicObject(contributor, false);
   }
   tr.passIfNoException();

   tr.test("owner");
   {
      Url url("/api/3.0/contributors/?owner=900");

      // get contributor
      DynamicObject contributors;
      assertNoException(
         messenger->getFromBitmunk(&url, contributors));

      printf("\nContributors:\n");
      dumpDynamicObject(contributors, false);
   }
   tr.passIfNoException();

   tr.test("media");
   {
      Url url("/api/3.0/contributors/?media=1");

      // get contributor
      DynamicObject contributors;
      assertNoException(
         messenger->getFromBitmunk(&url, contributors));

      printf("\nContributors:\n");
      dumpDynamicObject(contributors, false);
   }
   tr.passIfNoException();

   tr.ungroup();
}

static void permissionGetTest(Node& node, TestRunner& tr)
{
   tr.group("permissions");

   Messenger* messenger = node.getMessenger();

   tr.test("group get");
   {
      // create url for obtaining permission group information
      Url url("/api/3.0/permissions/100");

      // get permission group
      PermissionGroup group;
      assertNoException(
         messenger->getFromBitmunk(&url, group));

      printf("\nGroup:\n");
      dumpDynamicObject(group, false);
   }
   tr.passIfNoException();

   tr.ungroup();
}

static void reviewGetTest(Node& node, TestRunner& tr)
{
   tr.group("reviews");

   Messenger* messenger = node.getMessenger();

   tr.test("user get");
   {
      // create url for obtaining user reviews
      Url url("/api/3.0/reviews/user/900");

      // get account
      DynamicObject reviews;
      assertNoException(
         messenger->getSecureFromBitmunk(
            &url, reviews, node.getDefaultUserId()));

      printf("\nReviews:\n");
      dumpDynamicObject(reviews, false);
   }
   tr.passIfNoException();

   tr.test("media get");
   {
      // create url for obtaining media reviews
      Url url("/api/3.0/reviews/media/1");

      // get account
      DynamicObject reviews;
      assertNoException(
         messenger->getSecureFromBitmunk(
            &url, reviews, node.getDefaultUserId()));

      printf("\nReviews:\n");
      dumpDynamicObject(reviews, false);
   }
   tr.passIfNoException();

   tr.ungroup();
}

static void acquireLicenseTest(Node& node, TestRunner& tr)
{
   tr.group("media license");

   Messenger* messenger = node.getMessenger();

   tr.test("acquire signed media");
   {
      // create url for obtaining media license
      Url url("/api/3.0/sva/contracts/media/2");

      // get signed media
      Media media;
      assertNoException(
         messenger->postSecureToBitmunk(
            &url, NULL, &media, node.getDefaultUserId()));

      printf("\nSigned Media:\n");
      dumpDynamicObject(media, false);
   }
   tr.passIfNoException();

   tr.ungroup();
}

static void pingPerfTest(Node& node, TestRunner& tr)
{
   tr.group("ping perf");

   Messenger* messenger = node.getMessenger();

   // number of loops for each test
   Config cfg = tr.getApp()->getConfig();
   //node.getConfigManager()->getConfig(
   //   tester->getApp()->getMetaConfig()["groups"]["main"]->getString());
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

static void accountGetPerfTest(Node& node, TestRunner& tr)
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

static bool run(TestRunner& tr)
{
   if(tr.isTestEnabled("fixme"))
   {
      // load and start node
      Node* node = Tester::loadNode(tr/*, "test-node-services"*/);
      assertNoException(
         node->start());

      //cout << "You may need to remove testuser5.profile from /tmp/ to "
      //   "run the password update test." << endl;

      // login the devuser
      //node.login("devuser", "password");
      //node.login("testuser5", "password");
      assertNoExceptionSet();

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

      // stop and unload node
      node->stop();
      Tester::unloadNode(tr);
   }

   if(tr.isTestEnabled("fixme"))
   {
      // load and start node
      Node* node = Tester::loadNode(tr/*, "test-node-services"*/);
      assertNoException(
         node->start());

      // login the devuser
      //node.login("devuser", "password");
      //node.login("testuser5", "password");
      assertNoExceptionSet();

      Config cfg = tr.getApp()->getConfig();
      const char* test = cfg["monarch.test.Tester"]["test"]->getString();
      bool all = (strcmp(test, "all") == 0);

      if(all || (strcmp(test, "ping") == 0))
      {
         pingPerfTest(*node, tr);
      }

      // stop and unload node
      node->stop();
      Tester::unloadNode(tr);
   }

   return true;
};

} // end namespace

MO_TEST_MODULE_FN(
   "bitmunk.tests.node-services.test", "1.0", bm_tests_node_services::run)
