---------------------------------------------------------------------------
-- @author Uli Schlachter &lt;psychon@znc.in&gt;
-- @copyright 2014 Uli Schlachter
-- @module naughty
---------------------------------------------------------------------------

local naughty = require("naughty.core")
local capi = {awesome = awesome}
if dbus then
    naughty.dbus = require("naughty.dbus")
end

naughty.action = require("naughty.action")
naughty.list = require("naughty.list")
naughty.layout = require("naughty.layout")
naughty.widget = require("naughty.widget")
naughty.container = require("naughty.container")
naughty.action = require("naughty.action")
naughty.notification = require("naughty.notification")

-- Handle runtime errors during startup
if capi.awesome.startup_errors then
    -- Wait until `rc.lua` is executed before creating the notifications.
    -- Otherwise nothing is handling them (yet).
    awesome.connect_signal("startup", function()
        naughty.emit_signal(
            "request::display_error", capi.awesome.startup_errors, true
        )
    end)
end

-- Handle runtime errors after startup
do
    local in_error = false
    capi.awesome.connect_signal("debug::error", function (err)
        -- Make sure we don't go into an endless error loop
        if in_error then return end
        in_error = true

        naughty.emit_signal("request::display_error", tostring(err), false)

        in_error = false
    end)
end

return naughty

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
