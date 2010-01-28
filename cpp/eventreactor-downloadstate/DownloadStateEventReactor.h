/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_eventreactor_DownloadStateEventReactor_H
#define bitmunk_eventreactor_DownloadStateEventReactor_H

#include "bitmunk/eventreactor/EventReactor.h"
#include "bitmunk/node/Node.h"
#include "bitmunk/purchase/TypeDefinitions.h"

namespace bitmunk
{
namespace eventreactor
{

/**
 * A DownloadStateEventReactor reacts to download state events.
 *
 * @author Dave Longley
 */
class DownloadStateEventReactor : public EventReactor
{
protected:
   /**
    * The associated Bitmunk Node.
    */
   bitmunk::node::Node* mNode;

   /**
    * The associated ObserverContainer.
    */
   ObserverContainer* mObserverContainer;

   /**
    * A lock for reprocessing download states.
    */
   monarch::rt::ExclusiveLock mReprocessLock;

public:
   /**
    * Creates a new DownloadStateEventReactor.
    *
    * @param node the associated Bitmunk Node.
    */
   DownloadStateEventReactor(bitmunk::node::Node* node);

   /**
    * Destructs this DownloadStateEventReactor.
    */
   virtual ~DownloadStateEventReactor();

   /**
    * Sets the ObserverContainer to use.
    *
    * @param oc the ObserverContainer to use.
    */
   virtual void setObserverContainer(ObserverContainer* oc);

   /**
    * Gets the assigned ObserverContainer.
    *
    * @return the assigned ObserverContainer.
    */
   virtual ObserverContainer* getObserverContainer();

   /**
    * Starts reacting to events for a particular user.
    *
    * @param userId the ID of the user.
    */
   virtual void addUser(bitmunk::common::UserId userId);

   /**
    * Stops reacting to events for a particular user.
    *
    * @param userId the ID of the user.
    */
   virtual void removeUser(bitmunk::common::UserId userId);

   /**
    * Handles a directiveCreated event by processing the directive.
    *
    * @param e the event.
    */
   virtual void directiveCreated(monarch::event::Event& e);

   /**
    * Handles a downloadStateCreated event by initializing the download state.
    *
    * @param e the event.
    */
   virtual void downloadStateCreated(monarch::event::Event& e);

   /**
    * Handles a downloadStateInitialized event by acquiring a license next.
    *
    * @param e the event.
    */
   virtual void downloadStateInitialized(monarch::event::Event& e);

   /**
    * Handles a pieceStarted event by scheduling progress poll events with
    * the event daemon.
    *
    * @param e the event.
    */
   virtual void pieceStarted(monarch::event::Event& e);

   /**
    * Handles a downloadStopped event by removing progress poll events from
    * the event daemon.
    *
    * @param e the event.
    */
   virtual void downloadStopped(monarch::event::Event& e);

   /**
    * Handles a downloadComplete event by starting the purchase process.
    *
    * @param e the event.
    */
   virtual void downloadCompleted(monarch::event::Event& e);

   /**
    * Handles a purchaseComplete event by starting the file assembly process.
    *
    * @param e the event.
    */
   virtual void purchaseCompleted(monarch::event::Event& e);

   /**
    * Handles a reprocessRequired event by reprocessing a specific or all (as
    * specified in the event) non-processing download state(s).
    *
    * @param e the event.
    */
   virtual void reprocessRequired(monarch::event::Event& e);

protected:
   /**
    * Posts a download state ID to the purchase module's 3.0 API.
    *
    * @param userId the user ID to post for.
    * @param dsId the download state ID to post.
    * @param action the action to take with the download state, to be appended
    *               to the API url.
    */
   virtual void postDownloadStateId(
      bitmunk::common::UserId userId,
      bitmunk::purchase::DownloadStateId dsId, const char* action);
};

} // end namespace eventreactor
} // end namespace bitmunk
#endif
