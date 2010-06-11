/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */

#include "bitmunk/common/BitmunkValidator.h"
#include "bitmunk/common/Signer.h"
#include "bitmunk/common/Tools.h"
#include "bitmunk/test/Tester.h"
#include "bitmunk/test/Test.h"
#include "monarch/crypto/AsymmetricKeyFactory.h"
#include "monarch/crypto/DefaultBlockCipher.h"
#include "monarch/crypto/DigitalEnvelope.h"
#include "monarch/data/json/JsonWriter.h"
#include "monarch/io/ByteArrayInputStream.h"
#include "monarch/io/ByteArrayOutputStream.h"
#include "monarch/rt/Exception.h"
#include "monarch/test/Test.h"
#include "monarch/test/TestModule.h"

using namespace std;
using namespace bitmunk::common;
using namespace bitmunk::protocol;
using namespace bitmunk::node;
using namespace bitmunk::test;
using namespace monarch::crypto;
using namespace monarch::data;
using namespace monarch::io;
using namespace monarch::net;
using namespace monarch::rt;
using namespace monarch::util;
using namespace monarch::test;
namespace v = monarch::validation;

static void runProfileTest(TestRunner& tr)
{
   tr.test("Profile");
   
   Profile p;
   ProfileId pid = 1;
   UserId uid = 1;
   const char* pw = "password";
   
   // generate profile
   PublicKeyRef publicKey = p.generate();
   p.setId(pid);
   p.setUserId(uid);
   assertNoException();

   // save to buffer   
   ByteBuffer buffer;
   ByteArrayOutputStream os(&buffer);
   p.save(pw, &os);
   assertNoException();
   
   // load from buffer
   ByteArrayInputStream is(buffer.data(), buffer.length());
   Profile p2;
   p2.setUserId(uid);
   p2.load(pw, &is);
   assertNoException();
   
   // ensure profile ids are equal
   assert(p.getId() == p2.getId());
   
   // ensure private key PEMs are equal
   AsymmetricKeyFactory afk;
   string pem1 = afk.writePrivateKeyToPem(p.getPrivateKey(), pw);
   string pem2 = afk.writePrivateKeyToPem(p2.getPrivateKey(), pw);
   
   // create signatures with each key
   DigitalSignature ds1(p.getPrivateKey());
   DigitalSignature ds2(p2.getPrivateKey());
   const char* signData = "some data to sign";
   unsigned int len = strlen(signData);
   ds1.update(signData, len);
   ds2.update(signData, len);
   
   // get the signatures for each key
   char sig1[ds1.getValueLength()];
   char sig2[ds2.getValueLength()];
   unsigned int length1;
   unsigned int length2;
   ds1.getValue(sig1, length1);
   ds2.getValue(sig2, length2);
   
   // verify the signatures for each key against the original public key
   DigitalSignature* ds3 = new DigitalSignature(publicKey);
   ds3->update(signData, len);
   assert(ds3->verify(sig1, length1));
   delete ds3;
   
   ds3 = new DigitalSignature(publicKey);
   ds3->update(signData, len);
   assert(ds3->verify(sig2, length2));
   delete ds3;
   
   tr.pass();
}

static void runPayeeResolveTest(TestRunner& tr)
{
   tr.group("ResolvePayeeAmounts");
   
   // account id constants
   enum
   {
      BITMUNK,
      TAX,
      ARTIST,
      DISTRIBUTOR,
      ISP,
      SELLER
   };
   
   tr.test(PAYEE_AMOUNT_TYPE_FLATFEE);
   {
      PayeeList list;
      
      list->append() = createPayee(
         1, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeUnresolved,
         "1.00", "", "", "one", PayeeRegular);
      list->append() = createPayee(
         1, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeUnresolved,
         "2.00", "", "", "two", PayeeRegular);
      list->append() = createPayee(
         2, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeUnresolved,
         "3.00", "", "", "three", PayeeRegular);
      
      BigDecimal total;
      Tools::resolvePayeeAmounts(list, &total);
      
      PayeeList assertList;
      
      assertList->append() = createPayee(
         1, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeResolved,
         "1.00", "", "", "one", PayeeRegular);
      assertList->append() = createPayee(
         1, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeResolved,
         "2.00", "", "", "two", PayeeRegular);
      assertList->append() = createPayee(
         2, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeResolved,
         "3.00", "", "", "three", PayeeRegular);
      
      assertComparePayeeLists(assertList, list);
      BigDecimal expectedTotal("6");
      assert(total == expectedTotal);
   }
   tr.pass();
   
   {
      BigDecimal license;
      BigDecimal multiplier("0.50");
      BigDecimal total;
      BigDecimal temp;
      BigDecimal min;
      char name[100];
      // pre-computed for comparison
      const char* totals[] = {
         // payee with min checks
         "0.15", "2.065", "3.98",
         // above min
         "5.895", "7.86", "9.825", "11.79", "13.755", "15.72", "17.685",
         "19.65", "21.615", "23.58", "25.545", "27.51", "29.475", "31.44",
         "33.405", "35.37", "37.335", "39.30"
      };
      for(int i = 0; i <= 20; ++i)
      {
         license = multiplier * i;
         snprintf(name, 100, "plicense %s", license.toString(true).c_str());
         
         tr.test(name);
         {
            PayeeList list;
            
            list->append() = createPayee(
               1, PAYEE_AMOUNT_TYPE_PLICENSE, PayeeUnresolved,
               "", "1.00", "", "one", PayeeRegular);
            list->append() = createPayee(
               1, PAYEE_AMOUNT_TYPE_PLICENSE, PayeeUnresolved,
               "", "2.00", "", "two", PayeeRegular);
            list->append() = createPayee(
               2, PAYEE_AMOUNT_TYPE_PLICENSE, PayeeUnresolved,
               "", "0.50", "", "three", PayeeRegular);
            list->append() = createPayee(
               3, PAYEE_AMOUNT_TYPE_PLICENSE, PayeeUnresolved,
               "", "0.00", "", "four", PayeeRegular);
            list->append() = createPayee(
               4, PAYEE_AMOUNT_TYPE_PLICENSE, PayeeUnresolved,
               "", "0.33", "", "five", PayeeRegular);
            list->append() = createPayee(
               5, PAYEE_AMOUNT_TYPE_PLICENSE, PayeeUnresolved,
               "", "0.10", "0.15", "six", PayeeRegular);
            
            Tools::resolvePayeeAmounts(list, &total, &license);
            
            PayeeList assertList;
            
            temp = "1.00";
            temp *= license;
            assertList->append() = createPayee(
               1, PAYEE_AMOUNT_TYPE_PLICENSE, PayeeResolved,
               temp.toString(true).c_str(),
               "1.00", "", "one", PayeeRegular);
            
            temp = "2.00";
            temp *= license;
            assertList->append() = createPayee(
               1, PAYEE_AMOUNT_TYPE_PLICENSE, PayeeResolved,
               temp.toString(true).c_str(),
               "2.00", "", "two", PayeeRegular);
            
            temp = "0.50";
            temp *= license;
            assertList->append() = createPayee(
               2, PAYEE_AMOUNT_TYPE_PLICENSE, PayeeResolved,
               temp.toString(true).c_str(),
               "0.50", "", "three", PayeeRegular);
            
            temp = "0.00";
            temp *= license;
            assertList->append() = createPayee(
               3, PAYEE_AMOUNT_TYPE_PLICENSE, PayeeResolved,
               temp.toString(true).c_str(),
               "0.00", "", "four", PayeeRegular);
            
            temp = "0.33";
            temp *= license;
            assertList->append() = createPayee(
               4, PAYEE_AMOUNT_TYPE_PLICENSE, PayeeResolved,
               temp.toString(true).c_str(),
               "0.33", "", "five", PayeeRegular);
            
            temp = "0.10";
            temp *= license;
            min = "0.15";
            if(min > temp)
            {
               temp = min;
            }
            assertList->append() = createPayee(
               5, PAYEE_AMOUNT_TYPE_PLICENSE, PayeeResolved,
               temp.toString(true).c_str(),
               "0.10", "0.15", "six", PayeeRegular);
            
            assertComparePayeeLists(assertList, list);
            BigDecimal expectedTotal(totals[i]);
            assert(total == expectedTotal);
         }
         tr.pass();
      }
   }
   
   tr.test(PAYEE_AMOUNT_TYPE_PCUMULATIVE);
   {
      PayeeList list;
      
      list->append() = createPayee(
         1, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeUnresolved,
         "1.00", "", "", "", PayeeRegular);
      list->append() = createPayee(
         1, PAYEE_AMOUNT_TYPE_PCUMULATIVE, PayeeUnresolved, 
         "", "0.10", "", "", PayeeRegular);
      list->append() = createPayee(
         2, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeUnresolved,
         "1.00", "", "", "", PayeeRegular);
      list->append() = createPayee(
         2, PAYEE_AMOUNT_TYPE_PCUMULATIVE, PayeeUnresolved,
         "", "1.00", "", "", PayeeRegular);
      
      BigDecimal total;
      Tools::resolvePayeeAmounts(list, &total);
      
      PayeeList assertList;
      
      assertList->append() = createPayee(
         1, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeResolved,
         "1.00", "", "", "", PayeeRegular);
      assertList->append() = createPayee(
         1, PAYEE_AMOUNT_TYPE_PCUMULATIVE, PayeeResolved,
         "0.10", "0.10", "", "", PayeeRegular);
      assertList->append() = createPayee(
         2, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeResolved,
         "1.00", "", "", "", PayeeRegular);
      assertList->append() = createPayee(
         2, PAYEE_AMOUNT_TYPE_PCUMULATIVE, PayeeResolved,
         "2.10", "1.00", "", "", PayeeRegular);
      
      assertComparePayeeLists(assertList, list);
      BigDecimal expectedTotal("4.20");
      assert(total == expectedTotal);
   }
   tr.pass();
   
   tr.test(PAYEE_AMOUNT_TYPE_PTOTAL);
   {
      PayeeList list;
      
      list->append() = createPayee(
         1, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeUnresolved,
         "1.00", "", "", "", PayeeRegular);
      list->append() = createPayee(
         1, PAYEE_AMOUNT_TYPE_PTOTAL, PayeeUnresolved,
         "", "0.10", "", "", PayeeRegular);
      list->append() = createPayee(
         1, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeUnresolved,
         "1.00", "", "", "", PayeeRegular);
      list->append() = createPayee(
         1, PAYEE_AMOUNT_TYPE_PTOTAL, PayeeUnresolved,
         "", "0.50", "", "", PayeeRegular);
      
      BigDecimal total;
      Tools::resolvePayeeAmounts(list, &total);
      
      PayeeList assertList;
      
      assertList->append() = createPayee(
         1, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeResolved,
         "1.00", "", "", "", PayeeRegular);
      assertList->append() = createPayee(
         1, PAYEE_AMOUNT_TYPE_PTOTAL, PayeeResolved,
         "0.20", "0.10", "", "", PayeeRegular);
      assertList->append() = createPayee(
         1, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeResolved,
         "1.00", "", "", "", PayeeRegular);
      assertList->append() = createPayee(
         1, PAYEE_AMOUNT_TYPE_PTOTAL, PayeeResolved,
         "1.00", "0.50", "", "", PayeeRegular);
      
      assertComparePayeeLists(assertList, list);
      BigDecimal expectedTotal("3.20");
      assert(total == expectedTotal);
   }
   tr.pass();
   
   tr.test("Tax");
   {
      PayeeList list;
      
      list->append() = createPayee(
         1, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeUnresolved,
         "1.00", "", "", "", PayeeRegular);
      list->append() = createPayee(
         1, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeUnresolved,
         "1.00", "", "", "", PayeeRegular);
      list->append() = createPayee(
         2, PAYEE_AMOUNT_TYPE_TAX, PayeeUnresolved,
         "", "0.05", "", "", PayeeRegular);
      
      BigDecimal total;
      Tools::resolvePayeeAmounts(list, &total);
      
      PayeeList assertList;
      
      assertList->append() = createPayee(
         1, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeResolved,
         "1.00", "", "", "", PayeeRegular);
      assertList->append() = createPayee(
         1, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeResolved,
         "1.00", "", "", "", PayeeRegular);
      assertList->append() = createPayee(
         2, PAYEE_AMOUNT_TYPE_TAX, PayeeResolved,
         "0.10", "0.05", "", "", PayeeRegular);
      
      assertComparePayeeLists(assertList, list);
      BigDecimal expectedTotal("2.10");
      assert(total == expectedTotal);
   }
   tr.pass();
   
   tr.test("Tax2");
   {
      PayeeList list;
      
      list->append() = createPayee(
         1, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeUnresolved,
         "1.00", "", "", "", PayeeRegular);
      list->append() = createPayee(
         1, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeUnresolved, 
         "1.00", "", "", "", PayeeRegular);
      list->append() = createPayee(
         2, PAYEE_AMOUNT_TYPE_TAX, PayeeUnresolved,
         "", "0.10", "", "", PayeeRegular);
      list->append() = createPayee(
         3, PAYEE_AMOUNT_TYPE_TAX, PayeeUnresolved,
         "", "0.10", "", "", PayeeRegular);
      
      BigDecimal total;
      Tools::resolvePayeeAmounts(list, &total);
      
      PayeeList assertList;
      
      assertList->append() = createPayee(
         1, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeResolved,
         "1.00", "", "", "", PayeeRegular);
      assertList->append() = createPayee(
         1, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeResolved,
         "1.00", "", "", "", PayeeRegular);
      assertList->append() = createPayee(
         2, PAYEE_AMOUNT_TYPE_TAX, PayeeResolved,
         "0.20", "0.10", "", "", PayeeRegular);
      assertList->append() = createPayee(
         3, PAYEE_AMOUNT_TYPE_TAX, PayeeResolved,
         "0.20", "0.10", "", "", PayeeRegular);
      
      assertComparePayeeLists(assertList, list);
      BigDecimal expectedTotal("2.40");
      assert(total == expectedTotal);
   }
   tr.pass();
   
   tr.test("full buy w/ tax");
   {
      // create, resolve, and check license
      PayeeList licenseList;
      
      licenseList->append() = createPayee(
         DISTRIBUTOR, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeUnresolved,
         "0.65", "", "", "Distributor Royalties", PayeeRegular);
      licenseList->append() = createPayee(
         BITMUNK, PAYEE_AMOUNT_TYPE_PTOTAL, PayeeUnresolved,
         "", "0.15", "0.15", "Bitmunk Fee", PayeeTaxExempt);
      licenseList->append() = createPayee(
         TAX, PAYEE_AMOUNT_TYPE_TAX, PayeeUnresolved,
         "", "0.065", "", "Sales Tax", PayeeRegular);
      
      // zero royalty on the license
      BigDecimal license;
      Tools::resolvePayeeAmounts(licenseList, &license);
      
      PayeeList licenseListCheck;
      
      licenseListCheck->append() = createPayee(
         DISTRIBUTOR, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeResolved,
         "0.65", "", "", "CD Baby Royalties", PayeeRegular);
      licenseListCheck->append() = createPayee(
         BITMUNK, PAYEE_AMOUNT_TYPE_PTOTAL, PayeeResolved,
         "0.15", "0.15", "0.15", "Bitmunk Fee", PayeeTaxExempt);
      licenseListCheck->append() = createPayee(
         TAX, PAYEE_AMOUNT_TYPE_TAX, PayeeResolved,
         "0.04225", "0.065", "", "Sales Tax", PayeeRegular);
      
      assert(license == "0.84225");
      assertComparePayeeLists(licenseList, licenseListCheck);

      // create, resolve, and check data
      PayeeList dataList;
      
      dataList->append() = createPayee(
         SELLER, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeUnresolved,
         "0.02", "", "", "Seller Fee", PayeeRegular);
      dataList->append() = createPayee(
         TAX, PAYEE_AMOUNT_TYPE_TAX, PayeeUnresolved,
         "", "0.065", "", "Sales Tax", PayeeRegular);
      dataList->append() = createPayee(
         BITMUNK, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeUnresolved,
         "0.01", "", "", "Bitmunk Fee", PayeeTaxExempt);
      
      BigDecimal data;
      Tools::resolvePayeeAmounts(dataList, &data, &license);
      
      PayeeList dataListCheck;
      
      dataListCheck->append() = createPayee(
         SELLER, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeResolved,
         "0.02", "", "", "Seller Fee", PayeeRegular);
      dataListCheck->append() = createPayee(
         TAX, PAYEE_AMOUNT_TYPE_TAX, PayeeResolved,
         "0.0013", "0.065", "", "Sales Tax", PayeeRegular);
      dataListCheck->append() = createPayee(
         BITMUNK, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeResolved,
         "0.01", "", "", "Bitmunk Fee", PayeeTaxExempt);

      BigDecimal total = license + data;
      //dumpDynamicObject(dataList);
      //dumpDynamicObject(dataListCheck);
      printf("d:%s l:%s t: %s\n",
         data.toString(true).c_str(),
         license.toString(true).c_str(),
         total.toString(true).c_str());
      assert(data == "0.0313");
      assertComparePayeeLists(dataList, dataListCheck);
      assert(total == "0.87355");
   }
   tr.pass();
   
   tr.test("artist+dist+isp+seller");
   {
      // create, resolve, and check license
      PayeeList licenseList;
      
      licenseList->append() = createPayee(
         ARTIST, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeUnresolved,
         "8.00", "", "", "Artist Royalties", PayeeRegular);
      licenseList->append() = createPayee(
         DISTRIBUTOR, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeUnresolved,
         "2.00", "", "", "Distributor Fee", PayeeRegular);
      licenseList->append() = createPayee(
         BITMUNK, PAYEE_AMOUNT_TYPE_PTOTAL, PayeeUnresolved,
         "", "0.15", "0.15", "Bitmunk Fee", PayeeTaxExempt);
      licenseList->append() = createPayee(
         TAX, PAYEE_AMOUNT_TYPE_TAX, PayeeUnresolved,
         "", "0.065", "", "Sales Tax", PayeeRegular);
      
      // zero royalty on the license
      BigDecimal license;
      Tools::resolvePayeeAmounts(licenseList, &license);
      
      PayeeList licenseListCheck;
      
      licenseListCheck->append() = createPayee(
         ARTIST, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeResolved,
         "8.00", "", "", "Artist Royalties", PayeeRegular);
      licenseListCheck->append() = createPayee(
         DISTRIBUTOR, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeResolved,
         "2.00", "", "", "Distributer Fee", PayeeRegular);
      licenseListCheck->append() = createPayee(
         BITMUNK, PAYEE_AMOUNT_TYPE_PTOTAL, PayeeResolved,
         "1.50", "0.15", "0.15", "Bitmunk Fee", PayeeTaxExempt);
      licenseListCheck->append() = createPayee(
         TAX, PAYEE_AMOUNT_TYPE_TAX, PayeeResolved,
         "0.65", "0.065", "", "Sales Tax", PayeeRegular);
      
      assert(license == "12.15");
      assertComparePayeeLists(licenseList, licenseListCheck);

      // create, resolve, and check data
      PayeeList dataList;
      
      // rate per GB and media size
      BigDecimal grate("0.19");
      BigDecimal gsize("4.5");
      BigDecimal gfee = gsize * grate;
      string datafee = gfee.toString(true);
      
      dataList->append() = createPayee(
         ISP, PAYEE_AMOUNT_TYPE_PTOTAL, PayeeUnresolved,
         "", "0.01", "0.01", "ISP Service Fee", PayeeRegular);
      dataList->append() = createPayee(
         ISP, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeUnresolved,
         datafee.c_str(), "", "", "ISP Data Fee", PayeeRegular);
      dataList->append() = createPayee(
         SELLER, PAYEE_AMOUNT_TYPE_PTOTAL, PayeeUnresolved,
         "", "0.02", "0.02", "Seller Fee", PayeeRegular);
      dataList->append() = createPayee(
         TAX, PAYEE_AMOUNT_TYPE_TAX, PayeeUnresolved,
         "", "0.065", "", "Sales Tax", PayeeRegular);
      dataList->append() = createPayee(
         BITMUNK, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeUnresolved,
         "0.01", "", "", "Bitmunk Key Fee", PayeeTaxExempt);
      
      BigDecimal data;
      Tools::resolvePayeeAmounts(dataList, &data, &license);
      
      PayeeList dataListCheck;
      
      dataListCheck->append() = createPayee(
         ISP, PAYEE_AMOUNT_TYPE_PTOTAL, PayeeResolved,
         "0.01", "0.01", "0.01", "ISP Service Fee", PayeeRegular);
      dataListCheck->append() = createPayee(
         ISP, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeResolved,
         "0.855", "", "", "ISP Data Fee", PayeeRegular);
      dataListCheck->append() = createPayee(
         SELLER, PAYEE_AMOUNT_TYPE_PTOTAL, PayeeResolved,
         "0.02", "0.02", "0.02", "Seller Fee", PayeeRegular);
      dataListCheck->append() = createPayee(
         TAX, PAYEE_AMOUNT_TYPE_TAX, PayeeResolved,
         "0.057525", "0.065", "", "Sales Tax", PayeeRegular);
      dataListCheck->append() = createPayee(
         BITMUNK, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeResolved,
         "0.01", "", "", "Bitmunk Key Fee", PayeeTaxExempt);

      BigDecimal total = license + data;
      //dumpDynamicObject(dataList);
      //dumpDynamicObject(dataListCheck);
      printf("d:%s l:%s t: %s\n",
         data.toString(true).c_str(),
         license.toString(true).c_str(),
         total.toString(true).c_str());
      assert(data == "0.952525");
      assertComparePayeeLists(dataList, dataListCheck);
      assert(total == "13.102525");
   }
   tr.pass();

   tr.test("cdbaby peerbuy");
   {
      // create, resolve, and check license
      PayeeList licenseList;
      
      licenseList->append() = createPayee(
         BITMUNK, PAYEE_AMOUNT_TYPE_PTOTAL, PayeeUnresolved,
         "", "0.1500000", "0.1500000", "Bitmunk Marketplace Service", 
         PayeeTaxExempt);
      licenseList->append() = createPayee(
         DISTRIBUTOR, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeUnresolved,
         "0.5965000", "", "", "CD Baby Artist Royalty", PayeeTaxExempt);
      licenseList->append() = createPayee(
         DISTRIBUTOR, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeUnresolved,
         "0.0535000", "", "", "CD Baby 9% Digital Distribution Cost", 
         PayeeTaxExempt);
      
      // zero royalty on the license
      BigDecimal license;
      Tools::resolvePayeeAmounts(licenseList, &license);
      
      PayeeList licenseListCheck;
      
      licenseListCheck->append() = createPayee(
         BITMUNK, PAYEE_AMOUNT_TYPE_PTOTAL, PayeeResolved,
         "0.1500000", "0.1500000", "0.1500000", "Bitmunk Marketplace Service", 
         PayeeTaxExempt);
      licenseListCheck->append() = createPayee(
         DISTRIBUTOR, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeResolved,
         "0.5965000", "", "", "CD Baby Artist Royalty", PayeeTaxExempt);
      licenseListCheck->append() = createPayee(
         DISTRIBUTOR, PAYEE_AMOUNT_TYPE_FLATFEE, PayeeResolved,
         "0.0535000", "", "", "CD Baby 9% Digital Distribution Cost", 
         PayeeTaxExempt);
      
      assert(license == "0.8000000");
      assertComparePayeeLists(licenseList, licenseListCheck);

      // create, resolve, and check data
      PayeeList dataList;
      
      dataList->append() = createPayee(
         BITMUNK, PAYEE_AMOUNT_TYPE_PLICENSE, PayeeUnresolved,
         "", "0.0800000", "0.0500000", "Bitmunk Download Service", 
         PayeeTaxExempt);

      BigDecimal data;
      Tools::resolvePayeeAmounts(dataList, &data, &license);
      
      PayeeList dataListCheck;

      dataListCheck->append() = createPayee(
         BITMUNK, PAYEE_AMOUNT_TYPE_PLICENSE, PayeeResolved,
         "0.0640000", "0.0800000", "0.0500000", "Bitmunk Download Service", 
         PayeeTaxExempt);

      BigDecimal total = license + data;
      //dumpDynamicObject(dataList);
      //dumpDynamicObject(dataListCheck);
      printf("d:%s l:%s t: %s\n",
         data.toString(true).c_str(),
         license.toString(true).c_str(),
         total.toString(true).c_str());
      assert(data == "0.0640000");
      assertComparePayeeLists(dataList, dataListCheck);
      assert(total == "0.8640000");
   }
   tr.pass();
   
   tr.test("Combo");
   {
      // FIXME:
   }
   tr.pass();
   
   tr.test("Double Resolution");
   {
      // FIXME:
   }
   tr.pass();
   
   tr.ungroup();
}

static void runValidatorTest(TestRunner& tr)
{
   tr.group("Validators");

   tr.test("valid zeroMoney");
   {
      const char* tests[] = {"0", "0.00", "0.0000000", NULL};
      v::ValidatorRef v = BitmunkValidator::zeroMoney();
      DynamicObject d;
      for(int i = 0; tests[i] != NULL; ++i)
      {
         d = tests[i];
         bool success = v->isValid(d);
         assertNoException();
         assert(success);
      }
   }
   tr.pass();

   tr.test("invalid zeroMoney");
   {
      const char* tests[] = {"0.", "0.0", ".0", "1.00", NULL};
      v::ValidatorRef v = BitmunkValidator::zeroMoney();
      DynamicObject d;
      for(int i = 0; tests[i] != NULL; ++i)
      {
         d = tests[i];
         bool success = v->isValid(d);
         assertException();
         assert(!success);
         Exception::clear();
      }
   }
   tr.pass();

   tr.ungroup();
}

static bool run(TestRunner& tr)
{
   if(tr.isDefaultEnabled())
   {
      runProfileTest(tr);
      runPayeeResolveTest(tr);
      runValidatorTest(tr);
   }
   return true;
}

MO_TEST_MODULE_FN("bitmunk.tests.common.test", "1.0", run)
