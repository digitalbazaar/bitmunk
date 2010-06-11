/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/purchase/Negotiator.h"

#include "bitmunk/common/NegotiateInterface.h"
#include "bitmunk/common/Signer.h"
#include "bitmunk/common/Tools.h"
#include "bitmunk/purchase/PurchaseDatabase.h"
#include "bitmunk/purchase/PurchaseModule.h"
#include "bitmunk/purchase/SellerPicker.h"
#include "monarch/data/json/JsonWriter.h"
#include "monarch/rt/DynamicObjectIterator.h"
#include "monarch/rt/RunnableDelegate.h"

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::purchase;
using namespace bitmunk::node;
using namespace monarch::config;
using namespace monarch::crypto;
using namespace monarch::data::json;
using namespace monarch::event;
using namespace monarch::fiber;
using namespace monarch::modest;
using namespace monarch::rt;
using namespace monarch::util;

Negotiator::Negotiator(Node* node, FiberId parentId) :
   NodeFiber(node, parentId),
   DownloadStateFiber(node, "Negotiator", &mFiberExitData),
   mSellerData(NULL),
   mMustFindSeller(false)
{
}

Negotiator::~Negotiator()
{
}

void Negotiator::negotiateSection()
{
   logDownloadStateMessage("negotiating contract section...");

   bool error = false;
   bool badSeller = false;

   // use negotiation interface
   NegotiateInterface* ni = dynamic_cast<NegotiateInterface*>(
      getNode()->getModuleApiByType("bitmunk.negotiate"));
   if(ni == NULL)
   {
      ExceptionRef e = new Exception(
         "No negotiate module.",
         "bitmunk.purchase.MissingNegotiateModule");
      Exception::set(e);
      error = true;
   }
   else
   {
      // get buyer's login data
      User user;
      UserId userId = BM_USER_ID(mDownloadState["userId"]);
      ProfileRef profile(NULL);
      error = !mNode->getLoginData(userId, &user, &profile);
      if(!error)
      {
         // set buyer information in contract section
         ContractSection cs = mSellerData["section"];
         BM_ID_SET(cs["buyer"]["userId"], userId);
         cs["buyer"]["username"] = user["username"];
         cs["buyer"]["profileId"] = profile->getId();

         // negotiate contract section with negotiation interface and then
         // with seller
         error =
            !(ni->negotiateContractSection(
               userId, mDownloadState["contract"], cs, false) &&
            negotiateWithSeller(userId, profile, cs, badSeller));
      }
   }

   // create message regarding operation completion
   DynamicObject msg;
   msg["badSeller"] = badSeller;
   if(!badSeller)
   {
      msg["contractNegotiated"] = !error;
   }

   if(badSeller || error)
   {
      DynamicObject d = Exception::getAsDynamicObject();
      msg["exception"] = d;
      MO_CAT_ERROR(BM_PURCHASE_CAT,
         "Negotiator exception: %s", JsonWriter::writeToString(d).c_str());
   }

   // send message to self
   messageSelf(msg);
}

void Negotiator::setMustFindSeller(bool must)
{
   mMustFindSeller = must;
}

void Negotiator::processMessages()
{
   logDownloadStateMessage("running...");

   /* A stack of FileProgresses that could use another seller. This stack is
      ordered (ascending) by existing seller count for file progresses that are
      not yet complete. */
   std::list<FileProgress> fpStack;

   // pick the files that could use another seller, filling the stack
   pickFiles(fpStack);

   // pop off each entry in the stack trying to find a seller for it
   bool error = false;
   bool sellerFound = false;
   while(!error && !sellerFound && !fpStack.empty())
   {
      // pop off the next file progress and get its associated seller pool
      SellerPool sp = fpStack.front()["sellerPool"];

      // pick seller if seller pool is not empty
      if(sp["sellerDataSet"]["resources"]->length() > 0)
      {
         while(!error && !sellerFound && pickSeller(sp))
         {
            // seller picked, now negotiate
            switch(negotiate())
            {
               case 0:
                  // yield, then pick another seller
                  yield();
                  continue;
               case 1:
                  // insert contract section into database
                  sellerFound = insertSection();
                  error = !sellerFound;
                  break;
               default:
                  // error
                  error = true;
                  break;
            }
         }
      }

      fpStack.pop_front();

      yield();
   }

   if(sellerFound)
   {
      // send complete message to parent
      DynamicObject msg;
      msg["negotiationComplete"] = true;
      messageParent(msg);
   }
   else
   {
      if(!mMustFindSeller)
      {
         logDownloadStateMessage(
            "no sellers found that match the buyer's preferences, but "
            "adding a new seller was optional");
      }
      else
      {
         logDownloadStateMessage(
            "no sellers found that match the buyer's preferences, raising "
            "an exception...");
      }

      // error
      ExceptionRef e = new Exception(
         "There are no sellers available that match the buyer's "
         "purchase preferences. A higher maximum price may be required to "
         "continue because a seller with a low enough price has gone "
         "offline.",
         "bitmunk.purchase.DownloadState.NoSellersAvailable");
      error ? Exception::push(e) : Exception::set(e);

      // send error event and message to parent
      DynamicObject msg;
      msg["negotiationError"] = true;
      msg["exception"] = Exception::convertToDynamicObject(e);
      msg["must"] = mMustFindSeller;
      messageParent(msg);
   }

   logDownloadStateMessage("exited");
}

void Negotiator::pickFiles(std::list<FileProgress>& fpStack)
{
   // Note: This method assumes that there are files in the download state
   // that have unassigned pieces.

   // Note: To help ensure that seller counts are evenly distributed amongst
   // files, first fill an std::map that maps seller count to entries, then
   // collapse those entries together so they are in sorted order
   map<int, DynamicObject> autoSort;

   // populate autoSort list with file progresses that have unassigned pieces
   int sellerCount;
   FileProgressIterator fpi = mDownloadState["progress"].getIterator();
   while(fpi->hasNext())
   {
      FileProgress& fp = fpi->next();
      if(fp["unassigned"]->length() > 0)
      {
         // insert entry into auto-sorting array
         sellerCount = fp["sellerData"]->length();
         map<int, DynamicObject>::iterator i = autoSort.find(sellerCount);
         if(i == autoSort.end())
         {
            // add array entry because it doesn't exist yet
            DynamicObject array;
            array->append(fp);
            autoSort[sellerCount] = array;
         }
         else
         {
            // update existing array entry
            autoSort[sellerCount]->append(fp);
         }
      }

      yield();
   }

   // compress auto-sorted map=>arrays into a 1-dimenstional file progress
   // stack with the file progresses with the fewest assigned sellers on top
   // (given preference)
   for(map<int, DynamicObject>::iterator i = autoSort.begin();
       i != autoSort.end(); ++i)
   {
      fpi = i->second.getIterator();
      while(fpi->hasNext())
      {
         // append to file progress stack
         fpStack.push_back(fpi->next());
      }
   }
}

bool Negotiator::pickSeller(SellerPool& sp)
{
   bool rval = false;

   // get the file progress
   FileId fileId = BM_FILE_ID(sp["fileInfo"]["id"]);
   FileProgress& progress = mDownloadState["progress"][fileId];

   // get the maximum price allowed for a chosen seller
   BigDecimal budget = progress["budget"]->getString();
   budget.setPrecision(7, Down);

   // get the blacklist map
   DynamicObject& blacklist = mDownloadState["blacklist"];

   // create the list of sellers that can be chosen from the seller pool
   // excluding "bad" sellers, sellers already negotiated with, and sellers
   // that are over the allowable price
   string key;
   BigDecimal price;
   price.setPrecision(7, Down);
   SellerDataList list;
   list->setType(Array);
   sp["sellerDataSet"]["resources"]->setType(Array);
   SellerDataIterator i = sp["sellerDataSet"]["resources"].getIterator();
   while(i->hasNext())
   {
      SellerData& sd = i->next();
      key = Tools::createSellerServerKey(sd["seller"]);
      if(key.length() > 0 &&
         !blacklist->hasMember(key.c_str()) &&
         !progress["sellers"]->hasMember(key.c_str()))
      {
         price = sd["price"]->getString();
         if(price <= budget)
         {
            list->append(sd);
         }
      }

      yield();
   }

   // pick the seller
   SellerData pick(NULL);
   if(SellerPicker::pickSeller(mDownloadState["preferences"], list, pick))
   {
      // clone seller data and set up contract section
      mSellerData = pick.clone();
      ContractSection& cs = mSellerData["section"];
      cs["seller"] = mSellerData["seller"].clone();

      // get file info
      FileInfo fi = progress["fileInfo"].clone();

      // create ware
      Ware& ware = cs["ware"];
      string wareId = "bitmunk:file:";
      wareId.append(fi["mediaId"]->getString());
      wareId.push_back('-');
      wareId.append(fi["id"]->getString());
      BM_ID_SET(ware["id"], wareId.c_str());
      BM_ID_SET(ware["mediaId"], BM_MEDIA_ID(fi["mediaId"]));
      ware["fileInfos"]->append(fi);

      // seller picked
      rval = true;
   }
   else
   {
      // no seller picked
      mSellerData.setNull();
   }

   return rval;
}

int Negotiator::negotiate()
{
   int rval = -1;

   string key = Tools::createSellerServerKey(mSellerData["seller"]);
   MO_CAT_DEBUG(BM_PURCHASE_CAT,
      "Negotiator: attempting to negotiate with seller (%s), "
      "uid: %" PRIu64 ", dsid: %" PRIu64 "",
      key.c_str(),
      BM_USER_ID(mDownloadState["userId"]),
      mDownloadState["id"]->getUInt64());

   // run a new negotiate section operation
   RunnableRef r = new RunnableDelegate<Negotiator>(
      this, &Negotiator::negotiateSection);
   Operation op = r;
   getNode()->runOperation(op);

   // wait for badSeller OR contractNegotiated message
   const char* keys[] = {"badSeller", "contractNegotiated", NULL};
   DynamicObject msg = waitForMessages(keys, &op)[0];

   // if a bad seller was detected
   if(msg["badSeller"]->getBoolean())
   {
      // seller cannot be contacted, add to bad sellers list
      DynamicObject blacklistEntry;
      blacklistEntry["seller"] = mSellerData["seller"].clone();
      blacklistEntry["time"] = System::getCurrentMilliseconds();
      mDownloadState["blacklist"][key.c_str()] = blacklistEntry;

      MO_CAT_DEBUG(BM_PURCHASE_CAT,
         "Negotiator: seller (%s) temporarily blacklisted, "
         "uid: %" PRIu64 ", dsid: %" PRIu64 "",
         key.c_str(),
         BM_USER_ID(mDownloadState["userId"]),
         mDownloadState["id"]->getUInt64());

      // pick another seller
      rval = 0;
   }
   else if(msg["contractNegotiated"]->getBoolean())
   {
      logDownloadStateMessage("contract negotiated");
      rval = 1;
   }
   else
   {
      logDownloadStateMessage("failed to negotiate contract");

      // convert exception from dynamic object, send error event
      ExceptionRef e = Exception::convertToException(msg["exception"]);
      Exception::set(e);
      sendErrorEvent();
   }

   return rval;
}

bool Negotiator::negotiateWithSeller(
   UserId userId, ProfileRef& profile, ContractSection& cs, bool& blacklist)
{
   bool rval = false;

   // do not blacklist by default
   blacklist = false;

   // negotiate contract with seller's server
   monarch::net::Url url;
   url.format("%s/api/3.0/sales/contract/negotiate?nodeuser=%" PRIu64,
      cs["seller"]["url"]->getString(),
      BM_USER_ID(cs["seller"]["userId"]));
   Contract c = mDownloadState["contract"].clone();
   c["sections"][cs["seller"]["userId"]->getString()]->append(cs);
   if(!getNode()->getMessenger()->post(&url, &c, &c, userId))
   {
      // could not talk to seller, black list them
      blacklist = true;
   }
   else
   {
      // get new section
      ContractSection& cs2 =
         c["sections"][cs["seller"]["userId"]->getString()][0];

      // compare file IDs (always only 1 file per ware)
      FileInfo& fi1 = cs["ware"]["fileInfos"][0];
      FileInfo& fi2 = cs2["ware"]["fileInfos"][0];
      FileId fileId1 = BM_FILE_ID(fi1["id"]);
      FileId fileId2 = BM_FILE_ID(fi2["id"]);

      if(!BM_FILE_ID_EQUALS(fileId1, fileId2))
      {
         // file ID does not match
         ExceptionRef e = new Exception(
            "Negotiated contract section changed file ID.",
            "bitmunk.purchase.Negotiator.FileIdOutOfSync");
         Exception::set(e);
      }
      else if(cs["ware"]["mediaId"] != cs2["ware"]["mediaId"])
      {
         // media ID does not match
         ExceptionRef e = new Exception(
            "Negotiated contract section changed media ID.",
            "bitmunk.purchase.Negotiator.MediaIdOutOfSync");
         Exception::set(e);
      }
      else if(cs["ware"]["id"] != cs2["ware"]["id"])
      {
         // ware ID does not match
         ExceptionRef e = new Exception(
            "Negotiated contract section changed ware ID.",
            "bitmunk.purchase.Negotiator.WareIdOutOfSync");
         Exception::set(e);
      }
      else if(fi1["contentSize"] != fi2["contentSize"])
      {
         // file sizes do not match
         ExceptionRef e = new Exception(
            "Negotiated contract section changed file content size.",
            "bitmunk.purchase.Negotiator.FileInfoOutOfSync");
         Exception::set(e);
      }
      else if(fi2["size"] < fi1["contentSize"])
      {
         // file size too small
         ExceptionRef e = new Exception(
            "Negotiated contract section file size is smaller than "
            "content size.",
            "bitmunk.purchase.Negotiator.FileTooSmall");
         Exception::set(e);
      }
      else
      {
         // update section
         ContractSection requested = cs;
         cs = mSellerData["section"] = cs2;

         // verify section, sign it if acceptable
         rval = verifySection(c, cs, profile);
      }
   }

   if(!rval)
   {
      ExceptionRef e = Exception::get();
      logException(e);

      // FIXME: do we always want to blacklist on error?
      blacklist = true;
   }

   return rval;
}

bool Negotiator::verifySection(
   Contract& c, ContractSection& cs, ProfileRef& profile)
{
   bool rval = false;

   // ensure price is not over budget
   if(checkSectionPrice(c, cs))
   {
      // buyer's signature on the contract section indicates approval
      rval = Signer::signContractSection(cs, profile, false);
   }

   return rval;
}

bool Negotiator::checkSectionPrice(Contract& c, ContractSection& cs)
{
   bool rval = true;

   // get license amount
   BigDecimal licenseAmount =
      c["media"]["licenseAmount"]->getString();

   // resolve payee amounts, get total
   // (total will be for ware payees, does not include license)
   BigDecimal total;
   Tools::resolvePayeeAmounts(
      cs["ware"]["payees"], &total, &licenseAmount);

   // update seller data so the price is accurate next time the seller is
   // attempted to be chosen
   mSellerData["price"] = total.toString(true).c_str();

   // make sure total is under or at budget (always only 1 file per ware)
   FileId fileId = BM_FILE_ID(cs["ware"]["fileInfos"][0]["id"]);
   BigDecimal budget =
      mDownloadState["progress"][fileId]["budget"]->getString();
   if(total > budget)
   {
      ExceptionRef e = new Exception(
         "Seller's amount exceeds advertised amount and is "
         "over the calculated budget.",
         "bitmunk.purchase.Negotiator.OverBudget");
      Exception::set(e);
      rval = false;

      MO_CAT_DEBUG(BM_PURCHASE_CAT,
         "Negotiator: seller's (%" PRIu64 ":%u) price too high: %s > %s, "
         "uid: %" PRIu64 ", dsid: %" PRIu64,
         BM_USER_ID(mSellerData["seller"]["userId"]),
         BM_SERVER_ID(mSellerData["seller"]["serverId"]),
         total.toString(true).c_str(),
         budget.toString(true).c_str(),
         BM_USER_ID(mDownloadState["userId"]),
         mDownloadState["id"]->getUInt64());
   }
   else
   {
      MO_CAT_DEBUG(BM_PURCHASE_CAT,
         "Negotiator: seller's (%" PRIu64 ":%u) negotiated price: %s, "
         "uid: %" PRIu64 ", dsid: %" PRIu64,
         BM_USER_ID(mSellerData["seller"]["userId"]),
         BM_SERVER_ID(mSellerData["seller"]["serverId"]),
         total.toString(true).c_str(),
         BM_USER_ID(mDownloadState["userId"]),
         mDownloadState["id"]->getUInt64());
   }

   return rval;
}

bool Negotiator::insertSection()
{
   bool rval;

   logDownloadStateMessage(
      "inserting negotiated contract section into database...");

   // update file progress
   ContractSection& cs = mSellerData["section"];
   const char* csHash = cs["hash"]->getString();
   string key = Tools::createSellerServerKey(mSellerData["seller"]);
   FileInfo& fi = cs["ware"]["fileInfos"][0];
   FileId fileId = BM_FILE_ID(fi["id"]);
   FileProgress& progress = mDownloadState["progress"][fileId];
   progress["sellerData"][csHash] = mSellerData;
   progress["sellers"][key.c_str()] = mSellerData["seller"];

   // insert seller data into database
   PurchaseDatabase* pd = PurchaseDatabase::getInstance(getNode());
   rval = pd->insertSellerData(mDownloadState, fileId, mSellerData);

   // send event
   if(!rval)
   {
      sendErrorEvent();
   }
   else
   {
      // send success event
      Event e;
      e["type"] = "bitmunk.purchase.Negotiator.negotiationComplete";
      sendDownloadStateEvent(e);
   }

   return rval;
}
