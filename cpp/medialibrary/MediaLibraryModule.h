/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_medialibrary_MediaLibraryModule_H
#define bitmunk_medialibrary_MediaLibraryModule_H

#include "bitmunk/nodemodule/NodeModule.h"

// module logging category
extern monarch::logging::Category* BM_MEDIALIBRARY_CAT;

namespace bitmunk
{
namespace medialibrary
{

class MediaLibrary;

/**
 * An MediaLibraryModule is a NodeModule that provides an interface for
 * managing a media library.
 *
 * @author Manu Sporny
 */
class MediaLibraryModule : public bitmunk::node::NodeModule
{
protected:
   /**
    * The media library which doubles as the interface for this module.
    */
   MediaLibrary* mMediaLibrary;

public:
   /**
    * Creates a new MediaLibraryModule.
    */
   MediaLibraryModule();

   /**
    * Destructs this MediaLibraryModule.
    */
   virtual ~MediaLibraryModule();

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

} // end namespace medialibrary
} // end namespace bitmunk
#endif
