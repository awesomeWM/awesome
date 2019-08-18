local naughty = require("naughty") --DOC_HIDE
local ruled = {notification = require("ruled.notification")}--DOC_HIDE

ruled.notification.connect_signal("request::rules", function() --DOC_HIDE
   -- Note that the the message is matched as a pattern.
   ruled.notification.append_rule {
       rule       = { message = "I am SPAM", has_focus = true },
       properties = { ignore  = true}
   }
end) --DOC_HIDE

awesome.emit_signal("startup") --DOC_HIDE

local c = client.gen_fake { class = "xchat" } --DOC_HIDE
client.focus = c --DOC_HIDE
assert(client.focus == c) --DOC_HIDE

local notif = naughty.notification { --DOC_HIDE
    message = "I am SPAM", --DOC_HIDE
    clients = {c}, --DOC_HIDE note that this is undocumented
} --DOC_HIDE

assert(#notif.clients == 1 and notif.clients[1] == client.focus) --DOC_HIDE
assert(notif.ignore) --DOC_HIDE
