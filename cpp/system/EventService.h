/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_system_EventService_H
#define bitmunk_system_EventService_H

#include "bitmunk/node/NodeService.h"
#include "monarch/rt/SharedLock.h"
#include "monarch/util/StringTools.h"

namespace bitmunk
{
namespace system
{

/**
 * EventService handles HTTP access to events.
 * 
 * @author Dave Longley
 */
class EventService :
public bitmunk::node::NodeService,
public monarch::event::Observer
{
protected:
   /**
    * The map of observer IDs to observers.
    */
   typedef std::map<
      const char*, monarch::event::Observer*, monarch::util::StringComparator>
      ObserverMap;
   ObserverMap mObservers;
   
   /**
    * A map of observer IDs to their information, including their ID and
    * their event queues. 
    */
   monarch::rt::DynamicObject mObserverInfo;
   
   /**
    * A lock for modifying the observers and their queues.
    */
   monarch::rt::SharedLock mObserverLock;
   
public:
   /**
    * Creates a new EventService.
    * 
    * @param node the associated node.
    * @param path the path this servicer handles requests for.
    */
   EventService(bitmunk::node::Node* node, const char* path);
   
   /**
    * Destructs this EventService.
    */
   virtual ~EventService();
   
   /**
    * Initializes this BtpService.
    * 
    * @return true if initialized, false if an Exception occurred.
    */
   virtual bool initialize();
   
   /**
    * Cleans up this BtpService.
    * 
    * Must be called after initialize() regardless of its return value.
    */
   virtual void cleanup();
   
   /**
    * The Observer method used by all Observers created by this EventService.
    * 
    * @param e the Event.
    * @param observerId the observer ID the event is for.
    */
   virtual void eventOccurred(monarch::event::Event& e, void* observerId);
   
   /**
    * The Observer method used by this EventService to handle maintenance
    * events.
    * 
    * @param e the Event.
    */
   virtual void eventOccurred(monarch::event::Event& e);
   
   /**
    * Creates new observer(s).
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool createObservers(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
   
   /**
    * Deletes existing observer(s).
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool deleteObservers(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
   
   /**
    * Registers observer(s) for events.
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool registerObservers(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
   
   /**
    * Unregisters observer(s) for events.
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool unregisterObservers(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
   
   /**
    * Gets the latest events queued with observer(s).
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getEvents(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
   
protected:
   /**
    * Creates a new observer. This method assumes the observer lock is
    * engaged before it is called.
    * 
    * @param obOut a pointer to set to the observer, if desired.
    * 
    * @return the ID of the new observer.
    */
   virtual const char* createObserver(monarch::event::Observer** obOut = NULL);
};

} // end namespace system
} // end namespace bitmunk
#endif
