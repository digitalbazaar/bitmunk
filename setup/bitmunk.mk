# Define the XPT and XPI directories used for Firefox extensions
XPT_DIR = $(DIST_DIR)/xpt
XPI_DIR = $(DIST_DIR)/xpi

# Include the component libraries in the list of libraries
LIBRARIES += $(patsubst %, $(LIB_DIR)/$(LIB_PREFIX)%.$(DYNAMIC_LIB_EXT), $(COMPONENT_LIBRARIES))

# Add generated headers and XPT files to the list of known files
ALL_GENERATED_HEADERS := $(patsubst %.idl, $(BUILD_DIR)/%.h, $(IDL_SOURCES))
ALL_XPTS := $(patsubst %.idl, $(XPT_DIR)/%.xpt, $(IDL_SOURCES))
ALL_DIRECTORIES += $(XPT_DIR) $(XPI_DIR)

# Build rules to build header and xpt files from IDL files
$(BUILD_DIR)/%.h: %.idl
	@echo "Building $@..."
	$(PCMD) $(XPIDL) -m header $(XULRUNNER_IDL_FLAGS) -o $(basename $@) $<

$(XPT_DIR)/%.xpt: %.idl
	@echo "Generating dist/$(subst .P,.o,$(subst $(BITMUNK)/dist/,,$@))..."
	$(PCMD) $(XPIDL) -m typelib $(XULRUNNER_IDL_FLAGS) -o $(basename $@) $<

# Override the default platform build rule with one that includes all
# generated headers
%-$(PLATFORM).o %-$(PLATFORM).P: %.cpp $(ALL_GENERATED_HEADERS)
	@echo "Compiling build/$(subst .P,.o,$(subst $(TOP_BUILD_DIR)/build/,,$@))..."
	$(PCMD) $(CXX) $(CXX_FLAGS) -c -MD -o $(basename $@).o $(INCLUDES) $(realpath $<)
	$(PCMD) cp $(basename $@).d $(basename $@).P; \
		sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' -e '/^$$/ d' -e 's/$$/ :/' < $(basename $@).d >> $(basename $@).P; \
		rm -f $(basename $@).d

