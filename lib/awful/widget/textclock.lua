---------------------------------------------------------------------------
-- This widget has moved to `wibox.widget.textclock`
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008-2009 Julien Danjou
-- @classmod awful.widget.textclock
---------------------------------------------------------------------------
local gdebug = require("gears.debug")

return gdebug.deprecate_class(
    require("wibox.widget.textclock"),
    "awful.widget.textclock",
    "wibox.widget.textclock"
)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
