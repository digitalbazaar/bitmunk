/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_eventreactor_EventReactorModule_H
#define bitmunk_eventreactor_EventReactorModule_H

#include "bitmunk/node/NodeModule.h"

// module logging category
extern monarch::logging::Category* BM_EVENTREACTOR_CAT;

namespace bitmunk
{
namespace eventreactor
{

class IEventReactorModule;

/**
 * An EventReactorModule is a NodeModule that provides an interface for
 * reacting to events.
 * 
 * @author Dave Longley
 */
class EventReactorModule : public bitmunk::node::NodeModule
{
protected:
   /**
    * The event reactor module interface.
    */
   IEventReactorModule* mInterface;
   
public:
   /**
    * Creates a new EventReactorModule.
    */
   EventReactorModule();
   
   /**
    * Destructs this EventReactorModule.
    */
   virtual ~EventReactorModule();
   
   /**
    * Adds additional dependency information. This includes dependencies
    * beyond the Bitmunk Node module dependencies.
    *
    * @param depInfo the dependency information to update.
    */
   virtual void addDependencyInfo(monarch::rt::DynamicObject& depInfo);

   /**
    * Initializes this Module with the passed Node.
    *
    * @param node the Node.
    *
    * @return true if initialized, false if an Exception occurred.
    */
   virtual bool initialize(bitmunk::node::Node* node);

   /**
    * Cleans up this Module just prior to its unloading.
    *
    * @param node the Node.
    */
   virtual void cleanup(bitmunk::node::Node* node);

   /**
    * Gets the API for this NodeModule.
    *
    * @param node the Node.
    *
    * @return the API for this NodeModule.
    */
   virtual monarch::kernel::MicroKernelModuleApi* getApi(
      bitmunk::node::Node* node);
};

} // end namespace eventreactor
} // end namespace bitmunk
#endif
