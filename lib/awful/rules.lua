---------------------------------------------------------------------------
--- This module has been moved to `ruled.client`
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Julien Danjou
-- @module awful.rules
---------------------------------------------------------------------------
local gdebug = require("gears.debug")

return gdebug.deprecate_class(
    require("ruled.client"),
    "awful.rules",
    "ruled.client",
    { deprecated_in = 5}
)
