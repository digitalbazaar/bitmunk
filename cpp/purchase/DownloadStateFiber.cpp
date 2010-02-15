/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/purchase/DownloadStateFiber.h"

#include "bitmunk/common/Tools.h"
#include "bitmunk/purchase/IPurchaseModule.h"
#include "bitmunk/purchase/PurchaseModule.h"
#include "monarch/data/json/JsonWriter.h"

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::purchase;
using namespace bitmunk::node;
using namespace monarch::data::json;
using namespace monarch::event;
using namespace monarch::net;
using namespace monarch::rt;
using namespace monarch::util;

DownloadStateFiber::DownloadStateFiber(
   Node* node, const char* name, DynamicObject* exitData) :
   mBitmunkNode(node),
   mExitData(exitData),
   mPurchaseDatabase(NULL),
   mDownloadState(NULL)
{
   mName = strdup(name);
   mPurchaseDatabase = PurchaseDatabase::getInstance(node);
}

DownloadStateFiber::~DownloadStateFiber()
{
   free(mName);
}

void DownloadStateFiber::setDownloadState(DownloadState& ds)
{
   // store download state
   mDownloadState = ds;

   // set fiber exit data if appropriate
   UserId userId = BM_USER_ID(ds["userId"]);
   if(mExitData != NULL)
   {
      *mExitData = DynamicObject();
      (*mExitData)["downloadStateId"] = ds["id"]->getUInt64();
      BM_ID_SET((*mExitData)["userId"], userId);
      (*mExitData)["name"] = mName;
   }

   // cast *must* work because we are inside the purchase module
   IPurchaseModule* ipm = dynamic_cast<IPurchaseModule*>(
      mBitmunkNode->getModuleApi("bitmunk.purchase.Purchase"));
   mTotalDownloadRate = ipm->getTotalDownloadRate();
   mDownloadThrottler = ipm->getDownloadThrottler(userId);
}

DownloadState& DownloadStateFiber::getDownloadState()
{
   return mDownloadState;
}

void DownloadStateFiber::logDownloadStateMessage(const char* msg)
{
   MO_CAT_INFO(BM_PURCHASE_CAT,
      "%s: UserId %" PRIu64 ", DownloadState %" PRIu64 ": %s",
      mName,
      BM_USER_ID(mDownloadState["userId"]),
      mDownloadState["id"]->getUInt64(),
      msg);
}

void DownloadStateFiber::logException(ExceptionRef ex)
{
   DynamicObject d = Exception::convertToDynamicObject(ex);

   MO_CAT_ERROR(BM_PURCHASE_CAT,
      "%s: UserId %" PRIu64 ", DownloadState %" PRIu64 ": "
      "exception occurred, %s",
      mName,
      BM_USER_ID(mDownloadState["userId"]),
      mDownloadState["id"]->getUInt64(),
      JsonWriter::writeToString(d, false, false).c_str());
}

void DownloadStateFiber::sendDownloadStateEvent(Event& e)
{
   MO_CAT_DEBUG(BM_PURCHASE_CAT,
      "%s: UserId %" PRIu64 ", DownloadState %" PRIu64 ": "
      "scheduling event '%s'",
      mName,
      BM_USER_ID(mDownloadState["userId"]),
      mDownloadState["id"]->getUInt64(),
      e["type"]->getString());

   DownloadStateId dsId = mDownloadState["id"]->getUInt64();
   e["details"]["downloadStateId"] = dsId;
   BM_ID_SET(e["details"]["userId"], BM_USER_ID(mDownloadState["userId"]));
   mBitmunkNode->getEventController()->schedule(e);
}

void DownloadStateFiber::sendErrorEvent()
{
   ExceptionRef ex = Exception::get();
   if(ex.isNull())
   {
      ex = new Exception(
         "No exception was set when one should have been."
         "bitmunk.purchase.InvalidException");
   }

   logException(ex);

   // send error event
   Event e;
   e["type"] = "bitmunk.purchase.DownloadState.exception";
   e["details"]["exception"] = Exception::convertToDynamicObject(ex);
   sendDownloadStateEvent(e);
}
