---------------------------------------------------------------------------
--- A layout with clients on top of each other filling all the space
--
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2016 Emmanuel Lepage Vallee
-- @release @AWESOME_VERSION@
-- @module awful.layout.dynamic.suit.max
---------------------------------------------------------------------------
local dynamic     = require( "awful.layout.dynamic.base"        )
local wibox       = require( "wibox"                            )
local base_layout = require( "awful.layout.dynamic.base_layout" )
local stack       = require( "awful.layout.dynamic.base_stack"  )

local function ctr()
    local main_layout = wibox.widget {
        {
            {
                id     = "main_stack",
                layout = stack,
            },
            layout = base_layout.vertical
        },
        layout = base_layout.horizontal
    }

    function main_layout:add(...)
        local mstack = main_layout:get_children_by_id("main_stack")[1]
        mstack:add(...)
    end

    return main_layout
end

local module                     = dynamic("max"       , ctr)
module.fullscreen                = dynamic("fullscreen", ctr)
module.fullscreen.honor_workarea = false
module.fullscreen.honor_gap      = false

return module
-- kate: space-indent on; indent-width 4; replace-tabs on;
