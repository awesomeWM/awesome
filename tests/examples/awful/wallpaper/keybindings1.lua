--DOC_NO_USAGE --DOC_GEN_IMAGE
local awful =  {--DOC_HIDE
    wallpaper = require("awful.wallpaper"), --DOC_HIDE
    key = require("awful.key"), --DOC_HIDE
    keyboard = require("awful.keyboard"), --DOC_HIDE
    hotkeys_popup = { widget = require("awful.hotkeys_popup.widget") } --DOC_HIDE
} --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

screen[1]._resize {x = 0, y = 0, width = 320, height = 196} --DOC_HIDE


awful.keyboard.append_global_keybindings({ --DOC_HIDE
    awful.key({ modkey, "Shift"   }, "j", function () awful.client.swap.byidx(  1)    end, --DOC_HIDE
              {description = "swap with next client by index", group = "client"}), --DOC_HIDE
    awful.key({ modkey, "Shift"   }, "k", function () awful.client.swap.byidx( -1)    end, --DOC_HIDE
              {description = "swap with previous client by index", group = "client"}),--DOC_HIDE
    awful.key({ modkey,           }, "u", function() end,--DOC_HIDE
              {description = "jump to urgent client", group = "client"}),--DOC_HIDE
    awful.key({ modkey,           }, "l",     function () awful.tag.incmwfact( 0.05)          end,--DOC_HIDE
              {description = "increase master width factor", group = "layout"}),--DOC_HIDE
    awful.key({ modkey,           }, "h",     function () awful.tag.incmwfact(-0.05)          end,--DOC_HIDE
              {description = "decrease master width factor", group = "layout"}),--DOC_HIDE
    awful.key({ modkey, "Shift"   }, "h",     function () awful.tag.incnmaster( 1, nil, true) end,--DOC_HIDE
              {description = "increase the number of master clients", group = "layout"}),--DOC_HIDE
    awful.key({ modkey, "Shift"   }, "l",     function () awful.tag.incnmaster(-1, nil, true) end,--DOC_HIDE
              {description = "decrease the number of master clients", group = "layout"}),--DOC_HIDE
    awful.key({ modkey, "Control" }, "h",     function () awful.tag.incncol( 1, nil, true)    end,--DOC_HIDE
              {description = "increase the number of columns", group = "layout"}),--DOC_HIDE
    awful.key({ modkey, "Control" }, "l",     function () awful.tag.incncol(-1, nil, true)    end,--DOC_HIDE
              {description = "decrease the number of columns", group = "layout"}),--DOC_HIDE
    awful.key({ modkey,           }, "space", function () awful.layout.inc( 1)                end,--DOC_HIDE
              {description = "select next", group = "layout"}),--DOC_HIDE
    awful.key({ modkey, "Shift"   }, "space", function () awful.layout.inc(-1)                end,--DOC_HIDEw
              {description = "select previous", group = "layout"}),--DOC_HIDE
})--DOC_HIDE

   --BROKEN
   awful.wallpaper {
       screen = screen[1],
       bg     = "#0000ff",
       widget = awful.hotkeys_popup.widget.new()
   }

require("gears.timer").run_delayed_calls_now() --DOC_HIDE
