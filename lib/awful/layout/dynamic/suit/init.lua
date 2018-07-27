---------------------------------------------------------------------------
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2016 Emmanuel Lepage Vallee
-- @release @AWESOME_VERSION@
-- @module awful.layout.dynamic.suit
---------------------------------------------------------------------------

return {
    tile     = require( "awful.layout.dynamic.suit.tile"     ),
    fair     = require( "awful.layout.dynamic.suit.fair"     ),
    max      = require( "awful.layout.dynamic.suit.max"      ),
    corner   = require( "awful.layout.dynamic.suit.corner"   ),
    treesome = require( "awful.layout.dynamic.suit.treesome" )
}
