/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_portmapper_FileBrowserService_H
#define bitmunk_portmapper_FileBrowserService_H

#include "bitmunk/node/NodeService.h"

namespace bitmunk
{
namespace filebrowser
{

/**
 * A FileBrowserService handles requests to browse the local file system.
 *
 * @author Manu Sporny
 */
class FileBrowserService : public bitmunk::node::NodeService
{
public:
   /**
    * Creates a new file browser service.
    *
    * @param node the node associated with this service.
    * @param path the web server path associated with this file browser service.
    */
   FileBrowserService(bitmunk::node::Node* node, const char* path);

   /**
    * Standard destructor.
    */
   virtual ~FileBrowserService();

   /**
    * Initializes this file browser service
    *
    * @return true if initialized, false if an Exception occurred.
    */
   virtual bool initialize();

   /**
    * Cleans up this file browser service.
    *
    * This method must be called after initialize() regardless of its
    * return value.
    */
   virtual void cleanup();

   /**
    * Retrieves the contents of a directory.
    *
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getDirectoryContents(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
};

} // end namespace filebrowser
} // end namespace bitmunk
#endif
