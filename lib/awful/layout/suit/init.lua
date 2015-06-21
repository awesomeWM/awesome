---------------------------------------------------------------------------
--- Suits for awful
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008 Julien Danjou
-- @release @AWESOME_VERSION@
-- @module awful.layout.suit
---------------------------------------------------------------------------

return
{
    corner = require("awful.layout.suit.corner");
    max = require("awful.layout.suit.max");
    tile = require("awful.layout.suit.tile");
    fair = require("awful.layout.suit.fair");
    floating = require("awful.layout.suit.floating");
    magnifier = require("awful.layout.suit.magnifier");
    spiral = require("awful.layout.suit.spiral");
}
