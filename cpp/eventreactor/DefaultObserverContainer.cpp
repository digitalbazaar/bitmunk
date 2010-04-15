/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/eventreactor/DefaultObserverContainer.h"

#include "bitmunk/common/Logging.h"
#include "monarch/event/ObserverDelegate.h"

#include <algorithm>

using namespace std;
using namespace monarch::event;
using namespace monarch::rt;
using namespace bitmunk::common;
using namespace bitmunk::protocol;
using namespace bitmunk::node;
using namespace bitmunk::eventreactor;

DefaultObserverContainer::DefaultObserverContainer(
   Node* node, EventReactor* er) :
   mNode(node),
   mEventReactor(er)
{
}

DefaultObserverContainer::~DefaultObserverContainer()
{
   // force unregister all users
   mUserObserverMapLock.lock();
   while(!mUserObserverMap.empty())
   {
      unregisterUser(mUserObserverMap.begin()->first);
   }
   mUserObserverMapLock.unlock();
}

void DefaultObserverContainer::registerUser(UserId userId)
{
   mUserObserverMapLock.lock();
   {
      if(mUserObserverMap.empty())
      {
         // register to receive logged out events
         mLogoutObserver = new ObserverDelegate<DefaultObserverContainer>(
            this, &DefaultObserverContainer::userLoggedOut);
         mNode->getEventController()->registerObserver(
            &(*mLogoutObserver), "bitmunk.common.User.loggedOut");
      }

      // only register user if observer list doesn't already exist,
      // else assume user is registered and return
      UserObserverMap::iterator i = mUserObserverMap.find(userId);
      if(i == mUserObserverMap.end())
      {
         mUserObserverMap.insert(make_pair(userId, new ObserverList()));
         mEventReactor->addUser(userId);
      }
   }
   mUserObserverMapLock.unlock();
}

bool DefaultObserverContainer::unregisterUser(UserId userId)
{
   bool rval;

   mUserObserverMapLock.lock();
   {
      // remove user
      mEventReactor->removeUser(userId);

      // remove all observers for user ID and unregister user
      rval = removeObservers(userId, true);

      if(mUserObserverMap.empty())
      {
         // unregister handling logged out events
         mNode->getEventController()->unregisterObserver(&(*mLogoutObserver));
         mLogoutObserver.setNull();
      }
   }
   mUserObserverMapLock.unlock();

   return rval;
}

void DefaultObserverContainer::userLoggedOut(Event& e)
{
   // unregister user
   unregisterUser(BM_USER_ID(e["details"]["userId"]));
}

void DefaultObserverContainer::addObserver(UserId userId, ObserverRef& ob)
{
   // only add observer if observer list exists
   UserObserverMap::iterator i = mUserObserverMap.find(userId);
   if(i != mUserObserverMap.end())
   {
      i->second->add(ob);
   }
}

bool DefaultObserverContainer::removeObservers(UserId userId, bool unregister)
{
   bool rval = false;

   UserObserverMap::iterator i = mUserObserverMap.find(userId);
   if(i != mUserObserverMap.end())
   {
      rval = true;
      ObserverList* list = i->second;
      list->unregisterFrom(mNode->getEventController());

      if(unregister)
      {
         // unregistering user, so erase list
         mUserObserverMap.erase(i);
         delete list;
      }
      else
      {
         // simply clear list
         list->clear();
      }
   }

   return rval;
}
