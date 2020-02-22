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
           rule       = { }, -- Match everything.
           properties = {
               append_actions = {
                  naughty.action {
                      name = "Snooze (5m)",
                  },
                  naughty.action {
                      name = "Snooze (15m)",
                  },
                  naughty.action {
                      name = "Snooze (1h)",
                  },
               },
           }
       }
   end)

awesome.emit_signal("startup") --DOC_HIDE

--DOC_NEWLINE

   -- Create a normal notification.
local notif =  --DOC_HIDE
   naughty.notification {
       title   = "A notification",
       message = "This is very informative",
       icon    = beautiful.awesome_icon,
       actions = {
           naughty.action { name = "Existing 1" },
           naughty.action { name = "Existing 2" },
       }
   }

local function show_notification(n) --DOC_HIDE
    local default = wibox.widget(def) --DOC_HIDE
    acommon._set_common_property(default, "notification", n) --DOC_HIDE

    local w, h = default:fit({dpi=96}, 9999, 9999) --DOC_HIDE
    default.forced_width = w + 250 --DOC_HIDE
    default.forced_height = h --DOC_HIDE
    parent.forced_width = w + 250 --DOC_HIDE

    parent:add(default) --DOC_HIDE
end --DOC_HIDE

parent.spacing = 10 --DOC_HIDE
show_notification(notif)  --DOC_HIDE
