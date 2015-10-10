---------------------------------------------------------------------------
--- AWesome Functions very UsefuL
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008 Julien Danjou
-- @release @AWESOME_VERSION@
-- @module awful
---------------------------------------------------------------------------

-- TODO: This is a hack for backwards-compatibility with 3.5, remove!
local util = require("awful.util")
local gtimer = require("gears.timer")
function timer(...)
    util.deprecate("gears.timer")
    return gtimer(...)
end

--TODO: This is a hack for backwards-compatibility with 3.5, remove!
-- Set awful.util.spawn* and awful.util.pread.
local spawn = require("awful.spawn")

util.spawn = function(...)
   util.deprecate("awful.spawn")
   return spawn.spawn(...)
end

util.spawn_with_shell = function(...)
   util.deprecate("awful.spawn.with_shell")
   return spawn.with_shell(...)
end

util.pread = function(...)
   util.deprecate("awful.spawn.pread")
   return spawn.pread(...)
end

return
{
    client = require("awful.client");
    completion = require("awful.completion");
    layout = require("awful.layout");
    placement = require("awful.placement");
    prompt = require("awful.prompt");
    screen = require("awful.screen");
    tag = require("awful.tag");
    util = require("awful.util");
    widget = require("awful.widget");
    keygrabber = require("awful.keygrabber");
    menu = require("awful.menu");
    mouse = require("awful.mouse");
    remote = require("awful.remote");
    key = require("awful.key");
    button = require("awful.button");
    wibox = require("awful.wibox");
    startup_notification = require("awful.startup_notification");
    tooltip = require("awful.tooltip");
    ewmh = require("awful.ewmh");
    titlebar = require("awful.titlebar");
    spawn = spawn;
}

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
