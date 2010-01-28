/*
 * Copyright (c) 2007-2008 Digital Bazaar, Inc.  All rights reserved.
 */
#ifndef bitmunk_data_StreamTimeParser_H
#define bitmunk_data_StreamTimeParser_H

#include "monarch/io/MutationAlgorithm.h"

// utility includes "pair" data structure
#include <utility>
#include <list>

namespace bitmunk
{
namespace data
{

/**
 * A StreamTimeParser is used to parse out sections of an data stream based on
 * time sets.
 * 
 * @author Dave Longley
 */
class StreamTimeParser : public monarch::io::MutationAlgorithm
{
protected:
   /**
    * The time sets to parse out of the stream. A time set includes
    * a beginning time and an ending time stored as a pair.
    */
   typedef std::pair<double, double> TimeSet;
   typedef std::list<TimeSet> TimeSetList;
   TimeSetList mTimeSets;
   
   /**
    * The current time in the stream so far.
    */
   double mTime;
   
public:
   /**
    * Creates a new StreamTimeParser that defaults to parsing no data out
    * of the stream it processes unless start and end are set. If end is less
    * than start, the time set will not be added.
    * 
    * @param start the start time in seconds.
    * @param end the end time in seconds.
    */
   StreamTimeParser(double start = 0, double end = -1);
   
   /**
    * Destructs this StreamTimeParser.
    */
   virtual ~StreamTimeParser();
   
   /**
    * Adds a time set to parse out of the stream. If end is less than
    * start, or time set is a duplicate, it will not be added. If the passed
    * time set intersects another the two will be merged.
    * 
    * @param start the start time in seconds.
    * @param end the end time in seconds.
    * 
    * @return true if time set was added, false if not.
    */
   virtual bool addTimeSet(double start, double end);
   
   /**
    * Removes a time set.
    * 
    * @param start the start time in seconds to remove.
    * @param end the end time in seconds to remove.
    */
   virtual void removeTimeSet(double start, double end);
   
   /**
    * Returns true if the passed time is within a valid time set,
    * false if not.
    * 
    * @param time the time to inspect.
    * 
    * @return true if the time is in a valid time set, false if not.
    */
   virtual bool isTimeValid(double time);
   
   /**
    * Sets the current time in the stream (in seconds).
    * 
    * @param time the current time in the stream (in seconds).
    */
   virtual void setCurrentTime(double time);
   
   /**
    * Gets the current time in the stream.
    * 
    * @return the current time in the stream.
    */
   virtual double getCurrentTime();
};

} // end namespace data
} // end namespace bitmunk
#endif
