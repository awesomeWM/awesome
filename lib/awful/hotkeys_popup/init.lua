---------------------------------------------------------------------------
--- Popup widget which shows current hotkeys and their descriptions.
--
-- @author Yauheni Kirylau &lt;yawghen@gmail.com&gt;
-- @copyright 2014-2015 Yauheni Kirylau
-- @module awful.hotkeys_popup
---------------------------------------------------------------------------

local hotkeys_popup = {
  widget = require("awful.hotkeys_popup.widget"),
}
local awful = { screen = require( "awful.screen" ) }

--- Show popup with hotkeys help on the screen with the mouse (awful.menu callable)
--
-- example usage: 
--     local hotkeys_popup = require("awful.hotkeys_popup")
--     myawesomemenu = {{ "hotkeys", function() hotkeys_popup.show_help() end },
--                      -- <more entries>
--     }
--
-- see `awful.hotkeys_popup.widget.show_help` for more information

hotkeys_popup.show = function() hotkeys_popup.widget.show_help( nil, awful.screen.focused() ) end

--- This is the same as awful.hotkeys_popup.widget.show_help
--
-- example usage: 
--     local hotkeys_popup = require("awful.hotkeys_popup")
--     myawesomemenu = {{ "hotkeys", function() hotkeys_popup.show_help() end },
--                      -- <more entries>
--     }
--
-- see `awful.hotkeys_popup.widget.show_help` for more information

hotkeys_popup.show_help = hotkeys_popup.widget.show_help
return hotkeys_popup

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
