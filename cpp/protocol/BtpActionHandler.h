/*
 * Copyright (c) 2007-2008 Digital Bazaar, Inc.  All rights reserved.
 */
#ifndef bitmunk_protocol_BtpActionHandler_H
#define bitmunk_protocol_BtpActionHandler_H

#include "bitmunk/protocol/BtpAction.h"

namespace bitmunk
{
namespace protocol
{

/**
 * A BtpActionHandler handles a BtpAction in some fashion. Its operator()
 * should receive the content of the action and then, if it is to respond,
 * it should send the result or an exception as deemed appropriate.
 * 
 * @author Dave Longley
 */
class BtpActionHandler
{
protected:
   /**
    * The type of btp authentication that should be used with this action.
    */
   BtpAction::BtpAuth mAuthType;
   
   /**
    * True if a secure connection is required, false if not.
    */
   bool mSecureConnectionOnly;
   
public:
   /**
    * Creates a new BtpActionHandler.
    * 
    * @param auth the type of btp authentication that should be used on the
    *             actions this handler handles.
    */
   BtpActionHandler(BtpAction::BtpAuth auth);
   
   /**
    * Destructs this BtpActionHandler.
    */
   virtual ~BtpActionHandler();
   
   /**
    * Checks if this action can be performed.  This method is appropriate for
    * early security checks.
    * 
    * @param action the BtpAction to check.
    * 
    * @return true if this action can be performed. 
    */
   virtual bool canPerformAction(bitmunk::protocol::BtpAction* action);

   /**
    * Performs the action.  Do whatever is necessary to carry out this
    * action.
    * 
    * @param action the BtpAction to handle.
    */
   virtual void performAction(bitmunk::protocol::BtpAction* action);

   /**
    * Handles the passed BtpAction, receiving its content and sending a result
    * or exception as appropriate.
    * 
    * The action's btp security should be checked according to the set
    * BtpAuth type. Outgoing security can be disabled on an action by clearing
    * its agent profile via action->setAgentProfile(NULL).
    * 
    * The default implementation checks canPerformAction() then calls
    * performAction().
    * 
    * @param action the BtpAction to handle.
    */
   virtual void operator()(BtpAction* action);
   
   /**
    * Determines if a btp security check is required for the passed action
    * based on its request contents. If the passed action has authentication
    * data or if this handler requires it, this method will return true.
    * 
    * @param action the action to check.
    * 
    * @return true if a btp security check is required for the passed action.
    */
   virtual bool securityCheckRequired(bitmunk::protocol::BtpAction* action);
   
   /**
    * Returns true if this handler requires a secure connection (SSL), false
    * if not.
    * 
    * @return true if this handler requires a secure connection, false if not.
    */
   virtual bool secureConnectionRequired();
};

// type definition for a reference counted BtpActionHandler
typedef monarch::rt::Collectable<BtpActionHandler> BtpActionHandlerRef;

} // end namespace protocol
} // end namespace bitmunk
#endif
