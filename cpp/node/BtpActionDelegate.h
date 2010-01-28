/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_node_BtpActionDelegate_H
#define bitmunk_node_BtpActionDelegate_H

#include "monarch/rt/DynamicObject.h"
#include "monarch/modest/Operation.h"
#include "monarch/modest/OperationDispatcher.h"
#include "bitmunk/protocol/BtpActionHandler.h"
#include "bitmunk/protocol/BtpMessage.h"

namespace bitmunk
{
namespace node
{

// type definition for a resource handler
typedef bitmunk::protocol::BtpActionHandlerRef ResourceHandler;

/**
 * A BtpActionDelegate is a BtpActionHandler that provides a means to map
 * the generic handler function to a handler object's member function that
 * either simply handles a BtpAction or that uses DynamicObjects for input
 * and output to handle a BtpAction.
 *
 * @author Dave Longley
 */
template<typename Handler>
class BtpActionDelegate : public bitmunk::protocol::BtpActionHandler
{
protected:
   /**
    * Typedef for simple action handler function.
    */
   typedef void (Handler::*SimpleActionFunction)(
      bitmunk::protocol::BtpAction* action);

   /**
    * Typedef for dynamic object action handle function.
    */
   typedef bool (Handler::*DynoActionFunction)(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);

   /**
    * The Node the handler runs on.
    */
   Node* mNode;

   /**
    * The actual handler object.
    */
   Handler* mHandler;

   /**
    * The handler's simple action handle function.
    */
   SimpleActionFunction mSimpleFunction;

   /**
    * The handler's dynamic object action handle function.
    */
   DynoActionFunction mDynoFunction;

   /**
    * Set to true if the nodeuser involved in an action must be the
    * same as the one performing the action. Only applicable when btp
    * authentication is used.
    */
   bool mSameUser;

public:
   /**
    * Creates a new BtpActionDelegate with the specified handler object and
    * function for handling an action.
    *
    * The BtpAuth type will determine how the action uses (or doesn't use)
    * btp security authentication. A "nodeuser" must be specified in the
    * action url's query parameters in order for any btp security to be
    * checked.
    *
    * BtpAction::Optional - Incoming btp security will be available for manual
    * checks and outgoing btp security will be enabled. No automatic security
    * checks will be conducted.
    *
    * BtpAction::AuthOptionalSslRequired - Same as BtpAction::AuthOptional
    * except an SSL connection must be used regardless of the use of
    * btp authentication.
    *
    * BtpAction::Required - Incoming btp security checks will be automatic. The
    * handler object's method will only be called if the security check passes.
    * Outgoing btp security will be enabled.
    *
    * @param node the node the handler runs on.
    * @param h the handler object.
    * @param f the handler's function for handling a simple action.
    * @param auth the type of btp security authentication to use.
    */
   BtpActionDelegate(
      Node* node, Handler* h, SimpleActionFunction f,
      bitmunk::protocol::BtpAction::BtpAuth auth);

   /**
    * Creates a new BtpActionDelegate with the specified handler object and
    * function for handling an action.
    *
    * The BtpAuth type will determine how the action uses (or doesn't use)
    * btp security authentication. A "nodeuser" must be specified in the
    * action url's query parameters in order for any btp security to be
    * checked.
    *
    * BtpAction::AuthOptional - Incoming btp security will be available for
    * manual checks and outgoing btp security will be enabled. No automatic
    * security checks will be conducted.
    *
    * BtpAction::AuthOptionalSslRequired - Same as BtpAction::AuthOptional
    * except an SSL connection must be used regardless of the use of
    * btp authentication.
    *
    * BtpAction::AuthRequired - Incoming btp security checks will be automatic.
    * The handler object's method will only be called if the security check
    * passes. Outgoing btp security will be enabled.
    *
    * @param node the node the handler runs on.
    * @param h the handler object.
    * @param f the handler's function for handling a dynamic object action.
    * @param auth the type of btp security authentication to use.
    */
   BtpActionDelegate(
      Node* node, Handler* h, DynoActionFunction f,
      bitmunk::protocol::BtpAction::BtpAuth auth);

   /**
    * Destructs this BtpActionDelegate.
    */
   virtual ~BtpActionDelegate();

   /**
    * Check if this action can be performed.  Performs a check on the security
    * of the message.
    *
    * @param action the BtpAction to handle.
    *
    * @return true if action can be performed with performAction().
    */
   virtual bool canPerformAction(bitmunk::protocol::BtpAction* action);

   /**
    * Handles the passed BtpAction, receiving its content and sending a result
    * or exception as appropriate.
    *
    * The action's btp security should be checked according to the set
    * BtpAuth type. Outgoing security can be disabled on an action by clearing
    * its agent profile via action->setAgentProfile(NULL).
    *
    * @param action the BtpAction to handle.
    */
   virtual void performAction(bitmunk::protocol::BtpAction* action);

   /**
    * Sets whether or not the nodeuser must be the same user as the one
    * performing the btp-authenticated action. Only applicable when btp
    * authentication is used.
    *
    * @param on true to require the action user to be the same as the nodeuser.
    */
   virtual void setSameUserRequired(bool on);
};

template<typename Handler>
BtpActionDelegate<Handler>::BtpActionDelegate(
   Node* node, Handler* h, SimpleActionFunction f,
   bitmunk::protocol::BtpAction::BtpAuth auth) :
   BtpActionHandler(auth)
{
   mNode = node;
   mHandler = h;
   mSimpleFunction = f;
   mDynoFunction = NULL;
   mSameUser = false;
}

template<typename Handler>
BtpActionDelegate<Handler>::BtpActionDelegate(
   Node* node, Handler* h, DynoActionFunction f,
   bitmunk::protocol::BtpAction::BtpAuth auth) :
   BtpActionHandler(auth)
{
   mNode = node;
   mHandler = h;
   mSimpleFunction = NULL;
   mDynoFunction = f;
   mSameUser = false;
}

template<typename Handler>
BtpActionDelegate<Handler>::~BtpActionDelegate()
{
}

template<typename Handler>
bool BtpActionDelegate<Handler>::canPerformAction(
   bitmunk::protocol::BtpAction* action)
{
   // get action messages
   bitmunk::protocol::BtpMessage* inMsg = action->getInMessage();
   bitmunk::protocol::BtpMessage* outMsg = action->getOutMessage();

   // determine profile for btp security
   bool pass = true;
   bitmunk::common::ProfileRef profile;

   // for storing header security status
   bitmunk::protocol::BtpMessage::SecurityStatus hss =
      bitmunk::protocol::BtpMessage::Unchecked;

   // get "nodeuser" from url query
   bool hasUser = false;
   bitmunk::common::UserId id = 0;
   monarch::rt::DynamicObject vars(NULL);
   if(action->getResourceQuery(vars))
   {
      if(vars->hasMember("nodeuser"))
      {
         id = vars["nodeuser"]->getUInt64();
         hasUser = true;
      }
   }

   // Note: This is an optimization to short-circuit code below. If
   // authentication is required but no incoming security, then breach.
   if(pass)
   {
      // see if authentication required and if incoming security was used
      bool reqAuth = (mAuthType == bitmunk::protocol::BtpAction::AuthRequired);
      bool inSec = inMsg->isHeaderSecurityPresent(
         action->getRequest()->getHeader());

      // see if required authentication, but no security present
      if(reqAuth && !inSec)
      {
         // breach
         inMsg->setSecurityStatus(bitmunk::protocol::BtpMessage::Breach);
         pass = action->checkSecurity();
      }
   }

   // determine if an incoming security check must be enforced
   bool checkSeq = pass && securityCheckRequired(action);

   // if a specific nodeuser was requested or security check is required
   // then a user operation has to be created
   if(pass && (hasUser || checkSeq))
   {
      // get the current operation
      monarch::modest::OperationDispatcher* od =
         mNode->getKernel()->getEngine()->getOperationDispatcher();
      monarch::modest::Operation op = od->getCurrentOperation();

      // since this operation is to be served by a particular user,
      // add a user operation to the node and retrieve the user's profile
      if(!mNode->addUserOperation(id, op, &profile))
      {
         // nodeuser not available
         monarch::rt::ExceptionRef e = new monarch::rt::Exception(
            "Resource not found. Requested node user not logged in.",
            "bitmunk.protocol.ResourceNotFound", 404);
         e->getDetails()["nodeuser"] = id;
         monarch::rt::Exception::set(e);
         pass = false;
      }
   }

   // check incoming security as appropriate
   if(pass && checkSeq)
   {
      // set public key source to handle incoming btp security
      inMsg->setPublicKeySource(mNode->getPublicKeyCache());

      // check incoming security on request header only (only updates
      // message security status so it can be queried by the Handler, does
      // not set any exceptions)
      inMsg->checkHeaderSecurity(action->getRequest()->getHeader());

      // if nodeuser must be the same as action user and it isn't, set
      // an exception -- note that we must check this here because user ID
      // will not be available in the incoming message via getUserId() until
      // we have checked incoming security
      if(mSameUser && id != inMsg->getUserId())
      {
         // breach, bad request (http 400)
         monarch::rt::ExceptionRef e = new monarch::rt::Exception(
            "Access to resource denied. Requested node user does not "
            "match action user.",
            "bitmunk.protocol.Unauthorized", 400);
         e->getDetails()["nodeuser"] = id;
         e->getDetails()["userId"] = inMsg->getUserId();
         monarch::rt::Exception::set(e);
         pass = false;
      }
      // setup outgoing btp security
      else if(!profile.isNull())
      {
         outMsg->setUserId(profile->getUserId());
         outMsg->setAgentProfile(profile);
      }
      else
      {
         // since no profile available for node user, resource not available
         monarch::rt::ExceptionRef e = new monarch::rt::Exception(
            "Resource not available. Requested node user "
            "cannot respond securely.",
            "bitmunk.protocol.SecureResourceNotAvailable", 400);
         e->getDetails()["nodeuser"] = id;
         monarch::rt::Exception::set(e);
         pass = false;
      }
   }

   if(pass)
   {
      // store header security status
      hss = inMsg->getSecurityStatus();

      // enforce security check if appropriate
      if(checkSeq)
      {
         pass = action->checkSecurity();
      }
   }

   if(pass)
   {
      // only receive content if using DynoFunction
      if(mDynoFunction != NULL)
      {
         // input content dyno
         monarch::rt::DynamicObject in;

         // receive action content as dyno
         // if security is required, see if message content is breached
         pass =
            action->receiveContent(in) &&
            (checkSeq ? action->checkSecurity() : true);

         if(pass)
         {
            // if content security passed, ensure header security status
            // is passed along so that if the header security failed or was
            // unchecked it can be queried by the handler as such
            // if content security failed anyway, just use it as the status
            if(inMsg->getSecurityStatus() ==
               bitmunk::protocol::BtpMessage::Secure)
            {
               inMsg->setSecurityStatus(hss);
            }
            else if(inMsg->getSecurityStatus() ==
                    bitmunk::protocol::BtpMessage::Unchecked &&
                    hss == bitmunk::protocol::BtpMessage::Breach)
            {
               // content was unchecked, but header failed, use header failure
               inMsg->setSecurityStatus(hss);
            }
         }
      }
   }

   return pass;
}

template<typename Handler>
void BtpActionDelegate<Handler>::performAction(
   bitmunk::protocol::BtpAction* action)
{
   bool success = true;

   if(mSimpleFunction != NULL)
   {
      // pass action to handler
      (mHandler->*mSimpleFunction)(action);
   }
   else if(mDynoFunction != NULL)
   {
      // input/output content dynos
      monarch::rt::DynamicObject in;
      monarch::rt::DynamicObject out;

      // receive action content as dyno
      // (cached from same call in canPerformAction)
      success = action->receiveContent(in);

      // pass action to handler
      if(success && (success = (mHandler->*mDynoFunction)(action, in, out)))
      {
         if(!out.isNull())
         {
            // send result
            action->sendResult(out);
         }
         else
         {
            // send header, but no output
            action->sendResult();
         }
      }
   }

   if(!success && !action->isResultSent())
   {
      // get last exception (create one if necessary -- but this will only
      // happen if a developer has failed to set an exception in a service)
      monarch::rt::ExceptionRef e = monarch::rt::Exception::get();
      if(e.isNull())
      {
         e = new monarch::rt::Exception(
            "An unspecified error occurred. "
            "No exception was set detailing the error.",
            "bitmunk.node.BtpActionError", 500);
         e->getDetails()["resource"] = action->getResource();
         monarch::rt::Exception::set(e);
      }

      // send exception (client's fault if code < 500)
      action->sendException(e, e->getCode() < 500);
   }
}

template<typename Handler>
void BtpActionDelegate<Handler>::setSameUserRequired(bool on)
{
   mSameUser = on;
}

} // end namespace node
} // end namespace bitmunk
#endif
