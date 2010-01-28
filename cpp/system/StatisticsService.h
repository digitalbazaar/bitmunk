/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_system_StatisticsService_H
#define bitmunk_system_StatisticsService_H

#include "bitmunk/node/NodeService.h"
#include "monarch/util/Timer.h"

namespace bitmunk
{
namespace system
{

/**
 * A StatisticsService services system statistics related btp actions.
 * 
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 */
class StatisticsService : public bitmunk::node::NodeService
{
protected:
   /**
    * Uptime timer since module created.
    */
   monarch::util::Timer mUptimeTimer;
   
public:
   /**
    * Creates a new StatisticsService.
    * 
    * @param node the associated Bitmunk Node.
    * @param path the path this servicer handles requests for.
    */
   StatisticsService(bitmunk::node::Node* node, const char* path);
   
   /**
    * Destructs this StatisticsService.
    */
   virtual ~StatisticsService();
   
   /**
    * Initializes this BtpService.
    * 
    * @return true if initialized, false if an Exception occurred.
    */
   virtual bool initialize();
   
   /**
    * Cleans up this BtpService.
    * 
    * Must be called after initialize() regardless of its return value.
    */
   virtual void cleanup();
   
   /**
    * Gets the DynamicObject debugging statistics.
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getDynoStats(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
   
   /**
    * Gets the system statistics.
    * 
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    * 
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getStats(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);

   /**
    * Gets the system uptime.
    *
    * @param action the BtpAction.
    * @param in the incoming DynamicObject.
    * @param out the outgoing DynamicObject.
    *
    * @return true if successful, false if an Exception occurred.
    */
   virtual bool getUptime(
      bitmunk::protocol::BtpAction* action,
      monarch::rt::DynamicObject& in, monarch::rt::DynamicObject& out);
};

} // end namespace system
} // end namespace bitmunk
#endif
