/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 * 
 * @author Dave Longley
 */
package
{
   import flash.net.Socket;
   
   /**
    * A helper class that contains the ID for a Socket.
    */
   public class PooledSocket extends Socket
   {
      public var id:String;
      // FIXME: might need to include a flag to specify SSL socket and
      // a buffer to store bytes
   }
}

