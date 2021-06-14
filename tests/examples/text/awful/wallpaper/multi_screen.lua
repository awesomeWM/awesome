--DOC_NO_USAGE
local awful =  { wallpaper = require("awful.wallpaper") } --DOC_HIDE

   local global_wallpaper = awful.wallpaper {
       -- [...] your content
   }

--DOC_NEWLINE

   screen.connect_signal("request::wallpaper", function()
       -- `screen` is the global screen module. It is also a list of all screens.
       global_wallpaper.screens = screen
   end)
