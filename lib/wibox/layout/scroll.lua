---------------------------------------------------------------------------
-- This class has been moved to `wibox.container.scroll`
--
-- @author Uli Schlachter (based on ideas from Saleur Geoffrey)
-- @copyright 2015 Uli Schlachter
-- @classmod wibox.layout.scroll
---------------------------------------------------------------------------
local gdebug = require("gears.debug")

return gdebug.deprecate_class(
    require("wibox.container.scroll"),
    "wibox.layout.scroll",
    "wibox.container.scroll"
)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
