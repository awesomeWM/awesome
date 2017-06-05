---------------------------------------------------------------------------
-- This class has been moved to `wibox.container.rotate`
--
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @classmod wibox.layout.rotate
---------------------------------------------------------------------------

local gdebug = require("gears.debug")

return gdebug.deprecate_class(
    require("wibox.container.rotate"),
    "wibox.layout.rotate",
    "wibox.container.rotate"
)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
