---------------------------------------------------------------------------
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008-2009 Julien Danjou
-- @release @AWESOME_VERSION@
-- @classmod awful.widget.common
---------------------------------------------------------------------------

-- Grab environment we need
local math = math
local type = type
local ipairs = ipairs
local pairs = pairs
local pcall = pcall
local setmetatable = setmetatable
local capi = { button = button }
local util = require("awful.util")
local wibox = require("wibox")
local imagebox = require("wibox.widget.imagebox")
local textbox = require("wibox.widget.textbox")
local dpi = require("beautiful").xresources.apply_dpi

--- Common utilities for awful widgets
local common = {}

--- Common method to create buttons.
-- @tab buttons
-- @param object
-- @treturn table
function common.create_buttons(buttons, object)
    if buttons then
        local btns = {}
        for kb, b in ipairs(buttons) do
            -- Create a proxy button object: it will receive the real
            -- press and release events, and will propagate them the the
            -- button object the user provided, but with the object as
            -- argument.
            local btn = capi.button { modifiers = b.modifiers, button = b.button }
            btn:connect_signal("press", function () b:emit_signal("press", object) end)
            btn:connect_signal("release", function () b:emit_signal("release", object) end)
            btns[#btns + 1] = btn
        end

        return btns
    end
end

--- Common update method.
-- @param w The widget.
-- @tab buttons
-- @func label Function to generate label parameters from an object.
--   The function gets passed an object from `objects`, and
--   has to return `text`, `bg`, `bg_image`, `icon`.
-- @tab data Current data/cache, indexed by objects.
-- @tab objects Objects to be displayed / updated.
function common.list_update(w, buttons, label, data, objects)
    -- update the widgets, creating them if needed
    w:reset()
    for i, o in ipairs(objects) do
        local cache = data[o]
        local ib, tb, bgb, tbm, ibm, l
        if cache then
            ib = cache.ib
            tb = cache.tb
            bgb = cache.bgb
            tbm = cache.tbm
            ibm = cache.ibm
        else
            ib = wibox.widget.imagebox()
            tb = wibox.widget.textbox()
            bgb = wibox.widget.background()
            tbm = wibox.layout.margin(tb, dpi(4), dpi(4))
            ibm = wibox.layout.margin(ib, dpi(4))
            l = wibox.layout.fixed.horizontal()

            -- All of this is added in a fixed widget
            l:fill_space(true)
            l:add(ibm)
            l:add(tbm)

            -- And all of this gets a background
            bgb:set_widget(l)

            bgb:buttons(common.create_buttons(buttons, o))

            data[o] = {
                ib  = ib,
                tb  = tb,
                bgb = bgb,
                tbm = tbm,
                ibm = ibm,
            }
        end

        local text, bg, bg_image, icon = label(o, tb)
        -- The text might be invalid, so use pcall.
        if text == nil or text == "" then
            tbm:set_margins(0)
        else
            if not pcall(tb.set_markup, tb, text) then
                tb:set_markup("<i>&lt;Invalid text&gt;</i>")
            end
        end
        bgb:set_bg(bg)
        if type(bg_image) == "function" then
            bg_image = bg_image(tb,o,m,objects,i)
        end
        bgb:set_bgimage(bg_image)
        if icon then
            ib:set_image(icon)
        else
            ibm:set_margins(0)
        end
        w:add(bgb)
   end
end

return common

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
