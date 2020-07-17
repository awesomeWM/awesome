local awful = require("awful")
local beautiful = require("beautiful")
local common = require("default.common")

local mylauncher = awful.widget.launcher({ image = beautiful.awesome_icon,
                                     menu = common.mymainmenu })
                                 

table.insert(globalwidgets.left, mylauncher)
