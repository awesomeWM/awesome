---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2014 Uli Schlachter
---------------------------------------------------------------------------

local object = require("gears.object")
local matrix_equals = require("gears.matrix").equals
local base = require("wibox.widget.base")
local say = require("say")
local assert = require("luassert")
local no_parent = base.no_parent_I_know_what_I_am_doing

_G.awesome.api_level = 9999

-- {{{ Own widget-based assertions
local function widget_fit(state, arguments)
    if #arguments ~= 3 then
        error("Have " .. #arguments .. " arguments, but need 3")
    end

    local widget = arguments[1]
    local given = arguments[2]
    local expected = arguments[3]
    local w, h = base.fit_widget(no_parent, { "fake context" }, widget, given[1], given[2])

    local fits = expected[1] == w and expected[2] == h
    if state.mod == fits then
        return true
    end
    -- For proper error message, mess with the arguments
    arguments[1] = given[1]
    arguments[2] = given[2]
    arguments[3] = expected[1]
    arguments[4] = expected[2]
    arguments[5] = w
    arguments[6] = h
    return false
end
say:set("assertion.widget_fit.positive", "Offering (%s, %s) to widget and expected (%s, %s), but got (%s, %s)")
assert:register("assertion", "widget_fit", widget_fit, "assertion.widget_fit.positive", "assertion.widget_fit.positive")

local function draw_layout_comparison(actual, expected)
    local lines = {
        "Widget layout does not match:\n",
    }

    local left, right = {}, {}

    local function times(char, len)
        local chars = {}
        for _ = 1, len do
            table.insert(chars, char)
        end
        return table.concat(chars, "")
    end

    local function wrap_line(width, x, line)
        -- Two columns occupied by the `|` characters
        local buffer = width - 2 - #line
        local buffer_left = math.floor(buffer / 2)
        local buffer_right = buffer - buffer_left

        return string.format(
            "%s|%s%s%s|",
            times(" ", x), times(" ", buffer_left), line, times(" ", buffer_right)
        )
    end

    local function draw_widget(widget)
        local height = widget._height
        local width = widget._width
        local x = widget._matrix.x0
        -- Two lines removed for the horizontal lines,
        -- three for the lines with text
        local fixed = 5
        local buffer_top = math.floor((height - fixed) / 2)
        local buffer_bottom = height - fixed - buffer_top

        local widget_lines = {times("-", width)}
        for _ = 1, buffer_top do
            table.insert(widget_lines, wrap_line(width, x, " "))
        end

        table.insert(widget_lines, wrap_line(width, x, widget._name or "Widget"))
        table.insert(widget_lines, wrap_line(width, x, string.format(
            "Size %d,%d",
            widget._width, widget._height
        )))
        table.insert(widget_lines, wrap_line(width, x, string.format(
            "Pos %d,%d",
            widget._matrix.x0, widget._matrix.y0
        )))

        for _ = 1, buffer_bottom do
            table.insert(widget_lines, wrap_line(width, x, " "))
        end

        table.insert(widget_lines, times("-", width))

        return widget_lines
    end

    local max_len = 0

    for _, widget in ipairs(actual) do
        for _, line in ipairs(draw_widget(widget)) do
            max_len = math.max(max_len, #line)
            table.insert(left, line)
        end
    end

    for _, widget in ipairs(expected) do
        for _, line in ipairs(draw_widget(widget)) do
            table.insert(right, line)
        end
    end

    table.insert(lines, string.format(
        "%-"..max_len.."s   %-"..max_len.."s",
        "Actual:", "Expected:"
    ))

    for i = 1, math.max(#left, #right) do
        table.insert(lines, string.format("%-"..max_len.."s   %s", left[i] or "", right[i] or ""))
    end

    -- Force one additional newline. Due to how the error message has to be set
    -- up, we want to force `say`'s trailing quote into a new line.
    table.insert(lines, "")

    return table.concat(lines, "\n")
end

local function widget_layout(state, arguments)
    if #arguments ~= 3 then
        error("Have " .. #arguments .. " arguments, but need 3")
    end

    local widget = arguments[1]
    local width_actual, height_actual = arguments[2][1], arguments[2][2]
    local expected = arguments[3]

    if not widget.layout then
        arguments[1] = "Method `layout` is missing. Is this even a widget?"
        return false
    end

    local children = widget:layout({"fake context"}, width_actual, height_actual)

    if type(children) ~= "table" then
        arguments[1] = string.format(
            "Expected table from widget:layout(), got %s",
            type(children)
        )
        return
    end

    local num_children = #children

    local fits = num_children == #expected
    -- If the numbers don't match, we can skip checking them individually
    if fits then
        for i = 1, num_children do
            local child, exp = children[i], expected[i]
            if child._widget ~= exp._widget or
                child._width ~= exp._width or
                child._height ~= exp._height or
                not matrix_equals(child._matrix, exp._matrix) then
                fits = false
                break
            end
        end
    end

    if state.mod == fits then
        return true
    end

    arguments[1] = draw_layout_comparison(children, expected)
    return false
end

-- Hack the API by using the first argument as arbitrary string.
-- This gives actual flexibility in the error message.
say:set("assertion.widget_layout.positive", "%s")
assert:register("assertion",
                "widget_layout",
                widget_layout,
                "assertion.widget_layout.positive",
                "assertion.widget_layout.positive")
-- }}}

local function test_container(container)
    local w1 = base.empty_widget()

    assert.is.same({}, container:get_children())

    container:set_widget(w1)
    assert.is.same({ w1 }, container:get_children())

    container:set_widget(nil)
    assert.is.same({}, container:get_children())

    container:set_widget(w1)
    assert.is.same({ w1 }, container:get_children())

    if container.reset then
        container:reset()
        assert.is.same({}, container:get_children())
    end
end

return {
    widget_stub = function(width, height)
        local w = object()
        w._private = {}
        w.is_widget = true
        w._private.visible = true
        w._private.opacity = 1
        if width or height then
            w.fit = function()
                return width or 10, height or 10
            end
        end
        w._private.widget_caches = {}
        w:connect_signal("widget::layout_changed", function()
            -- TODO: This is not completely correct, since our parent's caches
            -- are not cleared. For the time being, tests just have to handle
            -- this clearing-part themselves.
            w._private.widget_caches = {}
        end)

        return w
    end,

    test_container = test_container,
}

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
