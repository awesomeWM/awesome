---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2020 Uli Schlachter
---------------------------------------------------------------------------

local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)

local systray_widget = nil
local drawable_mock = {
    _set_systray_widget = function(widget)
        assert(systray_widget == nil)
        assert(widget ~= nil)
        systray_widget = widget
    end
}
local beautiful_mock = {}
package.loaded["wibox.drawable"] = drawable_mock
package.loaded.beautiful = beautiful_mock

local num_systray_icons
local systray_arguments
_G.awesome = {
    connect_signal = function() end,
    systray = function(first_arg, ...)
        if first_arg then
            assert(systray_arguments == nil) -- May only be called once
            systray_arguments = { first_arg, ... }
        end
        return num_systray_icons
    end
}
_G.screen = {
    connect_signal = function() end
}

local systray = require("wibox.widget.systray")

require("wibox.test_utils") -- For assert.widget_fit
require("wibox.widget.base").rect_to_device_geometry = function() return 0, 0, 0, 0 end

describe("wibox.widget.systray", function()
    local widget = nil
    before_each(function()
        widget = systray()
        assert(widget == systray_widget)
    end)

    local context = { wibox = { drawin = true } }
    local cr = { user_to_device_distance = function() return 1, 0 end }

    local function test_systray(available_size, expected_size, expected_base)
        systray_arguments = nil
        local spacing = beautiful_mock.systray_icon_spacing or 0

        assert.widget_fit(widget, available_size, expected_size)
        widget:draw(context, cr, unpack(available_size))
        assert.is.near(systray_arguments[4], expected_base, 0.000001)
        assert.is.same(systray_arguments, {true, 0, 0, systray_arguments[4], true, '#000000', false, spacing})
    end

    describe("no spacing", function()
        it("no icons", function()
            num_systray_icons = 0
            test_systray({ 100, 10 }, { 0, 0 }, 10)
            test_systray({ 100, 100 }, { 0, 0 }, 100)
        end)

        it("one icon", function()
            num_systray_icons = 1
            test_systray({ 100, 10 }, { 10, 10 }, 10)
            test_systray({ 100, 100 }, { 100, 100 }, 100)
        end)

        it("two icons", function()
            num_systray_icons = 2
            test_systray({ 100, 10 }, { 20, 10 }, 10)
            test_systray({ 100, 100 }, { 100, 100 }, 100 / 2)
        end)

        it("three icons", function()
            num_systray_icons = 3
            test_systray({ 100, 10 }, { 30, 10 }, 10)
            test_systray({ 100, 100 }, { 100, 100 }, 100 / 3)
        end)
    end)

    describe("10 spacing", function()
        beautiful_mock.systray_icon_spacing = 10

        it("no icons", function()
            num_systray_icons = 0
            test_systray({ 100, 10 }, { 0, 0 }, 10)
            test_systray({ 100, 100 }, { 0, 0 }, 100)
        end)

        it("one icon", function()
            num_systray_icons = 1
            test_systray({ 100, 10 }, { 10, 10 }, 10)
            test_systray({ 100, 100 }, { 100, 100 }, 100)
        end)

        it("two icons", function()
            num_systray_icons = 2
            test_systray({ 100, 10 }, { 30, 10 }, 10)
            -- Okay, so we want to place to icons next to each other in a square
            -- of size 100x100. Between them, there should be 10 pixels of
            -- space. So, we got 90 pixels for the icons, so 45 px per icon.
            test_systray({ 100, 100 }, { 100, 100 }, (100 - 10) / 2)
        end)

        it("three icons", function()
            num_systray_icons = 3
            test_systray({ 100, 10 }, { 50, 10 }, 10)
            test_systray({ 100, 100 }, { 100, 100 }, (100 - 2 * 10) / 3)
        end)
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
