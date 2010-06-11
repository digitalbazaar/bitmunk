/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/purchase/ContractService.h"

#include "bitmunk/common/NegotiateInterface.h"
#include "bitmunk/common/Signer.h"
#include "bitmunk/purchase/LicenseAcquirer.h"
#include "bitmunk/purchase/DownloadManager.h"
#include "bitmunk/purchase/DownloadStateInitializer.h"
#include "bitmunk/purchase/FileAssembler.h"
#include "bitmunk/purchase/IPurchaseModule.h"
#include "bitmunk/purchase/PurchaseDatabase.h"
#include "bitmunk/purchase/PurchaseModule.h"
#include "bitmunk/purchase/Purchaser.h"
#include "bitmunk/node/BtpActionDelegate.h"
#include "bitmunk/node/RestResourceHandler.h"
#include "monarch/event/EventWaiter.h"
#include "monarch/validation/Validation.h"

using namespace std;
using namespace monarch::event;
using namespace monarch::fiber;
using namespace monarch::io;
using namespace monarch::net;
using namespace monarch::rt;
using namespace bitmunk::common;
using namespace bitmunk::protocol;
using namespace bitmunk::node;
using namespace bitmunk::purchase;
namespace v = monarch::validation;

typedef BtpActionDelegate<ContractService> Handler;

#define EVENT_PURCHASE           "bitmunk.purchase"
#define EVENT_DOWNLOAD_STATE     EVENT_PURCHASE ".DownloadState"

ContractService::ContractService(Node* node, const char* path) :
   NodeService(node, path)
{
}

ContractService::~ContractService()
{
}

bool ContractService::initialize()
{
   // download states
   {
      RestResourceHandlerRef states = new RestResourceHandler();
      RestResourceHandlerRef deleteState = new RestResourceHandler();
      addResource("/downloadstates", states);
      addResource("/downloadstates/delete", deleteState);

      // POST .../downloadstates
      {
         Handler* handler = new Handler(
            mNode, this, &ContractService::createDownloadState,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef queryValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         v::ValidatorRef validator = new v::Map(
            "ware", new v::Map(
               "id", new v::Equals(
                  "bitmunk:bundle",
                  "Only peerbuy 'bundle' wares are permitted."),
               "mediaId", new v::Optional(new v::Int(v::Int::Positive)),
               "fileInfos", new v::Optional(new v::Each(
                  new v::Map(
                     "id", new v::Type(String),
                     "mediaId", new v::Int(v::Int::Positive),
                     NULL))),
               NULL),
            "sellerLimit", new v::Optional(new v::Int(v::Int::Positive)),
            "sellers", new v::Optional(new v::Each(
               new v::Map(
                  "userId", new v::Int(v::Int::Positive),
                  "serverId", new v::Int(v::Int::Positive),
                  // FIXME: This needs to ensure that value passed in is a URL
                  "url", new v::Type(String),
                  NULL))),
            NULL);

         states->addHandler(
            h, BtpMessage::Post, 0, &queryValidator, &validator);
      }

      // GET .../downloadstates
      {
         Handler* handler = new Handler(
            mNode, this, &ContractService::getDownloadStates,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         // FIXME: incomplete *must* be on at present
         v::ValidatorRef queryValidator = new v::Map(
            "incomplete", new v::Regex("^true$"),
            "licenseAcquired", new v::Optional(new v::Regex("^(true|false)$")),
            "downloadStarted", new v::Optional(new v::Regex("^(true|false)$")),
            "processing", new v::Optional(new v::Regex("^(true|false)$")),
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         states->addHandler(h, BtpMessage::Get, 0, &queryValidator);
      }

      // GET .../downloadstates/<downloadStateId>
      {
         Handler* handler = new Handler(
            mNode, this, &ContractService::getDownloadState,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef queryValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         states->addHandler(h, BtpMessage::Get, 1, &queryValidator);
      }

      // DELETE .../downloadstates/<downloadStateId>
      {
         Handler* handler = new Handler(
            mNode, this, &ContractService::deleteDownloadState,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef queryValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            "details", new v::Optional(new v::Regex("^(all|state)$")),
            NULL);

         states->addHandler(h, BtpMessage::Delete, 1, &queryValidator);
      }

      // POST .../downloadstates/delete
      {
         Handler* handler = new Handler(
            mNode, this, &ContractService::deleteDownloadState,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef validator = new v::Map(
            "downloadStateId", new v::Int(v::Int::Positive),
            NULL);

         v::ValidatorRef queryValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         deleteState->addHandler(
            h, BtpMessage::Post, 0, &queryValidator, &validator);
      }
   }

   // initialize
   {
      RestResourceHandlerRef initialize = new RestResourceHandler();
      addResource("/initialize", initialize);

      // POST .../initialize
      {
         Handler* handler = new Handler(
            mNode, this, &ContractService::initializeDownloadState,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef queryValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         v::ValidatorRef validator = new v::Map(
            "downloadStateId", new v::Int(v::Int::Positive),
            NULL);

         initialize->addHandler(
            h, BtpMessage::Post, 0, &queryValidator, &validator);
      }
   }

   // license
   {
      RestResourceHandlerRef license = new RestResourceHandler();
      addResource("/license", license);

      // POST .../license
      {
         Handler* handler = new Handler(
            mNode, this, &ContractService::acquireLicense,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef queryValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         v::ValidatorRef validator = new v::Map(
            "downloadStateId", new v::Int(v::Int::Positive),
            NULL);

         license->addHandler(
            h, BtpMessage::Post, 0, &queryValidator, &validator);
      }
   }

   // download
   {
      RestResourceHandlerRef download = new RestResourceHandler();
      addResource("/download", download);

      // POST .../download
      {
         Handler* handler = new Handler(
            mNode, this, &ContractService::downloadContractData,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef queryValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         v::ValidatorRef validator = new v::Map(
            "downloadStateId", new v::Int(v::Int::Positive),
            "preferences", new v::Optional(new v::Map(
               "fast", new v::Type(Boolean),
               "accountId", new v::Int(v::Int::Positive),
               "price", new v::Map(
                  "max", new v::Optional(new v::Regex(
                     "^[0-9]+\\.[0-9]{2,7}$",
                     "Monetary amount must be of the format 'x.xx'. "
                     "Example: 10.00")),
                  NULL),
               "sellerLimit", new v::Int(v::Int::Positive),
               // FIXME: add optional list of preferred sellers
               NULL)),
            NULL);

         download->addHandler(
            h, BtpMessage::Post, 0, &queryValidator, &validator);
      }
   }

   // pause
   {
      RestResourceHandlerRef pause = new RestResourceHandler();
      addResource("/pause", pause);

      // POST .../pause
      {
         Handler* handler = new Handler(
            mNode, this, &ContractService::pauseDownload,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef queryValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         v::ValidatorRef validator = new v::Map(
            "downloadStateId", new v::Int(v::Int::Positive),
            NULL);

         pause->addHandler(h, BtpMessage::Post, 0, &queryValidator, &validator);
      }
   }

   // purchase
   {
      RestResourceHandlerRef purchase = new RestResourceHandler();
      addResource("/purchase", purchase);

      // POST .../purchase
      {
         Handler* handler = new Handler(
            mNode, this, &ContractService::purchaseContractData,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef queryValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         v::ValidatorRef validator = new v::Map(
            "downloadStateId", new v::Int(v::Int::Positive),
            NULL);

         purchase->addHandler(
            h, BtpMessage::Post, 0, &queryValidator, &validator);
      }
   }

   // assemble
   {
      RestResourceHandlerRef assemble = new RestResourceHandler();
      addResource("/assemble", assemble);

      // POST .../assemble
      {
         Handler* handler = new Handler(
            mNode, this, &ContractService::assembleContractData,
            BtpAction::AuthRequired);
         handler->setSameUserRequired(true);
         ResourceHandler h = handler;

         v::ValidatorRef queryValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         v::ValidatorRef validator = new v::Map(
            "downloadStateId", new v::Int(v::Int::Positive),
            NULL);

         assemble->addHandler(
            h, BtpMessage::Post, 0, &queryValidator, &validator);
      }
   }

   return true;
}

void ContractService::cleanup()
{
   // remove resources
   removeResource("/downloadstates");
   removeResource("/downloadstates/delete");
   removeResource("/initialize");
   removeResource("/license");
   removeResource("/download");
   removeResource("/pause");
   removeResource("/purchase");
   removeResource("/assemble");
}

bool ContractService::createDownloadState(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = true;

   // interpret "in" as request to create download state
   // request should include:
   // WareId to use AND PurchasePreferences object

   MO_CAT_DEBUG(BM_PURCHASE_CAT, "Creating new DownloadState...");

   // get the action user
   UserId userId = action->getInMessage()->getUserId();

   // get buyer's login data
   User user;
   ProfileRef profile;
   if((rval = mNode->getLoginData(userId, &user, &profile)))
   {
      // initialize download state
      DownloadState ds;
      ds["id"] = 0;
      ds["userId"] = userId;
      ds["ware"] = in["ware"];
      ds["contract"]["version"] = "3.0";
      ds["contract"]["id"] = 0;
      ds["contract"]["media"]["id"] = in["ware"]["mediaId"];
      ds["contract"]["buyer"]["userId"] = userId;
      ds["contract"]["buyer"]["username"] = user["username"];
      ds["contract"]["buyer"]["profileId"] = profile->getId();
      ds["preferences"]->setType(Map);
      if(in->hasMember("sellers"))
      {
         ds["preferences"]["sellers"] = in["sellers"];
      }
      if(in->hasMember("sellerLimit"))
      {
         ds["preferences"]["sellerLimit"] = in["sellerLimit"];
      }
      ds["totalMedPrice"] = "0.00";
      ds["progress"]->setType(Map);
      ds["remainingPieces"] = 0;
      ds["initialized"] = false;
      ds["licenseAcquired"] = false;
      ds["licensePurchased"] = false;
      ds["dataPurchased"] = false;
      ds["filesAssembled"] = false;
      ds["activeSellers"]->setType(Map);
      ds["blacklist"]->setType(Map);

      // insert new download state entry into database
      PurchaseDatabase* pd = PurchaseDatabase::getInstance(mNode);
      if((rval = pd->insertDownloadState(ds)))
      {
         // send back DownloadState id
         out["downloadStateId"] = ds["id"]->getUInt64();

         // send download state created event
         Event e;
         e["type"] = EVENT_DOWNLOAD_STATE ".created";
         BM_ID_SET(e["details"]["userId"], userId);
         e["details"]["downloadState"] = ds;
         e["details"]["downloadStateId"] = ds["id"]->getString();
         mNode->getEventController()->schedule(e);
      }
   }
   else
   {
      ExceptionRef e = new Exception(
         "Could not create download state.",
         "bitmunk.purchase.ContractService.InvalidUser");
      Exception::push(e);
   }

   return rval;
}

bool ContractService::getDownloadStates(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = true;

   MO_CAT_DEBUG(BM_PURCHASE_CAT, "Retrieving download states...");

   // get the action user
   UserId userId = action->getInMessage()->getUserId();

   // get the query variables
   DynamicObject vars;
   action->getResourceQuery(vars);
   //bool hasIncomplete = vars->hasMember("incomplete");
   bool hasDownloadStarted = vars->hasMember("downloadStarted");
   bool hasLicenseAcquired = vars->hasMember("licenseAcquired");
   bool hasProcessing = vars->hasMember("processing");
   //bool incomplete = vars["incomplete"]->getBoolean();
   bool downloadStarted = vars["downloadStarted"]->getBoolean();
   bool licenseAcquired = vars["licenseAcquired"]->getBoolean();
   bool processing = vars["processing"]->getBoolean();

   // FIXME: right now "incomplete" filter *must* be true, so only
   // make call to get incomplete download states

   // retrieve the set of incomplete download states and filter them
   // based on the given filter
   DownloadStateList dsList;
   DownloadStateList filteredDownloadStates;
   filteredDownloadStates->setType(Array);
   PurchaseDatabase* pd = PurchaseDatabase::getInstance(mNode);
   if((rval = pd->getIncompleteDownloadStates(userId, dsList)))
   {
      // go through list of incomplete download states and filter out
      // those that do not match the user's query
      DynamicObjectIterator di = dsList.getIterator();
      while(di->hasNext())
      {
         DownloadState& ds = di->next();

         // default to including download state in list of incompletes
         bool includeDownloadState = true;

         // check to see if the download started filter was specified,
         // and if it was, check to see if download state matches it
         if(hasDownloadStarted)
         {
            includeDownloadState =
               (downloadStarted == ds["downloadStarted"]->getBoolean());
         }

         // check to see if the license acquisition filter was specified,
         // and if it was, check to see if download state matches it
         if(includeDownloadState && hasLicenseAcquired)
         {
            includeDownloadState =
               (licenseAcquired == ds["licenseAcquired"]->getBoolean());
         }

         // check to see if processing filter was specified, and if
         // it was, check to see if download state matches it
         if(includeDownloadState && hasProcessing)
         {
            if((!ds["processing"]->getBoolean() && processing) ||
               (ds["processing"]->getBoolean() && !processing))
            {
               includeDownloadState = false;
            }
         }

         // check to see if the previous filters succeeded
         if(includeDownloadState)
         {
            filteredDownloadStates->append(ds);
         }
      }
   }
   else
   {
      ExceptionRef e = new Exception(
         "Could not retrieve incomplete download states states for the "
         "given user.",
         "bitmunk.purchase.ContractService.DownloadStateRetrievalError");
      Exception::push(e);
   }

   if(rval)
   {
      // send back DownloadStates that were successfully filtered
      out["downloadStates"] = filteredDownloadStates;
   }

   return rval;
}

bool ContractService::deleteDownloadState(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = true;

   // get the action user
   UserId userId = action->getInMessage()->getUserId();

   // get the resource params
   DynamicObject params;
   action->getResourceParams(params);

   // check to see if the download state ID was specified as a parameter or
   // in the incoming object. It depends on if this method was called using
   // HTTP DELETE or HTTP POST
   DownloadStateId dsId = 0;
   if(!in.isNull() && in->hasMember("downloadStateId"))
   {
      // download state ID was specified in HTTP POST
      dsId = in["downloadStateId"]->getUInt64();
   }
   else if(params[0]->getUInt64() != 0)
   {
      // download state ID was specified in HTTP DELETE URL
      dsId = params[0]->getUInt64();
   }

   // prepare the download state
   DownloadState ds;
   ds["id"] = dsId;
   BM_ID_SET(ds["userId"], userId);

   // start listening for download interrupted event
   EventFilter filter;
   filter["details"]["downloadStateId"] = dsId;
   filter["details"]["userId"] = userId;
   EventWaiter ew(mNode->getEventController());
   ew.start(EVENT_DOWNLOAD_STATE ".downloadInterrupted", &filter);

   // populate download state
   PurchaseDatabase* pd = PurchaseDatabase::getInstance(mNode);
   if((rval = pd->populateDownloadState(ds)))
   {
      // if download is complete but assembly is not,
      // it must finish processing
      if(ds["remainingPieces"]->getUInt32() == 0 &&
         !ds["filesAssembled"]->getBoolean())
      {
         ExceptionRef e = new Exception(
            "The download state is currently active and must be "
            "finish before it can be deleted.",
            "bitmunk.purchase.ContractService.DownloadStateActive");
         BM_ID_SET(e->getDetails()["userId"], userId);
         e->getDetails()["downloadStateId"] = dsId;
         Exception::set(e);
         rval = false;
      }
      else if(ds["processing"]->getBoolean())
      {
         // try to pause download
         MO_CAT_INFO(BM_PURCHASE_CAT,
            "ContractService interrupting download...");

         // send message to interrupt download
         DynamicObject msg;
         msg["pause"] = true;
         msg["deleting"] = true;
         BM_ID_SET(msg["userId"], userId);
         msg["downloadStateId"] = ds["id"]->getUInt64();
         FiberId fiberId = ds["processorId"]->getUInt32();
         mNode->getFiberMessageCenter()->sendMessage(fiberId, msg);

         // wait for download interrupted event for up to 15 seconds
         // set exception if timeout is reached since we can't delete
         // the download if it completed successfully
         ew.waitForEvent(1000 * 15);
      }

      if(rval)
      {
         // start processing download state
         if((rval = pd->startProcessingDownloadState(ds, 0)))
         {
            // clean up all file pieces and then delete download state
            rval =
               pd->populateDownloadState(ds) &&
               cleanupFilePieces(ds) &&
               pd->deleteDownloadState(ds);
            if(rval)
            {
               // send download state deleted event
               Event e;
               e["type"] = EVENT_DOWNLOAD_STATE ".deleted";
               BM_ID_SET(e["details"]["userId"], userId);
               e["details"]["downloadState"] = ds;
               e["details"]["downloadStateId"] = ds["id"]->getString();
               mNode->getEventController()->schedule(e);

               // return the ID of the deleted state
               out["downloadStateId"] = ds["id"]->getUInt64();
            }
            else
            {
               // send error event
               Event e;
               e["type"] = EVENT_DOWNLOAD_STATE ".exception";
               BM_ID_SET(e["details"]["userId"], userId);
               e["details"]["downloadState"] = ds;
               e["details"]["downloadStateId"] = ds["id"]->getString();
               e["details"]["exception"] = Exception::getAsDynamicObject();
               mNode->getEventController()->schedule(e);

               // stop processing download state
               pd->stopProcessingDownloadState(ds, 0);
            }
         }
         else
         {
            ExceptionRef e = new Exception(
               "The download state is currently active and must be "
               "finish before it can be deleted.",
               "bitmunk.purchase.ContractService.DownloadStateActive");
            e->getDetails()["userId"] = userId;
            e->getDetails()["downloadStateId"] = dsId;
            Exception::set(e);
         }
      }
   }

   return rval;
}

bool ContractService::getDownloadState(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = true;

   // get the action user
   UserId userId = action->getInMessage()->getUserId();

   // details to retrieve from resource query, default to "all"
   DynamicObject vars;
   action->getResourceQuery(vars);
   const char* details = "all";
   if(vars->hasMember("details"))
   {
      details = vars["details"]->getString();
   }

   // get the resource params
   DynamicObject params;
   action->getResourceParams(params);

   // get the download state
   DownloadState ds;
   ds["id"] = params[0]->getUInt64();
   ds["userId"] = userId;
   PurchaseDatabase* pd = PurchaseDatabase::getInstance(mNode);
   if((rval = pd->populateDownloadState(ds)))
   {
      // return the full download state unless in progress mode
      if(strcmp(details, "state") == 0)
      {
         // reduce data to just progress related details

         // FIXME: move some/all of this to populateDownloadState
         // This is currently a quick list of data to prune. However, this
         // may leave client data out of sync. Some clients may need all
         // updates and a consistant view of the download state. This may
         // require a smarter sync mechanism or at least a more official
         // list of properties that are known to be updated by this call vs
         // a full update.
         // See also BPE purchase plugin - downloadstates.model.js -
         //    downloadStateChanged()

         ds->removeMember("contract");
         ds->removeMember("ware");
         FileProgressIterator fpi = ds["progress"].getIterator();
         while(fpi->hasNext())
         {
            FileProgress& fp = fpi->next();
            fp->removeMember("sellerData");
            fp->removeMember("sellerPool");
            fp->removeMember("assigned");
            fp->removeMember("downloaded");
            fp->removeMember("unassigned");
         }
      }

      out = ds;
   }

   return rval;
}

bool ContractService::initializeDownloadState(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = true;

   // get the action user
   UserId userId = action->getInMessage()->getUserId();

   DownloadState ds;
   ds["id"] = in["downloadStateId"];
   BM_ID_SET(ds["userId"], userId);

   // try to start processing the download state
   PurchaseDatabase* pd = PurchaseDatabase::getInstance(mNode);
   if((rval = pd->startProcessingDownloadState(ds, 0)))
   {
      // populate the download state
      if((rval = pd->populateDownloadState(ds)))
      {
         MO_CAT_INFO(BM_PURCHASE_CAT, "Initializing download state...");

         // nothing to send back
         out.setNull();

         // clear existing initialization from download state
         ds["initialized"] = false;

         // add DownloadStateInitializer fiber to handle state initialization
         DownloadStateInitializer* dsi = new DownloadStateInitializer(mNode);
         dsi->setUserId(userId);
         dsi->setDownloadState(ds);
         mNode->getFiberScheduler()->addFiber(dsi);
      }
      else
      {
         // error, stop processing download state
         pd->stopProcessingDownloadState(ds, 0);
      }
   }

   return rval;
}

bool ContractService::acquireLicense(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = true;

   // get the action user
   UserId userId = action->getInMessage()->getUserId();

   DownloadState ds;
   ds["id"] = in["downloadStateId"];
   BM_ID_SET(ds["userId"], userId);

   // try to start processing the download state
   PurchaseDatabase* pd = PurchaseDatabase::getInstance(mNode);
   if((rval = pd->startProcessingDownloadState(ds, 0)))
   {
      // populate the download state
      if((rval = pd->populateDownloadState(ds)))
      {
         MO_CAT_INFO(BM_PURCHASE_CAT, "Acquiring license...");

         // nothing to send back
         out.setNull();

         // clear existing license from download state
         ds["licenseAcquired"] = false;

         // add LicenseAcquirer fiber to handle the license acquisition
         LicenseAcquirer* la = new LicenseAcquirer(mNode);
         la->setUserId(userId);
         la->setDownloadState(ds);
         mNode->getFiberScheduler()->addFiber(la);
      }
      else
      {
         // error, stop processing download state
         pd->stopProcessingDownloadState(ds, 0);
      }
   }

   return rval;
}

bool ContractService::downloadContractData(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = true;

   // get the action user
   UserId userId = action->getInMessage()->getUserId();

   DownloadState ds;
   ds["id"] = in["downloadStateId"];
   BM_ID_SET(ds["userId"], userId);

   // try to start processing the download state
   PurchaseDatabase* pd = PurchaseDatabase::getInstance(mNode);
   if((rval = pd->startProcessingDownloadState(ds, 0)))
   {
      // populate the download state
      if((rval = pd->populateDownloadState(ds)))
      {
         if(in->hasMember("preferences"))
         {
            // set download state preferences
            ds["preferences"] = in["preferences"];
         }
         else
         {
            // validate the download state preferences
            v::ValidatorRef v = new v::Map(
               "fast", new v::Type(Boolean),
               "accountId", new v::Int(v::Int::Positive),
               "price", new v::Map(
                  "max", new v::Optional(new v::Regex(
                     "^[0-9]+\\.[0-9]{2,7}$",
                     "Monetary amount must be of the format 'x.xx'. "
                     "Example: 10.00")),
                  NULL),
               "sellerLimit", new v::Int(v::Int::Positive),
               // FIXME: add optional list of preferred sellers
               NULL);
            if(!v->isValid(ds["preferences"]))
            {
               ExceptionRef e = new Exception(
                  "No valid preferences set for download state."
                  "bitmunk.purchase.ContractService.InvalidPreferences");
               Exception::push(e);
               rval = false;
            }
         }
      }

      if(rval)
      {
         ds["downloadStarted"] = true;
         ds["downloadPaused"] = false;
         rval = pd->updatePreferences(ds) && pd->updateDownloadStateFlags(ds);

         // if the preferences and download state flags are updated and
         // valid, schedule the download manager to start
         if(rval)
         {
            MO_CAT_INFO(BM_PURCHASE_CAT, "Starting download...");

            // nothing to send back
            out.setNull();

            // start DownloadManager fiber to handle the download
            DownloadManager* dm = new DownloadManager(mNode);
            dm->setUserId(userId);
            dm->setDownloadState(ds);
            mNode->getFiberScheduler()->addFiber(dm);
         }
      }

      if(!rval)
      {
         // error, stop processing download state
         pd->stopProcessingDownloadState(ds, 0);
      }
   }

   return rval;
}

bool ContractService::pauseDownload(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = true;

   // get the action user and download state ID
   UserId userId = action->getInMessage()->getUserId();
   DownloadStateId dsId = in["downloadStateId"]->getUInt64();
   DownloadState ds;
   BM_ID_SET(ds["userId"], userId);
   ds["id"] = dsId;

   // populate the download state processing info
   PurchaseDatabase* pd = PurchaseDatabase::getInstance(mNode);
   if((rval = pd->populateProcessingInfo(ds)))
   {
      // allow pause only if download is in progress
      FiberId processorId = ds["processorId"]->getUInt32();
      if(ds["processing"]->getBoolean() && processorId != 0)
      {
         MO_CAT_INFO(BM_PURCHASE_CAT, "Pausing download...");

         // send message to pause download
         DynamicObject msg;
         msg["pause"] = true;
         BM_ID_SET(msg["userId"], userId);
         msg["downloadStateId"] = dsId;

         // only successful if message was delivered
         rval = mNode->getFiberMessageCenter()->sendMessage(processorId, msg);
      }
   }

   if(rval)
   {
      // nothing to send back
      out.setNull();
   }
   else
   {
      // download can not be paused at this time
      ExceptionRef e = new Exception(
         "Could not pause download. "
         "Download not in progress or has already completed.",
         "bitmunk.purchase.ContractService.DownloadNotInProgress");
      BM_ID_SET(e->getDetails()["userId"], userId);
      e->getDetails()["downloadStateId"] = dsId;
      Exception::set(e);
   }

   return rval;
}

bool ContractService::purchaseContractData(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = true;

   // get the action user
   UserId userId = action->getInMessage()->getUserId();

   DownloadState ds;
   ds["id"] = in["downloadStateId"];
   BM_ID_SET(ds["userId"], userId);

   // try to start processing the download state
   PurchaseDatabase* pd = PurchaseDatabase::getInstance(mNode);
   if((rval = pd->startProcessingDownloadState(ds, 0)))
   {
      // populate the download state
      if((rval = pd->populateDownloadState(ds)))
      {
         MO_CAT_INFO(BM_PURCHASE_CAT, "Starting purchase...");

         // nothing to send back
         out.setNull();

         // add Purchaser fiber to handle the purchase
         Purchaser* p = new Purchaser(mNode);
         p->setUserId(userId);
         p->setDownloadState(ds);
         mNode->getFiberScheduler()->addFiber(p);
      }
      else
      {
         // error, stop processing download state
         pd->stopProcessingDownloadState(ds, 0);
      }
   }

   return rval;
}

bool ContractService::assembleContractData(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval = true;

   // get the action user
   UserId userId = action->getInMessage()->getUserId();

   DownloadState ds;
   ds["id"] = in["downloadStateId"];
   BM_ID_SET(ds["userId"], userId);

   // try to start processing the download state
   PurchaseDatabase* pd = PurchaseDatabase::getInstance(mNode);
   if((rval = pd->startProcessingDownloadState(ds, 0)))
   {
      // populate the download state
      if((rval = pd->populateDownloadState(ds)))
      {
         MO_CAT_INFO(BM_PURCHASE_CAT, "Starting assembly of all files...");

         // nothing to send back
         out.setNull();

         // add FileAssembler fiber to handle assembling files
         FileAssembler* fa = new FileAssembler(mNode);
         fa->setUserId(userId);
         fa->setDownloadState(ds);
         mNode->getFiberScheduler()->addFiber(fa);
      }
      else
      {
         // error, stop processing download state
         pd->stopProcessingDownloadState(ds, 0);
      }
   }

   return rval;
}

bool ContractService::cleanupFilePieces(DownloadState& ds)
{
   bool rval = true;

   // iterate over all progress
   ds["progress"]->setType(Map);
   FileProgressIterator pi = ds["progress"].getIterator();
   while(rval && pi->hasNext())
   {
      FileProgress& progress = pi->next();

      // add all pieces to be deleted to a single list
      FilePieceList allPieces;
      allPieces->setType(Array);

      // iterate over unassigned file piece list
      {
         progress["unassigned"]->setType(Array);
         FilePieceIterator fpi = progress["unassigned"].getIterator();
         while(fpi->hasNext())
         {
            // only add unassigned pieces with paths
            FilePiece& fp = fpi->next();
            if(fp->hasMember("path"))
            {
               allPieces->append(fp);
            }
         }
      }

      // iterate over downloaded and assigned file piece lists
      const char* keys[2] = {"assigned", "downloaded"};
      for(int i = 0; i < 2; ++i)
      {
         progress[keys[i]]->setType(Map);
         DynamicObjectIterator fpli = progress[keys[i]].getIterator();
         while(fpli->hasNext())
         {
            FilePieceList& fpl = fpli->next();
            FilePieceIterator fpi = fpl.getIterator();
            while(fpi->hasNext())
            {
               allPieces->append(fpi->next());
            }
         }
      }

      // iterate over the file pieces, removing all of them
      uint32_t pieceCount = 0;
      FilePieceIterator fpi = allPieces.getIterator();
      while(rval && fpi->hasNext())
      {
         FilePiece& fp = fpi->next();

         MO_CAT_DEBUG(BM_PURCHASE_CAT,
            "UserId %" PRIu64 ", DownloadState %" PRIu64 ": "
            "removing piece '%s'",
            BM_USER_ID(ds["userId"]),
            ds["id"]->getUInt64(),
            fp["path"]->getString());

         // remove file piece
         File file(fp["path"]->getString());
         file->remove();
         ++pieceCount;
      }

      if(rval)
      {
         MO_CAT_INFO(BM_PURCHASE_CAT,
            "UserId %" PRIu64 ", DownloadState %" PRIu64 ": "
            "cleaned up %" PRIu32 " pieces for file ID '%s'",
            BM_USER_ID(ds["userId"]),
            ds["id"]->getUInt64(),
            pieceCount,
            BM_FILE_ID(progress["fileInfo"]["id"]));
      }
   }

   return rval;
}
