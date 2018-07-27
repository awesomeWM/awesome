---------------------------------------------------------------------------
--- A drop-in replacment for the stateless layout suits
--
-- This system also add the possibility to write handlers enabling the use
-- of tabs, spliters or custom client recorator.
--
-- Any wibox.layout compliant layout can be implemented. Monkey-patching
-- `awful.layout.dynamic.wrapper` also allow modules to define extra features
-- for tiled clients.
--
-- To enable this system, add `require("awful.layout.dynamic")` at the top
-- of your rc.lua
--
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2016 Emmanuel Lepage Vallee
-- @release @AWESOME_VERSION@
-- @module awful.layout.dynamic
---------------------------------------------------------------------------
local static = require( "awful.layout" )

local suits = {
    tile      = require("awful.layout.dynamic.suit.tile"      ),
    fair      = require("awful.layout.dynamic.suit.fair"      ),
    max       = require("awful.layout.dynamic.suit.max"       ),
    corner    = require("awful.layout.dynamic.suit.corner"    ),
    tree      = require("awful.layout.dynamic.suit.treesome"  ),
    magnifier = require("awful.layout.dynamic.suit.magnifier" ),
}

-- Monkeypatch the stateless layout system so the dynamic versions are used
local to_replace = {
    [static.suit.tile           ] = suits.tile           ,
    [static.suit.tile.left      ] = suits.tile.left      ,
    [static.suit.tile.bottom    ] = suits.tile.bottom    ,
    [static.suit.tile.top       ] = suits.tile.top       ,
    [static.suit.fair           ] = suits.fair           ,
    [static.suit.fair.horizontal] = suits.fair.horizontal,
    [static.suit.max            ] = suits.max            ,
    [static.suit.max.fullscreen ] = suits.max.fullscreen ,
    [static.suit.corner.nw      ] = suits.corner.nw      ,
    [static.suit.corner.ne      ] = suits.corner.ne      ,
    [static.suit.corner.sw      ] = suits.corner.sw      ,
    [static.suit.corner.se      ] = suits.corner.se      ,
    [static.suit.magnifier      ] = suits.magnifier      ,
}

for k, v in ipairs(static.layouts) do
    if to_replace[v] then
        static.layouts[k] = to_replace[v]
    end
end

static.suit.tile      = suits.tile
static.suit.fair      = suits.fair
static.suit.max       = suits.max
static.suit.corner    = suits.corner
static.suit.treesome  = suits.tree
static.suit.corner    = suits.corner
static.suit.magnifier = suits.magnifier

suits.register = require("awful.layout.dynamic.base").register

return suits
