---------------------------------------------------------------------------
--- Collection of containers that can be used in widget boxes
--
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @classmod wibox.container
---------------------------------------------------------------------------
local base = require("wibox.widget.base")

return setmetatable({
    rotate = require("wibox.container.rotate");
    margin = require("wibox.container.margin");
    mirror = require("wibox.container.mirror");
    constraint = require("wibox.container.constraint");
    scroll = require("wibox.container.scroll");
    background = require("wibox.container.background");
    radialprogressbar = require("wibox.container.radialprogressbar");
    arcchart = require("wibox.container.arcchart");
    place = require("wibox.container.place");
}, {__call = function(_, args) return base.make_widget_declarative(args) end})

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
