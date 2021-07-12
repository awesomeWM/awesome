local awful = require("awful")
local beautiful = require("beautiful")
local hotkeys_popup = require("awful.hotkeys_popup")
local common = {}


function common.init(args)
   local args = args or {}
   -- {{{ Variable definitions

   -- This is used later as the default terminal and editor to run.
   common.terminal = args.terminal or "xterm"
   common.editor = args.editor or os.getenv("EDITOR") or "nano"
   common.editor_cmd = args.editor_cmd or common.terminal .. " -e " .. common.editor

   -- Default modkey.
   -- Usually, Mod4 is the key with a logo between Control and Alt.
   -- If you do not like this or do not have such a key,
   -- I suggest you to remap Mod4 to another key using xmodmap or other tools.
   -- However, you can use another modifier like Mod1, but it may interact with others.
   common.modkey = args.modkey or "Mod4"
   -- }}}


   -- {{{ Menu
   -- Create a launcher widget and a main menu
   common.myawesomemenu = {
      { "hotkeys", function() hotkeys_popup.show_help(nil, awful.screen.focused()) end },
      { "manual", common.terminal .. " -e man awesome" },
      { "edit config", common.editor_cmd .. " " .. awesome.conffile },
      { "restart", awesome.restart },
      { "quit", function() awesome.quit() end },
   }
   common.mymainmenu = awful.menu({ items = { { "awesome", common.myawesomemenu, beautiful.awesome_icon },
            { "open terminal", common.terminal }
         }
      })
   
end

--return common
return setmetatable(common, {})
