---------------------------------------------------------------------------
--- Suits for awful
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008 Julien Danjou
-- @module awful.layout
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

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
