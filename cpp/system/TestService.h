/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_system_TestService_H
#define bitmunk_system_TestService_H

#include "bitmunk/node/NodeService.h"

namespace bitmunk
{
namespace system
{

/**
 * A TestService provides system control related btp actions.
 *
 * ping: Simple return of 200 OK with no content.
 *  
 * echo: For GET requests will return JSON with "echo" parameter set to null or
 * the content of the "echo" query paramter.  An optional "callback" query
 * parameter can be used to wrap the JSON in a function for JSONP use.  For
 * POST requests will use the JSON "echo" paramter or the "echo" form field.
 * 
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 */
class TestService : public bitmunk::node::NodeService
{
public:
   /**
    * Creates a new TestService.
    * 
    * @param node the associated Bitmunk Node.
    * @param path the path this servicer handles requests for.
    */
   TestService(bitmunk::node::Node* node, const char* path);
   
   /**
    * Destructs this TestService.
    */
   virtual ~TestService();
   
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
    * Simple server ping.
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getPing(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
   
   /**
    * Simple server echo.
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getEcho(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
   
   /**
    * Simple server echo via POST JSON or form "echo" paramter..
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool postEcho(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
};

} // end namespace system
} // end namespace bitmunk
#endif
