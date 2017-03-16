---------------------------------------------------------------------------
-- This class has been moved to `wibox.container.mirror`
--
-- @author dodo
-- @copyright 2012 dodo
-- @classmod wibox.layout.mirror
---------------------------------------------------------------------------

local gdebug = require("gears.debug")

return gdebug.deprecate_class(
    require("wibox.container.mirror"),
    "wibox.layout.mirror",
    "wibox.container.mirror"
)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
