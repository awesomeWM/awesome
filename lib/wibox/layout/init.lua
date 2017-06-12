---------------------------------------------------------------------------
--- Collection of layouts that can be used in widget boxes
--
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @classmod wibox.layout
---------------------------------------------------------------------------
local base = require("wibox.widget.base")

return setmetatable({
    fixed = require("wibox.layout.fixed");
    align = require("wibox.layout.align");
    flex = require("wibox.layout.flex");
    rotate = require("wibox.layout.rotate");
    manual = require("wibox.layout.manual");
    margin = require("wibox.layout.margin");
    mirror = require("wibox.layout.mirror");
    constraint = require("wibox.layout.constraint");
    scroll = require("wibox.layout.scroll");
    ratio = require("wibox.layout.ratio");
    stack = require("wibox.layout.stack");
    grid = require("wibox.layout.grid");
}, {__call = function(_, args) return base.make_widget_declarative(args) end})

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
