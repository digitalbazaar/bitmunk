/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/webui/RedirectService.h"

#include "bitmunk/common/Logging.h"
#include "bitmunk/node/BtpActionDelegate.h"
#include "monarch/io/ByteBuffer.h"
#include "monarch/io/ByteArrayInputStream.h"
#include "monarch/util/StringTools.h"
#include "monarch/validation/Validation.h"

using namespace std;
using namespace monarch::io;
using namespace monarch::http;
using namespace monarch::net;
using namespace monarch::rt;
using namespace monarch::util;
using namespace bitmunk::common;
using namespace bitmunk::protocol;
using namespace bitmunk::node;
using namespace bitmunk::webui;
namespace v = monarch::validation;

typedef BtpActionDelegate<RedirectService> Handler;

RedirectService::RedirectService(
   Node* node, const char* path, const char* source, const char* target) :
   NodeService(node, path)
{
   mSource = strdup(source);
   mTarget = strdup(target);
}

RedirectService::~RedirectService()
{
   free(mSource);
   free(mTarget);
}

bool RedirectService::initialize()
{
   // root
   {
      BtpActionHandlerRef redirectResource = new Handler(
         mNode, this, &RedirectService::redirect, BtpAction::AuthOptional);
      addResource(mSource, redirectResource);
   }

   return true;
}

void RedirectService::cleanup()
{
   // remove resources
   removeResource(mSource);
}

void RedirectService::redirect(BtpAction* action)
{
   string content;

   // build redirect url for root path
   if(strcmp(action->getRequest()->getHeader()->getPath(), "/") == 0)
   {
      // use https or http, append host and path
      string location;
      action->getRequest()->getHeader()->getField("Host", location);
      // support ssl or non-ssl
      location.insert(0, (action->getRequest()->getConnection()->isSecure() ?
         "https://" : "http://"));
      location.append(action->getRequest()->getHeader()->getPath());
      location.append(mTarget);

      // respond with 302
      HttpResponseHeader* header = action->getResponse()->getHeader();
      header->setStatus(302, "Found");
      header->setField("Location", location.c_str());

      content.append(
         "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n"
         "<html><head>\n"
         "<title>302 Found</title>\n"
         "</head><body>\n"
         "<h1>Found</h1>\n"
         "<p>The document has moved <a href=\"");
      content.append(location);
      content.append(
         "\">here</a>.</p>\n"
         "</body></html>");
   }
   // serve 404
   else
   {
      HttpResponseHeader* header = action->getResponse()->getHeader();
      header->setStatus(404, "Not Found");
      content.append(
         "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n"
         "<html><head>\n"
         "<title>404 Not Found</title>\n"
         "</head><body>\n"
         "<h1>Not Found</h1>\n"
         "<p>The document was not found.</p>\n"
         "</body></html>");
   }

   // create content input stream
   ByteBuffer b(content.length());
   b.put(content.c_str(), content.length(), false);
   ByteArrayInputStream bais(&b);

   // send response
   action->sendResult(&bais);
}
