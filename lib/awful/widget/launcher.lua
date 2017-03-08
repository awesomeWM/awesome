---------------------------------------------------------------------------
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008-2009 Julien Danjou
-- @classmod awful.widget.launcher
---------------------------------------------------------------------------

local setmetatable = setmetatable
local gtable = require("gears.table")
local spawn = require("awful.spawn")
local wbutton = require("awful.widget.button")
local button = require("awful.button")

local launcher = { mt = {} }

--- Create a button widget which will launch a command.
-- @param args Standard widget table arguments, plus image for the image path
-- and command for the command to run on click, or either menu to create menu.
-- @return A launcher widget.
function launcher.new(args)
    if not args.command and not args.menu then return end
    local w = wbutton(args)
    if not w then return end

    local b
    if args.command then
       b = gtable.join(w:buttons(), button({}, 1, nil, function () spawn(args.command) end))
    elseif args.menu then
       b = gtable.join(w:buttons(), button({}, 1, nil, function () args.menu:toggle() end))
    end

    w:buttons(b)
    return w
end

function launcher.mt:__call(...)
    return launcher.new(...)
end

return setmetatable(launcher, launcher.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
