/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_portmapper_FileBrowserModule_H
#define bitmunk_portmapper_FileBrowserModule_H

#include "bitmunk/node/BitmunkModule.h"

namespace bitmunk
{
namespace filebrowser
{

/**
 * A FileBrowserModule is a NodeModule that provides an interface for
 * browsing the Node's filesystem.
 *
 * @author Manu Sporny
 */
class FileBrowserModule : public bitmunk::node::BitmunkModule
{
public:
   /**
    * Creates a new FileBrowserModule.
    */
   FileBrowserModule();

   /**
    * Standard destructor for this FileBrowserModule.
    */
   virtual ~FileBrowserModule();

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

} // end namespace filebrowser
} // end namespace bitmunk
#endif
