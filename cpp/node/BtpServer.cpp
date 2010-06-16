/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include "bitmunk/node/BtpServer.h"

#include "bitmunk/common/Tools.h"
#include "bitmunk/node/CertificateCreator.h"
#include "monarch/crypto/AsymmetricKeyFactory.h"
#include "monarch/data/json/JsonWriter.h"
#include "monarch/net/NullSocketDataPresenter.h"
#include "monarch/net/SslSocketDataPresenter.h"

using namespace std;
using namespace monarch::config;
using namespace monarch::crypto;
using namespace monarch::data::json;
using namespace monarch::event;
using namespace monarch::http;
using namespace monarch::io;
using namespace monarch::net;
using namespace monarch::rt;
using namespace monarch::util;
using namespace bitmunk::common;
using namespace bitmunk::protocol;
using namespace bitmunk::node;

BtpServer::BtpServer(Node* node) :
mNode(node),
mHostAddress(NULL),
mSslContext(NULL),
mSocketDataPresenterList(NULL),
mServiceId(Server::sInvalidServiceId)
{
}

BtpServer::~BtpServer()
{
}

bool BtpServer::initialize(Config& cfg)
{
   bool rval = true;

   // create SSL server context ("TLS" is most secure and recent SSL but we
   // must use "ALL" to handle browsers that hit the api using earlier SSL
   // versions ... TLS is only required for BTP and not all SSL traffic to
   // the BtpServer must be BTP)
   mSslContext = new SslContext("ALL", false);

   // setup certificate file and private key
   File certFile(cfg["sslCertificate"]->getString());
   File pkeyFile(cfg["sslPrivateKey"]->getString());

   // generate new private key and self-signed certificate if requested
   // and if no private key or certificate file already exists
   if(cfg["sslGenerate"]->getBoolean() &&
      (!pkeyFile->exists() || !certFile->exists()))
   {
      MO_CAT_INFO(BM_NODE_CAT,
         "SSL X.509 self-signed certificate creation requested, creating...");

      CertificateCreator certCreator(mNode);
      rval = certCreator.createCertificate(pkeyFile, certFile);
      if(rval)
      {
         MO_CAT_INFO(BM_NODE_CAT, "SSL X.509 self-signed certificate created.");
      }
   }

   if(rval)
   {
      // set certificate and private key for SSL context
      rval =
         mSslContext->setCertificate(certFile) &&
         mSslContext->setPrivateKey(pkeyFile);
      if(rval)
      {
         // set default virtual host based on certificate common name
         ByteBuffer b(certFile->getLength());
         rval = certFile.readBytes(&b);
         if(rval)
         {
            AsymmetricKeyFactory afk;
            X509CertificateRef cert = afk.loadCertificateFromPem(
               b.data(), b.length());
            rval = !cert.isNull();
            if(rval)
            {
               DynamicObject subject = cert->getSubject();
               string commonName = cert->getField(subject, "CN");
               mSslContext->setVirtualHost(commonName.c_str());
            }
         }
      }
   }

   if(rval)
   {
      // setup host address for node
      const char* host = cfg["host"]->getString();
      uint32_t port = cfg["port"]->getUInt32();
      mHostAddress = new InternetAddress(host, port);

      // handle socket presentation layer that is SSL or non-SSL
      mSocketDataPresenterList = new SocketDataPresenterList(true);
      SslSocketDataPresenter* ssdp =
         new SslSocketDataPresenter(&(*mSslContext));
      NullSocketDataPresenter* nsdp = new NullSocketDataPresenter();
      mSocketDataPresenterList->add(ssdp);
      mSocketDataPresenterList->add(nsdp);

      // get the list of default domains
      if(cfg->hasMember("domains"))
      {
         mDefaultDomains = cfg["domains"].clone();
         if(mDefaultDomains->length() == 0)
         {
            // add wildcard if no domains specified
            mDefaultDomains->append() = "*";
         }
      }
      else
      {
         // no specified default domains, so use "*"
         mDefaultDomains = DynamicObject();
         mDefaultDomains->append() = "*";
      }

      MO_CAT_INFO(BM_NODE_CAT, "Running services on domains: %s",
         JsonWriter::writeToString(mDefaultDomains, false, false).c_str());
   }

   return rval;
}

void BtpServer::cleanup()
{
   // clean up services
   for(DomainMap::iterator i = mServices.begin(); i != mServices.end(); ++i)
   {
      free((char*)i->first);
      delete i->second;
   }

   // clean up
   mHostAddress.setNull();
   mSslContext.setNull();
   mSocketDataPresenterList.setNull();
   mDefaultDomains.setNull();
}

bool BtpServer::start()
{
   bool rval = true;

   // add http connection service
   mServiceId = mNode->getServer()->addConnectionService(
      &(*mHostAddress), &mHttpConnectionServicer,
      &(*mSocketDataPresenterList), "MainHttpService");
   if(mServiceId == Server::sInvalidServiceId)
   {
      // could not add connection service
      rval = false;
   }
   else
   {
      MO_CAT_INFO(BM_NODE_CAT, "Serving on %s",
         mHostAddress->toString(false).c_str());
   }

   return rval;
}

void BtpServer::stop()
{
   // remove http connection service
   if(mServiceId != Server::sInvalidServiceId)
   {
      mNode->getServer()->removePortService(mServiceId);
      mServiceId = Server::sInvalidServiceId;
   }
}

/**
 * Makes sure a btp service is added in both secure and non-secure mode (if
 * specified) or it isn't added at all.
 *
 * @param bs the service to add.
 * @param ssl the ssl mode.
 * @param hcs the servicer to add the service to.
 * @param domain the domain to add the service to.
 *
 * @return true if added, false if not.
 */
static bool _addService(
   BtpServiceRef& bs, Node::SslStatus ssl, HttpConnectionServicer* hcs,
   const char* domain)
{
   bool rval = true;

   // try to add secure service if applicable
   bool added = false;
   if(ssl == Node::SslOn || ssl == Node::SslAny)
   {
      // try to add the service
      rval = hcs->addRequestServicer(&(*bs), true, domain);
      added = true;
   }

   // if success, add non-secure service if applicable
   if(rval && (ssl == Node::SslOff || ssl == Node::SslAny))
   {
      // if non-secure service could not be added, remove the secure one
      // it was added
      if(!hcs->addRequestServicer(&(*bs), false, domain))
      {
         rval = false;
         if(added)
         {
            hcs->removeRequestServicer(&(*bs), true, domain);
         }
      }
   }

   return rval;
}

bool BtpServer::addService(
   BtpServiceRef& service, Node::SslStatus ssl, bool initialize,
   const char* domain)
{
   bool rval = true;

   if(initialize)
   {
      rval = service->initialize();
   }

   if(rval)
   {
      // get the domain list
      DynamicObject dl(NULL);
      if(domain == NULL)
      {
         dl = mDefaultDomains;
      }
      else
      {
         dl = DynamicObject();
         dl->append() = domain;
      }

      mBtpServiceLock.lock();
      {
         // keep track of domains with added services
         DynamicObject added;
         added->setType(Array);

         // for each domain, add service
         BtpServiceMaps* bsm = NULL;
         DynamicObjectIterator dli = dl.getIterator();
         while(rval && dli->hasNext())
         {
            const char* dom = dli->next()->getString();
            DomainMap::iterator di = mServices.find(dom);
            if(di == mServices.end())
            {
               // add a new maps entry
               bsm = new BtpServiceMaps;
               mServices[strdup(dom)] = bsm;
            }
            else
            {
               // use existing maps entry
               bsm = di->second;
            }

            // try to add the service
            rval = _addService(service, ssl, &mHttpConnectionServicer, dom);
            if(rval)
            {
               // added to the given domain
               added->append() = dom;

               // save reference to service
               const char* path = service->getPath();
               if(ssl == Node::SslOn || ssl == Node::SslAny)
               {
                  bsm->secure[path] = service;
                  MO_CAT_DEBUG(BM_NODE_CAT,
                     "Added SSL BTP service: %s%s", dom, path);
               }
               if(ssl == Node::SslOff || ssl == Node::SslAny)
               {
                  bsm->nonSecure[path] = service;
                  MO_CAT_DEBUG(BM_NODE_CAT,
                     "Added non-SSL BTP service: %s%s", dom, path);
               }
            }
         }

         // if service didn't add to a domain, remove it from all the
         // ones it was added to
         if(!rval)
         {
            dli = added.getIterator();
            while(dli->hasNext())
            {
               const char* dom = dli->next()->getString();
               removeService(service->getPath(), ssl, false, dom);
            }
         }
      }
      mBtpServiceLock.unlock();

      // failed to add service
      if(!rval)
      {
         if(initialize)
         {
            // clean up service
            service->cleanup();
         }

         // set exception
         ExceptionRef e = new Exception(
            "Could not add btp service.",
            "bitmunk.node.AddBtpServiceFailure");
         Exception::push(e);
      }
   }

   return rval;
}

void BtpServer::removeService(
   const char* path, Node::SslStatus ssl, bool cleanup,
   const char* domain)
{
   // get the domain list
   DynamicObject dl(NULL);
   if(domain == NULL)
   {
      dl = mDefaultDomains;
   }
   else
   {
      dl = DynamicObject();
      dl->append() = domain;
   }

   // build a unique list of services to cleanup
   UniqueList<BtpServiceRef> cleanupList;

   mBtpServiceLock.lock();
   {
      // for each domain, remove service and store it in cleanup list
      DynamicObjectIterator dli = dl.getIterator();
      while(dli->hasNext())
      {
         const char* dom = dli->next()->getString();
         DomainMap::iterator di = mServices.find(dom);
         if(di != mServices.end())
         {
            if(ssl == Node::SslOn || ssl == Node::SslAny)
            {
               BtpServiceMap::iterator i = di->second->secure.find(path);
               if(i != di->second->secure.end())
               {
                  cleanupList.add(i->second);
                  mHttpConnectionServicer.removeRequestServicer(
                     path, true, dom);
                  di->second->secure.erase(i);
                  MO_CAT_DEBUG(BM_NODE_CAT,
                     "Removed SSL BTP service: %s%s", dom, path);
               }
            }

            if(ssl == Node::SslOff || ssl == Node::SslAny)
            {
               BtpServiceMap::iterator i = di->second->nonSecure.find(path);
               if(i != di->second->nonSecure.end())
               {
                  cleanupList.add(i->second);
                  mHttpConnectionServicer.removeRequestServicer(
                     path, false, dom);
                  di->second->nonSecure.erase(i);
                  MO_CAT_DEBUG(BM_NODE_CAT,
                     "Removed non-SSL BTP service: %s%s", dom, path);
               }
            }
         }
      }
   }
   mBtpServiceLock.unlock();

   // clean up services
   if(cleanup)
   {
      IteratorRef<BtpServiceRef> i = cleanupList.getIterator();
      while(i->hasNext())
      {
         BtpServiceRef& bs = i->next();
         bs->cleanup();
      }
   }
}

BtpServiceRef BtpServer::getService(
   const char* path, Node::SslStatus ssl, const char* domain)
{
   BtpServiceRef rval;

   // get the domain list
   DynamicObject dl(NULL);
   if(domain == NULL)
   {
      dl = mDefaultDomains;
   }
   else
   {
      dl = DynamicObject();
      dl->append() = domain;
   }
   domain = dl.first()->getString();

   // look in services map for service
   mBtpServiceLock.lock();
   {
      DomainMap::iterator di = mServices.find(domain);
      if(di != mServices.end())
      {
         if(ssl == Node::SslOff || ssl == Node::SslAny)
         {
            BtpServiceMap::iterator i = di->second->nonSecure.find(path);
            if(i != di->second->nonSecure.end())
            {
               rval = i->second;
            }
         }

         if(rval.isNull() && ssl != Node::SslOff)
         {
            BtpServiceMap::iterator i = di->second->secure.find(path);
            if(i != di->second->secure.end())
            {
               rval = i->second;
            }
         }
      }
   }
   mBtpServiceLock.unlock();

   return rval;
}

InternetAddressRef BtpServer::getHostAddress()
{
   return mHostAddress;
}

bool BtpServer::addVirtualHost(ProfileRef& profile)
{
   bool rval = false;

   // FIXME: for future support of client-side certificates, enable peer
   // authentication (pass true instead of false to SslContext constructor)
   // and add same trusted certs used by BtpClient, then add a way to expose
   // the user in the client cert (currently taken from common name) to the
   // application as the bitmunk user ID of the caller, keep in mind agent
   // headers (acting on behalf of another bitmunk user ID) must still be
   // checked

   // create an SSL context for the user's virtual host
   SslContextRef ctx = new SslContext("TLS", false);
   rval =
      ctx->setCertificate(profile->getCertificate()) &&
      ctx->setPrivateKey(profile->getPrivateKey());

   if(rval)
   {
      string name = Tools::getBitmunkUserCommonName(profile->getUserId());
      ctx->setVirtualHost(name.c_str());
      MO_CAT_DEBUG(BM_NODE_CAT, "Adding virtual host '%s'", name.c_str());

      // add the virtual host
      rval = mSslContext->addVirtualHost(ctx);
   }

   return rval;
}

void BtpServer::removeVirtualHost(UserId userId, SslContextRef* ctx)
{
   string name = Tools::getBitmunkUserCommonName(userId);
   MO_CAT_DEBUG(BM_NODE_CAT, "Removing virtual host '%s'", name.c_str());
   mSslContext->removeVirtualHost(name.c_str(), ctx);
}
