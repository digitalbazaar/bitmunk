/*
 * Copyright (c) 2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_eventreactor_ObserverContainer_H
#define bitmunk_eventreactor_ObserverContainer_H

#include "bitmunk/common/TypeDefinitions.h"
#include "monarch/event/Observer.h"

namespace bitmunk
{
namespace eventreactor
{

/**
 * An ObserverContainer stores the observers for the users of an EventReactor.
 * When an EventReactor is created for a particular user, that user is
 * registered with the EventReactor's ObserverContainer, which in turn will
 * add all of the necessary observers to react to events. Lists of observers
 * are created and managed on a per-user basis, keyed by user ID.
 *
 * @author Dave Longley
 */
class ObserverContainer
{
public:
   /**
    * Creates a new ObserverContainer.
    */
   ObserverContainer() {};

   /**
    * Destructs this ObserverContainer.
    */
   virtual ~ObserverContainer() {};

   /**
    * Calls addUser() on the associated EventReactor, if the user has not
    * already been added. This should not be called from an EventReactor.
    *
    * @param userId the ID of the user.
    */
   virtual void registerUser(bitmunk::common::UserId userId) = 0;

   /**
    * Calls removeUser() on the associated EventReactor. This should not be
    * called from an EventReactor.
    *
    * @param userId the ID of the user.
    *
    * @return true if the user was unregistered, false if the user was not
    *         registered to begin with.
    */
   virtual bool unregisterUser(bitmunk::common::UserId userId) = 0;

   /**
    * Adds an observer for a particular user to this container so it can
    * be easily removed later. This should be called from an EventReactor
    * in its addUser() call.
    *
    * @param userId the ID of the user.
    * @param ob the observer to add.
    */
   virtual void addObserver(
      bitmunk::common::UserId userId, monarch::event::ObserverRef& ob) = 0;
};

} // end namespace eventreactor
} // end namespace bitmunk
#endif
