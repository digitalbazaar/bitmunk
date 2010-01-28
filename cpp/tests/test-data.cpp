/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#include <iostream>

#include "bitmunk/data/Id3v2Tag.h"
#include "bitmunk/data/Id3v2TagReader.h"
#include "bitmunk/data/Id3v2TagWriter.h"
#include "bitmunk/database/DatabaseManager.h"
#include "bitmunk/node/Node.h"
#include "bitmunk/test/Tester.h"
#include "monarch/test/Test.h"
#include "monarch/test/TestRunner.h"
#include "monarch/rt/DynamicObject.h"
#include "monarch/data/json/JsonWriter.h"
#include "monarch/data/InspectorInputStream.h"
#include "monarch/io/FileInputStream.h"
#include "monarch/io/FileOutputStream.h"
#include "monarch/io/MutatorInputStream.h"

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::data;
using namespace bitmunk::database;
using namespace bitmunk::node;
using namespace monarch::config;
using namespace monarch::data;
using namespace monarch::data::json;
using namespace monarch::io;
using namespace monarch::rt;
using namespace monarch::test;

#define TEST_MEDIA_ID 3
#define TEST_CONTRACT_ID 2008070010000000001ULL;
#define TEST_INPUT_FILENAME "/tmp/bmtestfile.mp3"
#define TEST_OUTPUT_FILENAME "/tmp/bmtestfile-id3v2.mp3"

void runId3v2Test(Node& node, TestRunner& tr, bitmunk::test::Tester& tester)
{
   tr.group("id3v2 tag writing");
   
   // get database manager
   DatabaseManager* dm = DatabaseManager::getInstance(&node);
   assertNoException();
   
   // populate media
   MediaManager* mm = dm->getMediaManager();
   Media media;
   media["id"] = TEST_MEDIA_ID;
   mm->populateMedia(media);
   assertNoException();
   
   // create fake contract with minimum information
   Contract contract;
   contract["id"] = TEST_CONTRACT_ID;
   contract["media"] = media;
   contract["sections"]->setType(Map);
   
   // create fake file info with minimum information
   FileInfo fileInfo;
   fileInfo["mediaId"] = TEST_MEDIA_ID;
   
   // write id3v2 tag
   tr.test("Id3v2Tag write");
   {
      // create bitmunk id3v2 tag
      Id3v2Tag tag(contract, fileInfo);
      
      //JsonWriter::writeDynamicObjectToStdOut(contract);
      
      // strip and embed new id3v2 tag
      Id3v2TagWriter stripper(NULL);
      Id3v2TagWriter embedder(&tag, false, tag.getFrameSource(), false);
      
      // set up read stream
      File inputFile(TEST_INPUT_FILENAME);
      FileInputStream fis(inputFile);
      MutatorInputStream stripStream(&fis, false, &stripper, false);
      MutatorInputStream embedStream(&stripStream, false, &embedder, false);
      
      // set up write stream
      File outputFile(TEST_OUTPUT_FILENAME);
      FileOutputStream fos(outputFile);
      
      // read data and write it out to a new file
      char b[2048];
      int numBytes;
      while((numBytes = embedStream.read(b, 2048)) > 0)
      {
         fos.write(b, numBytes);
      }
      
      // close streams
      embedStream.close();
      fos.close();
   }
   tr.passIfNoException();
   
   // read id3v2 tag
   tr.test("Id3v2Tag read");
   {
      // create empty bitmunk id3v2 tag
      Id3v2Tag tag;
      
      // create id3v2 tag reader
      Id3v2TagReader reader(&tag, tag.getFrameSink());
      
      // set up read stream
      File inputFile(TEST_OUTPUT_FILENAME);
      FileInputStream fis(inputFile);
      InspectorInputStream iis(&fis, false);
      iis.addInspector("id3v2", &reader, false);
      
      // inspect data
      iis.inspect();
      
      // close stream
      iis.close();
      
      // confirm contract is equivalent
      Contract& readContract = tag.getFrameSink()->getContract();
      assert(readContract == contract);
   }
   tr.passIfNoException();
   
   tr.ungroup();
}

class BmDataTester : public bitmunk::test::Tester
{
public:
   BmDataTester()
   {
      setName("Data Tester");
   }

   /**
    * Run automatic unit tests.
    */
   virtual int runAutomaticTests(TestRunner& tr)
   {
      // FIXME:
#if 0
      // create a node
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
      
      // login probably not necessary for data tests
      /*
      // Note: always print this out to avoid confusion
      const char* profileFile = "devuser.profile";
      const char* profilePath = getConfig["node"]["profilePath"]->getString();
      string prof = File::join(profilePath, profileFile, NULL);
      File file(prof.c_str());
      if(!file->exists())
      {
         cout << "You must copy " << profileFile << " from pki to " <<
            profilePath << " to run this." << endl;
         exit(1);
      }
      
      // login the devuser
      node.login("devuser", "password");
      assertNoException();
      */
      
      // run test
      runId3v2Test(node, tr, *this);
      
      // logout of client node
      //node.logout(0);
      
      // stop node
      node.stop();
#endif
      return 0;
   }

   /**
    * Runs interactive unit tests.
    */
   virtual int runInteractiveTests(TestRunner& tr)
   {
      return 0;
   }
};

#ifndef MO_TEST_NO_MAIN
BM_TEST_MAIN(BmDataTester)
#endif
