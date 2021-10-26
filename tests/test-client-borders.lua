-- Brute force every possible states and see what blows up.
--
-- The goal is to detect "other" code paths within the core
-- which have the side effect of accidentally marking the border
-- to be user-specified. When this happens, all theme variables
-- stop working.
--
-- For example, fullscreen caused an issue because the default
-- request::geometry modified the border_width.
local beautiful = require("beautiful")
local aclient = require("awful.client")
local test_client = require("_client")

local steps = {}
local used_colors = {}
local state_colors, state_widths = {}, {}
local state_setter = {}
local c = nil

-- Make sure it uses `beautiful.border_width` (default) again.
local function reset()
    client.focus = nil
    c.urgent     = false
    c.maximized  = false
    c.fullscreen = false

    -- Use the low level API because `floating` has an implicit/explicit
    -- mode just like the border.
    aclient.property.set(c, "floating", nil)
end

local function gen_random_color(state)
    while true do
        local str = "#"
        for _=1, 3 do
            local part = string.format("%x", math.ceil(math.random() * 255))
            part = #part == 1 and ("0"..part) or part
            str = str .. part
        end

        if not used_colors[str] then
            used_colors[str] = state
            return str
        end
    end
end

local function check_state(state)
    reset()

    if state_setter[state] then
        local col = "border_color" .. state
        local w = "border_width" .. state
        state_setter[state](client.get()[1])

        assert(c.border_color ~= nil)
        assert(c.border_width ~= nil)
        assert(
            c.border_color == state_colors[col],
            "Expected "..state_colors[col].." for "..state.. " but got "..c.border_color..
            " for "..(used_colors[c.border_color] or "nil")
        )

        if not state:find("full") then
            assert(c.border_width == state_widths[w])
        else
            assert(c.border_width == 0)
        end

        assert(not c._private._user_border_width)
        assert(not c._private._user_border_color)
    end

    return true
end

local function clear_theme()
    for k in pairs(beautiful) do
        if k:find("border_") then
            beautiful[k] = nil
        end
    end

    return true
end

for _, state1 in ipairs {"_fullscreen", "_maximized", "_floating" } do
    local color = "border_color" .. state1
    local width = "border_width" .. state1

    state_widths[width] = math.floor(math.random() * 10)
    state_colors[color] = gen_random_color(color)

    for _, state2 in ipairs {"_urgent", "_active", "_new", "_normal" } do
        color = "border_color" .. state1 .. state2
        width = "border_width" .. state1 .. state2

        state_widths[width] = math.floor(math.random() * 10)
        state_colors[color] = gen_random_color(color)
    end
end

for _, state in ipairs {"_urgent", "_active", "_new", "_normal" } do
    local color = "border_color" .. state
    local width = "border_width" .. state

    state_widths[width] = math.floor(math.random() * 10)
    state_colors[color] = gen_random_color(color)
    beautiful[width] = state_widths[width]
    beautiful[color] = state_colors[color]
end

function state_setter._urgent()
    c.urgent = true
end

function state_setter._fullscreen()
    c.fullscreen = true
end

function state_setter._floating()
    c.floating = true
end

function state_setter._active()
    client.focus = c
end

function state_setter._maximized()
    c.maximized = true
end

function state_setter._normal()
    -- no-op
end

test_client()

table.insert(steps, function()
    c = client.get()[1]
    return c and true or nil
end)

for _, state in ipairs {"_urgent", "_active", "_new", "_normal" } do
    table.insert(steps, function()
        return check_state(state)
    end)
end

table.insert(steps, clear_theme)

table.insert(steps, function()
    for _, state in ipairs {"_fullscreen", "_maximized", "_floating" } do
        local color = "border_color" .. state
        local width = "border_width" .. state

        beautiful[width] = state_widths[width]
        beautiful[color] = state_colors[color]
    end

    return true
end)

for _, state in ipairs {"_fullscreen", "_maximized", "_floating" } do
    table.insert(steps, function()
        return check_state(state)
    end)
end

table.insert(steps, clear_theme)

table.insert(steps, function()
    -- Add everything to the theme.
    for _, mode in ipairs {state_colors, state_widths} do
        for key, value in pairs(mode) do
            beautiful[key] = value
        end
    end
    return true
end)

for _, state1 in ipairs {"_fullscreen", "_maximized", "_floating" } do
    for _, state2 in ipairs {"_urgent", "_active", "_new", "_normal" } do
        table.insert(steps, function()
            return check_state(state1 .. state2)
        end)
    end
end

require("_runner").run_steps(steps)
