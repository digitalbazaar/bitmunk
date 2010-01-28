/*
 * Copyright (c) 2007-2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/protocol/BtpActionHandler.h"

using namespace monarch::rt;
using namespace bitmunk::common;
using namespace bitmunk::protocol;

BtpActionHandler::BtpActionHandler(BtpAction::BtpAuth auth) :
   mAuthType(auth)
{
   // require a secure connection if authentication is required or if ssl is
   mSecureConnectionOnly =
      mAuthType == BtpAction::AuthRequired ||
      mAuthType == BtpAction::AuthOptionalSslRequired;
}

BtpActionHandler::~BtpActionHandler()
{
}

bool BtpActionHandler::canPerformAction(BtpAction* action)
{
   return true;
}

void BtpActionHandler::performAction(BtpAction* action)
{
}

void BtpActionHandler::operator()(BtpAction* action)
{
   // enforce secure connection if appropriate
   if(mSecureConnectionOnly &&
      !action->getRequest()->getConnection()->isSecure())
   {
      // set exception
      ExceptionRef e = new Exception(
         "Secure connection (SSL) required by resource."
         "bitmunk.protocol.SecurityConnectionRequired");
      e->getDetails()["resource"] = action->getResource();
      Exception::set(e);
      
      // do nothing with action, connection will be severed by BtpService
   }
   else
   {
      if(canPerformAction(action))
      {
         performAction(action);
      }
      else
      {
         // send exception (client's fault if code < 500)
         ExceptionRef e = Exception::get();
         action->sendException(e, e->getCode() < 500);
      }
   }
}

bool BtpActionHandler::securityCheckRequired(BtpAction* action)
{
   // see if security required or if incoming security used
   return 
      mAuthType == BtpAction::AuthRequired ||
       action->getInMessage()->isHeaderSecurityPresent(
         action->getRequest()->getHeader());
}

bool BtpActionHandler::secureConnectionRequired()
{
   return mSecureConnectionOnly;
}
