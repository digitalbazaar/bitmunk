/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_webui_ForceSslService_H
#define bitmunk_webui_ForceSslService_H

#include "bitmunk/node/NodeService.h"
#include "bitmunk/node/Node.h"

namespace bitmunk
{
namespace webui
{

/**
 * A ForceSslService redirects all webui urls from http to https.
 * 
 * @author Dave Longley
 */
class ForceSslService : public bitmunk::node::NodeService
{
public:
   /**
    * Creates a new ForceSslService.
    * 
    * @param node the associated Bitmunk Node.
    * @param path the path this servicer handles requests for.
    */
   ForceSslService(bitmunk::node::Node* node, const char* path);
   
   /**
    * Destructs this ForceSslService.
    */
   virtual ~ForceSslService();
   
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
    * ForceSsls any http url to https.
    * 
    * @param action the BtpAction.
    */
   virtual void redirect(bitmunk::protocol::BtpAction* action);
};

} // end namespace webui
} // end namespace bitmunk
#endif
