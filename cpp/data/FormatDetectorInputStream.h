/*
 * Copyright (c) 2007-2008 Digital Bazaar, Inc.  All rights reserved.
 */
#ifndef bitmunk_data_FormatDetectorInputStream_H
#define bitmunk_data_FormatDetectorInputStream_H

#include <vector>

#include "monarch/data/InspectorInputStream.h"
#include "monarch/data/DataFormatInspector.h"
#include "bitmunk/common/TypeDefinitions.h"

namespace bitmunk
{
namespace data
{

/**
 * A FormatDetectorInputStream wraps an existing InspectorInputStream to try to
 * determine the format of the data flowing through that stream.
 *
 * @author Dave Longley
 * @author David I. Lehn
 */
class FormatDetectorInputStream : public monarch::io::FilterInputStream
{
protected:
   /**
    * The format inspectors.
    */
   typedef std::vector<monarch::data::DataFormatInspector*> InspectorList;
   InspectorList mInspectors;

   /**
    * Set to true once at least one data format inspector has recognized the
    * data format.
    */
   bool mDataFormatRecognized;

   /**
    * Creates the inspectors for this stream.
    */
   virtual void createInspectors();

public:
   /**
    * Creates a new FormatDetectorInputStream that tries to determine the
    * format of the data flowing through the passed InspectorInputStream.
    *
    * @param iis the InspectorInputStream to read from.
    * @param cleanup true to clean up the passed InputStream when destructing,
    *                false not to.
    */
   FormatDetectorInputStream(
      monarch::data::InspectorInputStream* iis, bool cleanup = false);

   /**
    * Destructs this FormatDetectorInputStream.
    */
   virtual ~FormatDetectorInputStream();

   /**
    * Scans the entire input stream, calling read() on it until it returns
    * 0 or -1.
    *
    * @param total if not NULL, will return the total number of bytes read.
    *
    * @return true if the entire stream was read, false if an exception
    *         occurred.
    */
   virtual bool detect(uint64_t* total = NULL);

   /**
    * Adds a DataFormatInspector to this stream. The passed inspector will
    * be stored according to its class name.
    *
    * If "cleanup" is true and the inspector is not added, the memory for
    * the inspector will not be freed by the call and it will be up to
    * the caller to handle the memory.
    *
    * @param name the name for the inspector.
    * @param dfi the DataFormatInspector to add.
    * @param cleanup true if this stream should manage the memory for
    *                the inspector, false if not.
    *
    * @return true if the inspector was added, false if not.
    */
   virtual bool addDataFormatInspector(
      const char* name, monarch::data::DataFormatInspector* dfi, bool cleanup);

   /**
    * Gets a DataFormatInspector by the given name.
    *
    * @param name the name for the DataFormatInspector to retrieve.
    *
    * @return the DataFormatInspector by the given name or NULL if
    *         none exists.
    */
   virtual monarch::data::DataFormatInspector* getDataFormatInspector(
      const char* name);

   /**
    * Gets all data format inspectors.
    *
    * @return all data format inspectors.
    */
   virtual std::vector<monarch::data::DataFormatInspector*>* getAllInspectors();

   /**
    * Gets all data format inspectors that recognized the stream.
    *
    * @return all data format inspectors that recognized the stream.
    */
   virtual std::vector<monarch::data::DataFormatInspector*> getInspectors();

   /**
    * Returns whether or not the data format has been recognized.
    *
    * @return true if the data format has been recognized, false if not.
    */
   virtual bool isFormatRecognized();

   /**
    * Gets an array of the format details from every inspector that recognized
    * the format, starting with the most likely detected stream format.
    *
    * @return an array of format details.
    */
   virtual monarch::rt::DynamicObject getFormatDetails();
};

} // end namespace data
} // end namespace bitmunk
#endif
