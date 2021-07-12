-- awesome_mode: api-level=4:screen=on
-- If LuaRocks is installed, make sure that packages installed through it are
-- found (e.g. lgi). If LuaRocks is not installed, do nothing.
pcall(require, "luarocks.loader")

-- Standard awesome library
local gears = require("gears")
local awful = require("awful")
require("awful.autofocus")
-- Widget and layout library
local wibox = require("wibox")
-- Theme handling library
local beautiful = require("beautiful")
-- Notification library
local naughty = require("naughty")
-- Declarative object management
local ruled = require("ruled")

-- Themes define colours, icons, font and wallpapers.
beautiful.init(gears.filesystem.get_themes_dir() .. "default/theme.lua")
require("default")
local common = require("default.common")
common.init {
  -- uncomment and edit any of those parametrs
  --terminal = "xterm",
  --modkey = "Mod4"
  --editor = "nano"
  --editor_cmd = "gvim"
}

require("default.error")
require("default.layouts")
require("default.wallpaper")
require("default.tags")
require("default.launcher")
require("default.taglist")
require("default.promptbox")
require("default.tasklist")
require("default.keyboardlayout")
require("default.systray")
require("default.textclock")
require("default.layoutbox")
require("default.panel")
require("default.bindings.global_mouse")
require("default.bindings.global_general_keys")
require("default.bindings.global_tags_keys")
require("default.bindings.global_focus_keys")
require("default.bindings.global_layout_keys")
require("default.bindings.client_mouse")
require("default.bindings.client_keys")
require("default.client_rules")
require("default.titlebars")
require("default.notifications")
require("default.focus")

