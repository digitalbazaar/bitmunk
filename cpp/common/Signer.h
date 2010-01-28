/*
 * Copyright (c) 2007-2008 Digital Bazaar, Inc.  All rights reserved.
 */
#ifndef bitmunk_common_Signer_H
#define bitmunk_common_Signer_H

#include "bitmunk/common/Profile.h"
#include "bitmunk/common/TypeDefinitions.h"

namespace bitmunk
{
namespace common
{

/**
 * A Signer provides methods for handling signing specific kinds of
 * content for transfer over Bitmunk.
 * 
 * @author Dave Longley
 */
class Signer
{
public:
   
   /**
    * Signs some content with the given Profile.
    * 
    * @param p the profile to sign with.
    * @param content the content to sign.
    * @param sig to be set to the produced hex-encoded signature.
    * 
    * @return true if successful, false if an exception occurred.
    */
   static bool sign(
      ProfileRef& p,
      std::string& content, std::string& signature);
   
   /**
    * Verifies the signature on some content with the given PublicKey.
    * 
    * @param pkey the PublicKey to verify with.
    * @param content the content to verify the signature on.
    * @param sig the hex-encoded signature (null-terminated).
    * 
    * @return true if verified, false if not.
    */
   static bool verify(
      monarch::crypto::PublicKeyRef& pkey,
      std::string& content, const char* signature);
   
   /**
    * Appends the basic content for a Media that is to be signed to the
    * passed string. This method is called by appendMediaContent().
    * 
    * @param m the Media.
    * @param str the string to append the content to.
    */
   static void appendBasicMediaContent(Media& m, std::string& str);
   
   /**
    * Appends the content for a Media that is to be signed to the
    * passed string. This method will call appendBasicMediaContent for
    * itself and, if it is a collection, its contents.
    * 
    * @param m the Media.
    * @param str the string to append the content to.
    */
   static void appendMediaContent(Media& m, std::string& str);
   
   /**
    * Appends the content for a Payee that is to be signed to the
    * passed string.
    * 
    * @param p the Payee.
    * @param str the string to append the content to.
    */
   static void appendPayeeContent(Payee& p, std::string& str);
   
   /**
    * Appends the content for a PayeeRule that is to be signed to the
    * passed string.
    * 
    * @param pr the PayeeRule.
    * @param str the string to append the content to.
    */
   static void appendPayeeRuleContent(PayeeRule& pr, std::string& str);
   
   /**
    * Appends the content for a map of contributors (type->Array(Contributors)).
    * 
    * @param contributors the map of contributors.
    * @param str the string to append the content to.
    */
   static void appendContributorsContent(
      monarch::rt::DynamicObject& contributors, std::string& str);
   
   /**
    * Appends the content for a ContractSection that is to be signed to the
    * passed string.
    * 
    * @param cs the ContractSection.
    * @param str the string to append the content to.
    */
   static void appendContractSectionContent(
      ContractSection& cs, std::string& str);
   
   /**
    * Appends the content for a Ware that is to be signed to the
    * passed string.
    * 
    * @param w the Ware.
    * @param str the string to append the content to.
    */
   static void appendWareContent(Ware& w, std::string& str);
   
   /**
    * Appends the content for a FileInfo that is to be signed to the
    * passed string.
    * 
    * @param fi the FileInfo.
    * @param str the string to append the content to.
    */
   static void appendFileInfoContent(FileInfo& fi, std::string& str);
   
   /**
    * Appends the content for a FilePiece that is to be signed to the
    * passed string.
    * 
    * @param csHash the hash of the contract section the piece is part of.
    * @param fileId the ID of the file the FilePiece is a part of.
    * @param fp the FilePiece.
    * @param str the string to append the content to.
    */
   static void appendFilePieceContent(
      const char* csHash, FileId fileId, FilePiece& fp, std::string& str);
   
   /**
    * Appends the content for an Address that is to be signed to the
    * passed string.
    * 
    * @param a the Address.
    * @param str the string to append the content to.
    */
   static void appendAddressContent(Address& a, std::string& str);
   
   /**
    * Appends the content for a CreditCard that is to be signed to the
    * passed string.
    * 
    * @param c the CreditCard.
    * @param str the string to append the content to.
    */
   static void appendCreditCardContent(CreditCard& c, std::string& str);
   
   /**
    * Appends the content for a Deposit that is to be signed to the
    * passed string.
    * 
    * @param d the Deposit.
    * @param str the string to append the content to.
    */
   static void appendDepositContent(Deposit& d, std::string& str);
   
   /**
    * Produces a signature for a Media. The signature will be
    * hex-encoded and stored in the Media. Make sure the user ID
    * and expiration date are set on the Media before calling
    * this method.
    *  
    * @param m the Media to produce a signature from.
    * @param p the Profile to sign with.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   static bool signMedia(Media& m, ProfileRef& p);
   
   /**
    * Attempts to verify the signature on a Media.
    * 
    * @param m the Media to verify the signature on.
    * @param pkey the PublicKey to verify the signature with.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   static bool verifyMedia(Media& m, monarch::crypto::PublicKeyRef& pkey);
   
   /**
    * Produces a buyer's signature for a Contract. The signature will be
    * hex-encoded and stored in the Contract.
    * 
    * @param c the Contract to sign.
    * @param p the Profile to sign with.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   static bool signContract(Contract& c, ProfileRef& p);
   
   /**
    * Attempts to verify a buyer's signature on a Contract.
    * 
    * @param c the Contract to verify the signature on.
    * @param pkey the PublicKey to verify the signature with.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   static bool verifyContract(Contract& c, monarch::crypto::PublicKeyRef& pkey);
   
   /**
    * Produces a signature for a ContractSection. The signature will be
    * hex-encoded and stored in the ContractSection.
    * 
    * @param cs the ContractSection to sign.
    * @param p the Profile to sign with.
    * @param seller true if a seller is signing, false if a buyer is signing.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   static bool signContractSection(Contract& cs, ProfileRef& p, bool seller);
   
   /**
    * Attempts to verify the signature(s) on a ContractSection. Since a
    * ContractSection has 2 possible signatures (one by the buyer and
    * one by the seller), two public keys can be passed to this method. If
    * the latter key is not passed, then the buyer's signature will not
    * be checked.
    * 
    * @param cs the ContractSection to verify.
    * @param sellerKey the PublicKey to verify the seller signature with.
    * @param buyerKey the PublicKey to verify the buyer signature with.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   static bool verifyContractSection(
      ContractSection& cs,
      monarch::crypto::PublicKeyRef& sellerKey,
      monarch::crypto::PublicKeyRef* buyerKey = NULL);
   
   /**
    * Produces a signature for a FilePiece. The signature will be
    * hex-encoded and stored in the FilePiece.
    * 
    * @param csHash the hash of the contract section the piece is part of. 
    * @param fileId the ID of the file the FilePiece is a part of.
    * @param fp the FilePiece to produce a signature from.
    * @param p the Profile to sign with.
    * @param seller true if the seller is signing, false if the bfp is.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   static bool signFilePiece(
      const char* csHash,
      FileId fileId, FilePiece& fp, ProfileRef& p, bool seller);
   
   /**
    * Attempts to verify the signature on a FilePiece.
    * 
    * @param csHash the hash of the contract section the piece is part of.
    * @param fileId the ID of the file the FilePiece is a part of.
    * @param fp the FilePiece to verify the signature on.
    * @param pkey the PublicKey to verify the signature with.
    * @param seller true if verifying the seller, false if verifying the bfp.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   static bool verifyFilePiece(
      const char* csHash,
      FileId fileId, FilePiece& fp, monarch::crypto::PublicKeyRef& pkey,
      bool seller);
   
   /**
    * Produces a signature for a Deposit. The signature will be
    * hex-encoded and stored in the Deposit.
    *  
    * @param d the Deposit to sign.
    * @param p the Profile to sign with.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   static bool signDeposit(Deposit& d, ProfileRef& p);
   
   /**
    * Attempts to verify the signature on a Deposit.
    * 
    * @param d the Deposit to verify the signature on.
    * @param pkey the PublicKey to verify the signature with.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   static bool verifyDeposit(Deposit& d, monarch::crypto::PublicKeyRef& pkey);
};

} // end namespace common
} // end namespace bitmunk
#endif
