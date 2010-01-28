#!/usr/bin/env python
#
# Generates documentation in a pseudo-javadoc format for REST APIs.
import sys, os, re
from string import Template
from optparse import OptionParser

##
# Logs a string to the proper location (file or console)
def log(str):
    print str

##
# RestDoc parses an input file and generates API documentation in a
# specified format.
class RestDoc:
    def __init__(self):
        self.api = []

    ##
    # Sets up the option string parser for this script.
    #
    # @param argv the argument list specified on the command line.
    def setupParser(self, argv):
        usage = "usage: %prog [options]"
        parser = OptionParser(usage)
        
        parser.add_option('-o', '--output-dir',
            action='store', dest='outputDir', default='./restdoc-output',
            help='The output directory for the API documentation files.')

        parser.add_option('-f', '--format',
            action='store', dest='format', default=0,
            help='The output format (MediaWiki). [Default: MediaWiki].')

        parser.add_option('-p', '--private-api',
            action='store_true', dest='privateApi', default=False,
            help='Generates private API documentation. [Default: False]')

        parser.add_option('-u', '--upload-documentation',
            action='store_true', dest='uploadDocumentation', default=False,
            help='Uploads the documentation to the Bitmunk wiki. ' + \
                 '[Default: False]')

        self.options, args = parser.parse_args(argv)
        largs = parser.largs

        return (self.options, args, largs)

    ##
    # Retrieves a set of text that is scoped with the given open and closing
    # characters.
    #
    # @param text the text to scan.
    # @param offset the starting offset in the text.
    # @param scopeStartChar the character that starts a scope.
    # @param scopeEndChar the character that ends a scope.
    # @param scopeLevel the starting scope level. The chunk is returned when
    #                   the scope level drops to 0.
    def getScopedText(self, text, offset, scopeStartChar, scopeEndChar, 
        scopeLevel=1):
        rval = ""
        scope = scopeLevel

        # Process the text until the scope balances out
        for c in text[offset:]:
            if(c == scopeStartChar):
                scope += 1
            elif(c == scopeEndChar):
                scope -= 1

            rval += c

            if(scope == 0):
                break

        return rval

    ##
    # Finds the index to the end of a searchString in a given set of text.
    #
    # @param text the set of characters to search.
    # @param searchString the string to search for in the text.
    # @param pos the starting offset into the text.
    #
    # @return -1 if the searchString wasn't found, otherwise return the 
    #         offset to the end of the searchString in the text.
    def findToken(self, text, searchString, pos):
        rval = text.find(searchString, pos)

        if(rval != -1):
            rval = rval + len(searchString)

        return rval

    ##
    # Gets a string value from a given offset into the file.
    #
    # @param text the text to parse for a string value.
    # @param pos the starting position of the search.
    # @param delim the start delimiter to use when retrieving the string.
    # @param delim the end delimiter to use when retrieving the string.
    #
    # @return a tuple containing the updated position in the text and the 
    #         value of the retrieved text string or None if no string was 
    #         retrieved.
    def getStringValue(self, text, pos, sdelim='"', edelim='"'):
        updatedPos = pos
        stringValue = None

        # Find the start and ending '"' character. The string value is 
        # everything inbetween.
        spos = text.find(sdelim, pos)
        if(spos != -1):
            epos = text.find(edelim, spos+1)
            if(epos != -1):
                stringValue = text[spos+1:epos]
                updatedPos = epos

        return (updatedPos, stringValue)

    ##
    # Updates the URL for a given service by processing a C++ file containing
    # the declaration of the base URL.
    #
    # @param service the service object to update based on the given 
    #                 contents of the apiFile.
    # @param apiFile the file to read and extract the leading API URL from.
    def updateUrl(self, service, apiFile):
        try:
            cppFile = open(apiFile, 'r')
            cppText = cppFile.read()
            classPos = self.findToken(cppText, 
                " = new " + service['class'] + "(", 0)
            if(classPos != -1):
                (cPos, value) = self.getStringValue(cppText, classPos)
                if(value != None):
                    service['url'] = value + service['url'] 
            if(service['url'].endswith("/")):
                service['url'] = service['url'][:-1]
        except IOError:
            log("ERROR: A file named '" + apiFile + \
                "' doesn't exist. Failed to update fill URL for: " + \
                str(service))

    ##
    # Processes a service documentation attribute and updates the given
    # service with the value of the service attribute.
    #
    # @param service the service object to use when storing the attribute.
    # @param text the text to parse.
    def parseServiceAttribute(self, service, text):
        if(text.startswith("@pparam")):
            param = {}
            values = text.split()
            param['name'] = values[1]
            param['description'] = " ".join(values[2:])
            service["pathParameters"].append(param)
        elif(text.startswith("@qparam")):
            param = {}
            values = text.split()
            param['name'] = values[1]
            param['description'] = " ".join(values[2:])
            service["queryParameters"].append(param)
        elif(text.startswith("@visibility")):
            service["visibility"] = text.strip().split()[1]
        elif(text.startswith("@return")):
            service["return"] = " ".join(text.strip().split()[1:])
        else:
            service['description'] = text.strip()

    ##
    # Parses the documentation text to extract service features.
    #
    # @param text the documentationt text to use when parsing.
    #
    # @return a dictionary of extracted values.
    def parseServiceDocumentation(self, text):
        rval = {}

        # Setup the skeleton service attributes
        rval["description"] = "This call is currently undocumented."
        rval["visibility"] = "private"
        rval["pathParameters"] = []
        rval["queryParameters"] = []
        rval["return"] = "This return value is currently undocumented."

        # Remove start/end comment block
        text = text.replace("/**", "")
        text = text.replace("*/", "")

        # Remove the leading comment stars and collapse whitespace
        leadingStar = re.compile("^.*\*")
        ntext = ""
        for line in text.split("\n"):
            nline = leadingStar.sub("", line)
            nline = " ".join(nline.split())
            ntext += nline.strip() + "\n"

        # Extract the documentation, path parameters, query parameters,
        # and return value.
        buffer = ""
        for line in ntext.split("\n"):
            if(line.startswith("@qparam")):
                self.parseServiceAttribute(rval, buffer)
                buffer = line
            elif(line.startswith("@pparam")):
                self.parseServiceAttribute(rval, buffer)
                buffer = line
            elif(line.startswith("@visibility")):
                self.parseServiceAttribute(rval, buffer)
                buffer = line
            elif(line.startswith("@return")):
                self.parseServiceAttribute(rval, buffer)
                buffer = line
            else:
                buffer += line
            buffer += " "
        self.parseServiceAttribute(rval, buffer)

        return rval

    ##
    # Gets all of the services with the given partial URL.
    #
    # @param text the text to parse.
    # @param offset the offset at which to begin parsing.
    # @param partialUrl The partial URL for the given service.
    # @param groupName the name of the group to which the set of services
    #        belong.
    # @param className the class name containing the set of services.
    #
    # @return a set of services that were extracted from the text.
    def getAllServices(self, text, position, partialUrl, groupName, className):
        rval = []

        allServicesText = self.getScopedText(text, position, "{", "}")

        #print "ALL SERVICES for", partialUrl, ":", allServicesText

        # break apart each service chunk
        cPos = 0
        astLength = len(allServicesText)
        while(cPos < astLength and cPos != -1):
            # find the next service chunk
            cPos = allServicesText.find("{", cPos)

            # find the previous documentation chunk
            cendPos = allServicesText.rfind("}", 0, cPos)
            if(cendPos == -1):
                cendPos = 0
            dPos = allServicesText.rfind("/**", cendPos, cPos)

            # if a documentation chunk was found, retrieve it
            documentationText = ""
            if(dPos != -1):
                documentationText = allServicesText[dPos:cPos]

            # if a service chunk was found, process it
            if(cPos != -1):
                serviceText = self.getScopedText( \
                    allServicesText, cPos+1, "{", "}")
                #print "   SERVICE CHUNK:", serviceText
                cPos += len(serviceText)

                # Extract the service info out of the documentation text, if
                # it exists.
                service = self.parseServiceDocumentation(documentationText)

                if(len(service["description"].strip()) == 0):
                    service["description"] = \
                        "This service is currently undocumented."
                    log("WARNING: " + className + " " + partialUrl + \
                       " is undocumented.")

                # Get the HTTP method for the service
                method = "UNKNOWN"
                mPos = self.findToken(serviceText, "BtpMessage:", 0)
                if(mPos != -1):
                    (tmp, method) = \
                        self.getStringValue(serviceText, mPos, ":", ",")
                    if(method != None):
                        method = method.upper()

                # Get the required authentication, if any
                authentication = "None"
                aPos = self.findToken(serviceText, "BtpAction::Aut", 0)
                if(aPos != -1):
                    (tmp, authentication) = \
                        self.getStringValue(serviceText, aPos, "h", ")")

                # Append the newly discovered values to the service
                service["url"] = partialUrl
                service["method"] = method
                service["authentication"] = authentication
                service["class"] = className
                service["group"] = groupName
                service["processed"] = False

                rval.append(service)

        return rval

    ##
    # Process a file that contains RESTdoc documentation
    #
    # @param file the file that contains the documentation.
    def processFile(self, file):
        services = []
        # Extract all of the service URIs out of the file
        try:
            cppFile = open(file, 'r')
            cppText = cppFile.read()
            cPos = 0
            cppTextLen = len(cppText)
            # Search for all services in the file
            while(cPos < cppTextLen and cPos != -1):
                # Get the partial URL for a resource
                pos = self.findToken(cppText, "addResource", cPos)
                if(pos != -1):
                    (cPos, partialUrl) = self.getStringValue(cppText, pos)
                    if(partialUrl != None):
                        #log("Adding service: " + value)
                        filename = os.path.basename(file)
                        groupName = filename.replace("Service.cpp", "").lower()
                        className = filename.replace(".cpp", "")

                        # Get all of the web service HTTP methods
                        allServices = self.getAllServices(cppText, cPos,
                            partialUrl, groupName, className)
                        services += allServices
                    else:
                        log("WARNING: Invalid partial URL near " + \
                            cppText[pos:pos+10])
                else:
                    cPos = pos
        except IOError:
            log("ERROR: A file named '" + file + "' doesn't exist, skipping.")

        # Find the full URL for each service
        dirname = os.path.dirname(file)
        dirlist = os.listdir(dirname)
        fullurl = "unknown/"
        moduleFile = ""
        for mfile in dirlist:
            if(mfile.endswith("Module.cpp")):
                moduleFile = os.path.join(dirname, mfile)

        # Update the full path to the URL
        for service in services:
            self.updateUrl(service, moduleFile)
            self.api.append(service)

        #print services

    ##
    # Generates a single service entry in wiki format.
    #
    # @param service the service definition.
    # 
    # @return the formatted wiki text for the service entry.
    def generateWikiServiceEntry(self, service):
        rval = ""

        # Generate the method documentation
        rval += "== " + service['method'] + " " + service['url']

        # Generate the trailing path parameters
        for pp in service['pathParameters']:
            rval += "/<" + pp['name'] + ">"
        # Remove any double-slashes
        rval = rval.replace("//", "/")

        # Generate the trailing query parameters
        if(len(service['queryParameters']) > 0):
            rval += "?"
            firstRun = True
            for qp in service['queryParameters']:
                if(firstRun):
                    rval += qp['name'] + "=" + "XYZ"
                    firstRun = False
                else:
                    rval += "&" + qp['name'] + "=" + "XYZ"
                    
        rval += " ==\n\n"

        # Generate the description string
        rval += service['description'] + "\n\n"

        # Generate the authentication string
        rval += "<i>Authentication</i>: " + \
            service['authentication'] + "<br />\n"

        # Generate the return value string
        rval += "<i>Returns</i>: " + service['return'] + "\n\n"

        # Generate the path parameters
        if(len(service['pathParameters']) > 0):
            rval += "=== Path Parameters ===\n"
            for pp in service['pathParameters']:
                rval += "* <b>" + pp['name'] + "</b> - " + \
                    pp['description'] + "\n"
            rval += "\n"

        # Generate the query parameters
        if(len(service['queryParameters']) > 0):
            rval += "=== Query Parameters ===\n"
            for qp in service['queryParameters']:
                rval += "* <b>" + qp['name'] + "</b> - " + \
                    qp['description'] + "\n"
            rval += "\n"

        return rval

    ##
    # Generates the wiki text for a given URL.
    #
    # @param url the URL to use when generating the wiki text.
    #
    # @return a text string containing all of the wiki markup for a given
    #         URL.
    def generateWikiText(self, url):
        rval =  "= Bitmunk WebServices API Documentation =\n\n"
        rval += "This page documents the Bitmunk " + url + " web service.\n\n"
        httpMethods = ("GET", "POST", "PUT", "DELETE")

        # Process all HTTP methods that match the given URL and mark them as
        # completed
        for method in httpMethods:
            for service in self.api:
                if(service['method'] == method and service['url'] == url and \
                   service['processed'] != True):
                    rval += self.generateWikiServiceEntry(service)
                    service['processed'] = True

        return rval

    ##
    # Outputs the given service to the output file in MediaWiki format.
    #
    # @param service The REST API documentation service to output to the
    #                 given file.
    # @param outputFile The output filename to use when writing the output
    #                   media wiki file.
    def wikifyService(self, service, outputFilename):
        try:
            wfile = open(outputFilename, 'w')
            wfile.write(self.generateWikiText(service['url']))
            wfile.close()
        except IOError:
            log("ERROR: Failed to write REST API to disk: " + \
                    str(service))

    ##
    # Generates the Table of Contents in wiki format.
    #
    # @return the formatted wiki text for the Table of Contents.
    def generateWikiToc(self):
        # Create the header documentation
        rval = "= Bitmunk REST API Calls =\n\n"
        rval += "This section contains all of the developer documentation\n"
        rval += "for interacting with the Bitmunk network via a standard\n"
        rval += "set of REST-based API calls. Programs written in Javascript,\n"
        rval += "Python, Ruby, C, C++, C#, Perl, and a number of other\n"
        rval += "programming languages can be written to interact with\n"
        rval += "Bitmunk via the following Web Services.\n\n"

        # Collect all of the groups that should be documented
        groupMap = {}
        for service in self.api:
            if(not groupMap.has_key(service["group"])):
                groupMap[service["group"]] = []
            # Only include the documentation in the TOC if the service
            # visibility is public
            if(service["visibility"] == "public"):
                groupMap[service["group"]].append(service["url"],)

        # Alphabetize the API calls
        groups = groupMap.keys()
        groups.sort()
        for group in groups:
            urls = groupMap[group]
            uniqueUrls = {}
            for url in urls:
                uniqueUrls[url] = True
            groupMap[group] = uniqueUrls.keys()
            groupMap[group].sort()            

        # Format the TOC and include any group with at least 1 URL
        for group in groups:
            if(len(groupMap[group]) > 0):
                rval += "== %s ==\n\n" % (group,) 
                for url in groupMap[group]:
                    rval += "* [[%s|%s]]\n" % (url[1:].replace("/", "-"), url)
                rval += "\n"

        return rval

    ##
    # Outputs a table of contents for the REST API services in MediaWiki format.
    #
    # @param tocName the name of the table of contents on the wiki
    def wikifyServiceToc(self, tocName):
        try:
            wfile = open(tocName, 'w')
            wfile.write(self.generateWikiToc())
            wfile.close()
        except IOError:
            log("ERROR: Failed to write REST API Table of Contents to disk: "+\
                    str(service))

    ##
    # Uploads the given documentation file to the given site.
    #
    # @param docFile the path to the documentation file to upload.
    # @param site the site to upload the documentation to.
    def uploadToWiki(self, docFile, site):
        # You must have PyWikipedia installed to upload content
        sys.path += ("../pywikipedia",)
        import wikipedia

        # Attempt to open the documentation file
        try:
            rfile = open(docFile, 'r')
            docText = rfile.read()
            rfile.close()
            wikiPath = docFile.split("/")[-1]
            page = wikipedia.Page(site, wikiPath)

            # Handle the rest-api URL differently than the other API URLs
            if(wikiPath != "rest-api"):
                # If the REST api page URL doesn't exist,
                # create it
                if(not page.exists()):
                    newText = docText + "= Notes =\n"
                    #print "CREATING PAGE:", wikiPath, "\n", docText
                    page.put(newText, "Initial page edit", botflag = False)
                else:
                    # Upload the top half of the REST api page,
                    # preserving the Notes section
                    currentText = page.get()
                    croppedText = "= Notes ="

                    # The notes offset is where the API-specific notes
                    # documentation starts.
                    notesOffset = currentText.find("= Notes =")
                    if(notesOffset != -1):
                        croppedText = currentText[notesOffset:]
                    newText = (docText + "\n\n" + croppedText.strip()).strip()
                    #print wikiPath, "\n", newText
                    page.put(newText, "Updating REST API documentation",
                        botflag = False)
            else:
                # Only perform updates to the latter part of
                # the rest-api wiki page
                currentText = page.get()
                croppedText = currentText[: \
                    currentText.find("= Bitmunk REST API Calls =")]
                newText = (croppedText + docText).strip()
                #print wikiPath, "\n", newText
                page.put(newText, "Updating REST API documentation",
                    botflag = False)
                
            log("INFO: Uploaded " + wikiPath + " ...")
        except IOError:
            log("ERROR: Failed to open " + str(docname) + ".")

    ##
    # Runs the RestDoc on the set of files passed in on the command line.
    def run(self, argv, stdout, environ):
        (self.options, args, largs) = self.setupParser(argv)

        # Process the input files
        for file in largs[1:]:
            log("INFO: Processing " + file)
            self.processFile(file)

        # Create the documentation directory
        if(not os.path.exists(self.options.outputDir)):
            try:
                os.makedirs(self.options.outputDir)
            except OSError:
                log("WARNING: Output directory failed to be created.")
                sys.exit(1)

        # Generate all of the documentation files.
        documentation = []
        for service in self.api:
            if(service["processed"] == False):
                if(self.options.privateApi or \
                   service["visibility"] == "public"):
                    # Format the service URL in a form that is friendly 
                    # to MediaWiki
                    rfname = service['url']
                    if(rfname.startswith('/')):
                        rfname = rfname[1:]
                    if(rfname.endswith('/')):
                        rfname = rfname[:-1]
                    rfname = rfname.replace('/', '-')
                    service["wikiPath"] = rfname
                    rfname = os.path.join(self.options.outputDir, rfname)

                    # Convert the service into wiki text
                    self.wikifyService(service, rfname)
                    documentation.append(rfname)

        # Generate the table of contents
        tocName = os.path.join(self.options.outputDir, "rest-api")
        self.wikifyServiceToc(tocName)
        documentation.append(tocName)

        # Note that a particular file has been processed
        log("INFO: Processed " + str(len(self.api)) + " web services.")

        # Upload the documentation to the bitmunk wiki
        if(self.options.uploadDocumentation):
            # You must have PyWikipedia installed to upload content
            sys.path += ("../pywikipedia",)
            import wikipedia

            site = wikipedia.getSite()

            # Upload each piece of documentation 
            for docFile in documentation:
                self.uploadToWiki(docFile, site)

# Call main function if run from command line.
if __name__ == "__main__":
    restdoc = RestDoc()
    restdoc.run(sys.argv, sys.stdout, os.environ)
