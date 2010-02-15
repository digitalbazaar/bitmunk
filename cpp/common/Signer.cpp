/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/common/Signer.h"

#include "monarch/crypto/DigitalSignature.h"
#include "monarch/util/Convert.h"

using namespace std;
using namespace monarch::crypto;
using namespace monarch::rt;
using namespace monarch::util;
using namespace bitmunk::common;

bool Signer::sign(ProfileRef& p, string& content, string& signature)
{
   bool rval;

   // create signature
   DigitalSignature* ds = p->createSignature();
   if(ds != NULL)
   {
      // update signature
      ds->update(content.c_str(), content.length());

      // get signature value
      unsigned int length = ds->getValueLength();
      char sig[length];
      ds->getValue(sig, length);

      // clean up signature
      delete ds;

      // hex-encode signature
      signature = Convert::bytesToHex(sig, length);
      rval = true;
   }
   else
   {
      ExceptionRef e = new Exception(
         "Could not sign data. Profile has no private key.",
         "bitmunk.common.MissingPrivateKey");
      Exception::set(e);
      rval = false;
   }

   return rval;
}

bool Signer::verify(PublicKeyRef& pkey, string& content, const char* signature)
{
   bool rval;

   // create signature
   DigitalSignature ds(pkey);

   // verify signature
   ds.update(content.c_str(), content.length());
   unsigned int length = strlen(signature);
   char data[length];
   Convert::hexToBytes(signature, length, data, length);
   if(!(rval = ds.verify(data, length)))
   {
      ExceptionRef e = new Exception(
         "Could not verify data. DigitalSignature could not be created.",
         "bitmunk.common.BadPublicKey");
      Exception::set(e);
   }

   return rval;
}

void Signer::appendBasicMediaContent(Media& m, string& str)
{
   // concatenation formula
   // ID + buyerId + expiration + type + owner ID + title +
   // release date + publicDomain + ccLicenses + distribution +
   // contributors
   str.append(m["id"]->getString());
   str.append(m["buyerId"]->getString());
   str.append(m["expiration"]->getString());
   str.append(m["type"]->getString());
   str.append(m["ownerId"]->getString());
   str.append(m["title"]->getString());
   str.append(m["releaseDate"]->getString());
   str.append(m["publicDomain"]->getString());
   str.append(m["ccLicenses"]->getString());
   str.append(m["distribution"]->getString());

   // handle contributors
   m["contributors"]->setType(Map);
   appendContributorsContent(m["contributors"], str);
}

void Signer::appendMediaContent(Media& m, string& str)
{
   // concatenation formula
   // basic media content +
   // (if collection): basic media content for each in collection in order
   // payees + dataPayees + licenseAmount + piecePayees
   appendBasicMediaContent(m, str);

   // if collection, append basic media contents for each in collection
   if(strcmp(m["type"]->getString(), "collection") == 0)
   {
      DynamicObjectIterator gi = m["contents"].getIterator();
      while(gi->hasNext())
      {
         DynamicObject& group = gi->next();
         MediaIterator mi = group.getIterator();
         while(mi->hasNext())
         {
            Media& media = mi->next();
            appendBasicMediaContent(media, str);
         }
      }
   }

   // append payment information
   {
      // handle license payees
      m["payees"]->setType(Array);
      PayeeIterator pi = m["payees"].getIterator();
      while(pi->hasNext())
      {
         Payee& p = pi->next();
         appendPayeeContent(p, str);
      }

      // handle payee rules
      m["payeeRules"]->setType(Array);
      pi = m["payeeRules"].getIterator();
      while(pi->hasNext())
      {
         PayeeRule& pr = pi->next();
         appendPayeeRuleContent(pr, str);
      }

      // handle license amount
      str.append(m["licenseAmount"]->getString());

      // handle piece payees
      m["piecePayees"]->setType(Array);
      pi = m["piecePayees"].getIterator();
      while(pi->hasNext())
      {
         Payee& p = pi->next();
         appendPayeeContent(p, str);
      }
   }
}

void Signer::appendPayeeContent(Payee& p, string& str)
{
   // concatenation formula
   // depends on amountType and amountResolved:
   //
   // FlatFee:
   // ID + amountType + amount + taxExempt (1/0)
   //
   // Other with amountResolved == true:
   // ID + amountType + amountResolved (0) + percentage + taxExempt (1/0)
   // + optional description
   //
   // Other with amountResolved == false:
   // ID + amountType + amountResolved (1) + percentage + taxExempt (1/0)
   // + optional description
   str.append(p["id"]->getString());

   const char* amountType = p["amountType"]->getString();
   str.append(amountType);

   if(strcmp(amountType, PAYEE_AMOUNT_TYPE_FLATFEE) == 0)
   {
      str.append(p["amount"]->getString());
   }
   else
   {
      // include amount if resolved
      if(p["amountResolved"]->getBoolean())
      {
         // payee resolved
         str.push_back('1');
         str.append(p["amount"]->getString());
      }
      else
      {
         // payee not resolved
         str.push_back('0');
      }

      str.append(p["percentage"]->getString());
   }

   // FIXME: here for backwards compatibility
   if(p->hasMember("nontaxable"))
   {
      p["taxExempt"] = p["nontaxable"];
   }
   str.push_back(p["taxExempt"]->getBoolean() ? '1' : '0');

   if(p->hasMember("description"))
   {
      str.append(p["description"]->getString());
   }
}

void Signer::appendPayeeRuleContent(PayeeRule& pr, string& str)
{
   // concatenation formula
   // media ID + type + value
   str.append(pr["mediaId"]->getString());
   str.append(pr["type"]->getString());
   str.append(pr["value"]->getString());
}

void Signer::appendContributorsContent(DynamicObject& contributors, string& str)
{
   // concatenation formula
   // contributor type name +
   // foreach(contributor):
   // ID + name + owner ID + role + role ID + description + type

   // FIXME: enforce alphabetization of contributors by type here
   // the stl map iterator only does so by implementation
   DynamicObjectIterator ti = contributors.getIterator();
   while(ti->hasNext())
   {
      DynamicObject& type = ti->next();
      str.append(ti->getName());

      DynamicObjectIterator ci = type.getIterator();
      while(ci->hasNext())
      {
         DynamicObject& c = ci->next();
         str.append(c["id"]->getString());
         str.append(c["name"]->getString());
         str.append(c["ownerId"]->getString());
         str.append(c["role"]->getString());
         str.append(c["roleId"]->getString());
         str.append(c["description"]->getString());
         str.append(c["type"]->getString());
      }
   }
}

void Signer::appendContractSectionContent(ContractSection& cs, string& str)
{
   // concatenation formula:
   // buyer content + seller content + alpha negotiation terms + ware content

   // Note: This data CANNOT include the contract's transaction ID or
   // a buyer's account ID. This data is not available to the seller.
   //
   // The signed data must include all particular contract terms that
   // the seller agrees to that cannot be changed.

   // add buyer details
   if(cs["buyer"]->hasMember("delegateId"))
   {
      str.append(cs["buyer"]["delegateId"]->getString());
   }
   str.append(cs["buyer"]["profileId"]->getString());
   str.append(cs["buyer"]["userId"]->getString());
   str.append(cs["buyer"]["username"]->getString());

   // add seller details
   str.append(cs["seller"]["profileId"]->getString());
   str.append(cs["seller"]["serverId"]->getString());
   str.append(cs["seller"]["url"]->getString());
   str.append(cs["seller"]["userId"]->getString());
   str.append(cs["seller"]["username"]->getString());

   // add negotiation terms
   // FIXME: when we start using this, this code should ENFORCE
   // alphabetizing, here it just assumes based on the map implementation
   cs["negotiationTerms"]->setType(Map);
   DynamicObjectIterator i = cs["negotiationTerms"].getIterator();
   while(i->hasNext())
   {
      DynamicObject& next = i->next();
      str.append(next->getString());
   }

   // add ware content
   appendWareContent(cs["ware"], str);
}

void Signer::appendWareContent(Ware& w, string& str)
{
   // concatenation formula
   // ID + media ID + fileInfos + payees
   str.append(w["id"]->getString());
   str.append(w["mediaId"]->getString());

   w["fileInfos"]->setType(Array);
   DynamicObjectIterator i = w["fileInfos"].getIterator();
   while(i->hasNext())
   {
      FileInfo& fi = i->next();
      appendFileInfoContent(fi, str);
   }

   w["payees"]->setType(Array);
   i = w["payees"].getIterator();
   while(i->hasNext())
   {
      Payee& p = i->next();
      appendPayeeContent(p, str);
   }
}

void Signer::appendFileInfoContent(FileInfo& fi, string& str)
{
   // Note: Bitmunk < 3.2 (used ID + size + mediaId)
   // Bitmunk >= 3.2 uses (ID + contentSize + mediaId)

   // concatenation formula
   // ID + contentSize + mediaId
   str.append(fi["id"]->getString());
   str.append(fi["contentSize"]->getString());
   str.append(fi["mediaId"]->getString());
}

void Signer::appendFilePieceContent(
   const char* csHash, FileId fileId, FilePiece& fp, string& str)
{
   // concatenation formula
   // csHash + fileId + ciphered ("true"/"false") + index + size
   // + if(ciphered) open key data + encrypted piece key data
   // ciphered must be true for peerbuy and must be false for webbuy
   str.append(csHash);
   str.append(fileId);
   str.append(fp["ciphered"]->getString());
   str.append(fp["index"]->getString());
   str.append(fp["size"]->getString());
   if(fp["ciphered"]->getBoolean())
   {
      str.append(fp["openKey"]["data"]->getString());
      str.append(fp["pieceKey"]["data"]->getString());
   }
}

void Signer::appendAddressContent(Address& a, string& str)
{
   // concatenation formula
   // street + locality + region + postalCode + countryCode
   str.append(a["street"]->getString());
   str.append(a["locality"]->getString());
   str.append(a["region"]->getString());
   str.append(a["postalCode"]->getString());
   str.append(a["countryCode"]->getString());
}

void Signer::appendCreditCardContent(CreditCard& c, string& str)
{
   // concatenation formula
   // type + number + expMonth + expYear + cvm + Address
   str.append(c["type"]->getString());
   str.append(c["number"]->getString());
   str.append(c["expMonth"]->getString());
   str.append(c["expMonth"]->getString());
   str.append(c["cvm"]->getString());
   appendAddressContent(c["address"], str);
}

void Signer::appendDepositContent(Deposit& d, string& str)
{
   // concatenation formula
   // version 1:
   // type + date + (type relevant info) + payees + total
   // version 2:
   // type + date + gateway + (type relevant info) + payees + total
   str.append(d["type"]->getString());
   str.append(d["date"]->getString());

   // handle signature version
   if(d->hasMember("signatureVersion"))
   {
      uint32_t version = d["signatureVersion"]->getUInt32();
      if(version == 2)
      {
         str.append(d["gateway"]->getString());
      }
   }

   // FIXME: only supports credit cards for now
   if(strcmp(d["type"]->getString(), "creditcard") == 0)
   {
      appendCreditCardContent(d["source"], str);
   }

   d["payees"]->setType(Array);
   PayeeIterator i = d["payees"].getIterator();
   while(i->hasNext())
   {
      Payee& p = i->next();
      appendPayeeContent(p, str);
   }

   str.append(d["total"]->getString());
}

bool Signer::signMedia(Media& m, ProfileRef& p)
{
   bool rval;

   // get concatenated media content
   string content;
   appendMediaContent(m, content);

   // sign content
   string signature;
   if((rval = sign(p, content, signature)))
   {
      // store signature in Media
      m["signature"] = signature.c_str();
      m["signer"]["userId"] = p->getUserId();
      m["signer"]["profileId"] = p->getId();
   }

   return rval;
}

bool Signer::verifyMedia(Media& m, PublicKeyRef& pkey)
{
   bool rval = true;

   // ensure signature field exists
   if(!m->hasMember("signature"))
   {
      ExceptionRef e = new Exception(
         "Could not verify Media, it has no signature.",
         "bitmunk.common.VerifyMedia");
      Exception::set(e);
      rval = false;
   }
   else
   {
      // get concatenated media content
      string content;
      appendMediaContent(m, content);

      // verify signature
      if(!verify(pkey, content, m["signature"]->getString()))
      {
         ExceptionRef e = new Exception(
            "Could not verify Media, the signature did not match.",
            "bitmunk.common.VerifyMedia");
         Exception::set(e);
         rval = false;
      }
   }

   return rval;
}

bool Signer::signContract(Contract& c, ProfileRef& p)
{
   bool rval;

   // get media signature as content to sign
   string content = c["media"]["signature"]->getString();

   // sign content
   string signature;
   if((rval = sign(p, content, signature)))
   {
      // store signature in Contract
      c["signature"] = signature.c_str();
   }

   return rval;
}

bool Signer::verifyContract(Contract& c, PublicKeyRef& pkey)
{
   bool rval = true;

   // ensure signature field exists
   if(!c->hasMember("signature"))
   {
      ExceptionRef e = new Exception(
         "Could not verify Contract, it has no signature.",
         "bitmunk.common.VerifyContract");
      Exception::set(e);
      rval = false;
   }
   else
   {
      // get media signature as content to verify
      string content = c["media"]["signature"]->getString();

      // verify signature
      if(!verify(pkey, content, c["signature"]->getString()))
      {
         ExceptionRef e = new Exception(
            "Could not verify Contract, the signature did not match.",
            "bitmunk.common.VerifyContract");
         Exception::set(e);
         rval = false;
      }
   }

   return rval;
}

bool Signer::signContractSection(
   ContractSection& cs, ProfileRef& p, bool seller)
{
   bool rval;

   // get concatenated contract section content
   string content;
   appendContractSectionContent(cs, content);

   // sign content
   string signature;
   if((rval = sign(p, content, signature)))
   {
      // store signature in ContractSection
      if(seller)
      {
         cs["sellerSignature"] = signature.c_str();
         cs["sellerProfileId"] = p->getId();
      }
      else
      {
         cs["buyerSignature"] = signature.c_str();
         cs["buyerProfileId"] = p->getId();
      }
   }

   return rval;
}

bool Signer::verifyContractSection(
   ContractSection& cs, PublicKeyRef& sellerKey, PublicKeyRef* buyerKey)
{
   bool rval = true;

   // ensure signature fields exist
   if(!cs->hasMember("sellerSignature"))
   {
      ExceptionRef e = new Exception(
         "Could not verify ContractSection, it has no seller signature.",
         "bitmunk.common.VerifyContractSection");
      Exception::set(e);
      rval = false;
   }
   else if(buyerKey != NULL)
   {
      if(!cs->hasMember("buyerSignature"))
      {
         ExceptionRef e = new Exception(
            "Could not verify ContractSection, it has no buyer signature.",
            "bitmunk.common.VerifyContractSection");
         Exception::set(e);
         rval = false;
      }
   }

   if(rval)
   {
      // get concatenated contract section content
      string content;
      appendContractSectionContent(cs, content);

      // verify seller's signature
      if(!verify(sellerKey, content, cs["sellerSignature"]->getString()))
      {
         // code 0 for seller signature failure
         ExceptionRef e = new Exception(
            "Could not verify ContractSection, the seller signature did "
            "not match.", "bitmunk.common.VerifyContractSection", 0);
         Exception::set(e);
         rval = false;
      }

      // check buyer signature as appropriate
      if(buyerKey != NULL)
      {
         if(!verify(*buyerKey, content, cs["buyerSignature"]->getString()))
         {
            // code 1 for buyer signature failure
            ExceptionRef e = new Exception(
               "Could not verify ContractSection, the buyer signature did "
               "not match.", "bitmunk.common.VerifyContractSection", 1);
            Exception::set(e);
            rval = false;
         }
      }
   }

   return rval;
}

bool Signer::signFilePiece(
   const char* csHash, FileId fileId, FilePiece& fp, ProfileRef& p, bool seller)
{
   bool rval;

   // get concatenated file piece content
   string content;
   appendFilePieceContent(csHash, fileId, fp, content);

   // sign content
   string signature;
   if((rval = sign(p, content, signature)))
   {
      if(seller)
      {
         // store signature in FilePiece
         fp["sellerSignature"] = signature.c_str();
         fp["sellerProfileId"] = p->getId();
      }
      else
      {
         // store signature in FilePiece
         fp["bfpSignature"] = signature.c_str();
         fp["bfpId"] = p->getId();
      }
   }

   return rval;
}

bool Signer::verifyFilePiece(
   const char* csHash,
   FileId fileId, FilePiece& fp, PublicKeyRef& pkey, bool seller)
{
   bool rval = true;

   const char* sigKey = (seller) ? "sellerSignature" : "bfpSignature";

   // ensure signature fields exist
   if(!fp->hasMember(sigKey))
   {
      ExceptionRef e = new Exception(
         "Could not verify FilePiece, it has no signature.",
         "bitmunk.common.VerifyFilePiece");
      e->getDetails()["seller"] = seller;
      e->getDetails()["bfp"] = !seller;
      Exception::set(e);
      rval = false;
   }
   else
   {
      // get concatenated file piece content
      string content;
      appendFilePieceContent(csHash, fileId, fp, content);

      // verify signature
      if(!verify(pkey, content, fp[sigKey]->getString()))
      {
         ExceptionRef e = new Exception(
            "Could not verify FilePiece, the signature did not match.",
            "bitmunk.common.VerifyFilePiece");
         e->getDetails()["seller"] = seller;
         e->getDetails()["bfp"] = !seller;
         Exception::set(e);
         rval = false;
      }
   }

   return rval;
}

bool Signer::signDeposit(Deposit& d, ProfileRef& p)
{
   bool rval;

   // get concatenated deposit content
   d["signatureVersion"] = 2;
   string content;
   appendDepositContent(d, content);

   // sign content
   string signature;
   if((rval = sign(p, content, signature)))
   {
      // store signature in Deposit
      d["signature"] = signature.c_str();
      d["signer"]["userId"] = p->getUserId();
      d["signer"]["profileId"] = p->getId();
   }
   else
   {
      // remove signature version
      d->removeMember("signatureVersion");
   }

   return rval;
}

bool Signer::verifyDeposit(Deposit& d, PublicKeyRef& pkey)
{
   bool rval = true;

   // ensure signature fields exist
   if(!d->hasMember("signature"))
   {
      ExceptionRef e = new Exception(
         "Could not verify Deposit, it has no signature.",
         "bitmunk.common.VerifyDeposit");
      Exception::set(e);
      rval = false;
   }
   else
   {
      // get concatenated deposit content
      string content;
      appendDepositContent(d, content);

      // verify signature
      if(!verify(pkey, content, d["signature"]->getString()))
      {
         ExceptionRef e = new Exception(
            "Could not verify Deposit, the signature did not match.",
            "bitmunk.common.VerifyDeposit");
         Exception::set(e);
         rval = false;
      }
   }

   return rval;
}
