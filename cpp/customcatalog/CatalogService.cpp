/*
 * Copyright (c) 2007-2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/customcatalog/CatalogService.h"

#include "bitmunk/common/BitmunkValidator.h"
#include "bitmunk/node/BtpActionDelegate.h"
#include "bitmunk/node/RestResourceHandler.h"
#include "bitmunk/medialibrary/IMediaLibrary.h"
#include "monarch/data/json/JsonReader.h"

using namespace monarch::data::json;
using namespace monarch::net;
using namespace monarch::rt;
using namespace bitmunk::common;
using namespace bitmunk::customcatalog;
using namespace bitmunk::medialibrary;
using namespace bitmunk::protocol;
using namespace bitmunk::node;
using namespace std;
namespace v = monarch::validation;

typedef BtpActionDelegate<CatalogService> Handler;

CatalogService::CatalogService(Node* node, Catalog* catalog, const char* path) :
   NodeService(node, path),
   mCatalog(catalog)
{
}

CatalogService::~CatalogService()
{
}

bool CatalogService::initialize()
{
   // netaccess/test
   {
      RestResourceHandlerRef test = new RestResourceHandler();
      addResource("/netaccess/test", test);

      /**
       * Responds to a token sent to this server to determine if another
       * entity can access it over the net. If the token matches the unique
       * token used by this catalog, this method will return with success.
       *
       * @tags payswarm-api public-api
       *
       * @qparam nodeuser the user on the node that should perform the
       *                  requested action on the caller's behalf.
       *
       * @return an HTTP 201 code if successful, an exception if not.
       */
      {
         // POST .../netaccess/test
         ResourceHandler h = new Handler(
            mNode, this, &CatalogService::netaccessTest,
            BtpAction::AuthRequired);

         v::ValidatorRef qValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         v::ValidatorRef validator = new v::Map(
            "ip", new v::Type(String),
            "token", new v::All(
               new v::Type(String),
               new v::Max(50, "Token must be 50 characters or less."),
               NULL),
            NULL);

         test->addHandler(h, BtpMessage::Post, 0, &qValidator, &validator);
      }
   }

   // server
   {
      RestResourceHandlerRef server = new RestResourceHandler();
      addResource("/server", server);

      /**
       * Gets a user's server information, including their server ID, server
       * token, and server url.
       *
       * @tags payswarm-api public-api
       *
       * @qparam nodeuser the user on the node that should perform the
       *                  requested action on the caller's behalf.
       *
       * @return an HTTP 200 code if successful, an exception if not.
       */
      {
         // GET .../server
         ResourceHandler h = new Handler(
            mNode, this, &CatalogService::getServerInfo,
            BtpAction::AuthRequired);

         //FIXME: Why don't we require do setSameUserRequired on this call?
         // isn't the server token supposed to be semi-secret?

         v::ValidatorRef qValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         server->addHandler(h, BtpMessage::Get, 0, &qValidator);
      }
   }

   // wares
   {
      RestResourceHandlerRef wares = new RestResourceHandler();
      addResource("/wares", wares);

      // Note: PeerBuy Ware IDs look like this:
      // bitmunk:file:<mediaId>[-<fileId>[-<bfpId>]]

      /**
       * Creates or updates an existing Ware in a user's catalog with the
       * given ware.
       *
       * @tags payswarm-api public-api
       *
       * @qparam nodeuser the user on the node that should perform the
       *                  requested action on the caller's behalf.
       *
       * @return HTTP 200 OK if the add was successful, an exception otherwise.
       */
      {
         // POST .../wares

         // a nodeuser must be specified when updating a user's catalog
         // and that user must be the same one that is using the service
         Handler* handler = new Handler(
            mNode, this, &CatalogService::postWare,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef qValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         v::ValidatorRef validator = new v::All(
            new v::Map(
               "id", new v::All(
                  new v::Type(String),
                  new v::Min(1, "Ware ID too short. 1 character minimum."),
                  NULL),
               "mediaId", new v::Int(v::Int::Positive),
               "description", new v::Optional(new v::Type(String)),
               NULL),
            // either a single file info or an array of one file info
            new v::Any(
               new v::Map(
                  "fileInfo", new v::Map(
                     "id", BitmunkValidator::fileId(),
                     NULL),
                  NULL),
               new v::Map(
                  "fileInfos", new v::All(
                     new v::Type(Array),
                     new v::Min(1, "One file info object must be provided."),
                     new v::Max(1, "One file info object must be provided."),
                     new v::Each(new v::Map(
                        "id", BitmunkValidator::fileId(),
                        NULL)),
                     NULL),
                  NULL),
               NULL),
            // either a payee scheme OR a list of payees is permitted
            new v::Any(
               new v::Map(
                  "payeeSchemeId", new v::Int(v::Int::Positive),
                   NULL),
               new v::Map(
                  "payees", BitmunkValidator::dataPayees(),
                  NULL),
               NULL),
            NULL);

         wares->addHandler(h, BtpMessage::Post, 0, &qValidator, &validator);
      }

      /**
       * Populates Wares that is contained in a user's catalog based on ware
       * IDs or file IDs.
       *
       * @tags payswarm-api public-api
       *
       * @qparam nodeuser the user on the node that should perform the
       *                  requested action on the caller's behalf.
       *
       * @return HTTP 200 OK and a Ware if the ware exists, an exception
       *         otherwise.
       */
      {
         // GET .../wares?
         //    [id=<wareId>][&id=<wareId>...]
         //    [&fileId=<fileId>][&fileId=<fileId>]
         //    [&details=<true|false>]
         ResourceHandler h = new Handler(
            mNode, this, &CatalogService::getWareSet,
            BtpAction::AuthOptional);

         // FIXME: confusing due to array query
         v::ValidatorRef qValidator = new v::Map(
            "id", new v::Optional(new v::Each(new v::Type(String))),
            "fileId", new v::Optional(new v::Each(new v::Type(String))),
            "details", new v::Optional(
               new v::Each(new v::Regex("^(true|false)$"))),
            "nodeuser", new v::Each(new v::Int(v::Int::Positive)),
            NULL);

         wares->addHandler(
            h, BtpMessage::Get, 0, &qValidator, NULL,
            RestResourceHandler::ArrayQuery);
      }

      /**
       * Populates a Ware that is contained in a user's catalog.
       *
       * @tags payswarm-api public-api
       * @pparam wareId the ID of the ware to populate.
       * @qparam nodeuser the user on the node that should perform the
       *                  requested action on the caller's behalf.
       *
       * @return HTTP 200 OK and a Ware if the ware exists, an exception
       *         otherwise.
       */
      {
         // GET .../wares/<wareId>
         ResourceHandler h = new Handler(
            mNode, this, &CatalogService::getWare,
            BtpAction::AuthOptional);

         v::ValidatorRef qValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         wares->addHandler(h, BtpMessage::Get, 1, &qValidator);
      }

      /**
       * Deletes a Ware that is contained in a user's catalog.
       *
       * @tags payswarm-api public-api
       * @pparam wareId the ID of the ware to delete.
       * @qparam nodeuser the user on the node that should perform the
       *                  requested action on the caller's behalf.
       *
       * @return HTTP 200 OK if the ware was deleted successfully, or an
       *         exception otherwise.
       */
      {
         // DELETE .../wares/<wareId>

         // a nodeuser must be specified when updating a user's catalog
         // and that user must be the same one that is using the service
         Handler* handler = new Handler(
            mNode, this, &CatalogService::deleteWare,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef qValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         wares->addHandler(h, BtpMessage::Delete, 1, &qValidator);
      }
   }

   // payee schemes
   {
      RestResourceHandlerRef payeeSchemes = new RestResourceHandler();
      addResource("/payees/schemes", payeeSchemes);

      /**
       * Gets a set of payee schemes from a user's custom catalog given a set
       * of filters.
       *
       * @tags payswarm-api public-api
       * @qparam nodeuser the user on the node that should perform the
       *                  requested action on the caller's behalf.
       *
       * @return an HTTP 200 code if successful, an exception if not.
       */
      {
         // GET .../payees/schemes
         //    [&details=<true|false>]
         ResourceHandler h = new Handler(
            mNode, this, &CatalogService::getPayeeSchemes,
            BtpAction::AuthRequired);

         v::ValidatorRef qValidator = new v::Map(
            "details", new v::Optional(
               new v::Each(new v::Regex("^(true|false)$"))),
            "nodeuser", new v::Each(new v::Int(v::Int::Positive)),
            NULL);

         payeeSchemes->addHandler(
            h, BtpMessage::Get, 0, &qValidator, NULL,
            RestResourceHandler::ArrayQuery);
      }

      /**
       * Gets an existing payee scheme from a user's custom catalog.
       *
       * @tags payswarm-api public-api
       * @pparam schemeId the ID of the payee scheme to get.
       * @qparam nodeuser the user on the node that should perform the
       *                  requested action on the caller's behalf.
       *
       * @return an HTTP 200 code if successful, an exception if not.
       */
      {
         // GET .../payees/schemes/<schemeId>
         ResourceHandler h = new Handler(
            mNode, this, &CatalogService::getPayeeScheme,
            BtpAction::AuthRequired);

         v::ValidatorRef qValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         payeeSchemes->addHandler(h, BtpMessage::Get, 1, &qValidator);
      }

      /**
       * Creates a new payee scheme in a user's custom catalog. The message
       * body must contain a PayeeScheme object.
       *
       * @tags payswarm-api public-api
       * @qparam nodeuser the user on the node that should perform the
       *                  requested action on the caller's behalf.
       *
       * @return an HTTP 200 code if successful, an exception if not.
       */
      {
         // POST .../payees/schemes
         Handler* handler = new Handler(
            mNode, this, &CatalogService::addPayeeScheme,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef qValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         v::ValidatorRef validator = new v::Map(
            "description", new v::All(
               new v::Type(String),
               new v::Min(1, "Description too short. 1 character minimum."),
               NULL),
            "payees", BitmunkValidator::dataPayees(),
            NULL);

         payeeSchemes->addHandler(
            h, BtpMessage::Post, 0, &qValidator, &validator);
      }

      /**
       * Updates an existing payee scheme in a user's custom catalog. The
       * message body must contain an array of Payee objects.
       *
       * @tags payswarm-api public-api
       * @pparam schemeId the ID of the payee scheme to update.
       * @qparam nodeuser the user on the node that should perform the
       *                  requested action on the caller's behalf.
       *
       * @return an HTTP 200 code if successful, an exception if not.
       */
      {
         // POST .../payees/schemes/<schemeId>
         Handler* handler = new Handler(
            mNode, this, &CatalogService::updatePayeeScheme,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef qValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         // a nodeuser must be specified when updating a user's catalog
         // and that user must be the same one that is using the service
         v::ValidatorRef validator = new v::Map(
            "description", new v::All(
               new v::Type(String),
               new v::Min(1, "Description too short. 1 character minimum."),
               NULL),
            "payees", BitmunkValidator::dataPayees(),
            NULL);

         payeeSchemes->addHandler(
            h, BtpMessage::Post, 1, &qValidator, &validator);
      }

      /**
       * Deletes an existing payee scheme from a user's custom catalog.
       *
       * @tags payswarm-api public-api
       * @pparam schemeId the ID associated with the scheme that should be
       *                  removed.
       * @qparam nodeuser the user on the node that should perform the
       *                  requested action on the caller's behalf.
       *
       * @return an HTTP 201 code if successful, an exception if not.
       */
      // DELETE .../payees/schemes/<schemeId>
      {
         // a nodeuser must be specified when updating a user's catalog
         // and that user must be the same one that is using the service
         Handler* handler = new Handler(
            mNode, this, &CatalogService::deletePayeeScheme,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef qValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         payeeSchemes->addHandler(h, BtpMessage::Delete, 1, &qValidator);
      }
   }

   return true;
}

void CatalogService::cleanup()
{
   // remove resources
   removeResource("/netaccess/test");
   removeResource("/server");
   removeResource("/wares");
   removeResource("/payees/schemes");
}

bool CatalogService::netaccessTest(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = false;

   // get nodeuser
   DynamicObject vars;
   action->getResourceQuery(vars);
   UserId userId = vars["nodeuser"]->getUInt64();

   // get netaccess token
   string token;
   if(mCatalog->getConfigValue(userId, "netaccessToken", token))
   {
      // check token
      if(strcmp(token.c_str(), in["token"]->getString()) == 0)
      {
         rval = true;
         out.setNull();
      }
      else
      {
         ExceptionRef e = new Exception(
            "Incorrect netaccess token.",
            "bitmunk.catalog.InvalidNetAccessToken");
         Exception::set(e);
         action->getResponse()->getHeader()->setStatus(400, "Bad Request");
      }
   }
   else
   {
      ExceptionRef e = new Exception(
         "Could not get netaccess token to check against.",
         "bitmunk.catalog.NetAccessTokenNotFound");
      Exception::set(e);
   }

   return rval;
}

bool CatalogService::getServerInfo(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = false;

   // get nodeuser
   DynamicObject vars;
   action->getResourceQuery(vars);
   UserId userId = vars["nodeuser"]->getUInt64();

   // get seller information
   Seller seller;
   string serverToken;
   if(mCatalog->populateSeller(userId, seller, serverToken))
   {
      rval = true;
      out["serverUrl"] = seller["url"];
      out["serverId"] = seller["serverId"];
      out["serverToken"] = serverToken.c_str();

      // get any netaccess exception
      string json;
      if(mCatalog->getConfigValue(userId, "netaccessException", json) &&
         json.length() > 0)
      {
         DynamicObject dyno;
         rval = JsonReader::readFromString(dyno, json.c_str(), json.length());
         out["netaccessException"] = dyno;
      }
   }

   return rval;
}

bool CatalogService::postWare(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = false;

   // get user ID for action user
   UserId userId = action->getInMessage()->getUserId();

   // build ware from input
   Ware w;
   w["id"] = in["id"];
   w["mediaId"] = in["mediaId"];
   w["description"] = in["description"];
   if(in->hasMember("fileInfo"))
   {
      w["fileInfos"]->append(in["fileInfo"]);
   }
   else
   {
      w["fileInfos"] = in["fileInfos"];
   }
   if(in->hasMember("payeeSchemeId"))
   {
      w["payeeSchemeId"] = in["payeeSchemeId"];
   }
   else
   {
      // use given payees list
      w["payees"] = in["payees"];
   }

   // update ware
   if((rval = mCatalog->updateWare(userId, w)))
   {
      // return ID of ware that was updated and its payee scheme ID
      out["wareId"] = w["id"];
      out["payeeSchemeId"] = w["payeeSchemeId"];
   }
   else
   {
      ExceptionRef e = new Exception(
         "Failed to update the ware in the catalog.",
         "bitmunk.catalog.UpdateWareFailed");
      e->getDetails()["ware"] = w;
      Exception::push(e);
   }

   return rval;
}

bool CatalogService::getWare(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = false;

   // get parameters
   DynamicObject params;
   action->getResourceParams(params);

   // get user ID for nodeuser
   DynamicObject vars;
   action->getResourceQuery(vars);
   UserId userId = vars["nodeuser"]->getUInt64();

   // populate ware (use "out" as Ware)
   out["id"] = params[0]->getString();
   rval = mCatalog->populateWare(userId, out);

   return rval;
}

bool CatalogService::getWareSet(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = false;

   // get the user ID
   UserId userId = action->getInMessage()->getUserId();

   // get file set filters
   DynamicObject filters;
   action->getResourceQuery(filters, true);

   // get the query parameters from the set of filters that were passed in
   DynamicObject query;
   // ids/fileids are in a resource query array
   if(filters->hasMember("id"))
   {
      query["ids"] = filters["id"];
   }
   if(filters->hasMember("fileId"))
   {
      query["fileIds"] = filters["fileId"];
   }

   if(filters->hasMember("default"))
   {
      query["default"] =
         filters["default"][filters["default"]->length() - 1]->getBoolean();
   }
   else
   {
      query["default"] = true;
   }
   // get the ware set that corresponds with the given query filters
   ResourceSet wareSet;
   if((rval = mCatalog->populateWareSet(userId, query, wareSet)))
   {
      // return the set of files retrieved via the query
      out = wareSet;
   }

   return rval;
}

bool CatalogService::deleteWare(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = false;

   // get parameters
   DynamicObject params;
   action->getResourceParams(params);

   // get user ID for action user
   UserId userId = action->getInMessage()->getUserId();

   // remove ware
   Ware w;
   w["id"] = params[0]->getString();
   rval = mCatalog->removeWare(userId, w);
   out.setNull();

   return rval;
}

bool CatalogService::getPayeeSchemes(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = false;

   // get parameters
   DynamicObject params;
   action->getResourceParams(params);

   // get query filters
   DynamicObject filters;
   action->getResourceQuery(filters);
   UserId userId = filters["nodeuser"]->getUInt64();

   // get the query parameters from the set of filters that were passed in
   DynamicObject query;
   if(filters->hasMember("default"))
   {
      query["default"] = filters["default"];
   }
   else
   {
      query["default"] = true;
   }

   // generate the resource set
   rval = mCatalog->populatePayeeSchemes(userId, query, out);

   return rval;
}

bool CatalogService::getPayeeScheme(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = false;

   // get parameters
   DynamicObject params;
   action->getResourceParams(params);

   // get user ID for nodeuser
   DynamicObject filters;
   action->getResourceQuery(filters);
   UserId userId = filters["nodeuser"]->getUInt64();

   DynamicObject query;
   if(filters->hasMember("default"))
   {
      query["default"] = filters["default"];
   }
   else
   {
      query["default"] = true;
   }

   // get payee scheme
   PayeeSchemeId psId = params[0]->getUInt32();
   out["id"] = psId;
   rval = mCatalog->populatePayeeScheme(userId, query, out);

   return rval;
}

bool CatalogService::addPayeeScheme(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = false;

   // get user ID for action user
   UserId userId = action->getInMessage()->getUserId();

   // create new payee scheme
   PayeeSchemeId psId = 0;
   rval = mCatalog->updatePayeeScheme(
      userId, psId, in["description"]->getString(), in["payees"]);
   if(rval)
   {
      out["payeeSchemeId"] = psId;
   }

   return rval;
}

bool CatalogService::updatePayeeScheme(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = false;

   // get parameters
   DynamicObject params;
   action->getResourceParams(params);

   // get user ID for action user
   UserId userId = action->getInMessage()->getUserId();

   // update payee scheme
   PayeeSchemeId psId = params[0]->getUInt32();
   rval = mCatalog->updatePayeeScheme(
      userId, psId, in["description"]->getString(), in["payees"]);
   if(rval)
   {
      out["payeeSchemeId"] = psId;
   }

   return rval;
}

bool CatalogService::deletePayeeScheme(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = false;

   // get parameters
   DynamicObject params;
   action->getResourceParams(params);

   // get user ID for action user
   UserId userId = action->getInMessage()->getUserId();

   // remove payee scheme
   PayeeSchemeId psId = params[0]->getUInt32();
   rval = mCatalog->removePayeeScheme(userId, psId);
   out.setNull();

   return rval;
}
