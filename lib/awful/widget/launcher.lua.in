---------------------------------------------------------------------------
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008-2009 Julien Danjou
-- @release @AWESOME_VERSION@
---------------------------------------------------------------------------

local setmetatable = setmetatable
local util = require("awful.util")
local wbutton = require("awful.widget.button")
local button = require("awful.button")

-- awful.widget.launcher
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
       b = util.table.join(w:buttons(), button({}, 1, nil, function () util.spawn(args.command) end))
    elseif args.menu then
       b = util.table.join(w:buttons(), button({}, 1, nil, function () args.menu:toggle() end))
    end

    w:buttons(b)
    return w
end

function launcher.mt:__call(...)
    return launcher.new(...)
end

return setmetatable(launcher, launcher.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
