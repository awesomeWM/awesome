---------------------------------------------------------------------------
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Julien Danjou
-- @release @AWESOME_VERSION@
---------------------------------------------------------------------------

-- Grab environment we need
local setmetatable = setmetatable
local ipairs = ipairs
local capi = { button = button }
local util = require("awful.util")

--- Create easily new buttons objects ignoring certain modifiers.
-- awful.button
local button = { mt = {} }

--- Modifiers to ignore.
-- By default this is initialized as { "Lock", "Mod2" }
-- so the Caps Lock or Num Lock modifier are not taking into account by awesome
-- when pressing keys.
-- @name ignore_modifiers
-- @class table
local ignore_modifiers = { "Lock", "Mod2" }

--- Create a new button to use as binding.
-- This function is useful to create several buttons from one, because it will use
-- the ignore_modifier variable to create more button with or without the ignored
-- modifiers activated.
-- For example if you want to ignore CapsLock in your buttonbinding (which is
-- ignored by default by this function), creating button binding with this function
-- will return 2 button objects: one with CapsLock on, and the other one with
-- CapsLock off.
-- @see button
-- @return A table with one or several button objects.
function button.new(mod, _button, press, release)
    local ret = {}
    local subsets = util.subsets(ignore_modifiers)
    for _, set in ipairs(subsets) do
        ret[#ret + 1] = capi.button({ modifiers = util.table.join(mod, set),
                                      button = _button })
        if press then
            ret[#ret]:connect_signal("press", function(bobj, ...) press(...) end)
        end
        if release then
            ret[#ret]:connect_signal("release", function (bobj, ...) release(...) end)
        end
    end
    return ret
end

function button.mt:__call(...)
    return button.new(...)
end

return setmetatable(button, button.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
