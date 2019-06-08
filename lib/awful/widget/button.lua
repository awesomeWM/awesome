---------------------------------------------------------------------------
-- A simple button widget.
-- @usage local button = awful.widget.button()
-- button:buttons(gears.table.join(
--     button:buttons(),
--     awful.button({}, 1, nil, function ()
--         print("Mouse was clicked")
--     end)
-- ))
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008-2009 Julien Danjou
-- @widgetmod awful.widget.button
---------------------------------------------------------------------------

local setmetatable = setmetatable
local abutton = require("awful.button")
local imagebox = require("wibox.widget.imagebox")
local widget = require("wibox.widget.base")
local surface = require("gears.surface")
local cairo = require("lgi").cairo

local button = { mt = {} }

--- Create a button widget. When clicked, the image is deplaced to make it like
-- a real button.
--
-- @constructorfct awful.widget.button
-- @param args Widget arguments. "image" is the image to display.
-- @return A textbox widget configured as a button.
function button.new(args)
    if not args or not args.image then
        return widget.empty_widget()
    end

    local w = imagebox()
    local orig_set_image = w.set_image
    local img_release
    local img_press

    function w:set_image(image)
        img_release = surface.load(image)
        img_press = img_release:create_similar(cairo.Content.COLOR_ALPHA, img_release.width, img_release.height)
        local cr = cairo.Context(img_press)
        cr:set_source_surface(img_release, 2, 2)
        cr:paint()
        orig_set_image(self, img_release)
    end
    w:set_image(args.image)
    w:buttons(abutton({}, 1, function () orig_set_image(w, img_press) end,
                             function () orig_set_image(w, img_release) end))

    w:connect_signal("mouse::leave", function(self) orig_set_image(self, img_release) end)

    return w
end

function button.mt:__call(...)
    return button.new(...)
end

--@DOC_widget_COMMON@

--@DOC_object_COMMON@

return setmetatable(button, button.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
