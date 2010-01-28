/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_purchase_DirectiveProcessor_H
#define bitmunk_purchase_DirectiveProcessor_H

#include "bitmunk/node/NodeFiber.h"

namespace bitmunk
{
namespace system
{

/**
 * The DirectiveProcessor fiber processes a single directive.
 * 
 * @author Dave Longley
 */
class DirectiveProcessor :
public bitmunk::node::NodeFiber
{
protected:
   /**
    * The directive to process.
    */
   monarch::rt::DynamicObject mDirective;
   
   /**
    * The result from processing the directive, if any.
    */
   monarch::rt::DynamicObject mResult;
   
public:
   /**
    * Creates a new DirectiveProcessor.
    * 
    * @param node the Node associated with this fiber.
    */
   DirectiveProcessor(bitmunk::node::Node* node);
   
   /**
    * Destructs this DirectiveProcessor.
    */
   virtual ~DirectiveProcessor();
   
   /**
    * Sets the directive to process. This must be called before scheduling
    * the fiber to run.
    * 
    * @param d the directive to process.
    */
   virtual void setDirective(monarch::rt::DynamicObject& d);
   
   /**
    * Creates a download state.
    */
   virtual void createDownloadState();
   
protected:
   /**
    * Processes messages, retrieved via getMessages(), and performs whatever
    * custom work is necessary.
    */
   virtual void processMessages();
   
   /**
    * Sends an event about directive processing.
    * 
    * @param error true if there was an error, false if not.
    */
   virtual void sendEvent(bool error);
};

} // end namespace system
} // end namespace bitmunk
#endif
