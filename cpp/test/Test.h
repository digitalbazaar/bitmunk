/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_test_Test_H
#define bitmunk_test_Test_H

#include "bitmunk/common/TypeDefinitions.h"
#include "monarch/crypto/BigDecimal.h"
#include "monarch/data/json/JsonWriter.h"

#include <string>
#include <cassert>

namespace bitmunk
{
namespace test
{

/**
 * Creates a payee given all of the payee information.
 * 
 * @param id the financial ID of the account.
 * @param type the type of payee.
 * @param resolved true if the payee is resolved, false otherwise.
 * @param amount the amount associated with the payee.
 * @param percentage the percentage associated with the payee.
 * @param min the minimum amount for the payee.
 * @param description the description associated with the payee.
 * @param taxExempt true if the payee is tax-exempt, false otherwise.
 * 
 * @return a fully constructed payee object.
 */
bitmunk::common::Payee createPayee(
   bitmunk::common::AccountId id,
   bitmunk::common::PayeeAmountType type,
   bool resolved,
   const char* amount,
   const char* percentage,
   const char* min,
   const char* description,
   bool taxExempt);

/**
 * Compares two payees.
 * 
 * @param p1 the first payee.
 * @param p2 the second payee.
 * 
 * @return true if both payees match, false otherwise.
 */
bool comparePayees(bitmunk::common::Payee& p1, bitmunk::common::Payee& p2);

/**
 * Compares two lists of payees. The two list must have the payees listed in
 * the same order to match.
 * 
 * @param list1 the first list of payees.
 * @param list2 the second list of payees.
 * 
 * @return true if both payee lists match, false otherwise.
 */
bool comparePayeeLists(
   bitmunk::common::PayeeList& list1, bitmunk::common::PayeeList& list2);

/**
 * Asserts that two payee lists match exactly.
 * 
 * @param list1 the first payee list to compare.
 * @param list2 the second payee list to compare.
 */
#define assertComparePayeeLists(list1, list2) \
   do { \
      if(!comparePayeeLists(list1, list2)) \
      { \
         printf("\nPayeeList 1=\n"); \
         monarch::rt::DynamicObject d = list1; \
         monarch::data::json::JsonWriter::writeToStdOut( \
            d, false, false); \
         printf("\nPayeeList 2=\n"); \
         d = list2; \
         monarch::data::json::JsonWriter::writeToStdOut( \
            d, false, false); \
         assert(false); \
      } \
   } while(0)

} // end namespace test
} // end namespace bitmunk

#endif
