local awful = require("awful")
local menubar = require("menubar")
local common = require("default.common")
local modkey = common.modkey

-- Menubar configuration
menubar.utils.terminal = common.terminal -- Set the terminal for applications that require it

-- General Awesome keys
awful.keyboard.append_global_keybindings({
    awful.key({ modkey }, "p", function() menubar.show() end,
              {description = "show the menubar", group = "launcher"}),
})
