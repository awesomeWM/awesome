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
local spy = require("luassert.spy")
local stub = require("luassert.stub")

local real_draw_widget = base.draw_widget
local widgets_drawn = nil

-- This function would reject stubbed widgets
local real_check_widget = base.check_widget
base.check_widget = function()
end

local function stub_draw_widget(wibox, cr, widget, x, y, width, height)
    assert.is.equal("wibox", wibox)
    assert.is.equal("cr", cr)
    table.insert(widgets_drawn, { widget, x, y, width, height })
end

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
    real_check_widget = real_check_widget,

    widget_stub = function(width, height)
        local w = object()
        w.visible = true
        w:add_signal("widget::updated")

        w.fit = function()
            return width or 10, height or 10
        end
        w.draw = function() end
        w._fit_geometry_cache = cache.new(w.fit)

        spy.on(w, "fit")
        stub(w, "draw")

        return w
    end,

    stub_draw_widget = function()
        base.draw_widget = stub_draw_widget
        widgets_drawn = {}
    end,

    revert_draw_widget = function()
        base.draw_widget = real_draw_widget
        widgets_drawn = nil
    end,

    check_widgets_drawn = function(expected)
        assert.is.equals(#expected, #widgets_drawn)
        for k, v in pairs(expected) do
            -- widget, x, y, width, height
            -- Compared like this so we get slightly less bad error messages
            assert.is.equals(expected[k][1], widgets_drawn[k][1])
            assert.is.equals(expected[k][2], widgets_drawn[k][2])
            assert.is.equals(expected[k][3], widgets_drawn[k][3])
            assert.is.equals(expected[k][4], widgets_drawn[k][4])
            assert.is.equals(expected[k][5], widgets_drawn[k][5])
        end
        widgets_drawn = {}
    end
}

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
