-- This test suite tries to prevent the legacy notification popups from
-- regressing as the new notification API is improving.
local spawn     = require("awful.spawn")
local naughty   = require("naughty"    )
local gdebug    = require("gears.debug")
local cairo     = require("lgi"        ).cairo
local beautiful = require("beautiful")

-- This module test deprecated APIs
require("gears.debug").deprecate = function() end

local steps = {}

local has_cmd_notify, has_cmd_send = false

-- Use `notify-send` instead of the shimmed version to better test the dbus
-- to notification code.
local function check_cmd()
    local path = os.getenv("PATH")
    local pos = 1
    while path:find(":", pos) do
        local np = path:find(":", pos)
        local p = path:sub(pos, np-1).."/"

        pos = np+1

        local f = io.open(p.."notify-send")
        if f then
            f:close()
            has_cmd_notify = true
        end

        f = io.open(p.."dbus-send")
        if f then
            f:close()
            has_cmd_send = true
        end

        if has_cmd_notify and has_cmd_send then return end
    end
end

check_cmd()

-- Can't test anything of value the documentation example tests don't already
-- hit.
if not has_cmd_send then
    require("gears.debug").print_warning("Did not find dbus-send, skipping test")
    require("_runner").run_steps {}
    return
end
if not has_cmd_notify then
    require("gears.debug").print_warning("Did not find notify-send, skipping some tests")
end

local active, destroyed, reasons, counter = {}, {}, {}, 0

local default_width, default_height = 0, 0

local function added_callback(n)
    table.insert(active, n)
    counter = counter + 1
end

naughty.connect_signal("added", added_callback)

local function destroyed_callback(n, reason)
    local found = false

    for k, n2 in ipairs(active) do
        if n2 == n then
            found = true
            table.remove(active, k)
        end
    end

    assert(found)

    if reason then
        reasons[reason] = reasons[reason] and reasons[reason] + 1 or 1
    end

    table.insert(destroyed, n)
end

naughty.connect_signal("destroyed", destroyed_callback)

table.insert(steps, function()
    if not has_cmd_notify then return true end

    spawn{ 'notify-send', 'title', 'message', '-t', '25000' }

    return true
end)

table.insert(steps, function()
    if not has_cmd_notify then return true end
    if #active ~= 1 then return end

    local n = active[1]

    assert(n.box)
    local offset = 2*n.box.border_width
    default_width  = n.box.width+offset
    default_height = n.box.height + offset + naughty.config.spacing

    assert(default_width  > 0)
    assert(default_height > 0)

    -- Make sure the expiration timer is started
    assert(n.timer)
    assert(n.timer.started)
    assert(n.is_expired == false)

    n:destroy()

    assert(#active == 0)

    return true
end)

-- Test pausing incoming notifications.
table.insert(steps, function()
    assert(not naughty.suspended)

    naughty.suspended = true

    -- There is some magic behind this, check it works
    assert(naughty.suspended)

    spawn{ 'notify-send', 'title', 'message', '-t', '25000' }

    return true
end)

-- Test resuming incoming notifications.
table.insert(steps, function(count)
    if count ~= 4 then return end

    assert(#active == 0)
    assert(#naughty.notifications.suspended == 1)
    assert(naughty.notifications.suspended[1]:get_suspended())

    naughty.resume()

    assert(not naughty.suspended)
    assert(#naughty.notifications.suspended == 0)
    assert(#active == 1)

    active[1]:destroy()
    assert(#active == 0)

    spawn{ 'notify-send', 'title', 'message', '-t', '1' }

    return true
end)

-- Test automatic expiration.
table.insert(steps, function()
    if counter ~= 3 then return end

    return true
end)

table.insert(steps, function()
    if #active > 0 then return end

    -- It expired after one milliseconds, so it should be gone as soon as
    -- it is registered.
    assert(#active == 0)

    assert(not naughty.expiration_paused)
    naughty.expiration_paused = true

    -- There is some magic behind this, make sure it works
    assert(naughty.expiration_paused)

    spawn{ 'notify-send', 'title', 'message', '-t', '1' }

    return true
end)

-- Test disabling automatic expiration.
table.insert(steps, function()
    if counter ~= 4 then return end

    -- It should not expire by itself, so that should always be true
    assert(#active == 1)

    return true
end)

-- Wait long enough to avoid races.
table.insert(steps, function(count)
    if count ~= 4 then return end

    assert(#active == 1)
    assert(active[1].is_expired)

    naughty.expiration_paused = false
    assert(not naughty.expiration_paused)

    return true
end)

-- Make sure enabling expiration process the expired queue.
table.insert(steps, function()
    -- Right now this doesn't require a step for itself, but this could change
    -- so better not "document" the instantaneous clearing of the queue.
    if #active > 0 then return end

    spawn{ 'notify-send', 'low',      'message', '-t', '25000', '-u', 'low' }
    spawn{ 'notify-send', 'normal',   'message', '-t', '25000', '-u', 'normal' }
    spawn{ 'notify-send', 'critical', 'message', '-t', '25000', '-u', 'critical' }

    return true
end)

-- Test the urgency level and default preset.
table.insert(steps, function()
    if counter ~= 7 then return end

    while #active > 0 do
        active[1]:destroy()
    end

    return true
end)

-- Test what happens when the screen has the maximum number of notification it
-- can display at one.
table.insert(steps, function()
    local wa = mouse.screen.workarea
    local max_notif = math.floor(wa.height/default_height)

    -- Everything should fit, otherwise the math is wrong in
    -- `neughty.layout.legacy` and its a regression.
    for i=1, max_notif do
        spawn{ 'notify-send', 'notif '..i, 'message', '-t', '25000', '-u', 'low' }
    end

    return true
end)

-- Test vertical overlapping
local function test_overlap()
    local wa = mouse.screen.workarea

    for _, n1 in ipairs(active) do
        assert(n1.box)

        -- Check for overlapping the workarea
        assert(n1.box.y+default_height < wa.y+wa.height)
        assert(n1.box.y >= wa.y)

        -- Check for overlapping each other
        for _, n2 in ipairs(active) do
            assert(n2.box)
            if n1 ~= n2 then
                local geo1, geo2 = n1.box:geometry(), n2.box:geometry()
                assert(geo1.height == geo2.height)
                assert(geo1.height + 2*n1.box.border_width + naughty.config.spacing
                    == default_height)

                if n1.position == n2.position then
                    assert(
                        geo1.y >= geo2.y+default_height or
                        geo2.y >= geo1.y+default_height
                    )
                end
            end
        end
    end
end

-- Check the lack of overlapping and the presence of the expected content.
table.insert(steps, function()
    local wa = mouse.screen.workarea
    local max_notif = math.floor(wa.height/default_height)
    if counter ~= 7 + max_notif then return end

    assert(#active == max_notif)

    test_overlap()

    -- Now add even more!
    for i=1, 5 do
        spawn{ 'notify-send', 'notif '..i, 'message', '-t', '25000', '-u', 'low' }
    end

    return true
end)

-- Test the code to hide the older notifications when there is too many for the
-- screen.
table.insert(steps, function()
    local wa = mouse.screen.workarea
    local max_notif = math.floor(wa.height/default_height)
    if counter ~= 7 + max_notif + 5 then return end

    -- The other should have been hidden
    assert(#active == max_notif)

    assert(reasons[naughty.notification_closed_reason.too_many_on_screen] == 5)

    test_overlap()

    while #active > 0 do
        active[1]:destroy()
    end

    return true
end)

local positions = {
    "top_left"      , "top_middle"    , "top_right"     ,
    "bottom_left"   , "bottom_middle" , "bottom_right"  ,
}

-- Test each corners.
table.insert(steps, function()
    for _, pos in ipairs(positions) do
        for i=1, 3 do
            -- Skip dbus for this one.
            naughty.notification {
                position = pos,
                title    = "At "..pos.." "..i,
                message  = "some message",
                timeout  = 25000,
            }
        end
    end

    return true
end)

table.insert(steps, function()
    if #active ~= #positions*3 then return end

    test_overlap()

    while #active > 0 do
        active[1]:destroy()
    end

    return true
end)

local big_icon     = cairo.ImageSurface(cairo.Format.ARGB32, 256, 256)
local cr           = cairo.Context(big_icon)
local small_icon   = cairo.ImageSurface(cairo.Format.ARGB32, 32 , 32 )
local cr2          = cairo.Context(small_icon)
local wierd_ratio1 = cairo.ImageSurface(cairo.Format.ARGB32, 256, 128)
local cr3          = cairo.Context(wierd_ratio1)
local wierd_ratio2 = cairo.ImageSurface(cairo.Format.ARGB32, 128, 256)
local cr4          = cairo.Context(wierd_ratio2)

-- Checkboard shirt pattern icon!
for i=1, 5 do
    for j=1, 5 do
        cr:set_source_rgb(
            i%2 == 1 and 1 or 0, j%2 == 1 and 1 or 0, i%2 == 0 and 0 or 1
        )
        cr:rectangle( (i-1)*48, (j-1)*48, 48, 48 )
        cr:fill()

        cr2:set_source_rgb(
            i%2 == 1 and 1 or 0, j%2 == 1 and 1 or 0, i%2 == 0 and 0 or 1
        )
        cr2:rectangle( (i-1)*6, (j-1)*6, 6, 6 )
        cr2:fill()

        cr3:set_source_rgb(
            i%2 == 1 and 1 or 0, j%2 == 1 and 1 or 0, i%2 == 0 and 0 or 1
        )
        cr3:rectangle( (i-1)*48, (j-1)*24, 48, 24 )
        cr3:fill()

        cr4:set_source_rgb(
            i%2 == 1 and 1 or 0, j%2 == 1 and 1 or 0, i%2 == 0 and 0 or 1
        )
        cr4:rectangle( (i-1)*24, (j-1)*48, 24, 48 )
        cr4:fill()
    end
end

-- Test the icon size constraints.
table.insert(steps, function()
    beautiful.notification_icon_size = 64

    -- Icons that are too large (they should be downscaled)
    local n1 = naughty.notification {
        icon    = big_icon,
        title   = "Has a nice icon!",
        message = "big",
        timeout = 25000,
    }

    assert(n1.iconbox)
    assert(n1.iconbox._private.image:get_width () == beautiful.notification_icon_size)
    assert(n1.iconbox._private.image:get_height() == beautiful.notification_icon_size)
    assert(n1.iconbox._private.image:get_width () == n1.size_info.icon_w)
    assert(n1.iconbox._private.image:get_height() == n1.size_info.icon_h)
    assert(n1.size_info.icon_scale_factor == 1/4)

    -- Icons that are too small (they should not be upscaled)
    local n2 = naughty.notification {
        icon    = small_icon,
        title   = "Has a nice icon!",
        message = "small",
        timeout = 25000,
    }

    assert(n2.iconbox)
    assert(n2.iconbox._private.image:get_width () == 32)
    assert(n2.iconbox._private.image:get_height() == 32)
    assert(n2.iconbox._private.image:get_width () == n2.size_info.icon_w)
    assert(n2.iconbox._private.image:get_height() == n2.size_info.icon_h)
    assert(not n2.size_info.icon_scale_factor)

    -- Downscaled non square icons (aspect ratio should be kept).
    local n3 = naughty.notification {
        icon    = wierd_ratio1,
        title   = "Has a nice icon!",
        message = "big",
        timeout = 25000,
    }

    local n4 = naughty.notification {
        icon    = wierd_ratio2,
        title   = "Has a nice icon!",
        message = "big",
        timeout = 25000,
    }

    assert(n3.iconbox)
    assert(n3.iconbox._private.image:get_width () == beautiful.notification_icon_size)
    assert(n3.iconbox._private.image:get_height() == beautiful.notification_icon_size/2)
    assert(n3.iconbox._private.image:get_width () == n3.size_info.icon_w)
    assert(n3.iconbox._private.image:get_height() == n3.size_info.icon_h)
    assert(n3.size_info.icon_scale_factor == 1/4)

    assert(n4.iconbox)
    assert(n4.iconbox._private.image:get_width () == beautiful.notification_icon_size/2)
    assert(n4.iconbox._private.image:get_height() == beautiful.notification_icon_size)
    assert(n4.iconbox._private.image:get_width () == n4.size_info.icon_w)
    assert(n4.iconbox._private.image:get_height() == n4.size_info.icon_h)
    assert(n4.size_info.icon_scale_factor == 1/4)

    -- The notification size should change proportionally to the icon size.
    assert(n1.box.width  == n2.box.width  + 32)
    assert(n1.box.height == n2.box.height + 32)
    assert(n1.box.height == n3.box.height + 32)
    assert(n1.box.width  == n4.box.width  + 32)
    assert(n1.box.height == n4.box.height)
    assert(n1.box.width  == n3.box.width )

    -- Make sure unconstrained icons work as expected.
    beautiful.notification_icon_size = nil

    local n5 = naughty.notification {
        icon    = big_icon,
        title   = "Has a nice icon!",
        message = "big",
        timeout = 25000,
    }

    assert(n5.iconbox)
    assert(n5.iconbox._private.image:get_width () == 256)
    assert(n5.iconbox._private.image:get_height() == 256)
    assert(n5.iconbox._private.image:get_width () == n5.size_info.icon_w)
    assert(n5.iconbox._private.image:get_height() == n5.size_info.icon_h)
    assert(not n5.size_info.icon_scale_factor)

    -- Make sure invalid icons don't prevent the message from being shown.
    local n6 = naughty.notification {
        icon    = "this/is/an/invlid/path",
        title   = "Has a nice icon!",
        message = "Very important life saving advice",
        timeout = 25000,
    }

    n1:destroy()
    n2:destroy()
    n3:destroy()
    n4:destroy()
    n5:destroy()
    n6:destroy()
    assert(#active == 0)

    return true
end)

-- Test notifications with size constraints.
table.insert(steps, function()
    local str = "foobar! "
    assert(#active == 0)

    -- 2^9 foobars is a lot of foobars.
    for _=1, 10 do
        str = str .. str
    end

    -- First, see what happen without any constraint and enormous messages.
    -- This also test notifications larger than the workarea.

    local n1 = naughty.notification {
        title   = str,
        message = str,
        timeout = 25000,
    }

    -- Same, but with an icon and larger borders.
    local n2 = naughty.notification {
        icon         = big_icon,
        title        = str,
        message      = str,
        timeout      = 25000,
        border_width = 40,
    }

    local wa = mouse.screen.workarea
    assert(n1.box.width +2*n1.box.border_width <= wa.width )
    assert(n1.box.height+2*n1.box.border_width <= wa.height)
    assert(n2.box.width +2*n2.box.border_width <= wa.width )
    assert(n2.box.height+2*n2.box.border_width <= wa.height)

    n1:destroy()
    n2:destroy()

    -- Now set a maximum size and try again.
    beautiful.notification_max_width  = 256
    beautiful.notification_max_height = 96

    local n3 = naughty.notification {
        title   = str,
        message = str,
        timeout = 25000,
    }

    assert(n3.box.width  <= 256)
    assert(n3.box.height <= 96 )

    -- Now test when the icon is larger than the maximum.
    local n4 = naughty.notification {
        icon    = big_icon,
        title   = str,
        message = str,
        timeout = 25000,
    }

    assert(n4.box.width  <= 256)
    assert(n4.box.height <= 96 )
    assert(n4.iconbox._private.image:get_width () == n4.size_info.icon_w)
    assert(n4.iconbox._private.image:get_height() == n4.size_info.icon_h)
    assert(n4.size_info.icon_w <= 256)
    assert(n4.size_info.icon_h <= 96 )

    n3:destroy()
    n4:destroy()
    assert(#active == 0)

    return true
end)

-- Test more advanced features than what notify-send can provide.
if has_cmd_send then

    local cmd = { 'dbus-send',
        '--type=method_call',
        '--print-reply=literal',
        '--dest=org.freedesktop.Notifications',
        '/org/freedesktop/Notifications',
        'org.freedesktop.Notifications.Notify',
        'string:"Awesome test"',
        'uint32:0',
        'string:""',
        'string:"title"',
        'string:"message body"',
        'array:string:1,one,2,two,3,three',
        'dict:string:string:"",""',
        'int32:25000'
    }

    -- Test the actions.
    table.insert(steps, function()

        assert(#active == 0)

        spawn(cmd)

        return true
    end)

    table.insert(steps, function()
        if #active == 0 then return end

        assert(#active == 1)
        local n = active[1]

        assert(n.box)
        assert(#n.actions == 3)
        assert(n.actions[1].name == "one"  )
        assert(n.actions[2].name == "two"  )
        assert(n.actions[3].name == "three")

        n:destroy()

        return true
    end)

    --TODO Test too many actions.

    --TODO Test action with long names.

    local nid, name_u, message_u, actions_u = nil

    -- Test updating a notification.
    table.insert(steps, function()
        spawn.easy_async(cmd, function(out)
            nid = tonumber(out:match(" [0-9]+"):match("[0-9]+"))
        end)

        return true
    end)

    table.insert(steps, function()
        if #active == 0 or not nid then return end

        local n = active[1]

        n:connect_signal("property::title"  , function() name_u    = true end)
        n:connect_signal("property::message", function() message_u = true end)
        n:connect_signal("property::actions", function() actions_u = true end)

        local update = { 'dbus-send',
        '--type=method_call',
        '--print-reply=literal',
        '--dest=org.freedesktop.Notifications',
        '/org/freedesktop/Notifications',
        'org.freedesktop.Notifications.Notify',
        'string:"Awesome test"',
        'uint32:' .. nid,
        'string:""',
        'string:updated title',
        'string:updated message body',
        'array:string:1,four,2,five,3,six',
        'dict:string:string:"",""',
        'int32:25000',
        }

        spawn(update)

        return true
    end)

    -- Test if all properties have been updated.
    table.insert(steps, function()
        if not name_u    then return end
        if not message_u then return end
        if not actions_u then return end

        -- No new notification should have been created.
        assert(#active == 1)

        local n = active[1]

        assert(n.title   == "updated title"       )
        assert(n.message == "updated message body")

        assert(#n.actions == 3)
        assert(n.actions[1].name == "four" )
        assert(n.actions[2].name == "five" )
        assert(n.actions[3].name == "six"  )

        return true
    end)

end

-- Now check if the old deprecated (but still supported) APIs don't have errors.
table.insert(steps, function()
    -- Tests are (by default) not allowed to call deprecated APIs
    gdebug.deprecate = function() end

    local n = naughty.notification {
        title   = "foo",
        message = "bar",
        timeout = 25000,
    }

    -- Make sure the suspension don't cause errors
    assert(not naughty.is_suspended())
    assert(not naughty.suspended)
    naughty.suspend()
    assert(naughty.is_suspended())
    assert(naughty.suspended)
    naughty.resume()
    assert(not naughty.is_suspended())
    assert(not naughty.suspended)
    naughty.toggle()
    assert(naughty.is_suspended())
    assert(naughty.suspended)
    naughty.toggle()
    assert(not naughty.is_suspended())
    assert(not naughty.suspended)
    naughty.suspended = not naughty.suspended
    assert(naughty.is_suspended())
    assert(naughty.suspended)
    naughty.suspended = not naughty.suspended
    assert(not naughty.is_suspended())
    assert(not naughty.suspended)

    -- Replace the text
    assert(n.message == "bar")
    assert(n.text    == "bar")
    assert(n.title   == "foo")
    naughty.replace_text(n, "foobar", "baz")
    assert(n.title   == "foobar")
    assert(n.message == "baz")
    assert(n.text    == "baz")

    -- Test the ID system
    n.id = 1337
    assert(n.id == 1337)
    assert(naughty.getById(1337) == n)
    assert(naughty.get_by_id(1337) == n)
    assert(naughty.getById(42) ~= n)
    assert(naughty.get_by_id(42) ~= n)

    -- The timeout
    naughty.reset_timeout(n, 1337)

    -- Destroy using the old API
    local old_count = #destroyed
    naughty.destroy(n)
    assert(old_count == #destroyed - 1)

    -- Destroy using the old API, while suspended
    local n2 = naughty.notification {
        title   = "foo",
        message = "bar",
        timeout = 25000,
    }
    naughty.suspended = true
    naughty.destroy(n2)
    assert(old_count == #destroyed - 2)
    naughty.suspended = false

    -- The old notify function and "text" instead of "message"
    naughty.notify { text = "foo" }

    -- Finish by testing disconnect_signal
    naughty.disconnect_signal("destroyed", destroyed_callback)
    naughty.disconnect_signal("added", added_callback)

    return true
end)

-- Test many screens.

require("_runner").run_steps(steps)
