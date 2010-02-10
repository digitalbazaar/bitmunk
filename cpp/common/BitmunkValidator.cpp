/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/common/BitmunkValidator.h"

#include "monarch/util/StringTools.h"
#include "monarch/validation/Validation.h"

using namespace bitmunk::common;
using namespace monarch::rt;
using namespace monarch::util;
namespace v = monarch::validation;

static const char* WHITESPACE_REGEX = "^[^[:space:]](.*)[^[:space:]]$";

static const char* EMAIL_REGEX =
   "^[a-zA-Z][\\.a-zA-Z0-9_-]*[a-zA-Z0-9]@"
   "[a-zA-Z0-9][\\.a-zA-Z0-9_-]*[a-zA-Z0-9]\\.[a-zA-Z][a-zA-Z\\.]*[a-zA-Z]$";

static const char* BITMUNK_EMAIL_REGEX = "@bitmunk\\.com$";

v::Validator* BitmunkValidator::username()
{
   return new v::All(
      new v::Type(String),
      new v::Regex(WHITESPACE_REGEX,
         "The username must not start or end with whitespace."),
      new v::Min(4, "The username is too short. 4 characters minimum."),
      new v::Max(100, "The username is too long. 100 characters maximum."),
      NULL);
}

v::Validator* BitmunkValidator::email()
{
   return new v::All(
      new v::Type(String),
      new v::Max(320,
         "The email address is too long. 320 characters maximum."),
      new v::Regex(EMAIL_REGEX,
         "The email address is not the correct format (user@example.com)."),
      new v::Regex(WHITESPACE_REGEX,
         "The email address must not start or end with whitespace."),
      new v::Not(new v::Regex(BITMUNK_EMAIL_REGEX),
         "The email domain (@bitmunk.com) is not allowed."),
      NULL);
}

v::Validator* BitmunkValidator::password()
{
   // FIXME: add validator for valid password characters
   return new v::All(
      v::ValidatorContext::MaskInvalidValues,
      new v::Type(monarch::rt::String),
      new v::Min(8, "The password is too short. 8 characters minimum."),
      new v::Max(30, "The password is too long. 30 characters maximum."),
      NULL);
}

v::Validator* BitmunkValidator::emailCode()
{
   return new v::All(
      v::ValidatorContext::MaskInvalidValues,
      new v::Type(monarch::rt::String),
      new v::Regex(WHITESPACE_REGEX,
         "The email code must not start or end with whitespace."),
      new v::Min(7, "The email code is too short. Must be 7 characters."),
      new v::Max(7, "The email code is too long. Must be 7 characters."),
      NULL);
}

v::Validator* BitmunkValidator::money()
{
   return new v::Regex(
      "^[0-9]+\\.[0-9]{2}$",
      "The monetary amount must be in the following format: 'x.xx'. "
      "Example: 10.00");
}

v::Validator* BitmunkValidator::preciseMoney()
{
   return new v::Regex(
      "^[0-9]+\\.[0-9]{2,7}$",
      "The monetary amount must be in the following format: 'x.xx'. "
      "Example: 10.00");
}

v::Validator* BitmunkValidator::precisePositiveMoney()
{
   return new v::All(
      preciseMoney(),
      new v::Not(new v::Regex(
         "^[.0]+$"),
         "The monetary amount must be greater than 0.00. "),
      NULL);
}

v::Validator* BitmunkValidator::zeroMoney()
{
   // "0" or preciseMoney() format with only zeros
   return new v::Regex(
      "^(0|0+\\.0{2,7})$",
      "A monetary amount of zero must consist of only the digit 0.");
}

v::Validator* BitmunkValidator::percentage()
{
   // FIXME: may want to be more clear about percentage vs multiplier confusion
   return new v::Regex(
      "^[0-9]+\\.[0-9]{2,7}$",
      "The percentage must be in the following format: 'x.xx'. "
      "Example: 0.50");
}

v::Validator* BitmunkValidator::zeroPercentage()
{
   // FIXME: may want to be more clear about percentage vs multiplier confusion
   // "0" or percentage() format with only zeros
   return new v::Regex(
      "^(0|0+\\.0{2,7})$",
      "A percentage value of zero must consist of only the digit 0.");
}

static v::Validator* _payees(v::Validator* payee, int minCount = 0)
{
   v::All* rval = new v::All(
      new v::Type(Array),
      new v::Each(payee),
      NULL);
   if(minCount > 0)
   {
      rval->addValidator(new v::Min(minCount));
   }
   return rval;
}

v::Validator* BitmunkValidator::licensePayees()
{
   // FIXME: add any validator for percentage vs. amount, etc ... do
   // for other payee validators in here as well

   return _payees(new v::Map(
      "id", new v::Int(v::Int::Positive),
      // can have any of these types
      "amountType", new v::Any(
         new v::Equals(PAYEE_AMOUNT_TYPE_FLATFEE),
         new v::Equals(PAYEE_AMOUNT_TYPE_PCUMULATIVE),
         new v::Equals(PAYEE_AMOUNT_TYPE_PTOTAL),
         new v::Equals(PAYEE_AMOUNT_TYPE_TAX),
         NULL),
      "amount", preciseMoney(),
      // must be resolved
      "amountResolved", new v::Equals(true),
      "percentage", new v::Optional(new v::Type(String)),
      "min", new v::Optional(new v::Type(String)),
      // must have a non-zero media ID
      "mediaId", new v::Int(v::Int::Positive),
      "description", new v::Optional(new v::Type(String)),
      // FIXME: side note: we should really change "nontaxable" to "taxExempt"
      // tax-exempt value is optional
      "nontaxable", new v::Optional(new v::Type(Boolean)),
      NULL));
}

v::Validator* BitmunkValidator::dynamicLicensePayees()
{
   return _payees(new v::Map(
      "id", new v::Int(v::Int::Positive),
      // can have any of these types
      "amountType", new v::Any(
         new v::Equals(PAYEE_AMOUNT_TYPE_FLATFEE),
         new v::Equals(PAYEE_AMOUNT_TYPE_PCUMULATIVE),
         new v::Equals(PAYEE_AMOUNT_TYPE_PTOTAL),
         new v::Equals(PAYEE_AMOUNT_TYPE_TAX),
         NULL),
      "amount", new v::Optional(preciseMoney()),
      // if provided, must not be resolved
      "amountResolved", new v::Optional(new v::Equals(false)),
      "percentage", new v::Optional(new v::Type(String)),
      "min", new v::Optional(new v::Type(String)),
      // if provided, must have a non-zero media ID
      "mediaId", new v::Optional(new v::Int(v::Int::Positive)),
      "description", new v::Optional(new v::Type(String)),
      // if provided, cannot be tax-exempt
      "nontaxable", new v::Optional(new v::Equals(false)),
      NULL));
}

v::Validator* BitmunkValidator::piecePayees()
{
   return _payees(new v::Map(
      "id", new v::Int(v::Int::Positive),
      // can only be flat fees
      "amountType", new v::Equals(PAYEE_AMOUNT_TYPE_FLATFEE),
      "amount", preciseMoney(),
      // resolved is irrelevant since they are flat fees only
      "amountResolved", new v::Optional(new v::Type(Boolean)),
      "percentage", new v::Optional(new v::Type(String)),
      "min", new v::Optional(new v::Type(String)),
      // must have a non-zero media ID
      "mediaId", new v::Int(v::Int::Positive),
      "description", new v::Optional(new v::Type(String)),
      // optionally tax-exempt
      "nontaxable", new v::Optional(new v::Type(Boolean)),
      NULL));
}

v::Validator* BitmunkValidator::dataPayees()
{
   // One or more data payess
   return _payees(new v::All(
      new v::Map(
         "id", new v::Int(v::Int::Positive),
         // can have any of these types
         "amountType", new v::Any(
            new v::Equals(PAYEE_AMOUNT_TYPE_FLATFEE),
            new v::Equals(PAYEE_AMOUNT_TYPE_PLICENSE),
            new v::Equals(PAYEE_AMOUNT_TYPE_PCUMULATIVE),
            new v::Equals(PAYEE_AMOUNT_TYPE_PTOTAL),
            NULL),
         // if provided, cannot be resolved
         "amountResolved", new v::Optional(new v::Equals(false)),
         "min", new v::Optional(new v::Any(
            money(),
            zeroMoney(),
            NULL)),
         // data payees are not for any particular media so their media ID,
         // if provided, must be set to 0
         "mediaId", new v::Optional(new v::Int(v::Int::Zero)),
         "description", new v::Optional(new v::Type(String)),
         // if provided, cannot be tax-exempt
         // FIXME: what about bitmunk data payees? these are tax-exempt correct?
         "nontaxable", new v::Optional(new v::Equals(false)),
         NULL),
      new v::Any(
         new v::Map(
            "amount", new v::Optional(zeroMoney()),
            "percentage", precisePositiveMoney(),
            NULL),
         new v::Map(
            "amount", precisePositiveMoney(),
            "percentage", new v::Optional(zeroPercentage()),
            NULL),
         NULL),
      NULL), 1);
}

v::Validator* BitmunkValidator::warePayees()
{
   return _payees(new v::Map(
      "id", new v::Int(v::Int::Positive),
      // can have any of these types
      "amountType", new v::Any(
         new v::Equals(PAYEE_AMOUNT_TYPE_FLATFEE),
         new v::Equals(PAYEE_AMOUNT_TYPE_PLICENSE),
         new v::Equals(PAYEE_AMOUNT_TYPE_PCUMULATIVE),
         new v::Equals(PAYEE_AMOUNT_TYPE_PTOTAL),
         NULL),
      "amount", new v::Optional(preciseMoney()),
      // if provided, must not be resolved
      "amountResolved", new v::Optional(new v::Type(Boolean)),
      "percentage", new v::Optional(new v::Type(String)),
      "min", new v::Optional(new v::Type(String)),
      // if provided, must have a non-zero media ID
      "mediaId", new v::Optional(new v::Int(v::Int::NonNegative)),
      "description", new v::Optional(new v::Type(String)),
      // if provided, cannot be tax-exempt
      "nontaxable", new v::Optional(new v::Equals(false)),
      NULL));
}

v::Validator* BitmunkValidator::mediaId()
{
   return new v::Int(v::Int::Positive,
      "The media ID must be a positive integer.");
}

v::Validator* BitmunkValidator::fileId()
{
   return new v::Regex(
      "^[0-9a-f]{40}$",
      "The file ID must be a 40 character lower-case hexadecimal string.");
}

v::Validator* BitmunkValidator::address()
{
   return new v::Map(
      "street", new v::All(
         new v::Type(String),
         new v::Regex(
            "(^$)|(^[^[:space:]]{1}.*$)",
            "The street address must not start with whitespace."),
         new v::Regex(
            "(^$)|(^.*[^[:space:]]{1}$)",
            "The street address must not end with whitespace."),
         new v::Min(1, "The street address is too short. 1 character minimum."),
         NULL),
      "locality", new v::All(
         new v::Type(String),
         new v::Regex(
            "(^$)|(^[^[:space:]]{1}.*$)",
            "The locality/city name must not start with whitespace."),
         new v::Regex(
            "(^$)|(^.*[^[:space:]]{1}$)",
            "The locality/city name must not end with whitespace."),
         new v::Min(
            1, "The locality/city is too short. 1 character minimum."),
         NULL),
      "postalCode", new v::All(
         new v::Type(String),
         new v::Regex(
            "(^$)|(^[^[:space:]]{1}.*$)",
            "The postal code must not start with whitespace."),
         new v::Regex(
            "(^$)|(^.*[^[:space:]]{1}$)",
            "The postal code must not end with whitespace."),
         new v::Min(
            1, "The postal code is too short. 1 character minimum."),
         NULL),
      "region", new v::All(
         new v::Type(String),
         new v::Regex(
            "(^$)|(^[^[:space:]]{1}.*$)",
            "The region/state must not start with whitespace."),
         new v::Regex(
            "(^$)|(^.*[^[:space:]]{1}$)",
            "The region/state must not end with whitespace."),
         new v::Min(
            1, "region/state too short. 1 character minimum."),
         NULL),
      "countryCode", new v::All(
         new v::Type(String),
         new v::Min(
            2,
            "The country code is too short. It must be exactly 2 characters."),
         new v::Max(
            2,
            "The country code is too long. It must be exactly 2 characters."),
         NULL),
      "hash", new v::Optional(new v::Type(String)),
      NULL);

   // FIXME: add support for signed addresses?
   /*
   "signature", new v::Type(String),
   "signer", new v::Map(
      "userId", new v::Int(v::Int::Positive),
      "profileId", new v::Int(v::Int::Positive),
      NULL),
   */
}

v::Validator* BitmunkValidator::identity()
{
   return new v::Map(
      "userId", new v::Int(v::Int::Positive),
      "firstName", new v::All(
         new v::Type(String),
         new v::Regex(
            "^[^[:space:]]{1}.*$",
            "The first name must not start with whitespace."),
         new v::Regex(
            "^.*[^[:space:]]{1}$",
            "The first name must not end with whitespace."),
         new v::Min(
            1, "The first name is too short. 1 character minimum."),
         NULL),
      "lastName", new v::Optional(new v::All(
         new v::Type(String),
         new v::Regex(
            "(^$)|(^[^[:space:]]{1}.*$)",
            "The last name must not start with whitespace."),
         new v::Regex(
            "(^$)|(^.*[^[:space:]]{1}$)",
            "The last name must not end with whitespace."),
         NULL)),
      "address", BitmunkValidator::address(),
      NULL);
}

v::Validator* BitmunkValidator::ipv4()
{
   return new v::Regex(
      "^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\."
      "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\."
      "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\."
      "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$",
      "An IPv4 address must be in dotted-notation: "
      "xxx.xxx.xxx.xxx");
}

v::Validator* BitmunkValidator::date()
{
   return new v::Regex(
      "^[2-9][0-9]{3}-"
      "(0[1-9]|1[0-2])-"
      "([0-2][0-9]|3[0-1]) "
      "([0-1][0-9]|2[0-3]):"
      "([0-5][0-9]):"
      "([0-5][0-9])$",
      "Date must be of the format 'YYYY-MM-DD HH:MM:SS'.");
}
