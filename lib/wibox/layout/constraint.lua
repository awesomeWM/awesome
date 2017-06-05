---------------------------------------------------------------------------
-- This class has been moved to `wibox.container.`
--
-- @author Lukáš Hrázký
-- @copyright 2012 Lukáš Hrázký
-- @classmod wibox.layout.constraint
---------------------------------------------------------------------------

local gdebug = require("gears.debug")

return gdebug.deprecate_class(
    require("wibox.container.constraint"),
    "wibox.layout.constraint",
    "wibox.container.constraint"
)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
