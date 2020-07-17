local awful = require("awful")
local common = require("default.common")
local modkey = common.modkey

-- General Awesome keys
awful.keyboard.append_global_keybindings({
    awful.key({ modkey,           }, "w", function () common.mymainmenu:show() end,
              {description = "show main menu", group = "awesome"}),
    awful.key({ modkey, "Control" }, "r", awesome.restart,
              {description = "reload awesome", group = "awesome"}),
    awful.key({ modkey, "Shift"   }, "q", awesome.quit,
              {description = "quit awesome", group = "awesome"}),
    awful.key({ modkey,           }, "Return", function () awful.spawn(common.terminal) end,
              {description = "open a terminal", group = "launcher"}),
})
