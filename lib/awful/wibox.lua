---------------------------------------------------------------------------
--- This module is deprecated and has been renamed `awful.wibar`
--
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2016 Emmanuel Lepage Vallee
-- @module awful.wibox
---------------------------------------------------------------------------
local util = require("awful.util")

return util.deprecate_class(require("awful.wibar"), "awful.wibox", "awful.wibar")

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
