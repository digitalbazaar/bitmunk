/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/system/EventService.h"

#include "bitmunk/node/BtpActionDelegate.h"
#include "bitmunk/node/RestResourceHandler.h"
#include "bitmunk/system/SystemModule.h"
#include "monarch/event/ObserverDelegate.h"
#include "monarch/data/json/JsonWriter.h"
#include "monarch/util/Random.h"

#include <algorithm>

using namespace std;
using namespace monarch::data::json;
using namespace monarch::event;
using namespace monarch::io;
using namespace monarch::rt;
using namespace monarch::util;
using namespace bitmunk::common;
using namespace bitmunk::protocol;
using namespace bitmunk::node;
using namespace bitmunk::system;
namespace v = monarch::validation;

typedef BtpActionDelegate<EventService> Handler;

// observer timeout is 5 minutes
#define OBSERVER_TIMEOUT (uint64_t)(5 * 60000)
#define OBSERVER_TIMEOUT_EVENT "bitmunk.webui.EventService.checkObserverTimeout"

/* Note: mObserverInfo is a map key'd off of string Observer IDs, where
 * the map entries are formatted as such:
 *
 * string(Observer ID) => info:
 * {
 *    "id"            : string, the observer's ID
 *    "events"        : [] of events that have occurred in order
 *    "time"          : time, in ms, of the last request to check the event queue
 *    "coalesceRules" : [] of event data maps that indicate which events to
 *                      coalesce together, which means only the latest event
 *                      that matches the entry will remain in an event queue,
 *                      others will be removed (especially useful for
 *                      progress updates)
 * }
 *
 * Coalesce rules are used to indicate that only the latest event in a set
 * of events that match a particular rule should be included in an event queue.
 *
 * For instance, if there are two events:
 *
 * eventA (occurred first):
 * {
 *    "details": {
 *       "foo" : "abc",
 *       "bar" : "xyz"
 *    }
 * }
 *
 * eventB (occurred second):
 * {
 *    "details": {
 *       "foo" : "abc"
 *    }
 * }
 *
 * Then they could be coalesced (meaning only the latest event
 * will appear in the observer's queue) by using the given coalesce rule:
 *
 * {
 *    "details": {
 *       "foo" : true
 * }
 *
 * If "foo" were set to false, then events where "foo"'s value is not
 * equal would be coalesced. If details where set to true instead of
 * a map, then any event with all of the same details would be coalesced.
 */

EventService::EventService(Node* node, const char* path) :
   NodeService(node, path)
{
}

EventService::~EventService()
{
}

bool EventService::initialize()
{
   // create observers
   {
      RestResourceHandlerRef obc = new RestResourceHandler();
      addResource("/observers/create", obc);

      // POST .../observers/create
      {
         Handler* handler = new Handler(
            mNode, this, &EventService::createObservers,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef qValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         // validate content
         v::ValidatorRef validator = new v::Map(
            "count", new v::Int(v::Int::Positive),
            NULL);

         obc->addHandler(h, BtpMessage::Post, 0, &qValidator, &validator);
      }
   }

   // delete observers
   {
      RestResourceHandlerRef obd = new RestResourceHandler();
      addResource("/observers/delete", obd);

      // POST .../observers/delete
      {
         Handler* handler = new Handler(
            mNode, this, &EventService::deleteObservers,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef qValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         // validate content
         v::ValidatorRef validator = new v::All(
            new v::Type(Array),
            new v::Each(new v::All(
               new v::Type(String),
               new v::Min(1, "Observer ID too short. 1 character minimum."),
               NULL)),
            NULL);

         obd->addHandler(h, BtpMessage::Post, 0, &qValidator, &validator);
      }
   }

   // register observers
   {
      RestResourceHandlerRef obr = new RestResourceHandler();
      addResource("/observers/register", obr);

      // POST .../observers/register
      {
         Handler* handler = new Handler(
            mNode, this, &EventService::registerObservers,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef qValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         // validate content
         v::ValidatorRef validator = new v::All(
            new v::Type(Array),
            new v::Each(new v::All(
               new v::Any(
                  new v::Map(
                     "events", new v::Type(Array),
                     NULL),
                  new v::Map(
                     "coalesceRules", new v::Type(Array),
                     NULL),
                  NULL),
               new v::Map(
                  // observer ID
                  "id", new v::Type(String),
                  // events to register for
                  "events", new v::Optional(
                     new v::Each(new v::Map(
                        "type", new v::All(
                           new v::Type(String),
                           new v::Min(
                              1, "Event type too short. 1 character minimum."),
                           NULL),
                        "filter", new v::Optional(new v::Type(Map)),
                        NULL))),
                  // coalesce rules to use
                  "coalesceRules", new v::Optional(
                     new v::Each(new v::Type(Map))),
                  NULL),
               NULL)),
            NULL);

         obr->addHandler(h, BtpMessage::Post, 0, &qValidator, &validator);
      }
   }

   // unregister observers
   {
      RestResourceHandlerRef obu = new RestResourceHandler();
      addResource("/observers/unregister", obu);

      // POST .../observers/unregister
      {
         Handler* handler = new Handler(
            mNode, this, &EventService::unregisterObservers,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef qValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         // validate content
         v::ValidatorRef validator = new v::All(
            new v::Type(Array),
            new v::Each(new v::Map(
               "observerId", new v::All(
                  new v::Type(String),
                  new v::Min(1, "Observer ID too short. 1 character minimum."),
                  NULL),
               // events type to unregister
               "events", new v::Optional(new v::All(
                  new v::Type(String),
                  new v::Min(1, "Event type too short. 1 character minimum."),
                  NULL)),
               // coalesce rules to remove
               "coalesceRules", new v::Optional(new v::Type(Array)),
               NULL)),
            NULL);

         obu->addHandler(h, BtpMessage::Post, 0, &qValidator, &validator);
      }
   }

   // events
   {
      RestResourceHandlerRef events = new RestResourceHandler();
      addResource("/", events);

      // POST .../
      {
         Handler* handler = new Handler(
            mNode, this, &EventService::getEvents,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef qValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         // FIXME: add jsonp query param support

         // validate content
         v::ValidatorRef validator = new v::Map(
            // observer IDs to receive events for
            "receive", new v::Optional(new v::All(
               new v::Type(Array),
               new v::Each(new v::All(
                  new v::Type(String),
                  new v::Min(1, "Observer ID too short. 1 character minimum."),
                  NULL)),
               NULL)),
            // optional poll interval to wait for events to occur
            "pollInterval", new v::Optional(
               new v::Int(v::Int::Positive)),
            // events to send
            "send", new v::Optional(new v::All(
               new v::Type(Array),
               new v::Each(new v::Map(
                  "type", new v::All(
                     new v::Type(String),
                     new v::Min(
                        1, "Event type too short. 1 character minimum."),
                     NULL),
                  NULL)),
               NULL)),
            // events to add to the event daemon
            "add", new v::Optional(new v::All(
               new v::Type(Array),
               new v::Each(new v::Map(
                  "event", new v::Map(
                     "type", new v::All(
                        new v::Type(String),
                        new v::Min(
                           1, "Event type too short. 1 character minimum."),
                        NULL),
                     NULL),
                  "interval", new v::Int(v::Int::Positive),
                  "count", new v::Int(),
                  "refs", new v::Optional(new v::Int(v::Int::Positive)),
                  NULL)),
               NULL)),
            // events to remove from the event daemon
            "remove", new v::Optional(new v::All(
               new v::Type(Array),
               new v::Each(new v::All(
                  new v::Any(
                     new v::Map(
                        "event", new v::Map(
                           "type", new v::All(
                              new v::Type(String),
                              new v::Min(
                                 1,
                                 "Event type too short. 1 character minimum."),
                              NULL),
                           NULL),
                        NULL),
                     new v::Map(
                        "eventType", new v::All(
                           new v::Type(String),
                           new v::Min(
                              1, "Event type too short. 1 character minimum."),
                           NULL),
                        NULL),
                     NULL),
                  new v::Map(
                     "interval", new v::Optional(new v::Int(v::Int::Positive)),
                     "refs", new v::Optional(new v::Int(v::Int::Positive)),
                     NULL),
                  NULL)),
               NULL)),
            NULL);

         events->addHandler(h, BtpMessage::Post, 0, &qValidator, &validator);
      }
   }

   // initialize observer info to a map
   mObserverInfo->setType(Map);

   return true;
}

static void _cleanupObserverTimeoutHandling(EventService* self, Node* node)
{
   // unregister from maintenance event
   node->getEventController()->unregisterObserver(
      self, OBSERVER_TIMEOUT_EVENT);

   // stop sending maintenance event
   node->getEventDaemon()->remove(OBSERVER_TIMEOUT_EVENT);
}

void EventService::cleanup()
{
   // remove resources
   removeResource("/observers/create");
   removeResource("/observers/delete");
   removeResource("/observers/register");
   removeResource("/observers/unregister");
   removeResource("/");

   _cleanupObserverTimeoutHandling(this, mNode);

   // unregister all remaining observers
   mObserverLock.lockExclusive();
   {
      for(ObserverMap::iterator i = mObservers.begin();
          i != mObservers.end(); i++)
      {
         // unregister observer entirely
         mNode->getEventController()->unregisterObserver(i->second);

         // erase observer
         delete i->second;
      }
      mObservers.clear();
   }
   mObserverLock.unlockExclusive();
}

/**
 * A recursive helper function that returns true if the passed coalesce rule
 * applies to the given events.
 *
 * @param rule the coalesce rule to check it against.
 * @param e1 the first event to check.
 * @param e2 the second event to check, NULL to only check the first.
 *
 * @return true if the rule applies to the events, false if not.
 */
static bool applyCoalesceRule(DynamicObject& rule, Event& e1, Event* e2)
{
   bool rval = true;

   // get rule and event types
   DynamicObjectType ruleType = rule->getType();
   DynamicObjectType e1Type = e1->getType();
   DynamicObjectType e2Type = (e2 == NULL) ? String : (*e2)->getType();

   if(ruleType == Map)
   {
      if(e1Type == Map && (e2 == NULL || e2Type == Map))
      {
         DynamicObjectIterator mi = rule.getIterator();
         while(rval && mi->hasNext())
         {
            DynamicObject& mv = mi->next();
            const char* name = mi->getName();

            // ensure both events have the given member
            if(e1->hasMember(name) && (e2 == NULL || (*e2)->hasMember(name)))
            {
               // traverse rule
               rval = applyCoalesceRule(
                  mv, e1[name], (e2 == NULL) ? NULL : &(*e2)[name]);
            }
            else
            {
               // events do not have rule member
               rval = false;
            }
         }
      }
      else
      {
         // types don't match rule
         rval = false;
      }
   }
   else if(ruleType == Array)
   {
      if(e1Type == Array && rule->length() <= e1->length() &&
         (e2 == NULL ||
         (e2Type == Array && rule->length() <= (*e2)->length())))
      {
         int idx = 0;
         DynamicObjectIterator ai = rule.getIterator();
         while(rval && ai->hasNext())
         {
            DynamicObject& av = ai->next();

            // traverse rule details
            rval = applyCoalesceRule(
               av, e1[idx], (e2 == NULL) ? NULL : &(*e2)[idx]);
            idx++;
         }
      }
      else
      {
         // size or types don't match rule
         rval = false;
      }
   }
   else if(ruleType == Boolean)
   {
      if(e2 == NULL)
      {
         // rule applies
         rval = true;
      }
      else
      {
         // compare event data for equality
         if(rule->getBoolean())
         {
            rval = (e1 == (*e2));
         }
         // compare event data for inequality
         else
         {
            rval = (e1 != (*e2));
         }
      }
   }
   else
   {
      // rule is invalid
      rval = false;
   }

   return rval;
}

void EventService::eventOccurred(Event& e, void* observerId)
{
   MO_CAT_INFO(BM_SYSTEM_CAT,
      "EventService Observer got event: %s", e["type"]->getString());

   // lock to read from observer map
   mObserverLock.lockShared();

   // get observer info
   const char* id = (const char*)observerId;
   DynamicObject info(NULL);
   if(!mObserverInfo->hasMember(id))
   {
      // observer no longer exists
      mObserverLock.unlockShared();
   }
   else
   {
      // get reference to observer info to add an event
      info = mObserverInfo[id];

      // release shared lock & acquire exclusive lock to write to queue
      mObserverLock.unlockShared();
      mObserverLock.lockExclusive();

      // check coalesce rules, if any exist
      if(info["coalesceRules"]->length() > 0)
      {
         DynamicObjectIterator ri = info["coalesceRules"].getIterator();
         while(ri->hasNext())
         {
            DynamicObject& rule = ri->next();

            // see if rule applies to the current event
            if(applyCoalesceRule(rule, e, NULL))
            {
               // remove any events from the queue where the rule applies
               DynamicObjectIterator ei = info["events"].getIterator();
               while(ei->hasNext())
               {
                  Event& ne = ei->next();
                  if(applyCoalesceRule(rule, e, &ne))
                  {
                     MO_CAT_DEBUG(BM_SYSTEM_CAT,
                        "Removing coalesced event:\n"
                        "%sReplaced by event:\n%s",
                        JsonWriter::writeToString(ne, false, false).c_str(),
                        JsonWriter::writeToString(e, false, false).c_str());

                     // remove coalesced event
                     ei->remove();
                  }
               }
            }
         }
      }

      // append event to queue (OK if info was dropped from observer map
      // while the locks were being switched, we will just be adding to
      // a dynamic object that will be collected after we return)
      info["events"]->append(e);

      // done appending
      mObserverLock.unlockExclusive();
   }
}

void EventService::eventOccurred(Event& e)
{
   MO_CAT_INFO(BM_SYSTEM_CAT,
      "EventService got event: %s", e["type"]->getString());

   mObserverLock.lockExclusive();
   {
      // do maintenance by checking last access time for each observer
      uint64_t now = System::getCurrentMilliseconds();
      DynamicObjectIterator i = mObserverInfo.getIterator();
      while(i->hasNext())
      {
         DynamicObject& next = i->next();

         if(now - next["time"]->getUInt64() > OBSERVER_TIMEOUT)
         {
            MO_CAT_INFO(BM_SYSTEM_CAT,
               "EventService deleting expired observer: %s", i->getName());

            ObserverMap::iterator itr = mObservers.find(i->getName());
            if(itr != mObservers.end())
            {
               // unregister observer entirely
               mNode->getEventController()->unregisterObserver(itr->second);

               // erase observer
               delete itr->second;
               mObservers.erase(itr);
            }

            // remove observer info
            i->remove();
         }
      }

      if(mObservers.empty())
      {
         _cleanupObserverTimeoutHandling(this, mNode);
      }
   }
   mObserverLock.unlockExclusive();
}

bool EventService::createObservers(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = true;

   // allow keep-alive
   setKeepAlive(action);

   // Note: "in" is a map with "count" set to the number of observers
   // to create

   // out is an array of new observer IDs
   out->setType(Array);

   int count = in["count"]->getInt32();
   for(int i = 0; i < count; i++)
   {
      // safe to lock on each iteration, allowing better concurrency
      mObserverLock.lockExclusive();
      {
         // create a new observer, return its ID
         out->append() = createObserver();
      }
      mObserverLock.unlockExclusive();
   }

   return rval;
}

bool EventService::deleteObservers(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = true;

   // allow keep-alive
   setKeepAlive(action);

   // Note: "in" is an array of IDs of observers to be deleted

   // out is an array of deleted observers
   out->setType(Array);

   DynamicObjectIterator i = in.getIterator();
   while(i->hasNext())
   {
      DynamicObject& next = i->next();
      const char* id = next->getString();

      // safe to lock on each iteration, allowing better concurrency
      mObserverLock.lockExclusive();
      {
         // find the observer to remove
         ObserverMap::iterator i = mObservers.find(id);
         if(i != mObservers.end())
         {
            // return observer ID
            out->append() = id;

            // unregister observer entirely
            mNode->getEventController()->unregisterObserver(i->second);

            // erase observer, remove event queue
            delete i->second;
            mObservers.erase(i);
            mObserverInfo->removeMember(id);
         }
      }
      mObserverLock.unlockExclusive();
   }

   // lock to clean up observer timeout event handling if appropriate
   mObserverLock.lockExclusive();
   {
      if(mObservers.empty())
      {
         _cleanupObserverTimeoutHandling(this, mNode);
      }
   }
   mObserverLock.unlockExclusive();

   return rval;
}

bool EventService::registerObservers(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = true;

   // allow keep-alive
   setKeepAlive(action);

   // Note: "in" is an array of entries with observer IDs, event types, and
   // filters to register, an ID of "0" will create an observer on the fly

   // return array of IDs for valid registered observers
   out->setType(Array);

   // create an array of valid observers so we don't have to look them
   // up multiple times in the observer map
   Observer* observers[in->length()];

   // create an array for invalid observer IDs
   DynamicObject invalidIds(NULL);

   // lock while checking and registering observers
   mObserverLock.lockExclusive();
   {
      // first assure all observers to be registered exist
      int index = 0;
      DynamicObjectIterator i = in.getIterator();
      for(; i->hasNext(); index++)
      {
         // find the observer (or skip it if it is to be created ... these
         // have an ID of "0")
         DynamicObject& entry = i->next();
         const char* id = entry["id"]->getString();
         if(strcmp(id, "0") == 0)
         {
            // point to invalid observer as a placeholder ...
            // this will be corrected below once the observer is created
            // on-the-fly (provided there are no invalid IDs found)
            observers[index] = NULL;
         }
         else
         {
            ObserverMap::iterator itr = mObservers.find(id);

            // observer ID is valid
            if(itr != mObservers.end())
            {
               observers[index] = itr->second;
            }
            // observer ID is invalid
            else
            {
               // add invalid ID to list, lazily creating list
               if(invalidIds.isNull())
               {
                  invalidIds = DynamicObject();
               }
               invalidIds->append() = id;
            }
         }
      }

      if(invalidIds.isNull())
      {
         // all observers valid, register them
         index = 0;
         i = in.getIterator();
         for(; i->hasNext(); index++)
         {
            DynamicObject& entry = i->next();
            const char* id = entry["id"]->getString();

            if(strcmp(id, "0") == 0)
            {
               // create the observer on-the-fly, correct observers entry
               Observer* out;
               id = createObserver(&out);
               observers[index] = out;
            }

            // append valid ID to output
            out->append() = id;

            // get the observer info
            DynamicObject& info = mObserverInfo[id];

            // add coalesce rules
            if(entry->hasMember("coalesceRules"))
            {
               info["coalesceRules"].merge(entry["coalesceRules"], true);
            }

            // register for events
            if(entry->hasMember("events"))
            {
               // register for specified events
               mNode->getEventController()->registerObserver(
                  observers[index], entry["events"]);
            }

            // update timeout
            info["time"] = System::getCurrentMilliseconds();
         }
      }
   }
   mObserverLock.unlockExclusive();

   if(!invalidIds.isNull())
   {
      // at least one observer ID was invalid
      ExceptionRef e = new Exception(
         "Observer(s) not found.",
         "bitmunk.webui.EventService.ObserverNotFound", 404);
      e->getDetails()["observerIds"] = invalidIds;
      Exception::set(e);
      rval = false;
   }

   return rval;
}

bool EventService::unregisterObservers(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = true;

   // allow keep-alive
   setKeepAlive(action);

   // Note: "in" is an array of unregistration entries (which have
   // "observerId" and an optional "events" array of events to
   // unregister the observer from -- if not provided the observer
   // is unregistered from all events)

   // return array of unregistered observers
   out->setType(Array);

   // unregister all specified observers
   DynamicObjectIterator i = in.getIterator();
   while(rval && i->hasNext())
   {
      DynamicObject& next = i->next();

      // safe to lock on each iteration, allowing better concurrency
      mObserverLock.lockExclusive();
      {
         // find the observer
         const char* id = next["observerId"]->getString();
         ObserverMap::iterator itr = mObservers.find(id);
         if(itr != mObservers.end())
         {
            // unregister entirely
            if((!next->hasMember("events") ||
               next["events"]->length() == 0) &&
               (!next->hasMember("coalesceRules") ||
               next["coalesceRules"]->length() == 0))
            {
               mObserverInfo[id]["coalesceRules"]->clear();
               mNode->getEventController()->unregisterObserver(itr->second);
            }
            else
            {
               // unregister from specified events
               if(next->hasMember("events"))
               {
                  mNode->getEventController()->unregisterObserver(
                     itr->second, next["events"]);
               }

               // remove specific coalesce rules
               if(next->hasMember("coalesceRules"))
               {
                  // iterate over rules to remove
                  DynamicObjectIterator rmi =
                     next["coalesceRules"].getIterator();
                  while(rmi->hasNext())
                  {
                     DynamicObject& removeRule = rmi->next();

                     // iterate over existing rules
                     DynamicObject& rules =
                        mObserverInfo[id]["coalesceRules"];
                     DynamicObjectIterator ri = rules.getIterator();
                     while(ri->hasNext())
                     {
                        DynamicObject& rule = ri->next();
                        if(removeRule == rule)
                        {
                           // remove matching rule
                           ri->remove();
                        }
                     }
                  }
               }
            }

            // return observer ID
            out->append() = id;
         }
      }
      mObserverLock.unlockExclusive();
   }

   return rval;
}

bool EventService::getEvents(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = true;

   // allow keep-alive
   setKeepAlive(action);

   // Note: "in" is a map with optional "send" array of events to be
   // sent out, and an optional "receive" array of IDs of observers
   // to receive the latest events for

   // return a map of observer IDs to events
   out->setType(Map);

   // send any posted events out
   if(in->hasMember("send"))
   {
      DynamicObjectIterator i = in["send"].getIterator();
      while(i->hasNext())
      {
         mNode->getEventController()->schedule(i->next());
      }
   }

   // add events to the event daemon
   if(in->hasMember("add"))
   {
      DynamicObjectIterator i = in["add"].getIterator();
      while(i->hasNext())
      {
         DynamicObject& next = i->next();

         // add event to the daemon
         mNode->getEventDaemon()->add(
            next["event"], next["interval"]->getUInt32(),
            next["count"]->getInt32(), next["refs"]->getInt32());
      }
   }

   // remove events from the event daemon
   if(in->hasMember("remove"))
   {
      DynamicObjectIterator i = in["remove"].getIterator();
      while(i->hasNext())
      {
         DynamicObject& next = i->next();

         // remove event from the daemon
         if(next->hasMember("event"))
         {
            if(next->hasMember("interval"))
            {
               // remove event @ interval
               mNode->getEventDaemon()->remove(
                  next["event"], next["interval"]->getUInt32(),
                  next["refs"]->getInt32());
            }
            else
            {
               // remove event @ any interval
               mNode->getEventDaemon()->remove(
                  next["event"], next["refs"]->getInt32());
            }
         }
         else
         {
            // remove any event with given event type
            mNode->getEventDaemon()->remove(
               next["eventType"]->getString(), next["refs"]->getInt32());
         }
      }
   }

   // receive events
   if(in->hasMember("receive"))
   {
      // create an array for invalid observer IDs
      DynamicObject invalidIds(NULL);

      // lock while finding any invalid observer IDs
      mObserverLock.lockExclusive();
      {
         // iterate over each observer ID
         const char* id;
         DynamicObjectIterator i = in["receive"].getIterator();
         while(i->hasNext())
         {
            DynamicObject& next = i->next();
            id = next->getString();

            // observer ID is invalid
            if(!mObserverInfo->hasMember(id))
            {
               // add invalid ID to list, lazily creating list
               if(invalidIds.isNull())
               {
                  invalidIds = DynamicObject();
               }
               invalidIds->append() = id;
            }
         }
      }
      mObserverLock.unlockExclusive();

      // invalid observer IDs found, so bail
      if(!invalidIds.isNull())
      {
         ExceptionRef e = new Exception(
            "Observer(s) not found.",
            "bitmunk.webui.EventService.ObserverNotFound", 404);
         e->getDetails()["observerIds"] = invalidIds;
         Exception::set(e);
         rval = false;
      }
      // no invalid observer IDs
      else
      {
         // if a poll interval is specific, keep waiting for events until
         // one becomes available, up to the keep-alive time

         // default to 4 minutes of timeout (less than standard 5 minute
         // keep-alive timeout so that an empty response will be returned
         // before a timeout exception is triggered)
         uint64_t remaining = 4 * 60 * 1000;
         uint64_t pollInterval = (in->hasMember("pollInterval") ?
            in["pollInterval"]->getUInt64() : 0);

         do
         {
            // lock while checking and receiving events
            mObserverLock.lockExclusive();
            {
               // get events for each observer, exclude empty event queues
               const char* id;
               DynamicObjectIterator i = in["receive"].getIterator();
               while(i->hasNext())
               {
                  DynamicObject& next = i->next();
                  id = next->getString();

                  // we need to recheck for the observer ID because it might
                  // have been deleted during a long poll ... in which case
                  // we don't report an error, but we also don't need to
                  // check it for any events since it doesn't exist
                  if(mObserverInfo->hasMember(id))
                  {
                     // return events and clear queue by creating a new one
                     DynamicObject& queue = mObserverInfo[id]["events"];
                     if(queue->length() > 0)
                     {
                        out[id] = queue;
                        DynamicObject d;
                        d->setType(Array);
                        mObserverInfo[id]["events"] = d;
                     }

                     // update timeout
                     mObserverInfo[id]["time"] =
                        System::getCurrentMilliseconds();
                  }
               }
            }
            mObserverLock.unlockExclusive();

            // no events found yet and user specified a poll interval
            if(out->length() == 0 && pollInterval > 0)
            {
               // wait for the poll interval if there is enough time remaining
               if(pollInterval <= remaining)
               {
                  // FIXME: sadly, this wastes a thread resource... remember
                  // to fix this when we move to asynchronous IO
                  rval = Thread::sleep(pollInterval);
                  if(rval)
                  {
                     if(action->getResponse()->getConnection()->isClosed())
                     {
                        ExceptionRef e = new Exception(
                           "Connection closed.",
                           "monarch.net.Socket");
                        Exception::set(e);
                        rval = false;
                     }
                     else
                     {
                        remaining -= pollInterval;
                     }
                  }
               }
            }
         }
         // keep polling while no events found, polling was requested, and
         // there is time remaining
         while(rval && out->length() == 0 && remaining > 0 && pollInterval > 0);
      }
   }

   return rval;
}

const char* EventService::createObserver(Observer** obOut)
{
   // Note: Assume mObserverLock is engaged.

   // Note: The observer ID is stored as a map key and in the
   // information so that it can be used as a key in the std::map
   // that stores observers.

   // get random number for ID
   char tmp[22];
   do
   {
      // get a random number between 1 and 1000000000
      uint64_t n = Random::next(1, 1000000000);
      snprintf(tmp, 22, "%" PRIu64, n);
   }
   while(mObserverInfo->hasMember(tmp));

   // set observer information: ID & event queue & timeout
   const char* id = tmp;
   mObserverInfo[id]["id"] = tmp;
   mObserverInfo[id]["events"]->setType(Array);
   mObserverInfo[id]["coalesceRules"]->setType(Array);
   mObserverInfo[id]["time"] = System::getCurrentMilliseconds();
   id = mObserverInfo[id]["id"]->getString();

   // insert a new observer
   ObserverDelegate<EventService>* ob =
      new ObserverDelegate<EventService>(
         this, &EventService::eventOccurred, (void*)id);
   mObservers.insert(make_pair(id, ob));
   if(obOut != NULL)
   {
      *obOut = ob;
   }

   if(mObservers.size() == 1)
   {
      // register to receive maintenance event
      mNode->getEventController()->registerObserver(
         this, OBSERVER_TIMEOUT_EVENT);

      // start sending maintenance event
      Event e;
      e["type"] = OBSERVER_TIMEOUT_EVENT;
      mNode->getEventDaemon()->add(e, OBSERVER_TIMEOUT, -1);
   }

   // return ID
   return id;
}
