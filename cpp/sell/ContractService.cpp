/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS

#include "bitmunk/sell/ContractService.h"

#include "bitmunk/bfp/IBfpModule.h"
#include "bitmunk/common/BitmunkValidator.h"
#include "bitmunk/common/CatalogInterface.h"
#include "bitmunk/common/NegotiateInterface.h"
#include "bitmunk/common/Signer.h"
#include "bitmunk/common/Tools.h"
#include "bitmunk/node/BtpActionDelegate.h"
#include "bitmunk/node/RestResourceHandler.h"
#include "monarch/crypto/AsymmetricKeyFactory.h"
#include "monarch/io/FileInputStream.h"
#include "monarch/util/Convert.h"
#include "monarch/validation/Validation.h"

using namespace std;
using namespace monarch::crypto;
using namespace monarch::http;
using namespace monarch::io;
using namespace monarch::net;
using namespace monarch::rt;
using namespace monarch::util;
using namespace bitmunk::bfp;
using namespace bitmunk::common;
using namespace bitmunk::protocol;
using namespace bitmunk::node;
using namespace bitmunk::sell;
namespace v = monarch::validation;

typedef BtpActionDelegate<ContractService> Handler;

ContractService::ContractService(Node* node, const char* path) :
   NodeService(node, path),
   mUploadThrottlerMap(NULL)
{
}

ContractService::~ContractService()
{
}

bool ContractService::initialize()
{
   bool rval = true;

   mSellerKeyCache.setPublicKeySource(this);

   // create and initialize upload throttler map
   mUploadThrottlerMap = new UploadThrottlerMap();
   rval = mUploadThrottlerMap->initialize(mNode);

   // negotiate
   if(rval)
   {
      // top level resource handlers
      RestResourceHandlerRef negotiate = new RestResourceHandler();
      addResource("/negotiate", negotiate);

      // POST .../negotiate?nodeuser=<sellerId>
      /**
       * Negotiates a contract section with a seller. The buyer provides a
       * contract section with their desired terms, the seller responds with
       * a signed contract section containing their acceptable terms.
       *
       * @tags public-api payswarm-api
       *
       * @return the seller's signed contract section, which the buyer may
       *         elect to accept.
       */
      {
         ResourceHandler h = new Handler(
            mNode, this, &ContractService::negotiateContract,
            BtpAction::AuthRequired);

         v::ValidatorRef qValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         v::ValidatorRef validator = new v::Map(
            "version", new v::Equals(
               "3.0", "Wrong contract version. Must be '3.0'"),
            "id", new v::Int(v::Int::NonNegative),
            "media", new v::Map(
               "id", new v::Int(v::Int::Positive),
               "type", new v::Type(String),
               "ownerId", new v::Int(v::Int::Positive),
               "title", new v::Type(String),
               "releaseDate", new v::Regex(
                  "^[0-9]{4}-"
                  "(0[1-9]|1[0-2])-"
                  "([0-2][0-9]|3[0-1])$",
                  "Date must be of the format 'YYYY-MM-DD'."),
               "publicDomain", new v::Type(Boolean),
               "ccLicenses", new v::Type(String),
               "distribution", new v::Type(String),
               "payees", BitmunkValidator::licensePayees(),
               "payeeRules", new v::All(
                  new v::Type(Array),
                  new v::Each(new v::Map(
                     "mediaId", new v::Int(v::Int::NonNegative),
                     "type", new v::Type(String),
                     "value", new v::Type(String),
                     NULL)),
                  NULL),
               "licenseAmount", BitmunkValidator::preciseMoney(),
               "piecePayees", BitmunkValidator::piecePayees(),
               "buyerId", new v::Int(v::Int::Positive),
               // FIXME: add expiration when we add it to media signing
               "signature", new v::Type(String),
               "signer", new v::Map(
                  "userId", new v::Int(v::Int::Positive),
                  "profileId", new v::Int(v::Int::Positive),
                  NULL),
               NULL),
            "buyer", new v::Map(
               "userId", new v::Int(v::Int::Positive),
               "username", new v::Optional(new v::Type(String)),
               "profileId", new v::Int(v::Int::Positive),
               "delegateId", new v::Optional(new v::Int(v::Int::Positive)),
               NULL),
            "sections", new v::All(
               new v::Type(Map),
               // for each array of contract sections, for each section
               new v::Each(new v::Each(
                  new v::Map(
                     "contractId",
                        new v::Optional(new v::Int(v::Int::NonNegative)),
                     "buyer", new v::Map(
                        "userId", new v::Int(v::Int::Positive),
                        "username", new v::Optional(new v::Type(String)),
                        "profileId", new v::Int(v::Int::Positive),
                        "delegateId",
                           new v::Optional(new v::Int(v::Int::Positive)),
                        NULL),
                     "seller", new v::Map(
                        "userId", new v::Int(v::Int::Positive),
                        "serverId", new v::Int(v::Int::Positive),
                        "username", new v::Optional(new v::Type(String)),
                        "url", new v::Type(String),
                        NULL),
                     "ware", new v::Map(
                        "id", new v::All(
                           new v::Type(String),
                           new v::Min(
                              1, "Ware ID too short. 1 character minimum."),
                           NULL),
                        NULL),
                     "webbuy", new v::Optional(new v::Type(Boolean)),
                     "negotiationTerms", new v::Optional(new v::Type(Map)),
                     NULL))),
               NULL),
            NULL);

         negotiate->addHandler(h, BtpMessage::Post, 0, &qValidator, &validator);
      }
   }

   // file piece
   if(rval)
   {
      RestResourceHandlerRef filePiece = new RestResourceHandler();
      addResource("/filepiece", filePiece);

      // POST .../filepiece?nodeuser=<sellerId>
      /**
       * Retrieves a filepiece from the seller.
       *
       * @tags public-api payswarm-api
       *
       * @return the encrypted data for the filepiece followed by trailers with
       *         signatures and encrypted key(s).
       */
      {
         ResourceHandler h = new Handler(
            mNode, this, &ContractService::getFilePiece,
            BtpAction::AuthRequired);

         v::ValidatorRef qValidator = new v::Map(
            "nodeuser", new v::Int(v::Int::Positive),
            NULL);

         v::ValidatorRef validator = new v::Map(
            "csHash", new v::All(
               new v::Type(String),
               new v::Min(
                  1, "Contract section hash too short. 1 character minimum."),
               NULL),
            "fileId", new v::All(
               new v::Type(String),
               new v::Min(
                  1, "File ID too short. 1 character minimum."),
               NULL),
            "mediaId", new v::Int(v::Int::Positive),
            "index", new v::Int(v::Int::NonNegative),
            // Note: This must be the standard piece size, the seller may
            // truncate if this is the last piece.
            "size", new v::Int(v::Int::NonNegative),
            "peerbuyKey", new v::All(
               new v::Type(String),
               new v::Min(
                  1, "PeerBuy key too short. 1 character minimum."),
               NULL),
            "sellerProfileId", new v::Int(v::Int::Positive),
            "bfpId", new v::Int(v::Int::Positive),
            NULL);

         filePiece->addHandler(h, BtpMessage::Post, 0, &qValidator, &validator);
      }
   }

   return rval;
}

void ContractService::cleanup()
{
   // remove resources
   removeResource("/negotiate");
   removeResource("/filepiece");

   // clean up upload throttler map
   if(mUploadThrottlerMap != NULL)
   {
      delete mUploadThrottlerMap;
      mUploadThrottlerMap = NULL;
   }
}

bool ContractService::negotiateContract(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval;

   // interpret "in" as a Contract with a single ContractSection
   Contract& c = in;

   // confirm SVA signature for media
   PublicKeyRef pkey = mNode->getPublicKeyCache()->getPublicKey(
      1, BM_PROFILE_ID(c["media"]["signer"]["profileId"]), NULL);
   if((rval = !pkey.isNull()))
   {
      // FIXME: this doesn't check to make sure the media matches the
      // media in the ware requested -- or check to see if it is in
      // the collection, so *any* valid media license would pass this test
      if(!(rval = Signer::verifyMedia(c["media"], pkey)))
      {
         ExceptionRef e = new Exception(
            "Media signature is invalid.",
            "bitmunk.sell.BadMediaSignature");
         Exception::set(e);
      }
   }

   if(rval)
   {
      // do negotiation
      NegotiateInterface* ni = dynamic_cast<NegotiateInterface*>(
         mNode->getModuleApiByType("bitmunk.negotiate"));
      if(ni == NULL)
      {
         ExceptionRef e = new Exception(
            "No negotiate module.",
            "bitmunk.sell.MissingNegotiateModule");
         Exception::set(e);
         rval = false;
      }
      else
      {
         // get query variables
         DynamicObject vars;
         action->getResourceQuery(vars);

         // get seller's profile
         UserId sellerId = BM_USER_ID(vars["nodeuser"]);
         ProfileRef profile(NULL);
         if((rval = mNode->getLoginData(sellerId, NULL, &profile)))
         {
            // negotiate contract section
            ContractSection& cs =
               c["sections"][vars["nodeuser"]->getString()][0];
            if((rval = ni->negotiateContractSection(sellerId, c, cs, true)))
            {
               // ensure seller's profile ID is included in section
               BM_ID_SET(cs["seller"]["profileId"], profile->getId());

               // sign contract section
               if((rval = Signer::signContractSection(cs, profile, true)))
               {
                  // if the contract section is not for webbuy, generate
                  // a peerbuy key
                  if(!cs["webbuy"]->getBoolean())
                  {
                     cs["webbuy"] = false;
                     addPeerBuyKey(cs, profile);
                  }

                  // return negotiated contract
                  out = c;
               }
            }
         }
      }
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not negotiate contract.", "bitmunk.sell.NegotiationError");
      Exception::push(e);
   }

   return rval;
}

bool ContractService::getFilePiece(
   BtpAction* action, DynamicObject& in, DynamicObject& out)
{
   bool rval;

   // get the node user
   DynamicObject vars;
   action->getResourceQuery(vars);
   UserId nodeuser = BM_USER_ID(vars["nodeuser"]);

   // "in" includes:
   // csHash          - hash of the contract section the piece is a part of
   // fileId          - ID of the file the piece is a part of
   // mediaId         - ID of the media the file is for
   // index           - the index of the piece
   // size            - the *standard* piece size for all pieces
   // peerbuyKey      - the key to get access to download the piece
   // sellerProfileId - the profile the seller used to sign the section
   // bfpId           - ID of the bfp to use

   const char* hash = in["csHash"]->getString();
   FileId fileId = BM_FILE_ID(in["fileId"]);
   MediaId mediaId = BM_MEDIA_ID(in["mediaId"]);
   uint32_t index = in["index"]->getUInt32();
   uint32_t size = in["size"]->getUInt32();
   const char* key = in["peerbuyKey"]->getString();
   ProfileId profileId = BM_PROFILE_ID(in["sellerProfileId"]);
   BfpId bfpId = BM_BFP_ID(in["bfpId"]);

   // verify that the key is valid
   if((rval = verifyPeerBuyKey(key, hash, nodeuser, profileId)))
   {
      // get catalog interface
      CatalogInterface* ci = dynamic_cast<CatalogInterface*>(
         mNode->getModuleApiByType("bitmunk.catalog"));

      // populate FileInfo
      FileInfo fi;
      BM_ID_SET(fi["id"], fileId);
      BM_ID_SET(fi["mediaId"], mediaId);
      if((rval = ci->populateFileInfo(nodeuser, fi)))
      {
         // ensure file exists
         File file(fi["path"]->getString());
         if(!file->exists())
         {
            ExceptionRef e = new Exception(
               "File not found.", "bitmunk.sell.FileNotFound", 404);
            BM_ID_SET(e->getDetails()["fileId"], BM_FILE_ID(fi["id"]));
            Exception::set(e);
            rval = false;
         }
         else
         {
            // FIXME: change send piece to just pass this key into the function
            // get seller's key to cache and check it
            PublicKeyRef sellerKey = getSellerKey(nodeuser);
            if(sellerKey.isNull())
            {
               ExceptionRef e = new Exception(
                  "Seller's encryption key not found.",
                  "bitmunk.sell.MissingSellerKey");
               Exception::set(e);
               rval = false;
            }
            else
            {
               // get the BFP module interface
               IBfpModule* ibm = dynamic_cast<IBfpModule*>(
                  mNode->getModuleApi("bitmunk.bfp.Bfp"));
               if(ibm == NULL)
               {
                  ExceptionRef e = new Exception(
                     "Could not get bfp module interface. No bfp module found.",
                     "bitmunk.bfp.MissingBfpModule");
                  Exception::set(e);
                  rval = false;
               }
               else
               {
                  // create bfp
                  Bfp* bfp = ibm->createBfp(bfpId);
                  if((rval = (bfp != NULL)))
                  {
                     // FIXME: create a PieceUploader fiber that will use
                     // asynchronous IO to send the file piece to optimize this

                     // send piece (do not send "out")
                     out.setNull();
                     rval = sendPiece(
                        action, bfp, nodeuser, fi, index, size, hash);

                     // free bfp
                     ibm->freeBfp(bfp);
                  }
               }
            }
         }
      }
   }

   if(!rval)
   {
      ExceptionRef e = new Exception(
         "Could not upload FilePiece.",
         "bitmunk.sell.TransmitError");
      Exception::push(e);
   }

   return rval;
}

PublicKeyRef ContractService::getPublicKey(
   UserId uid, ProfileId pid, bool* isDelegate)
{
   PublicKeyRef rval(NULL);

   // get seller key from bitmunk
   Url url;
   url.format("/api/3.0/users/keys/seller/%" PRIu64, uid);
   DynamicObject in;
   if(mNode->getMessenger()->getSecureFromBitmunk(&url, in))
   {
      // use factory to read seller key from PEM string
      const char* pem = in["sellerKey"]->getString();
      AsymmetricKeyFactory afk;
      rval = afk.loadPublicKeyFromPem(pem, strlen(pem));
   }

   // return whether or not public key belongs to a delegate
   if(isDelegate != NULL)
   {
      // always false for seller keys
      *isDelegate = false;
   }

   return rval;
}

PublicKeyRef ContractService::getSellerKey(UserId sellerId)
{
   return mSellerKeyCache.getPublicKey(sellerId, 0, NULL);
}

void ContractService::addPeerBuyKey(ContractSection& cs, ProfileRef& profile)
{
   // Note: The default algorithm for generating a peerbuy key is to sign
   // a contract section hash. This key is only shared between a particular
   // buyer and seller, and therefore, the algorithm can change on that
   // basis. Changing the algorithm would invalidate previously negotiated
   // contract sections with particular buyers. The key is not shared across
   // the network in any other capacity to allow for this flexibility.

   // get hex-encoded contract section hash as bytes
   string hex = Tools::getContractSectionHash(cs);
   cs["hash"] = hex.c_str();

   // sign hash
   DigitalSignature* ds = profile->createSignature();
   ds->update(hex.c_str(), hex.length());

   // get signature value
   unsigned int length = ds->getValueLength();
   char sig[length];
   ds->getValue(sig, length);
   delete ds;

   // hex-encode signature as the peerbuy key
   cs["peerbuyKey"] = Convert::bytesToHex(sig, length).c_str();
}

bool ContractService::verifyPeerBuyKey(
   const char* peerBuyKey, const char* csHash,
   UserId sellerId, ProfileId sellerProfileId)
{
   bool rval;

   PublicKeyRef pkey = mNode->getPublicKeyCache()->getPublicKey(
      sellerId, sellerProfileId, NULL);
   if((rval = !pkey.isNull()))
   {
      // create signature
      DigitalSignature ds(pkey);

      // verify signature on csHash
      ds.update(csHash, strlen(csHash));
      unsigned int length = strlen(peerBuyKey);
      char data[length];
      Convert::hexToBytes(peerBuyKey, length, data, length);
      if(!(rval = ds.verify(data, length)))
      {
         ExceptionRef e = new Exception(
            "Peerbuy key is not valid.",
            "bitmunk.sell.InvalidPeerBuyKey");
         Exception::set(e);
      }
   }
   else
   {
      ExceptionRef e = new Exception(
         "Peerbuy key is not valid.",
         "bitmunk.sell.InvalidPeerBuyKey");
      Exception::push(e);
   }

   return rval;
}

bool ContractService::sendPiece(
   BtpAction* action,
   Bfp* bfp, UserId sellerId, FileInfo& fi, uint32_t index, uint32_t size,
   const char* csHash)
{
   bool rval;

   // get seller's profile
   ProfileRef sellerProfile;
   mNode->getLoginData(sellerId, NULL, &sellerProfile);

   // get seller's key
   PublicKeyRef sellerKey = getSellerKey(sellerId);
   AsymmetricKeyFactory akf;
   string pem = akf.writePublicKeyToPem(sellerKey);

   // initialize bfp for peersell
   if((rval = bfp->initializePeerSell(csHash, pem.c_str())))
   {
      // set up response header
      HttpResponse* response = action->getResponse();
      HttpResponseHeader* header = response->getHeader();
      header->setStatus(200, "OK");
      header->setField("Transfer-Encoding", "chunked");

      // get connection and set bandwidth throttler
      HttpConnection* hc = response->getConnection();
      hc->setBandwidthThrottler(
         mUploadThrottlerMap->getUserThrottler(sellerId), false);

      // send header and save body output stream
      OutputStreamRef os;
      HttpTrailerRef trailer;
      if((rval = action->getOutMessage()->sendHeader(hc, header, os, trailer)))
      {
         // set up file piece
         FilePiece fp;
         fp["index"] = index;
         fp["size"] = size;

         // prepare and start reading file
         if((rval = bfp->preparePeerSellFile(fi) && bfp->startReading(fp)))
         {
            // write data to output stream
            char b[2048];
            int numBytes;
            while(rval && (numBytes = bfp->read(b, 2048)) > 0)
            {
               rval = os->write(b, numBytes);
            }

            if(numBytes == -1)
            {
               // FIXME: log it!
               // read error
               rval = false;
            }
            else
            {
               // sign the file piece as the seller
               if(!Signer::signFilePiece(
                  csHash, fi["id"]->getString(), fp, sellerProfile, true))
               {
                  ExceptionRef e = new Exception(
                     "Seller could not sign file piece.",
                     "bitmunk.sell.FilePieceSignError");
                  Exception::push(e);
                  rval = false;
               }
            }
         }

         // add file piece headers to trailer:

         // actual size of file piece
         trailer->setField("Bitmunk-Piece-Size", fp["size"]->getString());

         // bfp signature on piece (bfpId is known by buyer because it was
         // used in the file piece request)
         trailer->setField("Bitmunk-Bfp-Signature",
            fp["bfpSignature"]->getString());

         // seller signature on piece + seller profile ID
         trailer->setField("Bitmunk-Seller-Signature",
            fp["sellerSignature"]->getString());
         trailer->setField("Bitmunk-Seller-Profile-Id",
            fp["sellerProfileId"]->getString());

         // open key information (to be sent to SVA to unlock piece key)
         trailer->setField("Bitmunk-Open-Key-Algorithm",
            fp["openKey"]["algorithm"]->getString());
         trailer->setField("Bitmunk-Open-Key-Data",
            fp["openKey"]["data"]->getString());
         trailer->setField("Bitmunk-Open-Key-Length",
            fp["openKey"]["length"]->getString());

         // encrypted piece key information
         // (to be unlocked with open key by SVA)
         trailer->setField("Bitmunk-Piece-Key-Algorithm",
            fp["pieceKey"]["algorithm"]->getString());
         trailer->setField("Bitmunk-Piece-Key-Data",
            fp["pieceKey"]["data"]->getString());
         trailer->setField("Bitmunk-Piece-Key-Length",
            fp["pieceKey"]["length"]->getString());
      }

      // close stream (will call finish() and send trailer)
      if(!os.isNull())
      {
         os->close();
      }
   }

   return rval;
}
