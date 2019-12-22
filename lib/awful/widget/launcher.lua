---------------------------------------------------------------------------
--  A button widget which hosts a menu or starts a command.
--
--  Implementation of `awful.widget.button` with the ability to host an
--  `awful.menu` or a command to execute on user click.
--
-- This example is the default launcher.
--
--@DOC_awful_widget_launcher_default_EXAMPLE@
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008-2009 Julien Danjou
-- @widgetmod awful.widget.launcher
---------------------------------------------------------------------------

local setmetatable = setmetatable
local spawn = require("awful.spawn")
local wbutton = require("awful.widget.button")
local button = require("awful.button")

local launcher = { mt = {} }

--- Create a button widget which will launch a command.
---
-- **NOTE**: You need either command or menu argument for widget to create
-- successfully
-- @tparam table args
-- @tparam image args.image The image to display on the launcher button (refer to `wibox.widget.imagebox`).
-- @tparam[opt] string args.command Command to run on user click.
-- @tparam[opt] table args.menu Table of items to create a menu based on `awful.menu`.
-- @treturn awful.widget.launcher A launcher widget.
-- @constructorfct awful.widget.launcher
-- @see wibox.widget.imagebox
-- @see awful.menu
function launcher.new(args)
    if not args.command and not args.menu then return end
    local w = wbutton(args)
    if not w then return end

    if args.command then
        w:add_button(button({}, 1, nil, function () spawn(args.command) end))
    elseif args.menu then
        w:add_button(button({}, 1, nil, function () args.menu:toggle() end))
    end

    return w
end

function launcher.mt:__call(...)
    return launcher.new(...)
end

--@DOC_widget_COMMON@

--@DOC_object_COMMON@

return setmetatable(launcher, launcher.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
