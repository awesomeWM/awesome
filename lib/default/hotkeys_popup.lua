local awful = require("awful")
local hotkeys_popup = require("awful.hotkeys_popup")
local common = require("default.common")
local modkey = common.modkey
-- Enable hotkeys help widget for VIM and other apps
-- when client with a matching name is opened:
require("awful.hotkeys_popup.keys")
awful.keyboard.append_global_keybindings({
    awful.key({ modkey,           }, "s",      hotkeys_popup.show_help,
              {description="show help", group="awesome"})
})
