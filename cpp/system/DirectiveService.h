/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_system_DirectiveService_H
#define bitmunk_system_DirectiveService_H

#include "bitmunk/node/NodeService.h"

namespace bitmunk
{
namespace system
{

/**
 * A DirectiveService allows a user to start or end a session with a Bitmunk
 * node.
 * 
 * @author Dave Longley
 */
class DirectiveService : public bitmunk::node::NodeService
{
protected:
   /**
    * A lock for manipulating the directive cache.
    */
   monarch::rt::ExclusiveLock mCacheLock;
   
   /**
    * A cache for storing directives. It maps a user ID to map of directive
    * ID to directive. In the current implementation, directives are not
    * persistent. If a node is shutdown, it loses its list of directives.
    * Directives are intended to be transient.
    */
   monarch::rt::DynamicObject mDirectives;
   
public:
   /**
    * Creates a new DirectiveService.
    * 
    * @param node the associated node.
    * @param path the path this servicer handles requests for.
    */
   DirectiveService(bitmunk::node::Node* node, const char* path);
   
   /**
    * Destructs this DirectiveService.
    */
   virtual ~DirectiveService();
   
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
    * Creates a new directive and returns its ID.
    * 
    * HTTP equivalent: POST .../create
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool createDirective(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
   
   /**
    * Deletes an existing directive without processing it.
    * 
    * HTTP equivalent: POST .../delete/<directiveId>
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool deleteDirective(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
   
   /**
    * Gets a user's directives.
    * 
    * HTTP equivalent: GET .../
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getDirectives(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
   
   /**
    * Processes a bitmunk directive and deletes it.
    * 
    * HTTP equivalent: POST .../process/<directiveId>
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool processDirective(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
   
protected:
   /**
    * Generates a random ID for a directive, sets it in the directive, and
    * inserts the directive into the cache. This method assumes the cache lock
    * is engaged and that there is room in the cache.
    * 
    * @param cache a user's directive cache.
    * @param directive the directive to insert.
    */
   virtual void insertDirective(
      monarch::rt::DynamicObject& cache, monarch::rt::DynamicObject& directive);
   
   /**
    * Removes a directive from a user's cache, if it exists.
    * 
    * @param userId the ID of the user.
    * @param directiveId the ID of the directive to remove.
    * @param directive an object to store the removed directive in, if desired.
    */
   virtual void removeDirective(
      bitmunk::common::UserId userId,
      const char* directiveId, monarch::rt::DynamicObject* directive = NULL);
};

} // end namespace system
} // end namespace bitmunk
#endif
