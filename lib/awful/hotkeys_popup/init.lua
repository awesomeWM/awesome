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

--- awful.menu callable version of show popup with hotkeys help.
--
-- example usage: myawesomemenu = {{ "hotkeys", hotkeys_popup.show },
--
-- see `awful.hotkeys_popup.widget.show_help` for more information

hotkeys_popup.show = function() hotkeys_popup.widget.show_help( _, awful.screen.focused() ) end

--- Show popup with hotkeys help (shortcut to awful.hotkeys_popup.widget.show_help).
--
-- example usage: myawesomemenu = {{ "hotkeys", function() return false, hotkeys_popup.show_help end},
--
-- see `awful.hotkeys_popup.widget.show_help` for more information

hotkeys_popup.show_help = hotkeys_popup.widget.show_help
return hotkeys_popup

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
