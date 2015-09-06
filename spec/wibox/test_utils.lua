---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2014 Uli Schlachter
---------------------------------------------------------------------------

local object = require("gears.object")
local cache = require("gears.cache")
local matrix_equals = require("gears.matrix").equals
local base = require("wibox.widget.base")
local say = require("say")
local assert = require("luassert")

-- {{{ Own widget-based assertions
local function widget_fit(state, arguments)
    if #arguments ~= 3 then
        error("Have " .. #arguments .. " arguments, but need 3")
    end

    local widget = arguments[1]
    local given = arguments[2]
    local expected = arguments[3]
    local w, h = base.fit_widget({ "fake context" }, widget, given[1], given[2])

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

local function widget_layout(state, arguments)
    if #arguments ~= 3 then
        error("Have " .. #arguments .. " arguments, but need 3")
    end

    local widget = arguments[1]
    local given = arguments[2]
    local expected = arguments[3]
    local children = widget.layout and widget:layout({ "fake context" }, given[1], given[2]) or {}

    local fits = true
    if #children ~= #expected then
        fits = false
    else
        for i = 1, #children do
            local child, expected = children[i], expected[i]
            if child._widget ~= expected._widget or
                child._width ~= expected._width or
                child._height ~= expected._height or
                not matrix_equals(child._matrix, expected._matrix) then
                fits = false
                break
            end
        end
    end

    if state.mod == fits then
        return true
    end

    -- For proper error message, mess with the arguments
    arguments[1] = expected
    arguments[2] = children
    arguments[3] = given[1]
    arguments[4] = given[2]

    return false
end
say:set("assertion.widget_layout.positive", "Expected:\n%s\nbut got:\n%s\nwhen offering (%s, %s) to widget")
assert:register("assertion", "widget_layout", widget_layout, "assertion.widget_layout.positive", "assertion.widget_layout.positive")
-- }}}

return {
    widget_stub = function(width, height)
        local w = object()
        w:add_signal("widget::redraw_needed")
        w:add_signal("widget::layout_changed")
        w.visible = true
        w.opacity = 1
        if width or height then
            w.fit = function()
                return width or 10, height or 10
            end
        end
        w._widget_caches = {}

        return w
    end,
}

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
