---------------------------------------------------------------------------
--- AWesome Functions very UsefuL
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008 Julien Danjou
-- @module awful
---------------------------------------------------------------------------

require("awful._compat")

local deprecated = {
    ewmh = true
}

local ret = {
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
    keyboard = require("awful.keyboard");
    button = require("awful.button");
    wibar = require("awful.wibar");
    wibox = require("awful.wibox");
    startup_notification = require("awful.startup_notification");
    tooltip = require("awful.tooltip");
    permissions = require("awful.permissions");
    titlebar = require("awful.titlebar");
    wallpaper = require("awful.wallpaper");
    rules = require("awful.rules");
    popup = require("awful.popup");
    spawn = require("awful.spawn");
    screenshot = require("awful.screenshot");
}

-- Lazy load deprecated modules to reduce the numbers of loop dependencies.
return setmetatable(ret,{
    __index = function(_, key)
        if deprecated[key] then
            rawset(ret, key, require("awful."..key))
        end
        return rawget(ret, key)
    end
})

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
