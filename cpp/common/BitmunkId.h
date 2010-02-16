/*
 * Copyright (c) 2010 Digital Bazaar, Inc.  All rights reserved.
 */
#ifndef bitmunk_common_BitmunkId_H
#define bitmunk_common_BitmunkId_H

#define BM_ID_INVALID ""
#define BM_ID_NONE    "0"
#define BM_ID_ANY     "*"
#define BM_ID_UNKNOWN "?"

namespace bitmunk
{
namespace common
{

/**
 * A BitmunkId identifies a Bitmunk object. It contains a string ID
 * that is copied on construction and equality and free'd on destruction.
 *
 * There are several special values for a Bitmunk ID:
 *
 * "" means Invalid ID
 * "0" means No ID
 * "*" means Any ID
 * "?" means Unknown ID
 *
 * @author Dave Longley
 */
class BitmunkId
{
protected:
   /**
    * The string identifier.
    */
   char* mId;

public:
   /**
    * Creates a new BitmunkId with the Invalid ID string.
    */
   BitmunkId();

   /**
    * Creates a new BitmunkId with the specifed string ID.
    *
    * @param id the string ID to use.
    */
   BitmunkId(const char* id);

   /**
    * Creates a new BitmunkId by copying another one's ID.
    *
    * @param rhs the BitmunkId to copy.
    */
   BitmunkId(const BitmunkId& rhs);

   /**
    * Destructs this BitmunkId.
    */
   virtual ~BitmunkId();

   /**
    * Sets this BitmunkId equal to another one by copying its ID.
    *
    * @param rhs the BitmunkId to copy.
    *
    * @return a reference to this BitmunkIdenfier.
    */
   virtual const BitmunkId& operator=(const BitmunkId& rhs);

   /**
    * Compares this BitmunkId to another one for equality.
    *
    * @param rhs the other BitmunkId to compare to.
    *
    * @return true if the two IDs are the same, false if not.
    */
   virtual bool operator==(const BitmunkId& rhs) const;

   /**
    * Compares this BitmunkId to another one for inequality.
    *
    * @param rhs the other BitmunkId to compare to.
    *
    * @return true if the two IDs are *NOT* the same, false if they are.
    */
   virtual bool operator!=(const BitmunkId& rhs) const;

   /**
    * Determines if this BitmunkId is the Invalid ID.
    *
    * @return true if this BitmunkId is the Invalid ID, false if not.
    */
   virtual bool isInvalid() const;

   /**
    * Determines if this BitmunkId is the None ID.
    *
    * @return true if this BitmunkId is the None ID, false if not.
    */
   virtual bool isNone() const;

   /**
    * Determines if this BitmunkId is the Any ID.
    *
    * @return true if this BitmunkId is the Any ID, false if not.
    */
   virtual bool isAny() const;

   /**
    * Determines if this BitmunkId is the Unknown ID.
    *
    * @return true if this BitmunkId is the Unknown ID, false if not.
    */
   virtual bool isUnknown() const;

   /**
    * Gets the string value of this ID.
    *
    * @return the string value of this ID.
    */
   virtual const char* id() const;
};

} // end namespace common
} // end namespace bitmunk
#endif
