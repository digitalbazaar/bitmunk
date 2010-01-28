/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/negotiate/INegotiateModule.h"

#include "bitmunk/common/CatalogInterface.h"

using namespace monarch::rt;
using namespace bitmunk::common;
using namespace bitmunk::protocol;
using namespace bitmunk::node;
using namespace bitmunk::negotiate;

INegotiateModule::INegotiateModule(Node* node) :
   mNode(node)
{
}

INegotiateModule::~INegotiateModule()
{
}

bool INegotiateModule::negotiateContractSection(
   UserId userId, Contract& c, ContractSection& section, bool seller)
{
   bool rval = false;

   if(seller)
   {
      // try to get a catalog interface
      CatalogInterface* ci = dynamic_cast<CatalogInterface*>(
         mNode->getModuleApiByType("bitmunk.catalog"));
      if(ci == NULL)
      {
         // set exception, no available catalog
         ExceptionRef e = new Exception(
            "Could not negotiate contract section on behalf of seller. "
            "No available catalog module.", "bitmunk.negotiate.NoCatalog");
         Exception::set(e);
      }
      else
      {
         // populate seller from catalog
         Seller seller;
         rval = ci->populateSeller(userId, seller);
         if(rval)
         {
            if(seller["serverId"] != section["seller"]["serverId"])
            {
               ExceptionRef e = new Exception(
                  "Could not negotiate contract section. Given server "
                  "ID does not match seller's server.",
                  "bitmunk.negotiate.InvalidServerId");
               Exception::set(e);
               rval = false;
            }
         }

         // re-populate the ware to clean and update it
         rval = rval && ci->populateWare(userId, section["ware"]);
         if(rval)
         {
            // clear local file paths
            FileInfoIterator fii = section["ware"]["fileInfos"].getIterator();
            while(fii->hasNext())
            {
               FileInfo& fi = fii->next();
               fi->removeMember("path");
            }
         }
      }
   }
   else
   {
      // no negotiation
      rval = true;
   }

   return rval;
}
