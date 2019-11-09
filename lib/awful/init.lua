---------------------------------------------------------------------------
--- AWesome Functions very UsefuL
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008 Julien Danjou
-- @module awful
---------------------------------------------------------------------------

require("awful._compat")

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
    keyboard = require("awful.keyboard");
    button = require("awful.button");
    wibar = require("awful.wibar");
    wibox = require("awful.wibox");
    startup_notification = require("awful.startup_notification");
    tooltip = require("awful.tooltip");
    ewmh = require("awful.ewmh");
    titlebar = require("awful.titlebar");
    rules = require("awful.rules");
    popup = require("awful.popup");
    spawn = require("awful.spawn");
}

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
