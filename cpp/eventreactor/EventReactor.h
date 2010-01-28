/*
 * Copyright (c) 2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_eventreactor_IEventReactor_H
#define bitmunk_eventreactor_IEventReactor_H

#include "bitmunk/eventreactor/ObserverContainer.h"

namespace bitmunk
{
namespace eventreactor
{

/**
 * An EventReactor provides the implementation for adding observers for a
 * particular user to react to events. It uses the assigned ObserverContainer
 * to add observers.
 *
 * @author Dave Longley
 */
class EventReactor
{
public:
   /**
    * Creates a new EventReactor.
    */
   EventReactor() {};

   /**
    * Destructs this EventReactor.
    */
   virtual ~EventReactor() {};

   /**
    * Sets the ObserverContainer to use.
    *
    * @param oc the ObserverContainer to use.
    */
   virtual void setObserverContainer(ObserverContainer* oc) = 0;

   /**
    * Gets the assigned ObserverContainer.
    *
    * @return the assigned ObserverContainer.
    */
   virtual ObserverContainer* getObserverContainer() = 0;

   /**
    * Starts reacting to events for a particular user. Extending classes
    * implement this method to add all necessary observers to the associated
    * ObserverContainer for the user ID, keeping in mind that the container
    * is locked until all additions are complete.
    *
    * @param userId the ID of the user.
    */
   virtual void addUser(bitmunk::common::UserId userId) = 0;

   /**
    * Stops reacting to events for a particular user. Extending classes
    * implement this method to perform any clean up that is necessary before
    * a user is removed. Unregistering event handlers is unnecessary as this
    * is done automatically.
    *
    * The related ObserverContainer is locked until this method completes.
    *
    * @param userId the ID of the user.
    */
   virtual void removeUser(bitmunk::common::UserId userId) = 0;
};

} // end namespace eventreactor
} // end namespace bitmunk
#endif
