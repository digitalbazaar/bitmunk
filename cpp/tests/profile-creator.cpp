/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include <iostream>

// openssl includes
#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/engine.h>

#include "bitmunk/test/Tester.h"
#include "monarch/test/Test.h"
#include "monarch/test/TestModule.h"


#include "bitmunk/common/Profile.h"
#include "monarch/crypto/AsymmetricKeyFactory.h"
#include "monarch/data/json/JsonWriter.h"
#include "monarch/io/FileOutputStream.h"

using namespace std;
using namespace bitmunk::common;
using namespace monarch::crypto;
using namespace monarch::data::json;
using namespace monarch::io;
using namespace monarch::rt;

int main()
{
   // initialize openssl
   ERR_load_crypto_strings();
   SSL_library_init();
   SSL_load_error_strings();
   OpenSSL_add_all_algorithms();

   cout << "This program will create two files:" << endl;
   cout << "'<username>.profile' and '<username>.pub'" << endl << endl;

   char input[50];

   cout << "Enter profile username: ";
   cin.getline(input, 50);
   string username = input;

   cout << "Enter profile user ID: ";
   cin.getline(input, 50);
   UserId userId = strtoull(input, NULL, 10);

   cout << "Enter profile ID: ";
   cin.getline(input, 50);
   ProfileId profileId = strtoull(input, NULL, 10);

   cout << "Enter profile password: ";
   cin.getline(input, 50);
   string password = input;

//   cout << "username=" << username << endl;
//   cout << "user ID=" << userId << endl;
//   cout << "profile ID=" << profileId << endl;
//   cout << "password=" << password << endl;

   ProfileRef profile = new Profile();
   profile->setUserId(userId);
   profile->setId(profileId);
   PublicKeyRef publicKey = profile->generate();

   if(!publicKey.isNull())
   {
      // write out profile
      {
         string filename;
         filename.append(username);
         filename.append(".profile");
         File file(filename.c_str());
         FileOutputStream fos(file);
         profile->save(password.c_str(), &fos);
         fos.close();
      }

      // write out public key
      if(!Exception::isSet())
      {
         string filename;
         filename.append(username);
         filename.append(".pub");
         File file(filename.c_str());
         FileOutputStream fos(file);

         // use factory to write public key to PEM string
         AsymmetricKeyFactory afk;
         string publicKeyStr = afk.writePublicKeyToPem(publicKey);
         fos.write(publicKeyStr.c_str(), publicKeyStr.length());
         fos.close();
      }
   }

   // clean up openssl
   ERR_remove_state(0);
   ENGINE_cleanup();
   ERR_free_strings();
   EVP_cleanup();
   CRYPTO_cleanup_all_ex_data();

   if(Exception::isSet())
   {
      DynamicObject dyno = Exception::getAsDynamicObject();
      JsonWriter::writeToStdOut(dyno);
      exit(1);
   }

   return 0;
}
