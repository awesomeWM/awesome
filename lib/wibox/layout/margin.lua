---------------------------------------------------------------------------
-- This class has been moved to `wibox.container.margin`
--
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @classmod wibox.layout.margin
---------------------------------------------------------------------------

local gdebug = require("gears.debug")

return gdebug.deprecate_class(
    require("wibox.container.margin"),
    "wibox.layout.margin",
    "wibox.container.margin"
)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
