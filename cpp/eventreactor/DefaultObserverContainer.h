/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_eventreactor_DefaultObserverContainer_H
#define bitmunk_eventreactor_DefaultObserverContainer_H

#include "bitmunk/node/Node.h"
#include "monarch/event/ObserverList.h"
#include "bitmunk/eventreactor/ObserverContainer.h"
#include "bitmunk/eventreactor/EventReactor.h"

#include <map>

namespace bitmunk
{
namespace eventreactor
{

/**
 * A default implementation of an ObserverContainer. It handles automatic
 * unregistration of a user if they log out.
 *
 * @author Dave Longley
 */
class DefaultObserverContainer : public ObserverContainer
{
protected:
   /**
    * The associated Bitmunk Node.
    */
   bitmunk::node::Node* mNode;

   /**
    * A map of user ID to observer list.
    */
   typedef std::map<bitmunk::common::UserId, monarch::event::ObserverList*>
      UserObserverMap;
   UserObserverMap mUserObserverMap;

   /**
    * A lock for manipulating the observer lists.
    */
   monarch::rt::ExclusiveLock mUserObserverMapLock;

   /**
    * The user logged-out observer.
    */
   monarch::event::ObserverRef mLogoutObserver;

   /**
    * The associated EventReactor.
    */
   EventReactor* mEventReactor;

public:
   /**
    * Creates a new DefaultObserverContainer.
    *
    * @param node the associated Bitmunk Node.
    * @param er the associated EventReactor.
    */
   DefaultObserverContainer(bitmunk::node::Node* node, EventReactor* er);

   /**
    * Destructs this DefaultObserverContainer.
    */
   virtual ~DefaultObserverContainer();

   /**
    * Calls addUser() on the associated EventReactor, if the user has not
    * already been added. This should not be called from an EventReactor.
    *
    * @param userId the ID of the user.
    */
   virtual void registerUser(bitmunk::common::UserId userId);

   /**
    * Calls removeUser() on the associated EventReactor. This should not be
    * called from an EventReactor.
    *
    * @param userId the ID of the user.
    *
    * @return true if the user was unregistered, false if the user was not
    *         registered to begin with.
    */
   virtual bool unregisterUser(bitmunk::common::UserId userId);

   /**
    * Handles the user logged out event.
    *
    * @param e the event.
    */
   virtual void userLoggedOut(monarch::event::Event& e);

   /**
    * Adds an observer for a particular user to this container so it can
    * be easily removed later. This should be called from an EventReactor
    * in its addUser() call.
    *
    * @param userId the ID of the user.
    * @param ob the observer to add.
    */
   virtual void addObserver(
      bitmunk::common::UserId userId, monarch::event::ObserverRef& ob);

protected:
   /**
    * Removes all observers for the given user ID. This should be called from
    * an EventReactor in its removeUser() call.
    *
    * @param userId the ID of the user.
    * @param unregister true to unregister the user as well, false not to.
    *
    * @return true if the user's observers were removed, false if the user
    *         didn't have any observers to begin with.
    */
   virtual bool removeObservers(
      bitmunk::common::UserId userId, bool unregister);
};

} // end namespace eventreactor
} // end namespace bitmunk
#endif
