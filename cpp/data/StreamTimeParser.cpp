/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "bitmunk/data/StreamTimeParser.h"

using namespace std;
using namespace bitmunk::data;

StreamTimeParser::StreamTimeParser(double start, double end)
{
   setCurrentTime(0);
   addTimeSet(start, end);
}

StreamTimeParser::~StreamTimeParser()
{
}

bool StreamTimeParser::addTimeSet(double start, double end)
{
   bool rval = false;
   
   if(end > start && end >= 0 && start >= 0)
   {
      // make sure time set is not a duplicate
      bool duplicate = false;
      for(TimeSetList::iterator i = mTimeSets.begin();
          !duplicate && i != mTimeSets.end(); ++i)
      {
         duplicate = (i->first == start && i->second == end);
      }
      
      if(!duplicate)
      {
         mTimeSets.push_back(make_pair(start, end));
         rval = true;
      }
   }
   
   return rval;
}

void StreamTimeParser::removeTimeSet(double start, double end)
{
   // create a list of new time sets to add
   TimeSetList newTimeSets;
   
   for(TimeSetList::iterator i = mTimeSets.begin(); i != mTimeSets.end();)
   {
      // if start begins a time set
      if(start == i->first)
      {
         // if end is greater than or equal to this set's end, remove it
         if(end >= i->second)
         {
            i = mTimeSets.erase(i);
         }
         else
         {
            // update time set so that it starts at the end (remove
            // the beginning of the time set)
            i->first = end;
            ++i;
         }
      }
      else if(start > i->first)
      {
         // start is in middle of time set, set a new end (and split
         // the time set into two parts by first storing the old end)
         double oldEnd = i->second;
         i->second = start;
         
         // see where the old end is
         if(end < oldEnd)
         {
            // add a new time set
            newTimeSets.push_back(make_pair(end, oldEnd));
         }
         
         // increment iterator
         ++i;
      }
      else if(end > i->first)
      {
         // if time set to remove encapsulates the current one
         if(end >= i->second)
         {
            // remove set entirely
            i = mTimeSets.erase(i);
         }
         else
         {
            // set a new start time to the end
            i->first = end;
            ++i;
         }
      }
      else
      {
         // no overlap
         ++i;
      }
   }
   
   // add in the new time sets and merge all time sets
   for(TimeSetList::iterator i = newTimeSets.begin();
       i != newTimeSets.end(); ++i)
   {
      mTimeSets.push_back(make_pair(i->first, i->second));
   }
}

bool StreamTimeParser::isTimeValid(double time)
{
   bool rval = false;
   
   for(TimeSetList::iterator i = mTimeSets.begin();
       !rval && i != mTimeSets.end(); ++i)
   {
      // see if data is in time interval
      rval = (time >= i->first && time <= i->second);
   }
   
   return rval;
}

void StreamTimeParser::setCurrentTime(double time)
{
   mTime = time;
}

double StreamTimeParser::getCurrentTime()
{
   return mTime;
}
