---------------------------------------------------------------------------
-- @author Emmanuel Lepage Vallee
-- @copyright 2017 Emmanuel Lepage Vallee
---------------------------------------------------------------------------

local imagebox = require("wibox.widget.imagebox")
local cairo = require("lgi").cairo

describe("wibox.widget.imagebox", function()
    local widget
    before_each(function()
        widget = imagebox()
    end)

    describe("setting image", function()
        local redraw_needed, layout_changed
        before_each(function()
            widget:connect_signal("widget::redraw_needed", function()
                redraw_needed = redraw_needed + 1
            end)
            widget:connect_signal("widget::layout_changed", function()
                layout_changed = layout_changed + 1
            end)
            redraw_needed, layout_changed = 0, 0
        end)

        it("set_image", function()
            assert.is.equal(widget._private.image, nil)
            assert.is.equal(0, redraw_needed)
            assert.is.equal(0, layout_changed)

            local img = cairo.ImageSurface(cairo.Format.ARGB32, 20, 20);
            widget:set_image(img)
            assert.is.equal(widget._private.image, img)
            assert.is.equal(1, redraw_needed)
            assert.is.equal(1, layout_changed)

            widget:set_image(nil)
            assert.is.equal(widget._private.image, nil)
            assert.is.equal(2, redraw_needed)
            assert.is.equal(2, layout_changed)
        end)
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
