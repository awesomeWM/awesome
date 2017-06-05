---------------------------------------------------------------------------
--- This module has been moved to `wibox.widget.progressbar`
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Julien Danjou
-- @classmod awful.widget.progressbar
---------------------------------------------------------------------------
local gdebug = require("gears.debug")

return gdebug.deprecate_class(
    require("wibox.widget.progressbar"),
    "awful.widget.progressbar",
    "wibox.widget.progressbar"
)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
