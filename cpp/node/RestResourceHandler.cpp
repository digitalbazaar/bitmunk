/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/node/RestResourceHandler.h"

using namespace bitmunk::protocol;
using namespace bitmunk::node;
using namespace monarch::rt;

RestResourceHandler::RestResourceHandler() :
   // auth mode doesn't matter since we are not using it in this dispatcher
   BtpActionHandler(BtpAction::AuthOptional)
{
}

RestResourceHandler::~RestResourceHandler()
{
}

void RestResourceHandler::operator()(BtpAction* action)
{
   BtpMessage::Type type = action->getInMessage()->getType();
   DynamicObject paramDyno;
   action->getResourceParams(paramDyno);
   int paramCount = paramDyno->length();

   bool pass = false;
   HandlerMap::iterator hmi = mHandlers.find(paramCount);
   if(hmi == mHandlers.end())
   {
      // no handler found for specific param count, so check arbitrary params
      hmi = mHandlers.find(-1);
   }

   if(hmi == mHandlers.end())
   {
      // no handler found, send 404
      action->getResponse()->getHeader()->setStatus(404, "Not Found");
      ExceptionRef e = new Exception(
         "Resource not found.", "bitmunk.node.ResourceNotFound", 404);
      e->getDetails()["resource"] = action->getResource();
      Exception::set(e);
   }
   else
   {
      // find the map of valid methods for the given handler
      MethodMap* mm = &(*hmi).second;
      MethodMap::iterator mmi = mm->find(type);
      if(mmi == mm->end() && (*hmi).first != -1)
      {
         hmi = mHandlers.find(-1);
         if(hmi != mHandlers.end())
         {
            mm = &(*hmi).second;
            mmi = mm->find(type);
         }
      }

      // see if there is a resource match for the given request method
      if(mmi != mm->end())
      {
         HandlerInfo& info = (*mmi).second;

         // make sure there are no security violations
         if((info.handler->secureConnectionRequired() ||
             info.handler->securityCheckRequired(action)) &&
            !action->getRequest()->getConnection()->isSecure())
         {
            ExceptionRef e = new Exception(
               "Secure connection (SSL) required by resource.",
               "bitmunk.btp.SecureConnectionRequired");
            e->getDetails()["resource"] = action->getResource();
            Exception::set(e);

            // allow action to "pass", connection will be severed by BtpService
            // without any response data and without receiving content
            pass = true;
         }
         else if(info.handler->canPerformAction(action))
         {
            pass = true;

            // clear last exception, then do validation
            Exception::clear();

            if(!info.resourceValidator.isNull())
            {
               DynamicObject params;
               action->getResourceParams(params);
               pass = info.resourceValidator->isValid(params);
            }

            if(pass && !info.queryValidator.isNull())
            {
               DynamicObject query;
               action->getResourceQuery(query, info.flags & ArrayQuery);
               pass = info.queryValidator->isValid(query);
            }

            if(pass && !info.contentValidator.isNull())
            {
               DynamicObject content;
               action->receiveContent(content);
               pass = info.contentValidator->isValid(content);
            }

            if(pass)
            {
               info.handler->performAction(action);
            }
         }
      }
      // there is no resource match for the given request method
      else
      {
         // 405
         action->getResponse()->getHeader()->setStatus(
            405, "Method Not Allowed");
         ExceptionRef e = new Exception(
            "Method not allowed.", "bitmunk.node.MethodNotAllowed", 405);
         e->getDetails()["invalidMethod"] =
            action->getRequest()->getHeader()->getMethod();

         // add valid types, build allow header
         const char* typeStr;
         std::string allow;
         e->getDetails()["validTypes"]->setType(Array);
         for(mmi = mm->begin(); mmi != mm->end(); mmi++)
         {
            typeStr = BtpMessage::typeToString(mmi->first);
            e->getDetails()["validTypes"]->append() = typeStr;
            if(allow.length() > 0)
            {
               allow.append(", ");
            }
            allow.append(typeStr);
         }

         action->getResponse()->getHeader()->setField("Allow", allow.c_str());
         Exception::set(e);
      }
   }

   if(!pass && !action->isResultSent())
   {
      // get last exception (create one if necessary -- but this will only
      // happen if a developer has failed to set an exception in a service)
      ExceptionRef e = Exception::get();
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

void RestResourceHandler::addHandler(
   BtpActionHandlerRef& handler,
   BtpMessage::Type type,
   int paramCount,
   monarch::validation::ValidatorRef* queryValidator,
   monarch::validation::ValidatorRef* contentValidator,
   uint32_t flags)
{
   HandlerInfo& info = mHandlers[paramCount][type];
   if(queryValidator != NULL)
   {
      info.queryValidator = *queryValidator;
   }
   if(contentValidator != NULL)
   {
      info.contentValidator = *contentValidator;
   }
   info.handler = handler;
   info.flags = flags;
}

void RestResourceHandler::addHandler(
   BtpActionHandlerRef& handler,
   BtpMessage::Type type,
   monarch::validation::ValidatorRef* resourceValidator,
   monarch::validation::ValidatorRef* queryValidator,
   monarch::validation::ValidatorRef* contentValidator,
   uint32_t flags)
{
   int paramCount = (resourceValidator != NULL)
      ? (*resourceValidator)->length()
      : 0;
   HandlerInfo& info = mHandlers[paramCount][type];
   if(resourceValidator != NULL)
   {
      info.resourceValidator = *resourceValidator;
   }
   if(queryValidator != NULL)
   {
      info.queryValidator = *queryValidator;
   }
   if(contentValidator != NULL)
   {
      info.contentValidator = *contentValidator;
   }
   info.handler = handler;
   info.flags = flags;
}
