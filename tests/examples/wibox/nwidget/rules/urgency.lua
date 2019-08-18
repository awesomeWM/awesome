--DOC_GEN_IMAGE --DOC_NO_USAGE
local parent    = ... --DOC_HIDE
local naughty = require("naughty") --DOC_HIDE
local ruled = {notification = require("ruled.notification")}--DOC_HIDE
local wibox = require("wibox") --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE
local def = require("naughty.widget._default") --DOC_HIDE
local acommon = require("awful.widget.common") --DOC_HIDE

beautiful.notification_bg = beautiful.bg_normal --DOC_HIDE

   ruled.notification.connect_signal("request::rules", function()
       -- Add a red background for urgent notifications.
       ruled.notification.append_rule {
           rule       = { urgency = "critical" },
           properties = { bg = "#ff0000", fg = "#ffffff", timeout = 0 }
       }

       --DOC_NEWLINE

       -- Or green background for normal ones.
       ruled.notification.append_rule {
           rule       = { urgency = "normal" },
           properties = { bg      = "#00ff00", fg = "#000000"}
       }
   end)

awesome.emit_signal("startup") --DOC_HIDE

--DOC_NEWLINE

   -- Create a normal notification.
local notif =  --DOC_HIDE
   naughty.notification {
       title   = "A notification 1",
       message = "This is very informative",
       icon    = beautiful.awesome_icon,
       urgency = "normal",
   }

--DOC_NEWLINE

   -- Create a normal notification.
local notif2 =  --DOC_HIDE
   naughty.notification {
       title   = "A notification 2",
       message = "This is very informative",
       icon    = beautiful.awesome_icon,
       urgency = "critical",
   }

local function show_notification(n) --DOC_HIDE
    local default = wibox.widget(def) --DOC_HIDE
    acommon._set_common_property(default, "notification", n) --DOC_HIDE
    parent:add(default) --DOC_HIDE
end --DOC_HIDE

parent.spacing = 10 --DOC_HIDE
show_notification(notif)  --DOC_HIDE
show_notification(notif2) --DOC_HIDE
