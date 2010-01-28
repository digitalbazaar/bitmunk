/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_protocol_BtpClient_H
#define bitmunk_protocol_BtpClient_H

#include "bitmunk/protocol/BtpMessage.h"
#include "monarch/http/HttpClient.h"
#include "monarch/net/SslSessionCache.h"
#include "monarch/net/Url.h"

namespace bitmunk
{
namespace protocol
{

/**
 * A BtpClient is used to send BtpMessages to and receive BtpMessages from a
 * service that speaks BTP.
 *
 * @author Dave Longley
 */
class BtpClient
{
protected:
   /**
    * The BandwidthThrottler used for reading with this client.
    */
   monarch::net::BandwidthThrottler* mReadThrottler;

   /**
    * The BandwidthThrottler used for writing with this client.
    */
   monarch::net::BandwidthThrottler* mWriteThrottler;

   /**
    * An SSL context for handling SSL connections.
    */
   monarch::net::SslContextRef mSslContext;

   /**
    * The SSL session cache for re-using SSL sessions.
    */
   monarch::net::SslSessionCacheRef mSslSessionCache;

public:
   /**
    * Creates a new BtpClient.
    */
   BtpClient();

   /**
    * Destructs this BtpClient.
    */
   virtual ~BtpClient();

   /**
    * Creates a connection to a url. The returned connection must be deleted
    * by the caller. Urls with a scheme of "https" will be connected over SSL.
    *
    * @param userId the user ID of the peer to talk to, 0 to do non-peer.
    * @param url the url to connect to.
    * @param timeout the timeout in seconds (0 for no timeout).
    */
   virtual monarch::http::HttpConnection* createConnection(
      bitmunk::common::UserId userId,
      monarch::net::Url* url, uint32_t timeout = 30);

   /**
    * Sends the passed BtpMessage using the given HttpRequest. The response
    * header will be received with the passed HttpResponse. If the response
    * header indicates that an exception occurred, the exception will also
    * be received, otherwise the response body will not be received by this
    * call.
    *
    * @param url the url to send the request to.
    * @param out the BtpMessage to send out.
    * @param request the HttpRequest to send the message with.
    * @param in the BtpMessage to receive (NULL to receive nothing).
    * @param response the HttpResponse to receive a response with
    *                 (NULL to receive nothing).
    *
    * @return true if successful, false if not.
    */
   virtual bool sendMessage(
      monarch::net::Url* url,
      BtpMessage* out, monarch::http::HttpRequest* request,
      BtpMessage* in, monarch::http::HttpResponse* response);

   /**
    * Sends the passed BtpMessage to the given Url. If there is a BTP response,
    * the passed BtpMessage "in" will be populated and its content
    * written to its content sink.
    *
    * The outgoing message, if secure, should have its user set via setUser().
    * The outgoing message should have its content source set via
    * setContent() or setContentSource().
    *
    * If the incoming message is not NULL, it should have its content sink set
    * via setContent() or setContentSink().
    *
    * @param userId the ID of the bitmunk user to talk to, 0 for none.
    * @param url the url to send the message to.
    * @param out the outgoing request message.
    * @param in the incoming response message.
    * @param timeout the timeout in seconds.
    *
    * @return true if successful, false if an Exception occurred or was
    *         received.
    */
   virtual bool exchange(
      bitmunk::common::UserId userId,
      monarch::net::Url* url, BtpMessage* out, BtpMessage* in,
      uint32_t timeout = 30);

   /**
    * Gets this BtpClient's SSL context.
    *
    * @return this BtpClient's SSL context.
    */
   virtual monarch::net::SslContextRef& getSslContext();

   /**
    * Gets this BtpClient's SSL session cache.
    *
    * @return this BtpClient's SSL session cache.
    */
   virtual monarch::net::SslSessionCacheRef& getSslSessionCache();

   /**
    * Sets a BandwidthThrottler for all new connections made by
    * this BtpClient.
    *
    * @param bt the BandwidthThrottler to use with all new connections made
    *           by this client.
    * @param read true to use the BandwidthThrottler for reading, false
    *             to use it for writing.
    */
   virtual void setBandwidthThrottler(
      monarch::net::BandwidthThrottler* bt, bool read);

   /**
    * Gets the BandwidthThrottler for this BtpClient.
    *
    * @param read true to get the BandwidthThrottler used for reading, false
    *             to get the BandwidthThrottler used for writing.
    *
    * @return the BandwidthThrottler for this BtpClient.
    */
   virtual monarch::net::BandwidthThrottler* getBandwidthThrottler(bool read);

   /**
    * Checks the passed response code, setting an exception if appropriate.
    *
    * @param response the response with the code to check.
    *
    * @return true if no exception, false if one was set.
    */
   static bool checkResponseCode(monarch::http::HttpResponse* response);

   /**
    * Checks the btp security on a response. If btp user authentication fails,
    * it will be set in the "in" message as a breach. If the host security
    * fails, false will be returned.
    *
    * @param url the url.
    * @param request the HttpRequest.
    * @param response the HttpResponse.
    * @param in the "in" BtpMessage.
    *
    * @return true if host security passed, false if not.
    */
   static bool checkResponseSecurity(
      monarch::net::Url* url,
      monarch::http::HttpRequest* request,
      monarch::http::HttpResponse* response,
      BtpMessage* in);
};

} // end namespace protocol
} // end namespace bitmunk
#endif
