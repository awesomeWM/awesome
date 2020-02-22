local naughty = require("naughty") --DOC_HIDE
local ruled = {notification = require("ruled.notification")}--DOC_HIDE

local function get_mpris_actions() end --DOC_HIDE
local my_music_widget_template = nil --DOC_HIDE

ruled.notification.connect_signal("request::rules", function() --DOC_HIDE
   ruled.notification.append_rule {
       rule       = { has_instance = "amarok" },
       properties = {
           widget_template = my_music_widget_template,
           actions         = get_mpris_actions(),
           works           = true --DOC_HIDE
       }
   }
end) --DOC_HIDE

awesome.emit_signal("startup") --DOC_HIDE

local c = client.gen_fake { instance = "amarok" } --DOC_HIDE
assert(c.instance == "amarok") --DOC_HIDE
client.focus = c --DOC_HIDE
assert(client.focus == c) --DOC_HIDE

local notif = naughty.notification { --DOC_HIDE
    message = "I am SPAM", --DOC_HIDE
    clients = {c}, --DOC_HIDE note that this is undocumented
} --DOC_HIDE

assert(notif.works) --DOC_HIDE
