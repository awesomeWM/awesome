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

--- This is the same as awful.hotkeys_popup.widget.show_help.
--
-- example usage:
--
--     local hotkeys_popup = require("awful.hotkeys_popup")
--     myawesomemenu = {{ "hotkeys", function() hotkeys_popup.show_help() end },
--                      -- <more entries>
--     }
--
-- see `awful.hotkeys_popup.widget.show_help` for more information
-- @tparam[opt] client c The hostkeys for the client "c".
-- @tparam[opt] screen s The screen.
-- @tparam[opt=true] boolean show_args.show_awesome_keys Show AwesomeWM hotkeys.
-- When set to `false` only app-specific hotkeys will be shown.
-- @function awful.hotkeys_popup.show_help
-- @see awful.hotkeys_popup.widget.show_help

hotkeys_popup.show_help = hotkeys_popup.widget.show_help
return hotkeys_popup

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
