/*
 * Copyright (c) 2007-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_protocol_BtpMessage_H
#define bitmunk_protocol_BtpMessage_H

#include "monarch/io/InputStream.h"
#include "monarch/io/OutputStream.h"
#include "monarch/net/Url.h"
#include "bitmunk/common/Profile.h"
#include "bitmunk/common/PublicKeySource.h"
#include "monarch/crypto/DigitalSignature.h"
#include "monarch/http/HttpConnection.h"
#include "monarch/http/HttpRequest.h"
#include "monarch/http/HttpResponse.h"
#include "monarch/rt/DynamicObject.h"

namespace bitmunk
{
namespace protocol
{

/**
 * A BtpMessage is a message that uses the BTP (Bitmunk Transfer Protocol).
 * 
 * The current version of the BTP works on top of HTTP/1.1.
 * 
 * All clients and servers that support BTP must be able to handle a MIME
 * content type of "application/json" or "text/xml". Standard BTP messages
 * consist of a JSON or XML document as the content. A standard BTP response
 * message will return JSON or XML content, where the type of the object or
 * document, respectively, is specified according to the documentation for
 * the particular service, or, if an error occurs, the type of object/document
 * will be an exception.
 * 
 * A BtpMessage can be sent with or without authentication/integrity security.
 * A secure message includes authentication and non-repudiation measures via
 * digital signatures.
 * 
 * An insecure message is a simple HTTP/1.1 message with no special header
 * fields.
 * 
 * A secure message includes special HTTP header fields. Every secure message
 * must have: 
 * 
 * Btp-Version
 * Btp-User-Id
 * Btp-Agent-User-Id
 * Btp-Agent-Profile-Id
 * Btp-Header-Signature
 * 
 * If the secure message includes content, then it must also have:
 * 
 * Btp-Content-Signature
 * 
 * Descriptions of headers:
 * 
 * The Btp-Version is a string identifying the version of BTP used. "1.0"
 * is the only currently supported version.
 * 
 * The Btp-User-Id is a string identifying the user that the message is from.
 * This is not necessarily the ID of the user that signed the message, the
 * Btp-Agent-User-Id refers to the user that provided the signature. This
 * field *must* be provided in the http header, not the trailer.
 * 
 * The Btp-Agent-User-Id is a string identifying the user that signed the
 * message (known as the "agent"). This may be different from the Btp-User-Id
 * if another user is signing on behalf of the user the message is from. This
 * field *must* be provided in the http header, not the trailer.
 *  
 * The Btp-Agent-Profile-Id is a string identifying the profile containing
 * the private key used to produce the signature on the message. This profile
 * belongs to the user identified by Btp-Agent-User-Id. This allows for
 * multiple private keys to be assigned to a user, provided that there is
 * only 1 per profile. This field *must* be provided in the http header, not
 * the trailer.
 * 
 * The Btp-Header-Signature is a digital signature hex-encoded string. The
 * signature is produced from a hash of the request/response line concatenated
 * with the value of the host field. This *EXCLUDES* CRLFs. It *must* be
 * provided in the header, not the trailer. The digital signature algorithm
 * will either be RSA/SHA1 or DSA/SHA1 depending on the agent's crypto key.
 * 
 * The Btp-Content-Signature is a digital signature hex-encoded string. The
 * signature is produced from a hash of the content. The digital signature
 * algorithm will either be RSA/SHA1 or DSA/SHA1 depending on the agent's
 * crypto key.
 * 
 * The Btp-User-Id, Btp-Agent-User-Id, Btp-Agent-Profile-Id, and
 * Btp-Header-Signature fields *must* be included before the message body
 * if the message is secure.
 * 
 * The Btp-Content-Signature header field may be included in a set of trailer
 * headers if "chunked" Transfer-Encoding is being used.
 * 
 * Note: The reason for including the host field in the header signature:
 * 
 * There are two spoofing attacks someone might attempt:
 * 
 * 1. A server seeks to spoof other clients, so it collects the requests
 * and signatures of multiple valid clients that connect to it for later use.
 * 
 * 2. A client seeks to spoof other servers, so it collects the responses
 * and signatures of multiple valid servers it connects to for later use.
 * 
 * In the first case:
 * 
 * If all the valid clients signed was the request line, then any server that
 * collected requests and signatures could then act as any client that
 * connected to it by simply re-using their request and signature.
 * 
 * However, if the client must also sign the host field, then the server
 * could only ever make that client's request with the given host field. And
 * that host field would point *to itself*. If servers check the host header
 * for validity (to determine whether or not the specified host is actually
 * "them"), then any invalid host field could be rejected by the server as a
 * possible attack.
 * 
 * So the objective in case #1 is to encode what endpoint the valid client meant
 * to reach in the data that is signed. That way, only if the client's packet
 * is snooped (which SSL should protect against), could that client's signature
 * be reused *on the same endpoint*. Note the importance that the signature
 * would only be valid on the same endpoint as this also renders invalid an
 * attack where evil servers collect client signatures and give them to other
 * attacking clients who seek to contact non-evil servers.
 * 
 * In the second case:
 * 
 * Here a client collects multiple server responses so that when it
 * runs a server it can pretend to be other servers. To prevent this the
 * protocols requires that the client also check the response's host field to
 * ensure it did not change from the value that the client sent. This prevents
 * a server from using responses from other servers that run on other hosts.
 * 
 * @author Dave Longley
 */
class BtpMessage
{
public:
   /**
    * The types of BtpMessages.
    */
   enum Type
   {
      Undefined, Get, Put, Post, Delete, Head, Options, Trace, Connect
   };
   
   /**
    * The different types of security statuses for a BtpMessage.
    */
   enum SecurityStatus
   {
      Unchecked, Breach, Secure
   };
   
   /**
    * The different acceptable content-types for DynamicObjects.
    */
   enum DynoContentType
   {
      Invalid, Json, Xml, Form
   };
   
protected:
   /**
    * The type of message. This refers to the HTTP method used.
    */
   Type mType;
   
   /**
    * The SecurityStatus for this message.
    */
   SecurityStatus mSecurityStatus;
   
   /**
    * The UserId of the user that this message is from.
    */
   bitmunk::common::UserId mUserId;
   
   /**
    * The UserId of the agent that signed this message.
    */
   bitmunk::common::UserId mAgentUserId;
   
   /**
    * The ProfileId of the agent that signed this message.
    */
   bitmunk::common::ProfileId mAgentProfileId;
   
   /**
    * The Profile to sign outgoing secure messages with.
    */
   bitmunk::common::ProfileRef mAgentProfile;
   
   /**
    * The public key source to check incoming secure messages with.
    */
   bitmunk::common::PublicKeySource* mPublicKeySource;
   
   /**
    * The content type to use with this message.
    */
   char* mContentType;
   
   /**
    * An associated content source.
    */
   monarch::io::InputStream* mContentSource;
   
   /**
    * An associated content sink.
    */
   monarch::io::OutputStream* mContentSink;
   
   /**
    * Set to true if the associated content sink should be closed
    * once it has been written to, false if not.
    */
   bool mCloseSink;
   
   /**
    * An associated DynamicObject to be sent/received.
    */
   monarch::rt::DynamicObject mDynamicObject;
   
   /**
    * Any extra http headers to include in an outgoing message.
    */
   monarch::rt::DynamicObject mCustomHeaders;
   
   /**
    * Set to true if headers should be saved.
    */
   bool mSaveHeaders;
   
   /**
    * A reference to the HttpRequestHeader that was used when
    * sending/receiving a message.
    */
   monarch::http::HttpRequestHeader* mRequestHeader;
   
   /**
    * A reference to the HttpResponseHeader that was used when
    * sending/receiving a message.
    */
   monarch::http::HttpResponseHeader* mResponseHeader;
   
   /**
    * An HttpTrailer used in communication.
    */
   monarch::http::HttpTrailerRef mTrailer;
   
public:
   /**
    * Creates a new BtpMessage.
    */
   BtpMessage();
   
   /**
    * Destructs this BtpMessage.
    */
   virtual ~BtpMessage();
   
   /**
    * Checks to see if btp security is present on the passed header. This
    * method does not check to see if the authentication information is
    * correct, it just sees if it *exists*. This is useful for determining
    * whether a service that has optional security should respond with
    * security information or not.
    * 
    * @param header the header to check.
    * 
    * @return true if btp security is present, false if not.
    */
   virtual bool isHeaderSecurityPresent(monarch::http::HttpHeader* header);
   
   /**
    * Checks btp security on the passed header, setting this message's
    * security status as a result.
    * 
    * @param header the header to check.
    */
   virtual void checkHeaderSecurity(monarch::http::HttpHeader* header);
   
   /**
    * Checks btp security on some received content, using the passed
    * HttpHeader, HttpTrailer, and DigitalSignature. This message's security
    * status willbe set as a result.
    * 
    * @param header the HttpHeader for the content.
    * @param trailer the HttpTrailer for the content.
    * @param ds the DigitalSignature for the content.
    */
   virtual void checkContentSecurity(
      monarch::http::HttpHeader* header, monarch::http::HttpTrailer* trailer,
      monarch::crypto::DigitalSignature* ds);
   
   /**
    * A helper function that automatically sets the method, path, version,
    * user-agent, and host for an http request.
    *
    * @param url the Url the request will be made to. 
    * @param header the HttpRequestHeader to update.
    */    
   virtual void setupRequestHeader(
      monarch::net::Url* url, monarch::http::HttpRequestHeader* header);
   
   /**
    * Sends the header for this message over the passed connection, using
    * the passed header. This method can be used to send a manually set
    * http header which is to be followed by a manually sent content-body.
    * 
    * The output stream for writing the body will be set to the passed
    * OutputStreamRef parameter and it must be closed once the body has been
    * written to it.
    * 
    * Any http trailer that will be sent when the stream is closed will be
    * set to the passed HttpTrailerRef parameter. Additional fields can be set
    * on the trailer to be sent after the body output stream is closed.
    * 
    * For secure messages, digital signatures on the header and the content
    * will be handled automatically.
    * 
    * @param hc the connection to send the message over.
    * @param header the header to use with the message.
    * @param os the body OutputStreamRef to be set.
    * @param trailer the HttpTrailerRef to be set.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool sendHeader(
      monarch::http::HttpConnection* hc, monarch::http::HttpHeader* header,
      monarch::io::OutputStreamRef& os,
      monarch::http::HttpTrailerRef& trailer);
   
   /**
    * Sends this message in its entirety (header and content) to the passed
    * url using the passed HttpRequest and pre-set content source input stream
    * or dynamic object.
    * 
    * The request header's method, host, accept, and other basic fields
    * will be automatically set.
    * 
    * @param url the url to send the message to.
    * @param request the request to send the message with.
    * @param fillRequest true to auto-fill the request, false to leave it alone.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool send(
      monarch::net::Url* url, monarch::http::HttpRequest* request);
   
   /**
    * Sends this message in its entirety (header and content) using
    * the passed HttpResponse and pre-set content sink output stream or
    * dynamic object.
    * 
    * @param response the response to send the message with.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool send(monarch::http::HttpResponse* response);
   
   /**
    * Receives the content of this message using the passed request and
    * writes it to the pre-set content sink input stream or dynamic object.
    * It is assumed that the header for the passed request has already been
    * received.
    * 
    * Content will only be received if the message type is PUT or POST.
    * 
    * No security check will be run on the header, it is assumed this has
    * already been done via checkHeaderSecurity().
    * 
    * A btp security check will be run on any received content.
    * 
    * @param request the request to receive the message content with.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool receiveContent(monarch::http::HttpRequest* request);
   
   /**
    * Receives the content of this message using the passed response and
    * writes it to the pre-set content sink output stream or dynamic object.
    * It is assumed that the header for the passed response has already been
    * received.
    * 
    * No security check will be run on the header, it is assumed this has
    * already been done via checkHeaderSecurity().
    * 
    * A btp security check will be run on any received content.
    * 
    * @param response the response to receive the message content with.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool receiveContent(monarch::http::HttpResponse* response);
   
   /**
    * Gets a stream to manually receive the content of this message, after the
    * header has already been received.
    * 
    * No security check will be run on the header or the body. It is assumed
    * that checkHeaderSecurity() was already called, and if a security check
    * on the body is desired, checkContentSecurity() must be called manually
    * using the DigitalSignature returned from this method, after the entire
    * content stream has been read.
    * 
    * It is safe to close the returned input stream (this will not shutdown
    * the underlying connection input) or to leave it open.
    * 
    * @param request the http request.
    * @param is the InputStream to be set to the content stream to read.
    * @param trailer the HttpTrailer to be set to the trailers following
    *                the body.
    * @param ds the DigitalSignature to be set if the content is secure.
    */
   virtual void getContentReceiveStream(
      monarch::http::HttpRequest* request,
      monarch::io::InputStreamRef& is, monarch::http::HttpTrailerRef& trailer,
      monarch::crypto::DigitalSignatureRef& ds);
   
   /**
    * Gets a stream to manually receive the content of this message, after the
    * header has already been received.
    * 
    * No security check will be run on the header or the body. It is assumed
    * that checkHeaderSecurity() was already called, and if a security check
    * on the body is desired, checkContentSecurity() must be called manually
    * using the DigitalSignature returned from this method, after the entire
    * content stream has been read.
    * 
    * It is safe to close the returned input stream (this will not shutdown
    * the underlying connection input) or to leave it open.
    * 
    * @param response the http response.
    * @param is the InputStream to be set to the content stream to read.
    * @param trailer the HttpTrailer to be set to the trailers following
    *                the body.
    * @param ds the DigitalSignature to be set if the content is secure.
    */
   virtual void getContentReceiveStream(
      monarch::http::HttpResponse* response,
      monarch::io::InputStreamRef& is, monarch::http::HttpTrailerRef& trailer,
      monarch::crypto::DigitalSignatureRef& ds);
   
   /**
    * Sets the type of this message.
    * 
    * @param type the type of this message.
    */
   virtual void setType(Type type);
   
   /**
    * Gets the type of this message.
    * 
    * @return the type of this message.
    */
   virtual Type getType();
   
   /**
    * Sets the SecurityStatus for this message.
    * 
    * @param status the SecurityStatus for this message.
    */
   virtual void setSecurityStatus(SecurityStatus status);
   
   /**
    * Gets the SecurityStatus for this message.
    * 
    * @return the SecurityStatus for this message.
    */
   virtual SecurityStatus getSecurityStatus() const;
   
   /**
    * Sets the UserId of the user that this message is from. This value
    * must be set before sending a secure message.
    * 
    * When a message is received, this value will be overwritten.
    * 
    * Note: This is not necessarily the user that signed the message, the
    * agent user ID refers to that user. The agent user ID is set when the
    * message is sent (according to the agent profile) or when a message
    * is received.
    * 
    * @param id the UserId of the user that this message is from.
    */
   virtual void setUserId(bitmunk::common::UserId id);
   
   /**
    * Gets the UserId of the user that this message is from. When a message
    * is received, this value will be overwritten. It must be set properly
    * before sending a message.
    * 
    * Note: This is not necessarily the user that signed the message, the
    * agent user ID refers to that user.
    * 
    * @return the UserId of the user that this message is from, 0 if the
    *         message is unsigned.
    */
   virtual bitmunk::common::UserId getUserId() const;
   
   /**
    * Gets the UserId of the agent that signed this message. When a message
    * is sent or received, this value will be updated.
    * 
    * @return the UserId of the agent that signed this message, 0 if the
    *         message is unsigned.
    */
   virtual bitmunk::common::UserId getAgentUserId() const;
   
   /**
    * Gets the ProfileId of the agent that signed this message. When a
    * message is sent or received, this value will be updated.
    * 
    * @return the ProfileId of the agent that signed this message, 0 if it
    *         is unsigned.
    */
   virtual bitmunk::common::ProfileId getAgentProfileId() const;
   
   /**
    * Sets the Profile of the agent that will sign this message when it is
    * being *sent*. When a secure message is sent, the agent user ID and agent
    * profile IDs will be updated using the passed profile.
    * 
    * @param p the Profile of the agent that will sign this message when it
    *          is being *sent*.
    */
   virtual void setAgentProfile(bitmunk::common::ProfileRef& p);
   
   /**
    * Gets the Profile of the agent that will sign this message when it is
    * being *sent*.
    * 
    * @return the Profile of the agent that will sign this message when it is
    *         being *sent*.
    */
   virtual bitmunk::common::ProfileRef& getAgentProfile();
   
   /**
    * Sets the PublicKeySource for this message. This is where this message
    * can retrieve public keys for verifying the integrity of securely
    * transferred data.
    * 
    * @param pks the PublicKeySource for this message.
    */
   virtual void setPublicKeySource(bitmunk::common::PublicKeySource* source);
   
   /**
    * Gets the PublicKeySource for this message. This is where this message
    * can retrieve public keys for verifying the integrity of securely
    * transferred data.
    * 
    * @return the PublicKeySource for this message.
    */
   virtual bitmunk::common::PublicKeySource* getPublicKeySource();
   
   /**
    * Sets the content-type to use with this message.
    * 
    * @param contentType the content-type to use with this message.
    */
   virtual void setContentType(const char* contentType);
   
   /**
    * Gets the content-type to use with this message.
    * 
    * @return the content-type to use with this message.
    */
   virtual const char* getContentType();
   
   /**
    * Gets the custom headers object. Any headers in this object will be added
    * to an outgoing message. The object is a map, where values can either
    * be arrays of non-arrays and non-maps, or where values are non-maps.
    * 
    * @return the custom headers object for modification.
    */
   virtual monarch::rt::DynamicObject& getCustomHeaders();
   
   /**
    * Sets the content source to use with this message.
    * 
    * @param is the InputStream with the content to send with this message.
    */
   virtual void setContentSource(monarch::io::InputStream* is);
   
   /**
    * Gets the content source to use with this message.
    * 
    * @return the InputStream with the content to send with this message.
    */
   virtual monarch::io::InputStream* getContentSource();
   
   /**
    * Sets the content sink to use with this message.
    * 
    * @param os the OutputStream to write the received message content to.
    * @param close close the OutputStream when done.
    */
   virtual void setContentSink(monarch::io::OutputStream* os, bool close);
   
   /**
    * Gets the content sink to use with this message.
    * 
    * @return the OutputStream to write the received message content to.
    */
   virtual monarch::io::OutputStream* getContentSink();
   
   /**
    * Sets the DynamicObject to use with this message.
    * 
    * @param dyno the DynamicObject to use with this message.
    * @param contentType the content-type to use
    *                    (defaults to "application/json").
    */
   virtual void setDynamicObject(
      monarch::rt::DynamicObject& dyno, const char* contentType = NULL);
   
   /**
    * Gets the DynamicObject to use with this message.
    * 
    * @return the DynamicObject to use with this message.
    */
   virtual monarch::rt::DynamicObject& getDynamicObject();
   
   /**
    * Gets the HttpTrailer sent/received during communication.
    * 
    * @return the HttpTrailer used in communication.
    */
   virtual monarch::http::HttpTrailerRef& getTrailer();
   
   /**
    * Sets whether or not headers should be saved for this message.
    * 
    * @param save true to save headers, false not to.
    */
   virtual void setSaveHeaders(bool save);
   
   /**
    * Gets the request header that was used with this message, if any.
    * 
    * @return the request header that was used with this message.
    */
   virtual monarch::http::HttpRequestHeader* getRequestHeader();
   
   /**
    * Gets the response header that was used with this message, if any.
    * 
    * @return the response header that was used with this message.
    */
   virtual monarch::http::HttpResponseHeader* getResponseHeader();
   
   /**
    * Conversion from string to Type
    * 
    * @param type the string to convert.
    * 
    * @return the Type or Undefined.
    */
   static Type stringToType(const char* type);
   
   /**
    * Conversion from Type to string
    * 
    * @param type the Type to convert.
    * 
    * @return the string or NULL.
    */
   static const char* typeToString(Type type);
   
protected:
   /**
    * Checks the content-type specified in the given header to ensure
    * that it is a supported type for the transfer of a DynamicObject.
    * 
    * @param header the HttpHeader to check.
    * @param type the DynoContentType to update.
    * @param send true if sending content, false if receiving it.
    * 
    * @return true if the content-type was appropriate, false if an Exception
    *         occurred.
    */
   virtual bool checkContentType(
      monarch::http::HttpHeader* header, DynoContentType& type, bool send);
   
   /**
    * Adds the appropriate BTP header fields to the passed header. Any
    * security headers will be added according to the security settings
    * of this message.
    * 
    * @param header the header to update.
    * @param ds the DigitalSignature to be used on the message.
    */
   virtual void addHeaders(
      monarch::http::HttpHeader* header, monarch::crypto::DigitalSignature* ds);
   
   /**
    * Verifies the passed DigitalSignature against the passed hex-encoded
    * signature.
    * 
    * @param ds the DigitalSignature.
    * @param signature the hex-encoded signature.
    * 
    * @return true if verified, false if not.
    */ 
   virtual bool verifySignature(
      monarch::crypto::DigitalSignature* ds, std::string& signature);
   
   /**
    * Sends this message in its entirety (header and content) over the
    * passed connection, using the passed header and content source
    * input stream.
    * 
    * @param hc the connection to send the message over.
    * @param header the header to use with the message.
    * @param is the InputStream with content to read and send.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool sendHeaderAndStream(
      monarch::http::HttpConnection* hc,
      monarch::http::HttpHeader* header,
      monarch::io::InputStream* is);
   
   /**
    * Sends this message in its entirety (header and content) over the
    * passed connection, using the passed header and DynamicObject content.
    * 
    * @param hc the connection to send the message over.
    * @param header the header to use with the message.
    * @param dyno the DynamicObject with content to send.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool sendHeaderAndObject(
      monarch::http::HttpConnection* hc, monarch::http::HttpHeader* header,
      monarch::rt::DynamicObject& dyno);
   
   /**
    * Receives the content of this message, if there is any, after the header
    * has already been received, and writes it to the passed OutputStream
    * content sink. The sink will not be closed by this method.
    * 
    * No security check will be run on the header, it is assumed this has
    * already been done via checkHeaderSecurity().
    * 
    * If this message has a PublicKeySource, then a btp security check will
    * be run on the received content.
    * 
    * @param hc the connection overwhich to receive the content.
    * @param header the previously received header.
    * @param os the output stream to write the received content to.
    * 
    * @return true if successful, false if an Exception occurred or was
    *         received.
    */
   virtual bool receiveContentStream(
      monarch::http::HttpConnection* hc,
      monarch::http::HttpHeader* header, monarch::io::OutputStream* os);
   
   /**
    * Receives the content of this message, after the header has already
    * been received, and writes it to the passed DynamicObject.
    * 
    * No security check will be run on the header, it is assumed this has
    * already been done via checkHeaderSecurity().
    * 
    * If this message has a PublicKeySource, then a btp security check will
    * be run on the received content.
    * 
    * @param hc the connection overwhich to receive the content.
    * @param header the previously received header.
    * @param dyno the DynamicObject to write the received content to.
    * 
    * @return true if successful, false if an Exception occurred or was
    *         received.
    */
   virtual bool receiveContentObject(
      monarch::http::HttpConnection* hc, monarch::http::HttpHeader* header,
      monarch::rt::DynamicObject& dyno);
   
   /**
    * Gets a stream to manually receive the content of this message, after the
    * header has already been received.
    * 
    * No security check will be run on the header or the body. It is assumed
    * that checkHeaderSecurity() was already called, and if a security check
    * on the body is desired, checkContentSecurity() must be called manually
    * using the DigitalSignature returned from this method, after the entire
    * content stream has been read.
    * 
    * Closing the returned input stream will not shut down the connection's
    * input.
    * 
    * @param hc the connection overwhich to receive the content.
    * @param header the previously received header.
    * @param is the InputStream to be set to the content stream to read.
    * @param trailer the HttpTrailer to be set to the trailers following
    *                the body.
    * @param ds the DigitalSignature to be set if the content is secure.
    */
   virtual void getContentReceiveStream(
      monarch::http::HttpConnection* hc, monarch::http::HttpHeader* header,
      monarch::io::InputStreamRef& is, monarch::http::HttpTrailerRef& trailer,
      monarch::crypto::DigitalSignatureRef& ds);
};

} // end namespace protocol
} // end namespace bitmunk

#endif
