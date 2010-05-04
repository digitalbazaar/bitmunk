/*
 * Copyright (c) 2007-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_protocol_BtpAction_H
#define bitmunk_protocol_BtpAction_H

#include "bitmunk/protocol/BtpMessage.h"
#include "monarch/io/InputStream.h"

namespace bitmunk
{
namespace protocol
{

/**
 * A BtpAction constitutes some kind of action that a BTP client executes
 * on a BTP service's resource. The BTP service may provide a result for
 * the action using the BTP protocol.
 *
 * This object is used by BtpServices (not clients).
 *
 * @author Dave Longley
 */
class BtpAction
{
public:
   /**
    * An enumeration defining the how btp authentication security should be
    * used for a particular action.
    *
    * @author Dave Longley
    */
   enum BtpAuth
   {
      // incoming security check if present, outgoing if present
      AuthOptional,

      // same as AuthOptional except SSL must be on regardless of authentication
      AuthOptionalSslRequired,

      // automatic incoming security check, outgoing always on
      AuthRequired
   };

protected:
   /**
    * The resource for this action.
    */
   char* mResource;

   /**
    * Stores the base resource path (excluding resource parameters).
    */
   char* mBaseResourcePath;

   /**
    * The "in" BtpMessage from the client.
    */
   BtpMessage mInMessage;

   /**
    * The "out" BtpMessage from the service.
    */
   BtpMessage mOutMessage;

   /**
    * The HttpRequest to use to receive the content for this action.
    */
   monarch::http::HttpRequest* mRequest;

   /**
    * The HttpResponse to use to send a result or exception for this action.
    */
   monarch::http::HttpResponse* mResponse;

   /**
    * Stores any parsed resource parameters.
    */
   monarch::rt::DynamicObject mResourceParams;

   /**
    * Cache of parsed query variables in non-array mode.
    */
   monarch::rt::DynamicObject mQueryVars;

   /**
    * Cache of parsed query variables in array mode.
    */
   monarch::rt::DynamicObject mArrayQueryVars;

   /**
    * Stores any received content from
    * receiveContent(monarch::rt::DynamicObject& dyno).
    */
   monarch::rt::DynamicObject mContent;

   /**
    * Flag if content has already been received.
    */
   bool mContentReceived;

   /**
    * Flag if result has already been sent.
    */
   bool mResultSent;

   /**
    * Flag for automatic selection of content-encoding for sending content.
    */
   bool mSelectContentEncoding;

public:
   /**
    * Creates a new BtpAction from the passed HttpRequest.
    *
    * @param resource the resource this action is acting on.
    */
   BtpAction(const char* resource);

   /**
    * Destructs this BtpAction.
    */
   virtual ~BtpAction();

   /**
    * Adds a Content-Encoding header if Accept-Encoding includes a supported
    * compression method.
    */
   virtual void setContentEncoding();

   /**
    * Sets whether content-encoding will be automatically selected based on
    * the received Accept-Encoding header when sending content, if the
    * content-encoding header hasn't been set yet.
    *
    * @param on true to enable auto-selection of content-encoding, false to
    *           disable it.
    */
   virtual void setSelectContentEncoding(bool on);

   /**
    * Checks the btp security on the message for this action, setting an
    * exception if the message is unchecked or breached. The exception message
    * will include the resource name. This method will not check for a
    * secure connection (SSL), as this should be done at a lower level.
    *
    * @return true if the security check passed, false if an Exception occurred.
    */
   virtual bool checkSecurity();

   /**
    * Receives the content for this action and writes it to the passed
    * content sink output stream.
    *
    * @param os the content sink to write to.
    * @param true to close the sink when finished writing, false not to.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool receiveContent(monarch::io::OutputStream* os, bool close);

   /**
    * Receives the content for this action and writes it to the passed
    * DynamicObject.
    *
    * @param dyno the DynamicObject to write to.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool receiveContent(monarch::rt::DynamicObject& dyno);

   /**
    * Sends only the response header with no content.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool sendResult();

   /**
    * Sends the content in the passed input stream as the result of this
    * action.
    *
    * @param is the content source input stream.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool sendResult(monarch::io::InputStream* is);

   /**
    * Sends the passed DynamicObject as the result of this action. If the http
    * response code is set to zero, this method will automatically set it.
    *
    * @param dyno the DynamicObject to send.
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool sendResult(monarch::rt::DynamicObject& dyno);

   /**
    * Sends an exception as the result of this action. If the http response
    * code is set to zero, this method will automatically set it.
    *
    * @param e the reference to the Exception to send.
    * @param client true if the exception was due to the client, false if it
    *               was due to the server (this is used to set the response
    *               code if it is set to 0).
    *
    * @return true if successful, false if an exception occurred.
    */
   virtual bool sendException(monarch::rt::ExceptionRef& e, bool client);

   /**
    * Gets the resource of this action.
    *
    * @return the resource of this action.
    */
   virtual const char* getResource();

   /**
    * Gets the resource parameters that occur after a base path for the
    * resource.
    *
    * @param params the DynamicObject to store the parameters in.
    *
    * @return true if there are parameters, false if there aren't any.
    */
   virtual bool getResourceParams(monarch::rt::DynamicObject& params);

   /**
    * Gets the resource's query variables.
    *
    * @param vars the DynamicObject to store the variables in.
    * @param asArrays true to create an array to hold all values for each key,
    *           false to use only the last value for each key.
    *
    * @return true if there are variables, false if there aren't any.
    */
   virtual bool getResourceQuery(
      monarch::rt::DynamicObject& vars, bool asArrays = false);

   /**
    * Gets the "in" BtpMessage associated with this action. This is the
    * message that was sent by the client.
    *
    * @return the received BtpMessage associated with this action.
    */
   virtual BtpMessage* getInMessage();

   /**
    * Gets the "out" BtpMessage associated with this action. This is the
    * message that is sent by the service.
    *
    * @return the BtpMessage used by the service to send a response.
    */
   virtual BtpMessage* getOutMessage();

   /**
    * Sets the HttpRequest to use to receive the content for this action.
    *
    * @param request the HttpRequest to create the action from.
    */
   virtual void setRequest(monarch::http::HttpRequest* request);

   /**
    * Gets the HttpRequest associated with this action.
    *
    * @return the HttpRequest associated with this action.
    */
   virtual monarch::http::HttpRequest* getRequest();

   /**
    * Sets the HttpResponse to use to send the result of this action.
    *
    * @param response the HttpResponse to send the action result with.
    */
   virtual void setResponse(monarch::http::HttpResponse* response);

   /**
    * Gets the HttpResponse associated with this action.
    *
    * @return the HttpResponse associated with this action.
    */
   virtual monarch::http::HttpResponse* getResponse();

   /**
    * Sets the base resource path (excluding parameters).
    *
    * @param res the base resource path (excluding parameters).
    */
   virtual void setBaseResourcePath(const char* res);

   /**
    * Checks to see if the result for this action has already been sent.  If
    * it has been sent then further calls to sendResult will do nothing and
    * succeed.
    *
    * @return true if the result has been sent, false if not.
    */
   virtual bool isResultSent();

   /**
    * Set if the result has been sent.
    *
    * @param resultSent true if the result has been sent, false if not.
    */
   virtual void setResultSent(bool resultSent = true);

   /**
    * Gets the client's internet address.
    *
    * @param address the client's internet address to populate.
    *
    * @return true if successful, false if an error occurred.
    */
   virtual bool getClientInternetAddress(
      monarch::net::InternetAddress* address);
};

} // end namespace protocol
} // end namespace bitmunk
#endif
