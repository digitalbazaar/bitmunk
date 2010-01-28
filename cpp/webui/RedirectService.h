/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_webui_RedirectService_H
#define bitmunk_webui_RedirectService_H

#include "bitmunk/node/NodeService.h"
#include "bitmunk/node/Node.h"

namespace bitmunk
{
namespace webui
{

/**
 * A RedirectService redirects one url to another.
 * 
 * @author Dave Longley
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 */
class RedirectService : public bitmunk::node::NodeService
{
public:
   /**
    * Creates a new RedirectService.
    * 
    * @param node the associated Bitmunk Node.
    * @param path the path this servicer handles requests for.
    * @param source the source path to redirect from.
    * @param target the target path to redirect to.
    */
   RedirectService(
      bitmunk::node::Node* node, const char* path,
      const char* source, const char* target);
   
   /**
    * Destructs this RedirectService.
    */
   virtual ~RedirectService();
   
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
    * Redirects a url.
    * 
    * @param action the BtpAction.
    */
   virtual void redirect(bitmunk::protocol::BtpAction* action);
   
protected:
   /**
    * Source path
    */
   char* mSource;
   
   /**
    * Target path
    */
   char* mTarget; 
};

} // end namespace webui
} // end namespace bitmunk
#endif
