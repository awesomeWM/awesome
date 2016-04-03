---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2016 Uli Schlachter
-- @release @AWESOME_VERSION@
-- @classmod gears.screen
---------------------------------------------------------------------------

local screen = screen

local module = {}

--- Call a function for each existing and created-in-the-future screen.
-- @tparam function func The function to call.
function module.connect_for_each_screen(func)
    for s in screen do
        func(s)
    end
    screen.connect_signal("added", func)
end

--- Undo the effect of connect_for_each_screen.
-- @tparam function func The function that should no longer be called.
function module.disconnect_for_each_screen(func)
    screen.disconnect_signal("added", func)
end

return module

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
