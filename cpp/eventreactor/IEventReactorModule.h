/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_eventreactor_IEventReactorModule_H
#define bitmunk_eventreactor_IEventReactorModule_H

#include "bitmunk/eventreactor/EventReactor.h"
#include "monarch/kernel/MicroKernelModuleApi.h"

namespace bitmunk
{
namespace eventreactor
{

/**
 * An IEventReactorModule provides the interface for the EventReactorModule.
 *
 * @author Dave Longley
 */
class IEventReactorModule : public monarch::kernel::MicroKernelModuleApi
{
public:
   /**
    * Creates a new IEventReactorModule.
    */
   IEventReactorModule() {};

   /**
    * Destructs this IEventReactorModule.
    */
   virtual ~IEventReactorModule() {};

   /**
    * Sets a named EventReactor. The EventReactor will be deleted when it is
    * removed. This method assumes that no other EventReactor has been
    * installed for the given name, if one has been, memory will be leaked.
    *
    * @param name the name of the event reactor.
    * @param er the event reactor.
    */
   virtual void setEventReactor(const char* name, EventReactor* er) = 0;

   /**
    * Removes and deletes the EventReactor associated with the given name.
    *
    * @param name the name of the event reactor to delete.
    */
   virtual void removeEventReactor(const char* name) = 0;
};

} // end namespace eventreactor
} // end namespace bitmunk
#endif
