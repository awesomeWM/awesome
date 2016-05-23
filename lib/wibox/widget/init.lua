---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @release @AWESOME_VERSION@
-- @classmod wibox.widget
---------------------------------------------------------------------------
local base = require("wibox.widget.base")

return setmetatable({
    base = base;
    textbox = require("wibox.widget.textbox");
    imagebox = require("wibox.widget.imagebox");
    background = require("wibox.widget.background");
    systray = require("wibox.widget.systray");
    textclock = require("wibox.widget.textclock");
    progressbar = require("wibox.widget.progressbar");
}, {__call = function(_, args) return base.make_widget_declarative(args) end})

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
