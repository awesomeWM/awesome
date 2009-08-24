local setmetatable = setmetatable
local require = require

-- Widget layouts
module("awful.widget.layout")

--- Widgets margins.
-- <p>In this table you can set the margin you want the layout to use when
-- positionning your widgets.
-- For example, if you want to put 10 pixel free on left on a widget, add this:
-- <code>
-- awful.widget.layout.margins[mywidget] = { left = 10 }
-- </code>
-- </p>
-- @name margins
-- @class table
margins = setmetatable({}, { __mode = 'k' })

require("awful.widget.layout.horizontal")
require("awful.widget.layout.vertical")
require("awful.widget.layout.default")

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
