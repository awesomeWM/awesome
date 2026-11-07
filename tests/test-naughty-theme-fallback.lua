--- Test that the `beautiful.notification_*` theme variables are honored.
-- Regression test for #3786: keys present in `naughty.config.defaults` used to
-- shadow the theme because the property getter never consulted `beautiful`.

local naughty      = require("naughty")
local notification = require("naughty.notification")
local background   = require("naughty.container.background")
local beautiful    = require("beautiful")
local gshape       = require("gears.shape")
local gcolor       = require("gears.color")
local wibox        = require("wibox")

require("ruled.notification"):_clear()

-- Every property documented with a `beautiful.notification_*` fallback, along
-- with a value to set the theme variable to. Keep in sync with the
-- `beautiful_fallback` table of `naughty.notification`.
local themed = {
    { "border_width", 7             },
    { "margin"      , 11            },
    { "position"    , "bottom_left" },
    { "width"       , 321           },
    { "height"      , 123           },
    { "max_width"   , 345           },
    { "icon_size"   , 27            },
    { "opacity"     , 0.5           },
    { "font"        , "sans 11"     },
    { "fg"          , "#123456"     },
    { "bg"          , "#654321"     },
    { "border_color", "#abcdef"     },
    { "shape"       , gshape.octogon},
}

local function set_theme(value)
    for _, prop in ipairs(themed) do
        beautiful["notification_"..prop[1]] = value and prop[2] or nil
    end
end

local steps = {}

-- Run every scenario on both the legacy preset path and the ruled path.
for _, has_handler in ipairs {false, true} do
    -- Every theme variable must win over the built-in defaults.
    table.insert(steps, function()
        function naughty.get__has_preset_handler()
            return has_handler
        end

        set_theme(true)

        local n = notification { title = "t", message = "m", timeout = 0 }

        for _, prop in ipairs(themed) do
            assert(n[prop[1]] == prop[2], "theme " .. prop[1]
                .. " not applied: " .. tostring(n[prop[1]]))
        end

        -- The rendered widget must see it too.
        assert(background { notification = n }.border_width == 7)

        n:destroy()
        set_theme(false)

        return true
    end)

    -- An explicit value takes precedence over the theme.
    table.insert(steps, function()
        beautiful.notification_border_width = 7

        local n = notification {
            title = "t", message = "m", border_width = 3, timeout = 0
        }

        assert(n.border_width == 3,
            "explicit border_width ignored: " .. tostring(n.border_width))

        n:destroy()
        beautiful.notification_border_width = nil

        return true
    end)

    -- A preset also takes precedence over the theme.
    table.insert(steps, function()
        beautiful.notification_border_width = 7

        local n = notification {
            title = "t", message = "m", timeout = 0,
            preset = { border_width = 4 },
        }

        assert(n.border_width == 4,
            "preset border_width ignored: " .. tostring(n.border_width))

        n:destroy()
        beautiful.notification_border_width = nil

        return true
    end)

    -- The theme takes precedence over `naughty.config.defaults`. This pins the
    -- documented fallback order (see the `border_width` property doc).
    table.insert(steps, function()
        local old = naughty.config.defaults.border_width
        naughty.config.defaults.border_width = 9
        beautiful.notification_border_width  = 7

        local n = notification { title = "t", message = "m", timeout = 0 }

        assert(n.border_width == 7,
            "theme border_width shadowed by config.defaults: "
                .. tostring(n.border_width))

        n:destroy()
        beautiful.notification_border_width = nil

        n = notification { title = "t", message = "m", timeout = 0 }

        assert(n.border_width == 9,
            "config.defaults border_width not applied: "
                .. tostring(n.border_width))

        n:destroy()
        naughty.config.defaults.border_width = old

        return true
    end)

    -- With neither a theme nor an explicit value, the defaults are preserved.
    table.insert(steps, function()
        local n = notification { title = "t", message = "m", timeout = 0 }

        assert(n.border_width == naughty.config.defaults.border_width,
            "default border_width not preserved: " .. tostring(n.border_width))
        assert(n.margin == naughty.config.defaults.margin,
            "default margin not preserved: " .. tostring(n.margin))
        assert(n.position == "top_right",
            "default position not preserved: " .. tostring(n.position))

        n:destroy()

        return true
    end)
end

-- Changing the theme while a notification is on screen must not corrupt the
-- position index. `position` now falls back to the theme, and `beautiful`
-- changing emits no `property::position` for `naughty.core` to re-index with.
table.insert(steps, function()
    beautiful.notification_position = "top_right"

    local n = notification { title = "t", message = "m", timeout = 0 }

    beautiful.notification_position = "bottom_right"

    n:destroy()
    beautiful.notification_position = nil

    return true
end)

-- `naughty.config.defaults.screen` has no property getter to fall back to, so
-- it has to keep working on its own.
table.insert(steps, function()
    function naughty.get__has_preset_handler()
        return false
    end

    -- A second screen is needed, otherwise the default is indistinguishable
    -- from the focused screen the notification would have picked anyway.
    if screen.count() == 1 then
        screen[1]:split()
    end

    assert(screen.count() > 1)

    -- Pick the screen the notification would *not* have landed on by itself.
    local target = mouse.screen == screen[1] and 2 or 1

    naughty.config.defaults.screen = target

    local n = notification { title = "t", message = "m", timeout = 0 }

    assert(n.screen == screen[target],
        "config.defaults.screen ignored: " .. tostring(n.screen))

    n:destroy()
    naughty.config.defaults.screen = nil

    return true
end)

-- `naughty.list.notifications` has its own, more specific, `_normal`/`_selected`
-- theme variables. Now that the property getters fall back to the global
-- `beautiful.notification_*`, that global value must not silently override them.
local list_layout, current

local function new_list()
    list_layout = wibox.layout.fixed.vertical()
    naughty.list.notifications { base_layout = list_layout }
end

-- The bg the list actually painted, as set by `awful.widget.common.list_update`.
local function list_bg()
    local kids = list_layout:get_children()

    if #kids == 0 then return nil end

    local bgb = kids[1]:get_children_by_id("background_role")[1]

    return bgb and bgb.bg
end

-- The list needs its entries registered, and the assertions need a main loop
-- turn after the notification is created, hence the step pairs below.
--
-- `rc.lua` already connected a `request::display` handler, and its boxes are
-- only weakly referenced, so their garbage collection would make the position
-- of the boxes created here unpredictable. Replace it instead of adding a
-- second box per notification.
local last_box

naughty._reset_display_handlers()

naughty.connect_signal("request::display", function(n)
    last_box = naughty.layout.box { notification = n }
end)

-- The list-specific variable wins over the global one.
table.insert(steps, function()
    beautiful.notification_bg        = "#111111"
    beautiful.notification_bg_normal = "#222222"

    new_list()
    current = notification { title = "t", message = "m", timeout = 0 }

    return true
end)

table.insert(steps, function()
    assert(list_bg() == gcolor("#222222"),
        "notification_bg_normal was overridden by the global notification_bg")

    current:destroy()

    return true
end)

-- A value set on the notification itself still wins over both.
table.insert(steps, function()
    new_list()
    current = notification {
        title = "t", message = "m", timeout = 0, bg = "#333333"
    }

    return true
end)

table.insert(steps, function()
    assert(list_bg() == gcolor("#333333"),
        "explicit bg ignored by the notification list")

    current:destroy()
    beautiful.notification_bg_normal = nil

    return true
end)

-- With no list-specific variable, the global theme still reaches the list.
table.insert(steps, function()
    new_list()
    current = notification { title = "t", message = "m", timeout = 0 }

    return true
end)

table.insert(steps, function()
    assert(list_bg() == gcolor("#111111"),
        "global notification_bg did not reach the list")

    current:destroy()
    beautiful.notification_bg = nil

    return true
end)

-- A theme position change while a notification is displayed must not leave its
-- box behind in `naughty.layout.box`'s position index. A leftover would anchor
-- the next box at the old position below it instead of at the screen edge.
local stale_box

table.insert(steps, function()
    beautiful.notification_position = "top_right"
    current = notification { title = "t", message = "m", timeout = 0 }

    return true
end)

table.insert(steps, function()
    stale_box = last_box

    beautiful.notification_position = "bottom_right"
    current:destroy()
    beautiful.notification_position = nil

    return true
end)

table.insert(steps, function()
    current = notification { title = "t", message = "m", timeout = 0 }

    return true
end)

table.insert(steps, function()
    assert(last_box ~= stale_box)
    assert(last_box.y == stale_box.y,
        "box left in the old position index after a theme position change: y="
        .. last_box.y .. ", expected " .. stale_box.y)

    current:destroy()
    stale_box = nil

    return true
end)

require("_runner").run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
