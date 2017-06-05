---------------------------------------------------------------------------
--- This module has been moved to `wibox.widget.graph`
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Julien Danjou
-- @classmod awful.widget.graph
---------------------------------------------------------------------------
local gdebug = require("gears.debug")

return gdebug.deprecate_class(
    require("wibox.widget.graph"),
    "awful.widget.graph",
    "wibox.widget.graph"
)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
