---------------------------------------------------------------------------
--- Autofocus functions.
--
-- When loaded, this module makes sure that there's always a client that will
-- have focus on events such as tag switching, client unmanaging, etc.
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Julien Danjou
-- @module awful.autofocus
---------------------------------------------------------------------------

local client = client
local aclient = require("awful.client")
local timer = require("gears.timer")

local function filter_sticky(c)
    return not c.sticky and aclient.focus.filter(c)
end

--- Give focus when clients appear/disappear.
--
-- @param obj An object that should have a .screen property.
local function check_focus(obj)
    if not obj.screen.valid then return end
    -- When no visible client has the focus...
    if not client.focus or not client.focus:isvisible() then
        local c = aclient.focus.history.get(screen[obj.screen], 0, filter_sticky)
        if not c then
            c = aclient.focus.history.get(screen[obj.screen], 0, aclient.focus.filter)
        end
        if c then
            c:emit_signal("request::activate", "autofocus.check_focus",
                          {raise=false})
        end
    end
end

--- Check client focus (delayed).
-- @param obj An object that should have a .screen property.
local function check_focus_delayed(obj)
    timer.delayed_call(check_focus, {screen = obj.screen})
end

--- Give focus on tag selection change.
--
-- @param tag A tag object
local function check_focus_tag(t)
    local s = t.screen
    if (not s) or (not s.valid) then return end
    s = screen[s]
    check_focus({ screen = s })
    if client.focus and screen[client.focus.screen] ~= s then
        local c = aclient.focus.history.get(s, 0, filter_sticky)
        if not c then
            c = aclient.focus.history.get(s, 0, aclient.focus.filter)
        end
        if c then
            c:emit_signal("request::activate", "autofocus.check_focus_tag",
                          {raise=false})
        end
    end
end

tag.connect_signal("property::selected", function (t)
    timer.delayed_call(check_focus_tag, t)
end)
client.connect_signal("unmanage",            check_focus_delayed)
client.connect_signal("tagged",              check_focus_delayed)
client.connect_signal("untagged",            check_focus_delayed)
client.connect_signal("property::hidden",    check_focus_delayed)
client.connect_signal("property::minimized", check_focus_delayed)
client.connect_signal("property::sticky",    check_focus_delayed)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
