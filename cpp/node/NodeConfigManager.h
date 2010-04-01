/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef bitmunk_node_NodeConfigManager_H
#define bitmunk_node_NodeConfigManager_H

#include "bitmunk/common/TypeDefinitions.h"
#include "monarch/config/ConfigManager.h"
#include "monarch/kernel/MicroKernel.h"

#include <string>

/**
 * Current Bitmunk config version.
 */
#define BITMUNK_CONFIG_VERSION_3_0 "Bitmunk 3.0"
#define BITMUNK_CONFIG_VERSION BITMUNK_CONFIG_VERSION_3_0

namespace bitmunk
{
namespace node
{

/**
 * The Bitmunk NodeConfigManager wraps a Monarch ConfigManager with support
 * for a hierarchy of configs that can be used for packaging, system, system
 * user, command line, and bitmunk user customization.
 *
 * The pre-defined group ids are defined in hiararchical order below. The
 * hierarchy is defined in the config manager itself. Configs need only select
 * an appropriate group and a unique id.
 *
 * The "before" and "after" groups can be used as points to insert
 * configuration overrides when the raw primary group config may be written out
 * to a file. For instance, a bitmunk wrapper script may specify a development
 * environment config to use. This can be inserted at the "before system user"
 * level so that the main "system user" config file will override these
 * settings and be saved properly without all the extra development
 * customizations.
 *
 * "root":
 *    Generally empty config root.
 *
 * "boot":
 *    Hard-coded config needed before other configs are loaded.
 *
 * "before defaults",
 * "defaults",
 * "after defaults":
 *    Default configuration for the application and modules.
 *
 * "before system",
 * "system",
 * "after system":
 *    System level customization. Used for configs for all users on a computer
 *    or network.
 *
 * "before system user",
 * "system user",
 * "after system user":
 *    System user level customization. Used for configs for the system user
 *    that starts the bitmunk application.
 *
 * "command line":
 *    Adds command line config options.
 *
 * "main":
 *    The primary config for the node. This config is intended to be a
 *    read-only config used as a merge point of all previous configs.
 *
 * "before users",
 * "users",
 * "after users":
 *    Adds customization common to all users. Can be used to mask config values
 *    from showing up in "user #" configs. Note that this is soft security
 *    since users configs could be changed to have any other parent.
 *
 * "user #" (id):
 *    Customization for a specific Bitmunk user id.
 *
 * @author David I. Lehn
 */
class NodeConfigManager
{
protected:
   /**
    * MicroKernel to use to get the ConfigManager;
    */
   monarch::kernel::MicroKernel* mMicroKernel;

public:
   /**
    * Creates a new NodeConfigManager.
    */
   NodeConfigManager();

   /**
    * Destructs this NodeConfigManager.
    */
   virtual ~NodeConfigManager();

   /**
    * Set the MicroKernel used to get the ConfigManager.
    *
    * @param k the MicroKernel.
    */
   virtual void setMicroKernel(monarch::kernel::MicroKernel* k);

   /**
    * Get the MicroKernel.
    *
    * @return the MicroKernel.
    */
   virtual monarch::kernel::MicroKernel* getMicroKernel();

   /**
    * Get the ConfigManager from the MicroKernel.
    *
    * @return the ConfigManager.
    */
   virtual monarch::config::ConfigManager* getConfigManager();

   /**
    * Gets the "node" configuration.
    *
    * @param raw true to get the raw config, false to get the config as merged
    *            from main with all up-tree parents.
    *
    * @return the configuration of all overlayed configurations.
    */
   virtual monarch::config::Config getNodeConfig(bool raw = false);

   /**
    * Saves the system user config.
    *
    * FIXME: bitmunk app config system and NodeConfigManager need to be
    * FIXME: refactored. See code for more details.
    *
    * @return true on success, false on failure and exception will be set.
    */
   virtual bool saveSystemUserConfig();

   /**
    * Gets a specifc module configuration.
    *
    * @param moduleName the module name
    * @param raw true to get the raw config, false to get the config as merged
    *            from main with all up-tree parents.
    *
    * @return the configuration of all overlayed configurations.
    */
   virtual monarch::config::Config getModuleConfig(
      const char* moduleName, bool raw = false);

   /**
    * Loads the config for a Bitmunk user and adds it to this NodeConfigManager.
    * Config to be loaded will be determined based on userId, node.userDir
    * setting, and node.bitmunkHomePath setting.
    *
    * @param userId the user id.
    *
    * @return true on success, false on failure and exception will be set.
    */
   virtual bool loadUserConfig(bitmunk::common::UserId userId);

   /**
    * Saves the config for a Bitmunk user.  The save file will be determined
    * based on userId, node.userDir setting, and node.bitmunkHomePath setting.
    *
    * @param userId the user id.
    *
    * @return true on success, false on failure and exception will be set.
    */
   virtual bool saveUserConfig(bitmunk::common::UserId userId);

   /**
    * Gets the configuration for a bitmunk user.
    *
    * @param userId the user id.
    * @param raw true to get the raw config, false to get the config as merged
    *            with all up-tree parents.
    *
    * @return the configuration of all overlayed configurations.
    */
   virtual monarch::config::Config getUserConfig(
      bitmunk::common::UserId userId, bool raw = false);

   /**
    * Removes a bitmunk user configuration.
    *
    * @param userId the user id.
    *
    * @return true on success, false on failure and exception will be set.
    */
   virtual bool removeUserConfig(bitmunk::common::UserId userId);

   /**
    * Check if a config has the format of a user config. This checks if the
    * name is the right format and if the parent is correct.
    *
    * @param config the config to check.
    * @param userId the user id.
    *
    * @return true if a user config, false if not.
    */
   static bool isUserConfig(
      monarch::config::Config& config, bitmunk::common::UserId userId);

   /**
    * Check if a config has the format of a system user config. This checks if
    * the group name is correct.
    *
    * @param config the config to check.
    * @param userId the user id.
    *
    * @return true if a user config, false if not.
    */
   static bool isSystemUserConfig(monarch::config::Config& config);

   /**
    * Gets the configuration for a specific module for a specific user
    *
    * @param moduleName the module name
    * @param userId the user id.
    *
    * @return the configuration of all overlayed configurations.
    */
   virtual monarch::config::Config getModuleUserConfig(
      const char* moduleName, bitmunk::common::UserId userId);

   /**
    * Get the path used for bitmunk data.  This path is taken from the
    * node.bitmunkHomePath config value.  The path is expanded as much as
    * possible according to platform conventions and must eventually be
    * absolute or an exception will occur.
    *
    * @param userId the user id
    * @param path the output path to assign
    *
    * @return true on success, false if an exception occured.
    */
   virtual bool getBitmunkHomePath(std::string& path);

   /**
    * Expand a generic data path into a full path.  If the path is not absolute
    * then it is expanded according to platform conventions.  If the path is
    * still not absolute then it is assumed to be relative to the result of
    * getBitmunkHomePath().
    *
    * @param path the path to expand
    * @param expandedPath the expanded path will be placed into this
    *        string.
    *
    * @return true if the expansion was successful, false if an exception
    *         occurred.
    */
   virtual bool expandBitmunkHomePath(
      const char* path, std::string& expandedPath);

   /**
    * Get the path used for user data.  This path is named as the user id and
    * rooted at the node.usersPath config value.  If node.usersPath is not
    * absolute then it is relative to the value returned from
    * getBitmunkHomePath().
    *
    * @param userId the user id
    * @param path the output path to assign
    *
    * @return true on success, false if an exception occured.
    */
   virtual bool getUserDataPath(
      bitmunk::common::UserId userId, std::string& path);

   /**
    * Expand a user data path into a full path.  If the path is not absolute
    * then it is expanded according to platform conventions.  If the path is
    * still not absolute then it is assumed to be relative to the result of
    * getUserDataPath().
    *
    * @param path the path to expand
    * @param userId user id for this request
    * @param expandedPath the expanded path will be placed into this
    *        string.
    * @param url true if the path is for a url, false if not.
    *
    * @return true if the expansion was successful, false if an exception
    *         occurred.
    */
   virtual bool expandUserDataPath(
      const char* path, bitmunk::common::UserId userId,
      std::string& expandedPath);

   /**
    * Expands a given input URL to a user file path if the input scheme type can
    * be mapped to a file path. This currently only works for sqlite:// schemes.
    * FIXME: Implement file:// scheme handling
    *
    * @param userId the user id associated with the directory that should
    *               be expanded.
    * @param url the URL to translate to a file path in the user's directory.
    *
    * @return true if the translation was successful, false otherwise.
    */
   virtual bool translateUrlToUserFilePath(
      bitmunk::common::UserId userId, const char* url, std::string& filePath);
};

} // end namespace node
} // end namespace bitmunk
#endif
