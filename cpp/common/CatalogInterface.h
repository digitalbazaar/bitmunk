/*
 * Copyright (c) 2007-2008 Digital Bazaar, Inc.  All rights reserved.
 */
#ifndef bitmunk_common_CatalogInterface_H
#define bitmunk_common_CatalogInterface_H

#include "bitmunk/common/TypeDefinitions.h"

namespace bitmunk
{
namespace common
{

/**
 * A CatalogInterface is a standard interface into a catalog of wares. It is
 * expected that various catalog modules' interfaces will implement this
 * interface according to their own specifics.
 *
 * @author Dave Longley
 */
class CatalogInterface
{
public:
   /**
    * Creates a new CatalogInterface.
    */
   CatalogInterface() {};

   /**
    * Destructs this CatalogInterface.
    */
   virtual ~CatalogInterface() {};

   /**
    * Populates the seller associated with the given user ID.
    *
    * @param userId the ID of the user.
    * @param seller the seller object to populate.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool populateSeller(UserId userId, Seller& seller) = 0;

   /**
    * Adds or updates a Ware in the catalog with the passed Ware.
    *
    * @param userId the ID of the user the ware belongs to.
    * @param w the Ware to add/update in the catalog.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool updateWare(UserId userId, Ware& w) = 0;

   /**
    * Removes a Ware from the catalog, if it exists.
    *
    * @param userId the ID of the user the ware belongs to.
    * @param w the Ware to remove from the catalog.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool removeWare(UserId userId, Ware& w) = 0;

   /**
    * Populates a Ware based on its ID. Any extraneous information not
    * in the catalog will be removed from the ware.
    *
    * @param userId the ID of the user the ware belongs to.
    * @param w the Ware to populate.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool populateWare(UserId userId, Ware& w) = 0;

   /**
    * Populates a Ware bundle based on its media ID and the given
    * MediaPreferences. Any extraneous information not in the catalog
    * will be removed from the ware.
    *
    * @param userId the ID of the user the ware belongs to.
    * @param w the Ware to populate, with media ID set.
    * @param prefs the MediaPreferences for the Ware.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool populateWareBundle(
      UserId userId, Ware& w, MediaPreferenceList& prefs) = 0;

   /**
    * Populates a FileInfo based on its ID.
    *
    * @param userId the ID of the user the FileInfo belongs to.
    * @param fi the FileInfo to populate.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool populateFileInfo(UserId userId, FileInfo& fi) = 0;

   /**
    * Populates the FileInfos for a Ware.
    *
    * @param userId the ID of the user the ware belongs to.
    * @param w the Ware to populate the FileInfos of.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool populateFileInfos(UserId userId, Ware& w) = 0;

   /**
    * Populates a FileInfoList.
    *
    * @param userId the ID of the user the FileInfos belong to.
    * @param fil the FileInfoList with FileInfos that have their IDs set.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool populateFileInfoList(UserId userId, FileInfoList& fil) = 0;

   // FIXME: add methods for searching catalog, etc
};

} // end namespace common
} // end namespace bitmunk
#endif
