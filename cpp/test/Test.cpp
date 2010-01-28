/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/test/Test.h"

using namespace bitmunk::common;
using namespace bitmunk::test;
using namespace monarch::crypto;

Payee bitmunk::test::createPayee(
   AccountId id,
   PayeeAmountType type,
   bool resolved,
   const char* amount,
   const char* percentage,
   const char* min,
   const char* description,
   bool nontaxable)
{
   Payee p;

   p["id"] = id;
   p["amountType"] = type;
   p["amountResolved"] = resolved;
   p["amount"] = amount;
   p["percentage"] = percentage;
   p["min"] = min;
   p["description"] = description;
   p["nontaxable"] = nontaxable;
   
   return p;
}

bool bitmunk::test::comparePayees(Payee& p1, Payee& p2)
{
   bool rval;
   
   // check to see if the basic fields in both payees match
   rval =
      p1["id"]->getUInt64() == p2["id"]->getUInt64() &&
      p1["amountType"]->getInt32() == p2["amountType"]->getInt32() &&
      p1["amountResolved"]->getBoolean() == 
         p2["amountResolved"]->getBoolean() &&
      p1["nontaxable"]->getBoolean() == p2["nontaxable"]->getBoolean();
   
   // check to make sure that the amounts match
   if(rval)
   {
      BigDecimal num1 = p1["amount"]->getString();
      BigDecimal num2 = p2["amount"]->getString();
      if((rval = (num1 == num2)))
      {
         num1 = p1["percentage"]->getString();
         num2 = p2["percentage"]->getString();
         if((rval = (num1 == num2)))
         {
            num1 = p1["min"]->getString();
            num2 = p2["min"]->getString();
            rval = (num1 == num2);
         }
      }
   }
   
   return rval;
}

bool bitmunk::test::comparePayeeLists(PayeeList& list1, PayeeList& list2)
{
   bool rval = false;
   
   // ensure that the list lengths match
   if(list1->length() == list2->length())
   {
      rval = true;
      // compare each payee in each list (order matters)
      for(int i = 0; rval && i < list1->length(); i++)
      {
         rval = comparePayees(list1[i], list2[i]);
      }
   }
   
   return rval;
}
