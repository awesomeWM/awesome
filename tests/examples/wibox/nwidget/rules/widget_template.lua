--DOC_GEN_IMAGE --DOC_NO_USAGE
local parent    = ... --DOC_HIDE
local naughty = require("naughty") --DOC_HIDE
local ruled = {notification = require("ruled.notification")}--DOC_HIDE
local wibox = require("wibox") --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

local box = nil --DOC_HIDE
beautiful.notification_bg = beautiful.bg_normal --DOC_HIDE

   ruled.notification.connect_signal("request::rules", function()
       -- Add a red background for urgent notifications.
       ruled.notification.append_rule {
           rule       = { app_name = "mdp" },
           properties = {
               widget_template = {
                   {
                       {
                           {
                               {
                                   naughty.widget.icon,
                                   forced_height = 48,
                                   halign        = "center",
                                   valign        = "center",
                                   widget        = wibox.container.place
                               },
                               {
                                   halign = "center",
                                   widget = naughty.widget.title,
                               },
                               {
                                   halign = "center",
                                   widget = naughty.widget.message,
                               },
                               {
                                   orientation   = "horizontal",
                                   widget        = wibox.widget.separator,
                                   forced_height = 1,
                               },
                               {
                                   nil,
                                   {
                                       wibox.widget.textbox "⏪",
                                       wibox.widget.textbox "⏸",
                                       wibox.widget.textbox "⏩",
                                       spacing = 20,
                                       layout  = wibox.layout.fixed.horizontal,
                                   },
                                   expand = "outside",
                                   nil,
                                   layout = wibox.layout.align.horizontal,
                               },
                               spacing = 10,
                               layout  = wibox.layout.fixed.vertical,
                           },
                           margins = beautiful.notification_margin,
                           widget  = wibox.container.margin,
                       },
                       id     = "background_role",
                       widget = naughty.container.background,
                   },
                   strategy = "max",
                   width    = 160,
                   widget   = wibox.container.constraint,
               }
           }
       }
   end)

   naughty.connect_signal("request::display", function(n)
       box = --DOC_HIDE
       naughty.layout.box { notification = n }
   end)

awesome.emit_signal("startup") --DOC_HIDE

local notif2 =  --DOC_HIDE
   naughty.notification {  --DOC_HIDE
       title     = "Daft Punk",  --DOC_HIDE
       message   = "Harder, Better, Faster, Stronger",  --DOC_HIDE
       icon      = beautiful.awesome_icon, --DOC_HIDE
       icon_size = 128,  --DOC_HIDE
       app_name  = "mdp",  --DOC_HIDE
   }  --DOC_HIDE

assert(notif2.app_name == "mdp") --DOC_HIDE
assert(box) --DOC_HIDE

box.widget.forced_width = 250 --DOC_HIDE

parent:add(box.widget) --DOC_HIDE
