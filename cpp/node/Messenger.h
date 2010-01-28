/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_node_Messenger_H
#define bitmunk_node_Messenger_H

#include "bitmunk/protocol/BtpClient.h"
#include "monarch/config/ConfigManager.h"

namespace bitmunk
{
namespace node
{

// forward declarations
class Node;

/**
 * A Messenger is used to send and receive btp messages for a Bitmunk Node.
 *
 * @author Dave Longley
 */
class Messenger
{
protected:
   /**
    * The Node this Messenger works for.
    */
   Node* mNode;

   /**
    * The url to Bitmunk.
    */
   monarch::net::UrlRef mBitmunkUrl;

   /**
    * The secure url to Bitmunk.
    */
   monarch::net::UrlRef mSecureBitmunkUrl;

   /**
    * This Messenger's client communications interface.
    */
   bitmunk::protocol::BtpClient mClient;

public:
   /**
    * Creates a new Messenger for the passed Node.
    *
    * @param node the Node the Messenger is for.
    * @param cfg the configuration with bitmunk URLs.
    */
   Messenger(Node* node, monarch::config::Config& cfg);

   /**
    * Destructs this Messenger.
    */
   virtual ~Messenger();

   /**
    * Gets the base url to this Messenger's node,
    * i.e. "https://localhost:19100"
    *
    * @param ssl true to use 'https' false to use 'http'.
    *
    * @return the base url to this Messenger's node.
    */
   virtual std::string getSelfUrl(bool ssl);

   /**
    * Gets the url for Bitmunk.
    *
    * @return the url for Bitmunk.
    */
   virtual monarch::net::Url* getBitmunkUrl();

   /**
    * Gets the secure url for Bitmunk.
    *
    * @return the secure url for Bitmunk.
    */
   virtual monarch::net::Url* getSecureBitmunkUrl();

   /**
    * Sends the passed BtpMessage "out" to the given Url. If there is a BTP
    * response, the passed BtpMessage "in" will be populated and its content
    * written to its content sink.
    *
    * The outgoing message, if secure, will automatically have its secure
    * parameters set.
    *
    * The outgoing message should have its content source set via
    * setContent() or setContentSource().
    *
    * The incoming message, if secure, will automatically have its
    * secure parameters set.
    *
    * If the incoming message is not NULL, it should have its content sink set
    * via setContent() or setContentSink().
    *
    * @param peerId the ID of the bitmunk user to talk to,
    *           0 for none or to use nodeuser.
    * @param url the url to send the message to.
    * @param out the outgoing request message.
    * @param in the incoming response message.
    * @param userId the ID of the user making the btp-authenticated call, 0 to
    *               use no btp-authentication.
    * @param agentId the ID of the agent that will sign the message or 0 to
    *                use the passed user ID as the agent ID for a
    *                btp-authenticated call.
    * @param timeout the timeout in seconds.
    *
    * @return true if successful, false if an Exception occurred or was
    *         received.
    */
   virtual bool exchange(
      bitmunk::common::UserId peerId,
      monarch::net::Url* url,
      bitmunk::protocol::BtpMessage* out,
      bitmunk::protocol::BtpMessage* in,
      bitmunk::common::UserId userId = 0,
      bitmunk::common::UserId agentId = 0,
      uint32_t timeout = 30);

   /**
    * Sends the passed DynamicObject "out" to the given Url. If there is a BTP
    * response, the passed DynamicObject "in" will be populated.
    *
    * The outgoing message, if secure, will automatically have its secure
    * parameters set.
    *
    * The incoming message, if secure, will automatically have its
    * secure parameters set.
    *
    * @param peerId the ID of the bitmunk user to talk to,
    *           0 for none or to use nodeuser.
    * @param type the BtpMessage Type (i.e. Get, Post, Put, Delete).
    * @param url the url to send the message to.
    * @param out the outgoing object, NULL if none.
    * @param in the incoming object, NULL if none.
    * @param userId the ID of the user making the btp-authenticated call, 0 to
    *               use no btp-authentication.
    * @param agentId the ID of the agent that will sign the message or 0 to
    *                use the passed user ID as the agent ID for a
    *                btp-authenticated call.
    * @param headers any extra headers to add to the request.
    * @param timeout the timeout in seconds.
    *
    * @return true if successful, false if an Exception occurred or was
    *         received.
    */
   virtual bool exchange(
      bitmunk::common::UserId peerId,
      bitmunk::protocol::BtpMessage::Type type,
      monarch::net::Url* url,
      monarch::rt::DynamicObject* out,
      monarch::rt::DynamicObject* in,
      bitmunk::common::UserId userId = 0,
      bitmunk::common::UserId agentId = 0,
      monarch::rt::DynamicObject* headers = NULL,
      uint32_t timeout = 30);

   /**
    * Puts an object to the passed url, assuming the url is to a peer.
    *
    * @param url the url to communicate with.
    * @param out the object to send.
    * @param in the object to receive, NULL to receive none.
    * @param userId the ID of the user making the btp-authenticated call, 0 to
    *               use no btp-authentication.
    * @param agentId the ID of the agent that will sign the message or 0 to
    *                use the passed user ID as the agent ID for a
    *                btp-authenticated call.
    * @param headers any extra headers to add to the request.
    * @param timeout the timeout in seconds.
    *
    * @return true if successful, false if an exception occurred or was
    *         received.
    */
   virtual bool put(
      monarch::net::Url* url,
      monarch::rt::DynamicObject& out, monarch::rt::DynamicObject* in = NULL,
      bitmunk::common::UserId userId = 0, bitmunk::common::UserId agentId = 0,
      monarch::rt::DynamicObject* headers = NULL, uint32_t timeout = 30);

   /**
    * Puts an object to Bitmunk.
    *
    * @param url the relative url to be appended to the Bitmunk url.
    * @param out the object to send.
    * @param in the object to receive, NULL to receive none.
    * @param userId the ID of the user making the btp-authenticated call, 0 to
    *               use no btp-authentication.
    * @param agentId the ID of the agent that will sign the message or 0 to
    *                use the passed user ID as the agent ID for a
    *                btp-authenticated call.
    * @param headers any extra headers to add to the request.
    * @param timeout the timeout in seconds.
    *
    * @return true if successful, false if an exception occurred or was
    *         received.
    */
   virtual bool putToBitmunk(
      monarch::net::Url* url,
      monarch::rt::DynamicObject& out, monarch::rt::DynamicObject* in = NULL,
      bitmunk::common::UserId userId = 0, bitmunk::common::UserId agentId = 0,
      monarch::rt::DynamicObject* headers = NULL, uint32_t timeout = 30);

   /**
    * Securely puts an object to Bitmunk.
    *
    * @param url the relative url to be appended to the Bitmunk url.
    * @param out the object to send.
    * @param in the object to receive, NULL to receive none.
    * @param userId the ID of the user making the btp-authenticated call, 0 to
    *               use no btp-authentication.
    * @param agentId the ID of the agent that will sign the message or 0 to
    *                use the passed user ID as the agent ID for a
    *                btp-authenticated call.
    * @param headers any extra headers to add to the request.
    * @param timeout the timeout in seconds.
    *
    * @return true if successful, false if an exception occurred or was
    *         received.
    */
   virtual bool putSecureToBitmunk(
      monarch::net::Url* url,
      monarch::rt::DynamicObject& out, monarch::rt::DynamicObject* in = NULL,
      bitmunk::common::UserId userId = 0, bitmunk::common::UserId agentId = 0,
      monarch::rt::DynamicObject* headers = NULL, uint32_t timeout = 30);

   /**
    * Posts an object to the passed url, assuming the url is to a peer.
    *
    * @param url the url to communicate with.
    * @param out the object to send, NULL to send none.
    * @param in the object to receive, NULL to receive none.
    * @param userId the ID of the user making the btp-authenticated call, 0 to
    *               use no btp-authentication.
    * @param agentId the ID of the agent that will sign the message or 0 to
    *                use the passed user ID as the agent ID for a
    *                btp-authenticated call.
    * @param headers any extra headers to add to the request.
    * @param timeout the timeout in seconds.
    *
    * @return true if successful, false if an exception occurred or was
    *         received.
    */
   virtual bool post(
      monarch::net::Url* url,
      monarch::rt::DynamicObject* out, monarch::rt::DynamicObject* in = NULL,
      bitmunk::common::UserId userId = 0, bitmunk::common::UserId agentId = 0,
      monarch::rt::DynamicObject* headers = NULL, uint32_t timeout = 30);

   /**
    * Posts an object to Bitmunk.
    *
    * @param url the relative url to be appended to the Bitmunk url.
    * @param out the object to send, NULL to send none.
    * @param in the object to receive, NULL to receive none.
    * @param userId the ID of the user making the btp-authenticated call, 0 to
    *               use no btp-authentication.
    * @param agentId the ID of the agent that will sign the message or 0 to
    *                use the passed user ID as the agent ID for a
    *                btp-authenticated call.
    * @param headers any extra headers to add to the request.
    * @param timeout the timeout in seconds.
    *
    * @return true if successful, false if an exception occurred or was
    *         received.
    */
   virtual bool postToBitmunk(
      monarch::net::Url* url,
      monarch::rt::DynamicObject* out, monarch::rt::DynamicObject* in,
      bitmunk::common::UserId userId = 0, bitmunk::common::UserId agentId = 0,
      monarch::rt::DynamicObject* headers = NULL, uint32_t timeout = 30);

   /**
    * Securely posts an object to Bitmunk.
    *
    * @param url the relative url to be appended to the Bitmunk url.
    * @param out the object to send, NULL to send none.
    * @param in the object to receive, NULL to receive none.
    * @param userId the ID of the user making the btp-authenticated call, 0 to
    *               use no btp-authentication.
    * @param agentId the ID of the agent that will sign the message or 0 to
    *                use the passed user ID as the agent ID for a
    *                btp-authenticated call.
    * @param headers any extra headers to add to the request.
    * @param timeout the timeout in seconds.
    *
    * @return true if successful, false if an exception occurred or was
    *         received.
    */
   virtual bool postSecureToBitmunk(
      monarch::net::Url* url,
      monarch::rt::DynamicObject* out, monarch::rt::DynamicObject* in = NULL,
      bitmunk::common::UserId userId = 0, bitmunk::common::UserId agentId = 0,
      monarch::rt::DynamicObject* headers = NULL, uint32_t timeout = 30);

   /**
    * Gets an object from the passed url, assuming the url is to a peer.
    *
    * @param url the url to communicate with.
    * @param in the object to receive.
    * @param userId the ID of the user making the btp-authenticated call, 0 to
    *               use no btp-authentication.
    * @param agentId the ID of the agent that will sign the message or 0 to
    *                use the passed user ID as the agent ID for a
    *                btp-authenticated call.
    * @param headers any extra headers to add to the request.
    * @param timeout the timeout in seconds.
    *
    * @return true if successful, false if an exception occurred or was
    *         received.
    */
   virtual bool get(
      monarch::net::Url* url,
      monarch::rt::DynamicObject& in,
      bitmunk::common::UserId userId = 0, bitmunk::common::UserId agentId = 0,
      monarch::rt::DynamicObject* headers = NULL, uint32_t timeout = 30);

   /**
    * Gets an object from Bitmunk.
    *
    * @param url the relative url to be appended to the Bitmunk url.
    * @param in the object to receive.
    * @param userId the ID of the user making the btp-authenticated call, 0 to
    *               use no btp-authentication.
    * @param agentId the ID of the agent that will sign the message or 0 to
    *                use the passed user ID as the agent ID for a
    *                btp-authenticated call.
    * @param headers any extra headers to add to the request.
    * @param timeout the timeout in seconds.
    *
    * @return true if successful, false if an exception occurred or was
    *         received.
    */
   virtual bool getFromBitmunk(
      monarch::net::Url* url,
      monarch::rt::DynamicObject& in,
      bitmunk::common::UserId userId = 0, bitmunk::common::UserId agentId = 0,
      monarch::rt::DynamicObject* headers = NULL, uint32_t timeout = 30);

   /**
    * Securely gets an object from Bitmunk.
    *
    * @param url the relative url to be appended to the Bitmunk url.
    * @param in the object to receive.
    * @param userId the ID of the user making the btp-authenticated call, 0 to
    *               use no btp-authentication.
    * @param agentId the ID of the agent that will sign the message or 0 to
    *                use the passed user ID as the agent ID for a
    *                btp-authenticated call.
    * @param headers any extra headers to add to the request.
    * @param timeout the timeout in seconds.
    *
    * @return true if successful, false if an exception occurred or was
    *         received.
    */
   virtual bool getSecureFromBitmunk(
      monarch::net::Url* url,
      monarch::rt::DynamicObject& in,
      bitmunk::common::UserId userId = 0, bitmunk::common::UserId agentId = 0,
      monarch::rt::DynamicObject* headers = NULL, uint32_t timeout = 30);

   /**
    * Deletes a resource, assuming the url is to a peer.
    *
    * @param url the url to the resource.
    * @param in the object to receive, NULL to receive none.
    * @param userId the ID of the user making the btp-authenticated call, 0 to
    *               use no btp-authentication.
    * @param agentId the ID of the agent that will sign the message or 0 to
    *                use the passed user ID as the agent ID for a
    *                btp-authenticated call.
    * @param headers any extra headers to add to the request.
    * @param timeout the timeout in seconds.
    *
    * @return true if successful, false if an exception occurred or was
    *         received.
    */
   virtual bool deleteResource(
      monarch::net::Url* url,
      monarch::rt::DynamicObject* in = NULL,
      bitmunk::common::UserId userId = 0, bitmunk::common::UserId agentId = 0,
      monarch::rt::DynamicObject* headers = NULL, uint32_t timeout = 30);

   /**
    * Deletes a Bitmunk resource.
    *
    * @param url the relative url to be appended to the Bitmunk url.
    * @param in the object to receive, NULL to receive none.
    * @param userId the ID of the user making the btp-authenticated call, 0 to
    *               use no btp-authentication.
    * @param agentId the ID of the agent that will sign the message or 0 to
    *                use the passed user ID as the agent ID for a
    *                btp-authenticated call.
    * @param headers any extra headers to add to the request.
    * @param timeout the timeout in seconds.
    *
    * @return true if successful, false if an exception occurred or was
    *         received.
    */
   virtual bool deleteFromBitmunk(
      monarch::net::Url* url,
      monarch::rt::DynamicObject* in = NULL,
      bitmunk::common::UserId userId = 0, bitmunk::common::UserId agentId = 0,
      monarch::rt::DynamicObject* headers = NULL, uint32_t timeout = 30);

   /**
    * Securely deletes a Bitmunk resource.
    *
    * @param url the relative url to be appended to the Bitmunk url.
    * @param in the object to receive, NULL to receive none.
    * @param userId the ID of the user making the btp-authenticated call, 0 to
    *               use no btp-authentication.
    * @param agentId the ID of the agent that will sign the message or 0 to
    *                use the passed user ID as the agent ID for a
    *                btp-authenticated call.
    * @param headers any extra headers to add to the request.
    * @param timeout the timeout in seconds.
    *
    * @return true if successful, false if an exception occurred or was
    *         received.
    */
   virtual bool deleteSecureFromBitmunk(
      monarch::net::Url* url,
      monarch::rt::DynamicObject* in = NULL,
      bitmunk::common::UserId userId = 0, bitmunk::common::UserId agentId = 0,
      monarch::rt::DynamicObject* headers = NULL, uint32_t timeout = 30);

   /**
    * Gets this Messenger's BtpClient. The BtpClient can be used directly
    * if a message with different credentials (other than the logged in
    * user) needs to be sent, if the bandwidth throttlers for the client
    * need to be changed, or if some other special message needs to be sent
    * or received.
    *
    * @return this Messengers's BtpClient.
    */
   virtual bitmunk::protocol::BtpClient* getBtpClient();
};

// typedef for reference counted Messenger
typedef monarch::rt::Collectable<Messenger> MessengerRef;

} // end namespace node
} // end namespace bitmunk
#endif
