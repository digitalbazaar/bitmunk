/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/eventreactor/DownloadStateEventReactor.h"

#include "bitmunk/eventreactor/ErDownloadStateModule.h"
#include "bitmunk/purchase/IPurchaseModule.h"
#include "monarch/event/ObserverDelegate.h"

using namespace std;
using namespace monarch::event;
using namespace monarch::modest;
using namespace monarch::net;
using namespace monarch::rt;
using namespace bitmunk::common;
using namespace bitmunk::protocol;
using namespace bitmunk::purchase;
using namespace bitmunk::node;
using namespace bitmunk::eventreactor;

#define EVENT_DOWNLOAD_STATE     "bitmunk.purchase.DownloadState"
#define PROGRESS_POLL_INTERVAL   1000   // 1 second

DownloadStateEventReactor::DownloadStateEventReactor(Node* node) :
   mNode(node),
   mObserverContainer(NULL)
{
}

DownloadStateEventReactor::~DownloadStateEventReactor()
{
}

void DownloadStateEventReactor::setObserverContainer(ObserverContainer* oc)
{
   mObserverContainer = oc;
}

ObserverContainer* DownloadStateEventReactor::getObserverContainer()
{
   return mObserverContainer;
}

void DownloadStateEventReactor::addUser(UserId userId)
{
   MO_CAT_DEBUG(BM_EVENTREACTOR_DS_CAT,
      "Adding download state event reactor for user %" PRIu64 "...", userId);

   // add events in reverse order of occurrence to prevent half-baked automation

   // handle directiveCreated
   {
      ObserverRef ob(new ObserverDelegate<DownloadStateEventReactor>(
         this, &DownloadStateEventReactor::directiveCreated));

      // create event filter
      EventFilter ef;
      ef["details"]["userId"] = userId;
      ef["details"]["directive"]["type"] = "peerbuy";

      // register to receive events
      mNode->getEventController()->registerObserver(
         &(*ob), "bitmunk.system.Directive.created", &ef);

      // store observer
      mObserverContainer->addObserver(userId, ob);
   }

   // handle download state events
   {
      // create event filter
      EventFilter ef;
      ef["details"]["userId"] = userId;

      // downloadStateCreated
      {
         ObserverRef ob(new ObserverDelegate<DownloadStateEventReactor>(
            this, &DownloadStateEventReactor::downloadStateCreated));
         mNode->getEventController()->registerObserver(
            &(*ob), EVENT_DOWNLOAD_STATE ".created", &ef);
         mObserverContainer->addObserver(userId, ob);
      }

      // downloadStateInitialized
      {
         ObserverRef ob(new ObserverDelegate<DownloadStateEventReactor>(
            this, &DownloadStateEventReactor::downloadStateInitialized));
         mNode->getEventController()->registerObserver(
            &(*ob), EVENT_DOWNLOAD_STATE ".initialized", &ef);
         mObserverContainer->addObserver(userId, ob);
      }

      // pieceStarted
      {
         ObserverRef ob(new ObserverDelegate<DownloadStateEventReactor>(
            this, &DownloadStateEventReactor::pieceStarted));
         mNode->getEventController()->registerObserver(
            &(*ob), EVENT_DOWNLOAD_STATE ".pieceStarted", &ef);
         mObserverContainer->addObserver(userId, ob);
      }

      // downloadStopped
      {
         ObserverRef ob(new ObserverDelegate<DownloadStateEventReactor>(
            this, &DownloadStateEventReactor::downloadStopped));
         mNode->getEventController()->registerObserver(
            &(*ob), EVENT_DOWNLOAD_STATE ".downloadStopped", &ef);
         mObserverContainer->addObserver(userId, ob);
      }

      // downloadCompleted
      {
         ObserverRef ob(new ObserverDelegate<DownloadStateEventReactor>(
            this, &DownloadStateEventReactor::downloadCompleted));
         mNode->getEventController()->registerObserver(
            &(*ob), EVENT_DOWNLOAD_STATE ".downloadCompleted", &ef);
         mObserverContainer->addObserver(userId, ob);
      }

      // purchaseCompleted
      {
         ObserverRef ob(new ObserverDelegate<DownloadStateEventReactor>(
            this, &DownloadStateEventReactor::purchaseCompleted));
         mNode->getEventController()->registerObserver(
            &(*ob), EVENT_DOWNLOAD_STATE ".purchaseCompleted", &ef);
         mObserverContainer->addObserver(userId, ob);
      }

      // reprocessRequired
      {
         ObserverRef ob(new ObserverDelegate<DownloadStateEventReactor>(
            this, &DownloadStateEventReactor::reprocessRequired));
         mNode->getEventController()->registerObserver(
            &(*ob), EVENT_DOWNLOAD_STATE ".reprocessRequired", &ef);
         mObserverContainer->addObserver(userId, ob);
      }
   }

   // do a reprocess event for all of the user's download states
   Event e;
   e["type"] = EVENT_DOWNLOAD_STATE ".reprocessRequired";
   e["details"]["userId"] = userId;
   e["details"]["all"] = true;
   mNode->getEventController()->schedule(e);
}

void DownloadStateEventReactor::removeUser(UserId userId)
{
   // nothing to do yet
}

void DownloadStateEventReactor::directiveCreated(Event& e)
{
   UserId userId = BM_USER_ID(e["details"]["userId"]);
   const char* directiveId = e["details"]["directiveId"]->getString();

   MO_CAT_DEBUG(BM_EVENTREACTOR_DS_CAT,
      "Event reactor handling 'directiveCreated' event for user %" PRIu64 "...",
      userId);

   // process the directive if it is of type "peerbuy":
   DynamicObject& directive = e["details"]["directive"];
   if(strcmp(directive["type"]->getString(), "peerbuy") == 0)
   {
      Messenger* messenger = mNode->getMessenger();

      Url url;
      url.format("%s/api/3.0/system/directives/process/%s?nodeuser=%" PRIu64,
         messenger->getSelfUrl(true).c_str(), directiveId, userId);

      DynamicObject in;
      if(!messenger->post(&url, NULL, &in, userId))
      {
         // schedule exception event
         Event e2;
         e2["type"] = "bitmunk.eventreactor.EventReactor.exception";
         e2["details"]["userId"] = userId;
         e2["details"]["exception"] = Exception::getAsDynamicObject();
         mNode->getEventController()->schedule(e2);
      }
   }
}

void DownloadStateEventReactor::downloadStateCreated(Event& e)
{
   // get user ID
   UserId userId = BM_USER_ID(e["details"]["userId"]);
   DownloadStateId dsId = e["details"]["downloadStateId"]->getUInt64();

   // log activity
   MO_CAT_DEBUG(BM_EVENTREACTOR_DS_CAT,
      "Event reactor handling ds 'created' event for user %" PRIu64 "...",
      userId);

   // initialize download state
   postDownloadStateId(userId, dsId, "initialize");
}

void DownloadStateEventReactor::downloadStateInitialized(Event& e)
{
   // get user ID
   UserId userId = BM_USER_ID(e["details"]["userId"]);
   DownloadStateId dsId = e["details"]["downloadStateId"]->getUInt64();

   // log activity
   MO_CAT_DEBUG(BM_EVENTREACTOR_DS_CAT,
      "Event reactor handling ds 'initialized' event for user %" PRIu64 "...",
      userId);

   // acquire license
   postDownloadStateId(userId, dsId, "license");
}

void DownloadStateEventReactor::pieceStarted(Event& e)
{
   // schedule progress poll events with event daemon
   Event e2;
   e2["type"] = EVENT_DOWNLOAD_STATE ".progressPoll";
   e2["details"]["userId"] = e["details"]["userId"].clone();
   e2["details"]["downloadStateId"] =
      e["details"]["downloadStateId"].clone();

   // include unique event identifier
   e2["details"]["eventReactor"] = true;
   mNode->getEventDaemon()->add(e2, PROGRESS_POLL_INTERVAL, -1, 1);
}

void DownloadStateEventReactor::downloadStopped(Event& e)
{
   // remove progress poll events from event daemon
   Event e2;
   e2["type"] = EVENT_DOWNLOAD_STATE ".progressPoll";
   e2["details"]["userId"] = e["details"]["userId"].clone();
   e2["details"]["downloadStateId"] =
      e["details"]["downloadStateId"].clone();

   // include unique event identifier
   e2["details"]["eventReactor"] = true;
   mNode->getEventDaemon()->remove(e2);
}

void DownloadStateEventReactor::downloadCompleted(Event& e)
{
   // get user ID, download state ID
   UserId userId = BM_USER_ID(e["details"]["userId"]);
   DownloadStateId dsId = e["details"]["downloadStateId"]->getUInt64();

   // log activity
   MO_CAT_DEBUG(BM_EVENTREACTOR_DS_CAT,
      "Event reactor handling ds 'downloadCompleted' event for user %" PRIu64
      "...",
      userId);

   // start purchase
   postDownloadStateId(userId, dsId, "purchase");
}

void DownloadStateEventReactor::purchaseCompleted(Event& e)
{
   // get user ID
   UserId userId = BM_USER_ID(e["details"]["userId"]);
   DownloadStateId dsId = e["details"]["downloadStateId"]->getUInt64();

   // log activity
   MO_CAT_DEBUG(BM_EVENTREACTOR_DS_CAT,
      "Event reactor handling ds 'purchaseComplete' event for user %" PRIu64
      "...",
      userId);

   // start file assembly
   postDownloadStateId(userId, dsId, "assemble");
}

void DownloadStateEventReactor::reprocessRequired(Event& e)
{
   // get user ID
   UserId userId = BM_USER_ID(e["details"]["userId"]);

   // ensure operation is done as the user to protect against logout
   Operation op = mNode->currentOperation();
   if(mNode->addUserOperation(userId, op, NULL))
   {
      // lock to prevent double-reprocessing download states
      // Note: Reprocessing the download state isn't fatal, it just may fail
      // silently or be annoying by re-assembling already assembled files, etc.
      mReprocessLock.lock();
      {
         // log activity
         MO_CAT_DEBUG(BM_EVENTREACTOR_DS_CAT,
            "Event reactor handling ds 'reprocessRequired' "
            "event for user %" PRIu64 "...", userId);

         // get messenger
         Messenger* messenger = mNode->getMessenger();

         // process all queued directives
         {
            Url url;
            url.format("%s/api/3.0/system/directives?nodeuser=%" PRIu64,
               messenger->getSelfUrl(true).c_str(), userId);

            DynamicObject directives;
            if(messenger->get(&url, directives, userId))
            {
               DynamicObjectIterator i = directives.getIterator();
               while(i->hasNext())
               {
                  DynamicObject& directive = i->next();

                  // process the directive if it is of type "peerbuy":
                  if(strcmp(directive["type"]->getString(), "peerbuy") == 0)
                  {
                     url.format(
                        "%s/api/3.0/system/directives/process/%s?nodeuser=%"
                        PRIu64,
                        messenger->getSelfUrl(true).c_str(),
                        directive["id"]->getString(), userId);

                     DynamicObject in;
                     if(!messenger->post(&url, NULL, &in, userId))
                     {
                        // schedule exception event
                        Event e2;
                        e2["type"] =
                           "bitmunk.eventreactor.EventReactor.exception";
                        e2["details"]["exception"] =
                           Exception::getAsDynamicObject();
                        mNode->getEventController()->schedule(e2);
                     }
                  }
               }
            }
         }

         // create a list of download states to reprocess
         DownloadStateList list;
         list->setType(Array);

         if(e["details"]->hasMember("all") && e["details"]["all"]->getBoolean())
         {
            // get all non-processing download states
            Url url;
            url.format("%s/api/3.0/purchase/contracts/downloadstates"
               "?nodeuser=%" PRIu64 "&incomplete=true&processing=false",
               messenger->getSelfUrl(true).c_str(), userId);
            DynamicObject in;
            if(messenger->get(&url, in, userId))
            {
               // append states to list
               list.merge(in["downloadStates"], true);
            }
            else
            {
               // schedule exception event
               Event e2;
               e2["type"] = "bitmunk.eventreactor.EventReactor.exception";
               e2["details"]["exception"] =
                  Exception::getAsDynamicObject();
               mNode->getEventController()->schedule(e2);
            }
         }
         else if(e["details"]->hasMember("downloadStateId"))
         {
            // reprocess single download state
            DownloadStateId dsId = e["details"]["downloadStateId"]->getUInt64();
            IPurchaseModule* ipm = dynamic_cast<IPurchaseModule*>(
               mNode->getKernel()->getModuleApi(
                  "bitmunk.purchase.Purchase"));
            DownloadState ds;
            ds["id"] = dsId;
            BM_ID_SET(ds["userId"], userId);
            if(ipm->populateDownloadState(ds))
            {
               // append state to list
               list->append(ds);
            }
            else
            {
               // schedule exception event
               Event e2;
               e2["type"] = "bitmunk.eventreactor.EventReactor.exception";
               e2["details"]["exception"] =
                  Exception::getAsDynamicObject();
               mNode->getEventController()->schedule(e2);
            }
         }

         // reprocess all states in the list
         DynamicObjectIterator i = list.getIterator();
         while(i->hasNext())
         {
            DownloadState& ds = i->next();
            DownloadStateId dsId = ds["id"]->getUInt64();

            if(ds["downloadStarted"]->getBoolean())
            {
               // do purchase only if no pieces remain
               if(ds["remainingPieces"]->getUInt32() == 0)
               {
                  if(!ds["licensePurchased"]->getBoolean() ||
                     !ds["dataPurchased"]->getBoolean())
                  {
                     // do purchase
                     postDownloadStateId(userId, dsId, "purchase");
                  }
                  else if(!ds["filesAssembled"]->getBoolean())
                  {
                     // do file assembly
                     postDownloadStateId(userId, dsId, "assemble");
                  }
               }
               else if(!ds["downloadPaused"]->getBoolean())
               {
                  // restart download
                  postDownloadStateId(userId, dsId, "download");
               }
            }
            else if(!ds["initialized"]->getBoolean())
            {
               // initialize download state
               postDownloadStateId(userId, dsId, "initialize");
            }
            else if(!ds["licenseAcquired"]->getBoolean())
            {
               // acquire license
               postDownloadStateId(userId, dsId, "license");
            }
         }
      }
      mReprocessLock.unlock();
   }
}

void DownloadStateEventReactor::postDownloadStateId(
   UserId userId, DownloadStateId dsId, const char* action)
{
   Messenger* messenger = mNode->getMessenger();

   Url url;
   url.format("%s/api/3.0/purchase/contracts/%s?nodeuser=%" PRIu64,
      messenger->getSelfUrl(true).c_str(), action, userId);

   DynamicObject out;
   out["downloadStateId"] = dsId;
   if(!messenger->post(&url, &out, NULL, userId))
   {
      // schedule exception event
      Event e2;
      e2["type"] = "bitmunk.eventreactor.EventReactor.exception";
      BM_ID_SET(e2["details"]["userId"], userId);
      e2["details"]["downloadStateId"] = dsId;
      e2["details"]["exception"] = Exception::getAsDynamicObject();
      mNode->getEventController()->schedule(e2);
   }
}
