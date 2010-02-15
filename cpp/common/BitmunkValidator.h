/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_common_BitmunkValidators_H
#define bitmunk_common_BitmunkValidators_H

#include "bitmunk/common/TypeDefinitions.h"
#include "monarch/validation/Validation.h"

namespace bitmunk
{
namespace common
{

/**
 * The BitmunkValidator class contains several static methods for creating
 * common validators for complex Bitmunk structures like usernames or
 * email addresses.
 *
 * @author Mike Johnson
 */
class BitmunkValidator
{
public:
   /**
    * Creates a username validator.
    *
    * @return a pointer to the validator.
    */
   static monarch::validation::Validator* username();

   /**
    * Creates an email validator.
    *
    * @return a pointer to the validator.
    */
   static monarch::validation::Validator* email();

   /**
    * Creates a password validator.
    *
    * @return a pointer to the validator.
    */
   static monarch::validation::Validator* password();

   /**
    * Creates an email code validator.
    *
    * @return a pointer to the validator.
    */
   static monarch::validation::Validator* emailCode();

   /**
    * Creates a validator for validating money amounts that use 2 decimal
    * places. Use the preciseMoney() validator for 7 decimal places.
    *
    * See the validator code for more specifics.
    *
    * @return a pointer to the validator.
    */
   static monarch::validation::Validator* money();

   /**
    * Creates a validator for validating money amounts that use 7 decimal
    * places.
    *
    * See the validator code for more specifics.
    *
    * @return a pointer to the validator.
    */
   static monarch::validation::Validator* preciseMoney();

   /**
    * Creates a validator for validating money amounts that use 7 decimal
    * places and are positive.
    *
    * See the validator code for more specifics.
    *
    * @return a pointer to the validator.
    */
   static monarch::validation::Validator* precisePositiveMoney();

   /**
    * Creates a validator for validating money amounts that must be zero.
    *
    * See the validator code for more specifics.
    *
    * @return a pointer to the validator.
    */
   static monarch::validation::Validator* zeroMoney();

   /**
    * Creates a validator for validating percentages.
    *
    * See the validator code for more specifics.
    *
    * @return a pointer to the validator.
    */
   static monarch::validation::Validator* percentage();

   /**
    * Creates a validator for validating percentages that must be zero.
    *
    * See the validator code for more specifics.
    *
    * @return a pointer to the validator.
    */
   static monarch::validation::Validator* zeroPercentage();

   /**
    * Creates a validator for validating payees that are associated with a
    * license: they must be resolved, have a non-zero media ID and can be
    * of any amount type except percent of license.
    *
    * See the validator code for more specifics.
    *
    * @return a pointer to the validator.
    */
   static monarch::validation::Validator* licensePayees();

   /**
    * Creates a validator for validating payees that are associated with a
    * dynamic license: they must be unresolved and can be of any amount type
    * except percent of license.
    *
    * See the validator code for more specifics.
    *
    * @return a pointer to the validator.
    */
   static monarch::validation::Validator* dynamicLicensePayees();

   /**
    * Creates a validator for validating pieces payees. They must be flat
    * fee payees.
    *
    * See the validator code for more specifics.
    *
    * @return a pointer to the validator.
    */
   static monarch::validation::Validator* piecePayees();

   /**
    * Creates a validator for validating data payees. They must be unresolved,
    * cannot be a tax payee, and must have a zero media ID.
    *
    * See the validator code for more specifics.
    *
    * @return a pointer to the validator.
    */
   static monarch::validation::Validator* dataPayees();

   /**
    * Creates a validator for validating payees that can appear in wares. This
    * includes license and data payees.
    *
    * See the validator code for more specifics.
    *
    * @return a pointer to the validator.
    */
   static monarch::validation::Validator* warePayees();

   /**
    * Creates a validator for validating a standard bitmunk media ID.
    *
    * @return a pointer to the validator.
    */
   static monarch::validation::Validator* mediaId();

   /**
    * Creates a validator for validating a standard bitmunk file ID.
    *
    * @return a pointer to the validator.
    */
   static monarch::validation::Validator* fileId();

   /**
    * Creates a validator for validating a standard bitmunk identity address.
    *
    * @return a pointer to the validator.
    */
   static monarch::validation::Validator* address();

   /**
    * Creates a validator for validating a standard bitmunk identity.
    *
    * @return a pointer to the validator.
    */
   static monarch::validation::Validator* identity();

   /**
    * Creates a validator for validating IPv4 addresses.
    *
    * @return a pointer to the validator.
    */
   static monarch::validation::Validator* ipv4();

   /**
    * Creates a validator for validating dates with the format:
    *
    * 'YYYY-MM-DD'
    *
    * @return a pointer to the validator.
    */
   static monarch::validation::Validator* date();

   /**
    * Creates a validator for validating date-times with the format:
    *
    * 'YYYY-MM-DD HH:MM:SS'
    *
    * @return a pointer to the validator.
    */
   static monarch::validation::Validator* dateTime();
};

} // end namespace common
} // end namespace bitmunk
#endif
