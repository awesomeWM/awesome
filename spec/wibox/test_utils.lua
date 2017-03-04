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

    -- For proper error message, mess with the arguments
    arguments[1] = expected
    arguments[2] = children
    arguments[3] = given[1]
    arguments[4] = given[2]

    return false
end
say:set("assertion.widget_layout.positive", "Expected:\n%s\nbut got:\n%s\nwhen offering (%s, %s) to widget")
assert:register("assertion",
                "widget_layout",
                widget_layout,
                "assertion.widget_layout.positive",
                "assertion.widget_layout.positive")
-- }}}

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

        return w
    end,
}

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
