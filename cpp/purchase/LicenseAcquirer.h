/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_purchase_LicenseAcquirer_H
#define bitmunk_purchase_LicenseAcquirer_H

#include "bitmunk/purchase/DownloadStateFiber.h"
#include "bitmunk/node/NodeFiber.h"

namespace bitmunk
{
namespace purchase
{

/**
 * A LicenseAcquirer is used to acquire a signed license for data to be
 * downloaded.
 * 
 * It can do so by acquiring a new license from an SVA (or other valid license
 * source) or it can do so by acquiring a pre-paid license from the sqlite3
 * database.
 * 
 * If doing a remote operation, then create an Operation that will run a
 * btp-call and then go inactive. When the btp-call completes, the
 * LicenseAcquirer will be re-activated. In the future, we will want to
 * optimize these so they are non-blocking and only use a separate thread for
 * their IO when it is ready to be sent/received. There is currently no easy
 * way to "recover" when nonblocking IO would block for things like
 * reading/writing DynamicObjects.
 * 
 * @author Dave Longley
 */
class LicenseAcquirer : public bitmunk::node::NodeFiber, public DownloadStateFiber
{
public:
   /**
    * Creates a new LicenseAcquirer.
    * 
    * @param node the Node associated with this fiber.
    * @param parentId the ID of the parent fiber.
    */
   LicenseAcquirer(bitmunk::node::Node* node);
   
   /**
    * Destructs this LicenseAcquirer.
    */
   virtual ~LicenseAcquirer();
   
   /**
    * Acquires a license via an Operation.
    */
   virtual void acquireLicense();
   
protected:
   /**
    * Processes messages, retrieved via getMessages(), and performs whatever
    * custom work is necessary.
    */
   virtual void processMessages();
};

} // end namespace purchase
} // end namespace bitmunk
#endif
