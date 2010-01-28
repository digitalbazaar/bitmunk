/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#include <iostream>
#include <sstream>

#include "bitmunk/bfp/IBfpModule.h"
#include "bitmunk/common/Tools.h"
#include "bitmunk/database/DatabaseManager.h"
#include "bitmunk/node/Node.h"
#include "bitmunk/test/Tester.h"
#include "monarch/crypto/AsymmetricKeyFactory.h"
#include "monarch/crypto/DigitalEnvelope.h"
#include "monarch/rt/Exception.h"
#include "monarch/test/Test.h"
#include "monarch/test/TestRunner.h"
#include "monarch/rt/DynamicObject.h"
#include "monarch/data/json/JsonWriter.h"
#include "monarch/io/File.h"
#include "monarch/io/FileInputStream.h"
#include "monarch/io/FileOutputStream.h"
#include "monarch/io/OStreamOutputStream.h"
#include "monarch/util/Convert.h"
#include "monarch/util/Timer.h"

using namespace std;
using namespace bitmunk::bfp;
using namespace bitmunk::common;
using namespace bitmunk::database;
using namespace bitmunk::protocol;
using namespace bitmunk::node;
using namespace monarch::crypto;
using namespace monarch::data::json;
using namespace monarch::io;
using namespace monarch::net;
using namespace monarch::rt;
using namespace monarch::util;
using namespace monarch::test;

#define TEST_USER_ID     900
#define TEST_USER_ID_STR "900"

static IBfpModule* getIBfpInstance(Node* node)
{
   // get the BFP module interface
   IBfpModule* rval = dynamic_cast<IBfpModule*>(
      node->getModuleApi("bitmunk.bfp.Bfp"));
   if(rval == NULL)
   {
      ExceptionRef e = new Exception(
         "Could not get bfp module interface. No bfp module found.",
         "bitmunk.bfp.MissingBfpModule");
      Exception::set(e);
   }

   return rval;
}

void runBfpFactoryTest(
   Node& node, TestRunner& tr, bitmunk::test::Tester& tester)
{
   tr.group("BfpFactory");

   // get BfpModule interface
   IBfpModule* ibm = getIBfpInstance(&node);
   assertNoException();

   tr.test("Get valid bfp");
   {
      // create bfp version 1
      Bfp* bfp = ibm->createBfp(1);
      assertNoException();

      // free bfp
      ibm->freeBfp(bfp);
   }
   tr.passIfNoException();

   tr.test("Get invalid bfp");
   {
      // create bfp version 0
      Bfp* bfp = ibm->createBfp(0);
      ibm->freeBfp(bfp);
   }
   tr.passIfException();

   // clear exception
   Exception::clear();

   tr.ungroup();
}

void runBfpSetFileInfoIdTest(
   Node& node, TestRunner& tr, bitmunk::test::Tester& tester,
   const char* extension)
{
   tr.test("Bfp setFileInfoId");

   char filename[100];
   snprintf(filename, 100, "/tmp/bmtestfile.%s", extension);
   File file(filename);
   if(!file->exists())
   {
      string temp = filename;
      temp.append(" does not exist, not running test!");
      tr.warning(temp.c_str());
   }
   else
   {
      // get BfpModule interface
      IBfpModule* ibm = getIBfpInstance(&node);
      assertNoException();

      // create bfp version 1
      Bfp* bfp = ibm->createBfp(1);
      assertNoException();

      // set up file info
      FileInfo fi;
      fi["path"] = filename;
      fi["extension"] = extension;

      // set file info ID
      bfp->setFileInfoId(fi);

      //cout << endl << "FILE ID=" << fi["id"]->getString();
      // FIXME: we must set a rigid file for testing so we can
      // check the ID here against an excepted string

      // free bfp
      ibm->freeBfp(bfp);
   }

   tr.passIfNoException();
}

void runBfpWebBuyTest(
   Node& node, TestRunner& tr, bitmunk::test::Tester& tester,
   const char* extension)
{
   string msg = "Bfp WebBuy ";
   msg.append(extension);
   tr.group(msg.c_str());

   char filename[100];
   snprintf(filename, 100, "/tmp/bmtestfile.%s", extension);
   File file(filename);
   if(!file->exists())
   {
      string temp = filename;
      temp.append(" does not exist, not running test!");
      tr.warning(temp.c_str());
   }
   else
   {
      // get BfpModule interface
      IBfpModule* ibm = getIBfpInstance(&node);
      assertNoException();

      // create bfp version 1
      Bfp* bfp = ibm->createBfp(1);
      assertNoException();

      // set up media
      Media media;
      media["id"] = 2;
      DatabaseManager* dm = DatabaseManager::getInstance(&node);
      assert(dm != NULL);
      MediaManager* mm = dm->getMediaManager();
      mm->populateMedia(media, NULL);

      // set up contract section
      ContractSection cs;
      cs["seller"]["userId"] = TEST_USER_ID;

      // set up contract
      Contract c;
      c["version"] = "3.0";
      c["id"] = 1;
      c["media"] = media;
      c["buyer"]["userId"] = TEST_USER_ID;
      c["sections"][TEST_USER_ID_STR][0] = cs;

      tr.test("initialize");
      {
         bfp->initializeWebBuy(c, Tools::getContractSectionHash(cs).c_str());
      }
      tr.passIfNoException();

      // set up file info
      FileInfo fi;
      fi["path"] = filename;
      //fi["type"] =
      // FIXME: use mime type somewhere
      fi["extension"] = extension;
      bfp->setFileInfoId(fi);

      // create file
      File file(filename);

      // set up file piece, full size = 0
      FilePiece fp;
      snprintf(filename, 100, "/tmp/bmbfpwebbuy.%s", extension);
      fp["path"] = filename;
      fp["index"] = 0;
      fp["size"] = 0;

      tr.test("prepare file");
      {
         // prepare web buy file
         bfp->prepareWebBuyFile(fi);
      }
      tr.passIfNoException();

      tr.test("start reading");
      {
         // start reading
         bfp->startReading(fp);
      }
      tr.passIfNoException();

      tr.test("read");
      {
         File out(fp["path"]->getString());
         FileOutputStream fos(out);

         char b[2048];
         int numBytes;
         bool err = false;
         while((numBytes = bfp->read(b, 2048)) > 0 && !err)
         {
            err = !fos.write(b, numBytes);
         }

         fos.close();
      }
      tr.passIfNoException();

      // verify file ID is unchanged
      tr.test("verify file ID");
      {
         // calculate file ID and see if its the same
         string oldId = fi["id"]->getString();
         fi["id"] = "";
         bfp->setFileInfoId(fi);
         assertStrCmp(oldId.c_str(), fi["id"]->getString());
      }
      tr.passIfNoException();

      // free bfp
      ibm->freeBfp(bfp);
   }

   tr.ungroup();
}

void runBfpSinglePieceTest(
   Node& node, TestRunner& tr, bitmunk::test::Tester& tester,
   const char* extension)
{
   string msg = "PeerBuy one piece ";
   msg.append(extension);
   tr.group(msg.c_str());

   string sellerFileInfoId = "sellerFileInfoId";
   string buyerFileInfoId = "buyerFileInfoId";

   // setup the input filename
   char inputFilename[100];
   snprintf(inputFilename, 100, "/tmp/bmtestfile.%s", extension);
   File inputFile(inputFilename);

   // setup the encrypted filename
   char encryptedFilename[100];
   snprintf(encryptedFilename, 100, "/tmp/bmtestfile-%s-0000.fp", extension);
   File encryptedFile(encryptedFilename);

   // setup the decrypted filename
   char decryptedFilename[100];
   snprintf(decryptedFilename, 100, "/tmp/bmtestfile-decrypted.%s", extension);
   File decryptedFile(decryptedFilename);

   // populate the media
   Media media;
   media["id"] = 2;
   DatabaseManager* dm = DatabaseManager::getInstance(&node);
   assert(dm != NULL);
   MediaManager* mm = dm->getMediaManager();
   mm->populateMedia(media, NULL);

   // create the contract section
   ContractSection cs;
   cs["seller"]["userId"] = TEST_USER_ID;

   // create the contract
   Contract c;
   c["version"] = "3.0";
   c["id"] = 1;
   c["media"] = media;
   c["buyer"]["userId"] = TEST_USER_ID;
   c["sections"][TEST_USER_ID_STR][0] = cs;

   if(!inputFile->exists())
   {
      string temp = inputFilename;
      temp.append(" does not exist, not running test!");
      tr.warning(temp.c_str());
   }
   else
   {
      AsymmetricKeyFactory akf;
      PrivateKeyRef sellerDecryptKey;
      PublicKeyRef sellerEncryptKey;
      akf.createKeyPair("RSA", sellerDecryptKey, sellerEncryptKey);

      // get the seller BfpModule interface
      IBfpModule* ibm = getIBfpInstance(&node);
      assertNoException();

      // create bfp version 1
      Bfp* bfp = ibm->createBfp(1);
      assertNoException();

      // generate the contract section hash
      string csHash = Tools::getContractSectionHash(cs);

      // initialize the seller peer sell module
      tr.test("seller bfp initialization");
      {
         string pem = akf.writePublicKeyToPem(sellerEncryptKey);
         bfp->initializePeerSell(csHash.c_str(), pem.c_str());
      }
      tr.passIfNoException();

      // prepare the seller bfp for peer selling
      FileInfo fi;
      fi["path"] = inputFilename;
      fi["extension"] = extension;
      tr.test("seller bfp prepare");
      {
         uint64_t startTime = Timer::startTiming();
         bfp->preparePeerSellFile(fi);
         printf("[time:%gs]\n", Timer::getSeconds(startTime));
      }
      tr.passIfNoException();

      // setup the file piece information
      FilePiece fp;
      fp["path"] = encryptedFilename;
      fp["index"] = 0;
      fp["size"] = 0;

      // encode the file using the bfp for peer sell and write it to disk
      tr.test("seller bfp encrypt");
      {
         FileOutputStream fos(encryptedFile);

         // start the reading process in the BFP
         bfp->startReading(fp);
         assertNoException();

         // write the data to disk
         char b[2048];
         int numBytes;
         bool err = false;
         while((numBytes = bfp->read(b, 2048)) > 0 && !err)
         {
            err = !fos.write(b, numBytes);
         }

         fos.close();

         // save the seller file ID and check it after the file is decrypted
         fi["id"] = "";
         bfp->setFileInfoId(fi);
         sellerFileInfoId = fi["id"]->getString();
         fi["id"] = "";
      }
      tr.passIfNoException();

      // buyer BFP decode tests

      // Retrieve the openKey for unsealing the pieceKey
      SymmetricKey openKey;
      Tools::populateSymmetricKey(&openKey, fp["openKey"]);

      // unseal the decyption key
      DigitalEnvelope de;
      de.startOpening(sellerDecryptKey, &openKey);
      unsigned int length = fp["pieceKey"]["data"]->length();
      char sealedKey[length];
      Convert::hexToBytes(
         fp["pieceKey"]["data"]->getString(), length, sealedKey, length);
      ByteBuffer bb;
      de.update(sealedKey, length, &bb, true);
      de.finish(&bb, true);
      assertNoException();

      // Note: now the key is ready to decrypt the piece
      string readyKey = Convert::bytesToHex(bb.data(), bb.length());
      fp["pieceKey"]["data"] = readyKey.c_str();

      // initialize the buyer BFP
      tr.test("buyer bfp initialization");
      {
         bfp->initializePeerBuy(c);
      }
      tr.passIfNoException();

      // decode the file using the bfp for peer buy and write it to disk
      tr.test("buyer bfp decrypt");
      {
         FileOutputStream fos(decryptedFile);

         // prepare file for peer buy decryption
         fi["path"] = decryptedFilename;
         bfp->preparePeerBuyFile(fi);
         assertNoException();

         // start the reading process in the BFP
         bfp->startReading(fp);
         assertNoException();

         // write the data to disk
         char b[2048];
         int numBytes;
         bool err = false;
         while((numBytes = bfp->read(b, 2048)) > 0 && !err)
         {
            err = !fos.write(b, numBytes);
         }

         fos.close();

         // save the buyer file ID and check it against the seller file ID
         // after the file has been decrypted.
         fi["id"] = "";
         bfp->setFileInfoId(fi);
         buyerFileInfoId = fi["id"]->getString();
      }
      tr.passIfNoException();

      // buyer file ID test after decode must match seller file ID
      tr.test("seller and buyer file IDs match");
      {
         // check to make sure the file ID isn't a null string and that
         // the file IDs match, which should only happen when the data in
         // both files is identical.
         assert(strcmp(sellerFileInfoId.c_str(), "") != 0);
         assertStrCmp(sellerFileInfoId.c_str(), buyerFileInfoId.c_str());
      }
      tr.passIfNoException();

      // free bfp
      ibm->freeBfp(bfp);
   }
   tr.ungroup();
}

void runBfpMultiPieceTest(
   Node& node, TestRunner& tr, bitmunk::test::Tester& tester,
   const char* extension)
{
   string msg = "PeerBuy multiple pieces ";
   msg.append(extension);
   tr.group(msg.c_str());

   string sellerFileInfoId = "sellerFileInfoId";
   string buyerFileInfoId = "buyerFileInfoId";

   // setup the input filename
   char inputFilename[100];
   snprintf(inputFilename, 100, "/tmp/bmtestfile.%s", extension);
   File inputFile(inputFilename);

   // setup the decrypted filename
   char decryptedFilename[100];
   snprintf(
      decryptedFilename, 100, "/tmp/bmtestfile-mp-decrypted.%s", extension);
   File decryptedFile(decryptedFilename);

   // populate the media
   Media media;
   media["id"] = 2;
   DatabaseManager* dm = DatabaseManager::getInstance(&node);
   assert(dm != NULL);
   MediaManager* mm = dm->getMediaManager();
   mm->populateMedia(media, NULL);

   // create the contract section
   ContractSection cs;
   cs["seller"]["userId"] = TEST_USER_ID;

   // create the contract
   Contract c;
   c["version"] = "3.0";
   c["id"] = 1;
   c["media"] = media;
   c["buyer"]["userId"] = TEST_USER_ID;
   c["sections"][TEST_USER_ID_STR][0] = cs;

   if(!inputFile->exists())
   {
      string temp = inputFilename;
      temp.append(" does not exist, not running test!");
      tr.warning(temp.c_str());
   }
   else
   {
      AsymmetricKeyFactory akf;
      PrivateKeyRef sellerDecryptKey;
      PublicKeyRef sellerEncryptKey;
      akf.createKeyPair("RSA", sellerDecryptKey, sellerEncryptKey);

      // get the seller BfpModule interface
      IBfpModule* ibm = getIBfpInstance(&node);
      assertNoException();

      // create bfp version 1
      Bfp* bfp = ibm->createBfp(1);
      assertNoException();

      // generate the contract section hash
      string csHash = Tools::getContractSectionHash(cs);

      // initialize the seller peer sell module
      tr.test("seller bfp initialization");
      {
         string pem = akf.writePublicKeyToPem(sellerEncryptKey);
         bfp->initializePeerSell(csHash.c_str(), pem.c_str());
      }
      tr.passIfNoException();

      // prepare the seller bfp for multi-part peer selling
      FileInfo fi;
      fi["path"] = inputFilename;
      fi["extension"] = extension;

      // we call setFileInfoId() so that the file size is set correctly
      bfp->setFileInfoId(fi);
      assertNoException();
      sellerFileInfoId = fi["id"]->getString();

      tr.test("seller bfp prepare");
      {
         uint64_t startTime = Timer::startTiming();
         bfp->preparePeerSellFile(fi);
         printf("[time:%gs]\n", Timer::getSeconds(startTime));
      }
      tr.passIfNoException();

      // setup the file piece information

      // get piece size
      unsigned int pieceSize = 204800;
      if(strcmp(extension, "mp3") == 0)
      {
         // 250k
         pieceSize = 204800;
      }
      else
      {
         // 2 megs
         pieceSize = 2048000;
      }

      // get piece count
      uint64_t fileSize = fi["contentSize"]->getUInt64();
      int pieces = fileSize / pieceSize + ((fileSize % pieceSize != 0) ? 1 : 0);

      // create a list of file pieces
      DynamicObject fpList;
      for(int i = 0; i < pieces; i++)
      {
         char path[50];
         sprintf(path, "/tmp/bmtestfile-mp-%s-%04i.fp", extension, i);
         FilePiece& fp = fpList->append();
         fp["path"] = path;
         fp["index"] = i;
         fp["size"] = pieceSize;
      }

      tr.test("seller bfp encrypt pieces");
      {
         // prepare file buffer
         char b[2048];
         int numBytes;
         bool err = false;

         // iterate over pieces and output all of them
         DynamicObjectIterator i = fpList.getIterator();
         while(i->hasNext())
         {
            FilePiece& fp = i->next();

            // start the reading process in the BFP
            bfp->startReading(fp);
            assertNoException();

            // write file piece out
            File encryptedFilePiece(fp["path"]->getString());
            FileOutputStream fos(encryptedFilePiece);
            while((numBytes = bfp->read(b, 2048)) > 0 && !err)
            {
               err = !fos.write(b, numBytes);
            }

            fos.close();
            assertNoException();
         }
      }
      tr.passIfNoException();

      // buyer BFP decode tests

      tr.test("buyer bfp unseal keys");
      {
         // Note: key unsealing will be done by the SVA
         // iterate over pieces and unseal each key
         DynamicObjectIterator i = fpList.getIterator();
         while(i->hasNext())
         {
            FilePiece& fp = i->next();

            // Retrieve the openKey for unsealing the pieceKey
            SymmetricKey openKey;
            Tools::populateSymmetricKey(&openKey, fp["openKey"]);

            // unseal the decyption key
            DigitalEnvelope de;
            de.startOpening(sellerDecryptKey, &openKey);
            unsigned int length = fp["pieceKey"]["data"]->length();
            char sealedKey[length];
            Convert::hexToBytes(
               fp["pieceKey"]["data"]->getString(), length, sealedKey, length);
            ByteBuffer bb;
            de.update(sealedKey, length, &bb, true);
            de.finish(&bb, true);
            assertNoException();

            // Note: now the key is ready to decrypt the piece
            string readyKey = Convert::bytesToHex(bb.data(), bb.length());
            fp["pieceKey"]["data"] = readyKey.c_str();
         }
      }
      tr.passIfNoException();

      // initialize the buyer BFP
      tr.test("buyer bfp initialization");
      {
         bfp->initializePeerBuy(c);
      }
      tr.passIfNoException();

      // decode the file using the bfp for peer buy and write it to disk
      tr.test("buyer bfp decrypt pieces");
      {
         // when we are in purchase mode, we'll probably want to use some
         // kind of stream that has the null-byted file mmap'ed (where
         // we'll pass in a pointer to a page to the bfp->read() method)

         // open file for writing
         fi["path"] = decryptedFilename;
         FileOutputStream fos(decryptedFile);

         // prepare to write to file
         char b[2048];
         int numBytes;
         bool err = false;

         // prepare file for peer buy decryption
         bfp->preparePeerBuyFile(fi);

         // iterate over piece files and sew them together
         DynamicObjectIterator i = fpList.getIterator();
         while(i->hasNext())
         {
            FilePiece& fp = i->next();

            // start reading for a buyer
            printf("Processing piece %s...\n", fp["path"]->getString());

            // start the reading process in the BFP
            bfp->startReading(fp);
            assertNoException();

            // read piece
            while((numBytes = bfp->read(b, 2048)) > 0 && !err)
            {
               err = !fos.write(b, numBytes);
            }
            assertNoException();
         }
         fos.close();

         // save the buyer file ID and check it after the file is decrypted
         fi["id"] = "";
         bfp->setFileInfoId(fi);
         buyerFileInfoId = fi["id"]->getString();
      }
      tr.passIfNoException();

      // buyer file ID test after decode must match seller file ID
      tr.test("seller and buyer file IDs match");
      {
         // check to make sure the file ID isn't a null string and that
         // the file IDs match, which should only happen when the data in
         // both files is identical.
         assert(strcmp(sellerFileInfoId.c_str(), "") != 0);
         assertStrCmp(sellerFileInfoId.c_str(), buyerFileInfoId.c_str());
      }
      tr.passIfNoException();

      // free bfp
      ibm->freeBfp(bfp);
   }
   tr.ungroup();
}

class BmBfpTester : public bitmunk::test::Tester
{
public:
   BmBfpTester()
   {
      setName("Bfp Tester");
   }

   /**
    * Run automatic unit tests.
    */
   virtual int runAutomaticTests(TestRunner& tr)
   {
      // Note: always print this out to avoid confusion
      File file("/tmp/devuser.profile");
      cout << "You must copy devuser.profile from pki to " <<
         "/tmp/ to run this. If you're seeing security breaches, " <<
         "your copy may be out of date." << endl;
      if(!file->exists())
      {
         exit(1);
      }
      // FIXME:
#if 0
      // create a client node for communicating
      Node node;
      {
         success = setupNode(&node);
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
      assertNoException();

      // run tests
      //runBfpFactoryTest(node, tr, *this);
      //runBfpSetFileInfoIdTest(node, tr, *this, "mp3");
      //runBfpSetFileInfoIdTest(node, tr, *this, "avi");
      //runBfpWebBuyTest(node, tr, *this, "mp3");
      //runBfpWebBuyTest(node, tr, *this, "avi");
      //runBfpSinglePieceTest(node, tr, *this, "mp3");
      //runBfpSinglePieceTest(node, tr, *this, "avi");
      runBfpMultiPieceTest(node, tr, *this, "mp3");
      //runBfpMultiPieceTest(node, tr, *this, "avi");

      // logout of client node
      node.logout(0);

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
BM_TEST_MAIN(BmBfpTester)
#endif
