---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2016 Uli Schlachter
-- @release @AWESOME_VERSION@
-- @classmod gears.screen
---------------------------------------------------------------------------

local ascreen =  require("awful.screen")
local util = require("awful.util")

local module = {}

--- Call a function for each existing and created-in-the-future screen.
-- @tparam function func The function to call.
function module.connect_for_each_screen(func)
    util.deprecate("Use awful.screen.connect_for_each_screen")
    ascreen.connect_for_each_screen(func)
end

--- Undo the effect of connect_for_each_screen.
-- @tparam function func The function that should no longer be called.
function module.disconnect_for_each_screen(func)
    util.deprecate("Use awful.screen.disconnect_for_each_screen")
    ascreen.disconnect_for_each_screen(func)
end

return module

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
