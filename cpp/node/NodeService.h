/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_node_NodeService_H
#define bitmunk_node_NodeService_H

#include "bitmunk/protocol/BtpService.h"
#include "bitmunk/node/Node.h"

namespace bitmunk
{
namespace node
{

/**
 * A NodeService is a BtpService with a associated Node.
 *
 * @author David I. Lehn
 * @author Manu Sporny
 */
class NodeService : public bitmunk::protocol::BtpService
{
protected:
   /**
    * The associated Node.
    */
   Node* mNode;

   /**
    * The list of Exception types to exclude from exception event propagation
    * when one calls sendExceptionEvent method.
    */
   monarch::rt::DynamicObject mExceptionTypeBlockList;

public:
   /**
    * Creates a new NodeService.
    *
    * @param node the associated Node.
    * @param path the path this servicer handles requests for.
    * @param dynamicResources true to allow dynamic adding/removing of
    *                         resources, false not to.
    */
   NodeService(Node* node, const char* path, bool dynamicResources = false);

   /**
    * Destructs this NodeService.
    */
   virtual ~NodeService();

protected:
   /**
    * Adds an exception type to a list of exception types that should be
    * blocked from propagation when sendExceptionEvent is called.
    *
    * @param type the type of the Exception to block.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool addExceptionTypeToBlockList(const char* type);

   /**
    * Sends an exception event.
    *
    * @param e the Exception to email.
    * @param action the BtpAction the exception occurred during.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool sendExceptionEvent(
      monarch::rt::ExceptionRef e, bitmunk::protocol::BtpAction* action);

   /**
    * Sends a "scrubbed" event. This add a "request" variable to the event
    * details, but will first scrub its cookies and/or other secure details.
    *
    * @param e the event to scrub.
    * @param action the BtpAction.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool sendScrubbedEvent(
      monarch::event::Event& e, bitmunk::protocol::BtpAction* action);
};

} // end namespace node
} // end namespace bitmunk
#endif
