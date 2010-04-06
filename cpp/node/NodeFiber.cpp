/*
 * Copyright (c) 2008-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/node/NodeFiber.h"

#include "bitmunk/node/BitmunkModule.h"

using namespace bitmunk::common;
using namespace bitmunk::node;
using namespace monarch::event;
using namespace monarch::fiber;
using namespace monarch::modest;
using namespace monarch::rt;

NodeFiber::NodeFiber(Node* node, FiberId parentId, size_t stackSize) :
   MessagableFiber(node->getFiberMessageCenter(), stackSize),
   mNode(node),
   mParentId(parentId),
   mUserId(0),
   mFiberExitData(NULL)
{
}

NodeFiber::~NodeFiber()
{
}

void NodeFiber::run()
{
   // register with message center
   mMessageCenter->registerFiber(this);

   if(mUserId != 0)
   {
      // add fiber as a user fiber
      mNode->addUserFiber(getId(), mUserId);
      MO_CAT_DEBUG(BM_NODE_CAT, "Running user-owned NodeFiber %u", getId());
   }
   else
   {
      MO_CAT_DEBUG(BM_NODE_CAT, "Running NodeFiber %u", getId());
   }

   // process messages
   processMessages();

   if(mUserId != 0)
   {
      // remove fiber as a user fiber
      mNode->removeUserFiber(getId(), mUserId);
   }

   if(mParentId != 0)
   {
      // send message to parent fiber
      DynamicObject msg;
      msg["fiberId"] = getId();
      msg["exitChild"] = true;
      if(!mFiberExitData.isNull())
      {
         msg["exitData"] = mFiberExitData;
      }
      sendMessage(mParentId, msg);
   }

   // unregister with message center
   mMessageCenter->unregisterFiber(this);

   // send event that node fiber has exited
   Event e;
   e["type"] = "bitmunk.node.NodeFiber.exited";
   e["details"]["fiberId"] = getId();
   e["details"]["userId"] = mUserId;
   if(!mFiberExitData.isNull())
   {
      e["details"]["exitData"] = mFiberExitData.clone();
   }
   mNode->getEventController()->schedule(e);

   MO_CAT_DEBUG(BM_NODE_CAT, "NodeFiber %u exited", getId());
}

inline Node* NodeFiber::getNode()
{
   return mNode;
}

inline FiberId NodeFiber::getParentId()
{
   return mParentId;
}

inline void NodeFiber::setUserId(UserId userId)
{
   mUserId = userId;
}

inline UserId NodeFiber::getUserId()
{
   return mUserId;
}

DynamicObject NodeFiber::waitForMessages(const char** keys, Operation* op)
{
   DynamicObject list;
   list->setType(Array);

   while(list->length() == 0)
   {
      sleep();

      // get messages
      FiberMessageQueue* msgs = getMessages();
      while(!msgs->empty())
      {
         DynamicObject msg = msgs->front();
         msgs->pop_front();

         if(op != NULL && !op->isNull())
         {
            // check for interrupt key
            if(msg->hasMember("interrupt"))
            {
               // send interrupted event
               Event e;
               e["type"] = "bitmunk.node.NodeFiber.interrupted";
               e["details"]["fiberId"] = getId();
               mNode->getEventController()->schedule(e);

               (*op)->interrupt();
               while(!(*op)->stopped())
               {
                  sleep();
               }
            }
         }

         // check for keys
         for(int i = 0; keys[i] != NULL; i++)
         {
            if(msg->hasMember(keys[i]))
            {
               // only add message once
               list->append(msg);
               break;
            }
         }
      }
   }

   return list;
}

void NodeFiber::messageSelf(DynamicObject& msg)
{
   // send message to self
   msg["fiberId"] = getId();
   sendMessage(getId(), msg);
}

void NodeFiber::messageParent(DynamicObject& msg)
{
   if(mParentId != 0)
   {
      // send message to parent
      msg["fiberId"] = getId();
      sendMessage(mParentId, msg);
   }
}
