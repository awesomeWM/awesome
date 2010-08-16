--- awesome core API
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008-2009 Julien Danjou
module("awesome")

--- awesome global table.
-- @field font The default font.
-- @field font_height The default font height.
-- @field fg The default foreground color.
-- @field bg The default background color.
-- @field version The version of awesome.
-- #field release The release name of awesome.
-- @field conffile The configuration file which has been loaded.
-- @class table
-- @name awesome

--- Quit awesome.
-- @param -
-- @name quit
-- @class function

--- Execute another application, probably a window manager, to replace
-- awesome.
-- @param cmd The command line to execute.
-- @name exec
-- @class function

--- Restart awesome.
-- @param -
-- @name restart
-- @class function

--- Spawn a program.
-- @param cmd The command to launch.
-- @param use_sn Use startup-notification, true or false, default to true.
-- @return Process ID if everything is OK, or an error string if an error occured.

--- Add a global signal.
-- @param name A string with the event name.
-- @param func The function to call.
-- @name add_signal
-- @class function

--- Remove a global signal.
-- @param name A string with the event name.
-- @param func The function to call.
-- @name remove_signal
-- @class function

--- Emit a global signal.
-- @param name A string with the event name.
-- @param ... Signal arguments.
-- @name emit_signal
-- @class function
