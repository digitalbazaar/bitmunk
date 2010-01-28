/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_eventreactor_EventReactorService_H
#define bitmunk_eventreactor_EventReactorService_H

#include "bitmunk/node/NodeService.h"
#include "bitmunk/eventreactor/IEventReactorModule.h"
#include "monarch/util/StringTools.h"

#include <map>

namespace bitmunk
{
namespace eventreactor
{

/**
 * An EventReactorService controls EventReactors.
 *
 * @author Dave Longley
 */
class EventReactorService :
public bitmunk::node::NodeService,
public IEventReactorModule
{
protected:
   /**
    * A map of name to its EventReactor.
    */
   typedef std::map<const char*, EventReactor*, monarch::util::StringComparator>
      ReactorMap;
   ReactorMap mEventReactors;

public:
   /**
    * Creates a new EventReactorService.
    *
    * @param node the associated Bitmunk Node.
    * @param path the path this servicer handles requests for.
    */
   EventReactorService(bitmunk::node::Node* node, const char* path);

   /**
    * Destructs this EventReactorService.
    */
   virtual ~EventReactorService();

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
    * Creates an event reactor for the given user and name.
    *
    * HTTP equivalent: POST .../users/<userId>/<eventreactor-name>
    *
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool createReactor(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);

   /**
    * Deletes all event reactors for the given user.
    *
    * HTTP equivalent:
    *
    * DELETE .../users/<userId>
    * POST   .../delete/user/<userId>
    *
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool deleteReactors(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);

   /**
    * Gets the status of this service, including the names of supported
    * event reactors.
    *
    * HTTP equivalent: GET .../status
    *
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getStatus(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);

   /**
    * Sets a named EventReactor. The EventReactor will be deleted when it is
    * removed. This method assumes that no other IEventReactor has been
    * installed for the given name, if one has been, memory will be leaked.
    *
    * @param name the name of the event reactor.
    * @param er the event reactor.
    */
   virtual void setEventReactor(const char* name, EventReactor* er);

   /**
    * Removes and deletes the EventReactor associated with the given name.
    *
    * @param name the name of the event reactor to delete.
    */
   virtual void removeEventReactor(const char* name);
};

} // end namespace eventreactor
} // end namespace bitmunk
#endif
