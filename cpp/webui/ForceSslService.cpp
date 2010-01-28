/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/webui/ForceSslService.h"

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

typedef BtpActionDelegate<ForceSslService> Handler;

ForceSslService::ForceSslService(Node* node, const char* path) :
   NodeService(node, path)
{
}

ForceSslService::~ForceSslService()
{
}

bool ForceSslService::initialize()
{
   // root
   {
      BtpActionHandlerRef redirectResource = new Handler(
         mNode, this, &ForceSslService::redirect, BtpAction::AuthOptional);
      addResource("/", redirectResource);
   }
   
   return true;
}

void ForceSslService::cleanup()
{
   // remove resources
   removeResource("/");
}

void ForceSslService::redirect(BtpAction* action)
{
   // build https url:
   
   // use https, append host and path
   string location;
   action->getRequest()->getHeader()->getField("Host", location);
   location.insert(0, "https://");
   location.append(action->getRequest()->getHeader()->getPath());
   
   // respond with 302
   HttpResponseHeader* header = action->getResponse()->getHeader();
   header->setStatus(302, "Found");
   header->setField("Location", location.c_str());
   
   string content;
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
   
   // create content input stream
   ByteBuffer b(content.length());
   b.put(content.c_str(), content.length(), false);
   ByteArrayInputStream bais(&b);
   
   // send response
   action->sendResult(&bais);
}
