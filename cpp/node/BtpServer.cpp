/*
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 */
#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include "bitmunk/node/BtpServer.h"

#include "bitmunk/common/Tools.h"
#include "bitmunk/node/CertificateCreator.h"
#include "bitmunk/node/NodeModule.h"
#include "bitmunk/node/ProxyService.h"
#include "monarch/crypto/AsymmetricKeyFactory.h"
#include "monarch/net/NullSocketDataPresenter.h"
#include "monarch/net/SslSocketDataPresenter.h"

using namespace std;
using namespace monarch::config;
using namespace monarch::crypto;
using namespace monarch::event;
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

   // create SSL server context ("TLS" is most secure and recent SSL)
   mSslContext = new SslContext("TLS", false);

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
   }

   // add proxy service(s) if configured
   if(rval && cfg->hasMember("proxy"))
   {
      /* Proxy config example:
       *
       * {
       *    "/path/to/handler":
       *    {
       *       "*":
       *       {
       *          "host": "localhost:8080"
       *       }
       *    },
       *    "/hosted/by/another/server":
       *    {
       *       "/foo/bar":
       *       {
       *          "path": "/some/other/path"
       *       },
       *       "/foo/bar/2":
       *       {
       *          "host": "localhost:8080"
       *       }
       *    },
       *    "/hosted/by/another/server/foo/bar3":
       *    {
       *       "*":
       *       {
       *          "host": "localhost:8080",
       *          "path": "/new/path"
       *       }
       *    }
       * }
       *
       * Will proxy the following:
       *
       * http://myserver.com/path/to/handler/anything =>
       * http://localhost:8080/path/to/handler/anything
       *
       * http://myserver.com/hosted/by/another/server/foo/bar =>
       * http://myserver.com/some/other/path
       *
       * http://myserver.com/hosted/by/another/server/foo/bar2 =>
       * http://localhost:8080/hosted/by/another/server/foo/bar2
       *
       * http://myserver.com/hosted/by/another/server/foo/bar3/anything =>
       * http://localhost:8080/new/path/anything
       */
      BtpServiceRef bs(NULL);
      Config& proxy = cfg["proxy"];
      if(proxy->length() > 0)
      {
         ConfigIterator pi = proxy.getIterator();
         while(rval && pi->hasNext())
         {
            Config& path = pi->next();
            ProxyService* ps = new ProxyService(mNode, pi->getName());
            ConfigIterator ri = path.getIterator();
            while(ri->hasNext())
            {
               Config& rule = ri->next();
               string url;
               if(rule->hasMember("host"))
               {
                  url = rule["host"]->getString();
               }
               if(rule->hasMember("path"))
               {
                  url.append(rule["path"]->getString());
               }
               else
               {
                  url.append(pi->getName());
                  if(strcmp(ri->getName(), "*") != 0)
                  {
                     url.append(ri->getName());
                  }
               }
               ps->addMapping(ri->getName(), url.c_str());
            }
            bs = ps;
            rval = addService(bs, Node::SslOff, true);
         }
      }
   }

   return rval;
}

void BtpServer::cleanup()
{
   // remove any lingering services
   while(!mSecureServices.empty())
   {
      removeService(mSecureServices.begin()->first);
   }
   while(!mNonSecureServices.empty())
   {
      removeService(mNonSecureServices.begin()->first);
   }

   // remove http connection service
   if(mServiceId != Server::sInvalidServiceId)
   {
      mNode->getServer()->removePortService(mServiceId);
      mServiceId = Server::sInvalidServiceId;
   }

   // clean up
   mHostAddress.setNull();
   mSslContext.setNull();
   mSocketDataPresenterList.setNull();
}

bool BtpServer::addService(
   BtpServiceRef& service, Node::SslStatus ssl, bool initialize)
{
   bool rval = true;

   if(initialize)
   {
      rval = service->initialize();
   }

   if(rval)
   {
      mBtpServiceLock.lock();
      {
         // ensure service can be added by setting this to true only
         // after it has been added
         rval = false;

         if(ssl == Node::SslOn || ssl == Node::SslAny)
         {
            if(mHttpConnectionServicer.addRequestServicer(&(*service), true))
            {
               mSecureServices[service->getPath()] = service;
               rval = true;
            }
         }

         if(ssl == Node::SslOff || ssl == Node::SslAny)
         {
            if(mHttpConnectionServicer.addRequestServicer(&(*service), false))
            {
               mNonSecureServices[service->getPath()] = service;
               rval = true;
            }
            else if(rval)
            {
               // ensure secure servicer is removed
               mHttpConnectionServicer.removeRequestServicer(&(*service), true);
               mSecureServices.erase(service->getPath());
               rval = false;
            }
            else
            {
               rval = false;
            }
         }
      }
      mBtpServiceLock.unlock();

      // failed to add service
      if(!rval)
      {
         // clean up service
         service->cleanup();

         // set exception
         ExceptionRef e = new Exception(
            "Could not add btp service.",
            "bitmunk.node.AddBtpServiceFailure");
         Exception::push(e);
      }
      else
      {
         if(ssl == Node::SslOn || ssl == Node::SslAny)
         {
            MO_CAT_DEBUG(BM_NODE_CAT,
               "Added SSL BTP service: %s", service->getPath());
         }

         if(ssl == Node::SslOff || ssl == Node::SslAny)
         {
            MO_CAT_DEBUG(BM_NODE_CAT,
               "Added non-SSL BTP service: %s", service->getPath());
         }
      }
   }

   return rval;
}

void BtpServer::removeService(
   BtpServiceRef& service, Node::SslStatus ssl, bool cleanup)
{
   mBtpServiceLock.lock();
   {
      if(ssl == Node::SslOn || ssl == Node::SslAny)
      {
         mHttpConnectionServicer.removeRequestServicer(&(*service), true);
         mSecureServices.erase(service->getPath());
      }

      if(ssl == Node::SslOff || ssl == Node::SslAny)
      {
         mHttpConnectionServicer.removeRequestServicer(&(*service), false);
         mNonSecureServices.erase(service->getPath());
      }
   }
   mBtpServiceLock.unlock();

   if(ssl == Node::SslOn || ssl == Node::SslAny)
   {
      MO_CAT_DEBUG(BM_NODE_CAT,
         "Removed SSL BTP service: %s", service->getPath());
   }

   if(ssl == Node::SslOff || ssl == Node::SslAny)
   {
      MO_CAT_DEBUG(BM_NODE_CAT,
         "Removed non-SSL BTP service: %s", service->getPath());
   }

   if(cleanup)
   {
      service->cleanup();
   }
}

BtpServiceRef BtpServer::removeService(
   const char* path, Node::SslStatus ssl, bool cleanup)
{
   BtpServiceRef rval;

   BtpServiceRef sslOn;
   BtpServiceRef sslOff;

   mBtpServiceLock.lock();
   {
      if(ssl == Node::SslOn || ssl == Node::SslAny)
      {
         BtpServiceMap::iterator i = mSecureServices.find(path);
         if(i != mSecureServices.end())
         {
            sslOn = i->second;
            mHttpConnectionServicer.removeRequestServicer(path, true);
            mSecureServices.erase(i);
         }
      }

      if(ssl == Node::SslOff || ssl == Node::SslAny)
      {
         BtpServiceMap::iterator i = mNonSecureServices.find(path);
         if(i != mNonSecureServices.end())
         {
            sslOff = i->second;
            mHttpConnectionServicer.removeRequestServicer(path, false);
            mNonSecureServices.erase(i);
         }
      }
   }
   mBtpServiceLock.unlock();

   if(!sslOn.isNull())
   {
      MO_CAT_DEBUG(BM_NODE_CAT, "Removed SSL BTP service: %s", path);
      if(sslOff.isNull())
      {
         rval = sslOn;
      }
   }

   if(!sslOff.isNull())
   {
      MO_CAT_DEBUG(BM_NODE_CAT, "Removed non-SSL BTP service: %s", path);
      if(rval.isNull())
      {
         rval = sslOff;
      }
   }

   if(cleanup && !rval.isNull())
   {
      // ensure cleanup is not double-called for the same service
      if(sslOn == sslOff)
      {
         rval->cleanup();
      }
      else
      {
         if(!sslOn.isNull())
         {
            sslOn->cleanup();
         }

         if(!sslOff.isNull())
         {
            sslOff->cleanup();
         }
      }
   }

   return rval;
}

BtpServiceRef BtpServer::getService(const char* path, Node::SslStatus ssl)
{
   BtpServiceRef rval;

   mBtpServiceLock.lock();
   {
      if(ssl == Node::SslOff || ssl == Node::SslAny)
      {
         BtpServiceMap::iterator i = mNonSecureServices.find(path);
         if(i != mNonSecureServices.end())
         {
            rval = i->second;
         }
      }

      if(rval.isNull() && ssl != Node::SslOff)
      {
         BtpServiceMap::iterator i = mSecureServices.find(path);
         if(i != mSecureServices.end())
         {
            rval = i->second;
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
