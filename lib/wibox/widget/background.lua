---------------------------------------------------------------------------
-- This class has been moved to `wibox.container.background`
--
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @classmod wibox.widget.background
---------------------------------------------------------------------------
local gdebug = require("gears.debug")

return gdebug.deprecate_class(
    require("wibox.container.background"),
    "wibox.widget.background",
    "wibox.container.background"
)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
