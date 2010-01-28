/*
 * Copyright (c) 2009 Digital Bazaar, Inc.  All rights reserved.
 */
#ifndef bitmunk_common_StateMonitor_H
#define bitmunk_common_StateMonitor_H

#include "monarch/rt/DynamicObject.h"

namespace bitmunk
{
namespace common
{

/**
 * A StateMonitor keeps track of system state by means of an association from
 * keys to values. This class provides an interface such that values can be
 * stored in a central location for the purpose of statistics and other
 * tracking. Classes implementing this interface are designed to monitor
 * the instantaneous state of the system. Time varying data can be collected by
 * querying a StateMonitor periodically.
 * 
 * Clients must first create and initialize a state to monitor via one of the
 * add methods. After initialization the state can be updated and queried as
 * needed. Operations should be performed with the bulk state access methods if
 * possible for efficiency reasons. The DynamicObjects used for group access
 * methods should be immutable even after a call is complete. Callers must
 * assume that once a parameter is passed to a StateMonitor that it should no
 * longer be modified. This allows for an optimization of reusing adjustment
 * objects for multiple StateMonitor calls. It also allows for implementations
 * to perform updates asyncronously with saved request parameters.
 * 
 * Implementations of this interface must be thread safe and should in general
 * be designed for high update throughput. Implementations should attempt to
 * make group access methods transactional such that all operations are done on
 * an effectively locked monitor and do not have side effects if a failure
 * occurs.
 * 
 * The flags can be used to disable checking, control exceptions, and control
 * cloning. Errors that result from disabled checking or incorrect use of
 * cloning (or not cloning) will result in unknown behavior.  By default all
 * checks are run and Exceptions are thrown.
 * 
 * States are specified by strings and values are represented by
 * DynamicObjects. State strings may use any format however a common
 * convention is to use dotted separators which specify a data type and a
 * specific property. Other formats may be used as needed.
 * 
 * @author David I. Lehn <dlehn@digitalbazaar.com>
 */
class StateMonitor
{
public:
   /**
    * StateMonitor check, exception, and clone flags.
    */
   /** Disable checking. */
   static const int NO_FLAGS          = 0;
   /** Clone values in set(...) and adjust(...). */
   static const int CLONE             = (1 << 0);
   /** Throw exceptions during checking. */
   static const int CHECK_THROW       = (1 << 1);
   /** Check state name. */
   static const int CHECK_STATE_NAMES = (1 << 2);
   /** Check type compatibility. */
   static const int CHECK_TYPES       = (1 << 3);
   /** Check numeric overflow. */
   static const int CHECK_OVERFLOW    = (1 << 4);
   /** Check numeric underflow. */
   static const int CHECK_UNDERFLOW   = (1 << 5);
   /** Last flag value. */
   static const int LAST_FLAG         = CHECK_UNDERFLOW;
   /** All checks. */
   static const int CHECK_ALL =
      CHECK_STATE_NAMES | CHECK_TYPES | CHECK_OVERFLOW | CHECK_UNDERFLOW;
   /** Perform all checks. */
   static const int DEFAULT_FLAGS = CHECK_ALL | CHECK_THROW;
   
public:
   /**
    * Creates a new StateMonitor.
    */
   StateMonitor();

   /**
    * Destructs this StateMonitor.
    */
   virtual ~StateMonitor();

   /**
    * Add states.
    * 
    * Each state info describes a state being monitored.
    * 
    * StateInfo
    * {
    *    "init": DynamicObject
    *    "flags": UInt32 (optional)
    * }
    * 
    * @member init Value to initialize state to on add and reset.
    * @member flags Flags related to this state.
    * 
    * State info DynamicObjects may be shared between states and used for other
    * implementation specific operations. Changing a stateInfo DynamicObject
    * after adding it may result in undefined behavior.
    * 
    * @param stateInfos map of state names to StateInfos
    * 
    * @return true on success, false if an exception occured
    */
   virtual bool addStates(monarch::rt::DynamicObject& stateInfos) = 0;

   /**
    * Add a state. See addStates() documentation.
    * 
    * @param state the state name
    * @param stateInfo information about the state
    * 
    * @return true on success, false if an exception occured
    */
   virtual bool addState(const char* state, monarch::rt::DynamicObject& stateInfo);

   /**
    * Remove all states.
    */
   virtual void removeAll();

   /**
    * Remove states.
    * 
    * @param states map with keys to remove (values are unused)
    * 
    * @return true on success, false if an exception occured
    */
   virtual bool removeStates(monarch::rt::DynamicObject& states) = 0;

   /**
    * Remove a state.
    * 
    * @param state the state name
    * 
    * @return true on success, false if an exception occured
    */
   virtual bool removeState(const char* state);

   /**
    * Reset all states.
    */
   virtual void resetAll() = 0;

   /**
    * Reset states.
    * 
    * @param states map with keys to remove (values are unused)
    * 
    * @return true on success, false if an exception occured
    */
   virtual bool resetStates(monarch::rt::DynamicObject& states) = 0;

   /**
    * Reset a monitored state.
    * 
    * @param state the state name
    * 
    * @return true on success, false if an exception occured
    */
   virtual bool resetState(const char* state);

   /**
    * Set state values specified in a map. The values must be valid according
    * to the associated state infos.
    * 
    * For efficiency reasons a value reference is used rather than a clone.
    * Depending on how the state values are intended to be used the clone
    * option can be used to always create a clone of the values.
    * 
    * @param state the state name
    * @param value the state value
    * @param clone clone the values 
    * 
    * @return true on success, false if an exception occured
    */
   virtual bool setStates(
      monarch::rt::DynamicObject& states, bool clone = false) = 0;

   /**
    * Set a state value. The value must be valid according to the state info.
    * 
    * For efficiency reasons a value reference is used rather than a clone.
    * Depending on how this state value is intended to be used the clone
    * option can be used to always create a clone of the value.
    * 
    * @param state the state name
    * @param value the state value
    * @param clone clone the value
    * 
    * @return true on success, false if an exception occured
    */
   virtual bool setState(
      const char* state, monarch::rt::DynamicObject& value, bool clone = false);

   /**
    * Shortcut to set an UInt64 state value.
    * 
    * @param state the state name
    * 
    * @return true on success, false if an exception occured
    */
   virtual bool setUInt64State(const char* state, uint64_t value);

   /**
    * Get all state values.
    * 
    * A snapshot of the entire monitor state map will be created. This will
    * clone all values to avoid issues with pending or asyncronous value
    * changes.
    * 
    * @return a map with all monitor state values
    */
   virtual monarch::rt::DynamicObject getAll() = 0;

   /**
    * Get state values specified in a map.
    * 
    * All the values for the states specified in the map will be cloned and
    * assigned to the map.
    * 
    * @param states map of state names (set to any value)
    * 
    * @return true on success, false if an exception occured
    */
   virtual bool getStates(monarch::rt::DynamicObject& states) = 0;

   /**
    * Get a state value.
    * 
    * The values for the state will be cloned and assigned to the target.
    * 
    * @param state the state name
    * @param target location to store the state value
    * 
    * @return true on success, false if an exception occured
    */
   virtual bool getState(const char* state, monarch::rt::DynamicObject& target);

   /**
    * Shortcut to get an UInt64 state value.
    * 
    * @param state the state name
    * @param target location to store the state value
    * 
    * @return true on success, false if an exception occured
    */
   virtual bool getUInt64State(const char* state, uint64_t& target);

   /**
    * Adjust states.
    * 
    * State adjustment values must be appropriate types. Numerical adjustments
    * are added to current states. Other adjustments are set as new values.
    * State info specifies how to deal with special conditions such as
    * underflow and overflow.
    * 
    * For efficiency reasons a value reference is used rather than a clone.
    * Depending on how the state values are intended to be used the clone
    * option can be used to always create a clone of the values.
    * 
    * @param adjustments map of states and adjustment values
    * @param clone clone the values
    * 
    * @return true on success, false if an exception occured
    */
   virtual bool adjustStates(
      monarch::rt::DynamicObject& adjustments, bool clone = false) = 0;

   /**
    * Adjust an integer state. This is a shortcut to add a positve or negative
    * integer to an unsigned or signed integer state. Adjustments may be
    * positive or negative. The state info specifies how to handle overflow. 
    * 
    * @param state the state name
    * @param adjustment value to add to the state
    * 
    * @return true on success, false if an exception occured
    */
   virtual bool adjustInt64State(const char* state, int64_t adjustment);

   /**
    * Adjust an integer state. This is a shortcut to add a positve integer to
    * an integer state. The state info specifies how to handle overflow. 
    * 
    * @param state the state name
    * @param adjustment value to add to the state
    * 
    * @return true on success, false if an exception occured
    */
   virtual bool adjustUInt64State(const char* state, uint64_t adjustment);
};

} // end namespace common
} // end namespace bitmunk
#endif
