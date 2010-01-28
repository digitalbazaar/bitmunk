/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_node_NodeFiber_H
#define bitmunk_node_NodeFiber_H

#include "monarch/fiber/MessagableFiber.h"
#include "bitmunk/node/Node.h"

namespace bitmunk
{
namespace node
{

/**
 * A NodeFiber is a MessagableFiber that has access to a node's interface and
 * is scheduled by that node's FiberScheduler. The fiber can also be assigned
 * a user ID, if a particular user owns the work that is being executed on
 * the fiber and that work should be notified when the user is attempting to
 * log out of the node.
 *
 * NodeFibers may also be given a "parent" ID. This is the ID of the fiber
 * that spawned the NodeFiber and it may be waiting for a message from
 * its child to advance its work. A message can be sent to the parent of
 * a fiber via messageParent().
 *
 * The NodeFibers that are owned by a particular user can be interrupted via
 * the associated NodeFiberScheduler. This will happen automatically when a
 * user logs out of a node. Each NodeFiber will receive an interrupted message
 * and should handle it appropriately, exiting as quickly as possible.
 *
 * Any message sent by a NodeFiber will include in it these reserved fields:
 *
 * fiberId:   The ID of the fiber that sent the message.
 * interrupt: Set to true to interrupt a NodeFiber.
 *
 * @author Dave Longley
 */
class NodeFiber : public monarch::fiber::MessagableFiber
{
protected:
   /**
    * The Node associated with this fiber.
    */
   Node* mNode;

   /**
    * The ID of this fiber's parent, 0 for none.
    */
   monarch::fiber::FiberId mParentId;

   /**
    * The ID of the user that owns this Fiber, 0 for none.
    */
   bitmunk::common::UserId mUserId;

   /**
    * Data to include in an event when exiting.
    */
   monarch::rt::DynamicObject mFiberExitData;

public:
   /**
    * Creates a new NodeFiber.
    *
    * @param node the bitmunk Node associated with this fiber.
    * @param parentId the ID of the parent fiber, 0 for no parent.
    * @param stackSize the stack size to use, 0 for default.
    */
   NodeFiber(
      Node* node, monarch::fiber::FiberId parentId = 0, size_t stackSize = 0);

   /**
    * Destructs this NodeFiber.
    */
   virtual ~NodeFiber();

   /**
    * Overridden to send an exit event when this NodeFiber quits.
    */
   virtual void run();

   /**
    * Gets the Node associated with this fiber.
    *
    * @return the Node associated with this fiber.
    */
   virtual Node* getNode();

   /**
    * Gets the FiberId of the parent fiber.
    *
    * @return the FiberId of the parent fiber.
    */
   virtual monarch::fiber::FiberId getParentId();

   /**
    * Called *only* by the NodeFiberScheduler. Sets the ID of the user that
    * owns this fiber, 0 for none.
    *
    * @param userId the ID of the user that owns this fiber.
    */
   virtual void setUserId(bitmunk::common::UserId userId);

   /**
    * Gets the user ID of the user that owns this fiber, 0 for none.
    *
    * @return the ID of the user that owns this fiber.
    */
   virtual bitmunk::common::UserId getUserId();

protected:
   /**
    * A helper function that will cause this fiber to sleep until it
    * receives a message with any of the given keys.
    *
    * This method can also interrupt a passed operation if it receives a
    * message with the "interrupt" key. This key is automatically checked
    * for, it does not need to be included in the "keys" array. If the
    * message is received, the operation will be interrupted and this method
    * will then continue to wait for the other specified messages.
    *
    * @param keys the list of keys to look for.
    * @param op an Operation to interrupt if the "interrupt" key is received,
    *           NULL for no such Operation.
    *
    * @return the list of messages that contain the given keys, in the
    *         order they were received.
    */
   virtual monarch::rt::DynamicObject waitForMessages(
      const char** keys, monarch::modest::Operation* op = NULL);

   /**
    * A convenience method for sending a message to this fiber.
    *
    * @param msg the message to send.
    */
   virtual void messageSelf(monarch::rt::DynamicObject& msg);

   /**
    * A convenience method for sending a message to this fiber's parent, if
    * it has one. If it does not have a parent, this is a no-op.
    *
    * @param msg the message to send.
    */
   virtual void messageParent(monarch::rt::DynamicObject& msg);
};

} // end namespace node
} // end namespace bitmunk
#endif
