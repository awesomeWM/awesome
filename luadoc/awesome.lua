--- awesome core API
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008-2009 Julien Danjou
module("awesome")

--- awesome global table.
-- @field version The version of awesome.
-- @field release The release name of awesome.
-- @field conffile The configuration file which has been loaded.
-- @field startup_errors Error message for errors that occured during startup.
-- @field composite_manager_running True if a composite manager is running.
-- @class table
-- @name awesome

--- Quit awesome.
-- @name quit
-- @class function

--- Execute another application, probably a window manager, to replace
-- awesome.
-- @param cmd The command line to execute.
-- @name exec
-- @class function

--- Restart awesome.
-- @name restart
-- @class function

--- Spawn a program.
-- @param cmd The command to launch. Either a string or a table of strings.
-- @param use_sn Use startup-notification, true or false, default to true.
-- @return Process ID if everything is OK, or an error string if an error occured.

--- Load an image
-- @param name The file name
-- @return A cairo image surface as light user datum
-- @name load_image
-- @class function

--- Register a new xproperty.
-- @param name The name of the X11 property
-- @param type One of "string", "number" or "boolean"
-- @name register_xproperty
-- @class function

--- Add a global signal.
-- @param name A string with the event name.
-- @param func The function to call.
-- @name connect_signal
-- @class function

--- Remove a global signal.
-- @param name A string with the event name.
-- @param func The function to call.
-- @name disconnect_signal
-- @class function

--- Emit a global signal.
-- @param name A string with the event name.
-- @param ... Signal arguments.
-- @name emit_signal
-- @class function
