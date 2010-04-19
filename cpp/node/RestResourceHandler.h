/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_node_RestResourceHandler_H
#define bitmunk_node_RestResourceHandler_H

#include "monarch/validation/Validation.h"
#include "bitmunk/protocol/BtpActionHandler.h"
#include "bitmunk/protocol/BtpMessage.h"
#include <map>

namespace bitmunk
{
namespace node
{

/**
 * A RestResourceHandler is a BtpActionHandler dispatcher based on action
 * parameter count and input message method type. It simplifies setting up
 * a RESTful interface. Handlers can also have Validators for both the query
 * parameters and for the input content.
 *
 * The order of processing is important to consider. Under some circumstances
 * it may be more appropriate to do more direct handling of BtpActions.  This
 * class is intended to simplify a more general case.
 *
 * 1. The parameter count is checked. If no match is found then a HTTP 404
 *    type exception is thrown.
 * 2. The input message method type is checked. If no match is found then a
 *    HTTP 405 type exception is thrown.
 * 3. The action's canPerformAction() method is checked. This can be used to
 *    perform early security checks.
 * 4. The query is validated.
 * 5. The content is validated.
 *
 * If all tests pass then the handler's performAction() method is called. The
 * handler can perform any additional specialized tests it needs such as
 * checking permissions.
 *
 * @author David I. Lehn
 */
class RestResourceHandler : public bitmunk::protocol::BtpActionHandler
{
public:
   /**
    * Per-handler flags.
    */
   enum
   {
      // query variables should be processed as arrays
      ArrayQuery = 1 << 0
   };

protected:
   /**
    * Info for handlers.
    */
   struct HandlerInfo
   {
      monarch::validation::ValidatorRef resourceValidator;
      monarch::validation::ValidatorRef queryValidator;
      monarch::validation::ValidatorRef contentValidator;
      bitmunk::protocol::BtpActionHandlerRef handler;
      uint32_t flags;
   };

   /**
    * Map of method type to handler info.
    */
   typedef std::map<bitmunk::protocol::BtpMessage::Type, HandlerInfo> MethodMap;

   /**
    * Map of handler parameter count to method map, where a parameter count
    * of -1 is for arbitrary parameter counts.
    */
   typedef std::map<int, MethodMap> HandlerMap;

   /**
    * Handler info map.
    */
   HandlerMap mHandlers;

public:
   /**
    * Creates a new RestResourceHandler.
    */
   RestResourceHandler();

   /**
    * Destructs this RestResourceHandler.
    */
   virtual ~RestResourceHandler();

   /**
    * Handles the passed BtpAction. The parameter count and input message type
    * are used to further dispatch the action to the appropriate registered
    * handler.
    *
    * @param action the BtpAction to handle.
    */
   virtual void operator()(bitmunk::protocol::BtpAction* action);

   /**
    * Register an action handler for a specific message type and parameter
    * count. Handlers can also have query and content validators. These are
    * automatically checked if present.
    *
    * @param handler an action handler to register.
    * @param type a type to match against the action input message type.
    * @param paramCount the count to match with the action parameter count,
    *                   -1 for an arbitrary number if parameters.
    * @param queryValidator a Validator to check the input message query
    *           (optional)
    * @param contentValidator a Validator to check the input message content
    *           (optional)
    * @param flags flags for this handler.
    */
   virtual void addHandler(
      bitmunk::protocol::BtpActionHandlerRef& handler,
      bitmunk::protocol::BtpMessage::Type type,
      int paramCount = 0,
      monarch::validation::ValidatorRef* queryValidator = NULL,
      monarch::validation::ValidatorRef* contentValidator = NULL,
      uint32_t flags = 0);

   /**
    * Register an action handler for a specific message type. Handlers can
    * also have resource, query, and content validators. These are
    * automatically checked if present.
    *
    * @param handler an action handler to register.
    * @param type a type to match against the action input message type.
    * @param resourceValidator a Validator to check the input message resource
    * @param queryValidator a Validator to check the input message query
    *           (optional)
    * @param contentValidator a Validator to check the input message content
    *           (optional)
    * @param flags flags for this handler.
    */
   virtual void addHandler(
      bitmunk::protocol::BtpActionHandlerRef& handler,
      bitmunk::protocol::BtpMessage::Type type,
      monarch::validation::ValidatorRef* resourceValidator,
      monarch::validation::ValidatorRef* queryValidator = NULL,
      monarch::validation::ValidatorRef* contentValidator = NULL,
      uint32_t flags = 0);

protected:
   /**
    * Finds the handler for the given action.
    *
    * @param action the action to find the handler for.
    *
    * @return the handler iterator, mHandlers.end() if none could be found.
    */
   virtual HandlerMap::iterator findHandler(
      bitmunk::protocol::BtpAction* action);

   /**
    * Handles the passed BtpAction using the already-found handler.
    *
    * @param action the BtpAction to handle.
    * @param hmi the handler iterator to use.
    */
   virtual void handleAction(
      bitmunk::protocol::BtpAction* action, HandlerMap::iterator hmi);
};

// define counted reference rest resource handler
class RestResourceHandlerRef : public bitmunk::protocol::BtpActionHandlerRef
{
public:
   RestResourceHandlerRef(RestResourceHandler* ptr = NULL) :
      bitmunk::protocol::BtpActionHandlerRef(ptr) {};
   virtual ~RestResourceHandlerRef() {};

   /**
    * Returns a reference to the RestResourceHandler.
    *
    * @return a reference to the RestResourceHandler.
    */
   virtual RestResourceHandler& operator*() const
   {
      return (RestResourceHandler&)
         bitmunk::protocol::BtpActionHandlerRef::operator*();
   }

   /**
    * Returns a pointer to the RestResourceHandler.
    *
    * @return a pointer to the RestResourceHandler.
    */
   virtual RestResourceHandler* operator->() const
   {
      return (RestResourceHandler*)
         bitmunk::protocol::BtpActionHandlerRef::operator->();
   }
};

} // end namespace node
} // end namespace bitmunk
#endif
