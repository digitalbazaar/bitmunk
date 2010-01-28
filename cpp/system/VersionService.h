/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_system_VersionService_H
#define bitmunk_system_VersionService_H

#include "bitmunk/node/NodeService.h"

namespace bitmunk
{
namespace system
{

/**
 * A VersionService returns the version of the software.
 *
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 */
class VersionService : public bitmunk::node::NodeService
{
public:
   /**
    * Creates a new VersionService.
    * 
    * @param node the associated Bitmunk Node.
    * @param path the path this servicer handles requests for.
    */
   VersionService(bitmunk::node::Node* node, const char* path);
   
   /**
    * Destructs this VersionService.
    */
   virtual ~VersionService();
   
   /**
    * Initializes this BtpService.
    * 
    * @return true if initialized, false if an Exception occurred.
    */
   virtual bool initialize();
   
   /**
    * Cleans up this BtpService.
    * 
    * Must be called after initialize() regardless of its return value.
    */
   virtual void cleanup();
   
   /**
    * Get software version info.
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getVersion(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
};

} // end namespace system
} // end namespace bitmunk
#endif
