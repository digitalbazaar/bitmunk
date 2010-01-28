/*
 * Copyright (c) 2007-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_customcatalog_CatalogService_H
#define bitmunk_customcatalog_CatalogService_H

#include "bitmunk/node/NodeService.h"
#include "bitmunk/customcatalog/Catalog.h"

namespace bitmunk
{
namespace customcatalog
{

/**
 * A CatalogService services Catalog-related btp actions.
 *
 * @author Dave Longley
 */
class CatalogService : public bitmunk::node::NodeService
{
protected:
   /**
    * The associated catalog.
    */
   Catalog* mCatalog;

public:
   /**
    * Creates a new CatalogService.
    *
    * @param node the associated Bitmunk Node.
    * @param catalog the associated Catalog.
    * @param path the path this servicer handles requests for.
    */
   CatalogService(bitmunk::node::Node* node, Catalog* catalog, const char* path);

   /**
    * Destructs this CatalogService.
    */
   virtual ~CatalogService();

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
    * Receives a token from a client that is testing to see if it has
    * internet access to this catalog. Returns HTTP 201 if successful, an HTTP
    * 400 (bad request) if the token is incorrect.
    *
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool netaccessTest(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);

   /**
    * Gets the server information for a particular user.
    *
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getServerInfo(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);

   /**
    * Creates or updates an existing Ware in a user's catalog.
    *
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool postWare(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);

   /**
    * Populates a Ware in a user's catalog.
    *
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getWare(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);

   /**
    * Populates Wares in a user's catalog based on a list of ids.
    *
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getWareSet(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);

   /**
    * Deletes a Ware in a user's catalog.
    *
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool deleteWare(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);

   /**
    * Gets a payee schemes, given a set of filters, from a user's catalog.
    *
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getPayeeSchemes(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);

   /**
    * Gets a payee scheme from a user's catalog.
    *
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getPayeeScheme(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);

   /**
    * Adds a new payee scheme to a user's catalog.
    *
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool addPayeeScheme(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);

   /**
    * Updates an existing payee scheme in a user's catalog.
    *
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool updatePayeeScheme(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);

   /**
    * Removes a payee scheme from a user's catalog.
    *
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool deletePayeeScheme(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
};

} // end namespace customcatalog
} // end namespace bitmunk
#endif
