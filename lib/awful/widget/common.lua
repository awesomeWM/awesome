---------------------------------------------------------------------------
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008-2009 Julien Danjou
-- @classmod awful.widget.common
---------------------------------------------------------------------------

-- Grab environment we need
local type = type
local ipairs = ipairs
local capi = { button = button }
local wibox = require("wibox")
local gdebug = require("gears.debug")
local dpi = require("beautiful").xresources.apply_dpi
local base = require("wibox.widget.base")

--- Common utilities for awful widgets
local common = {}

--- Common method to create buttons.
-- @tab buttons
-- @param object
-- @treturn table
function common.create_buttons(buttons, object)
    if buttons then
        local btns = {}
        for _, b in ipairs(buttons) do
            -- Create a proxy button object: it will receive the real
            -- press and release events, and will propagate them to the
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

local function default_template()
    local ib  = wibox.widget.imagebox()
    local tb  = wibox.widget.textbox()
    local bgb = wibox.container.background()
    local tbm = wibox.container.margin(tb, dpi(4), dpi(4))
    local ibm = wibox.container.margin(ib, dpi(4))
    local l   = wibox.layout.fixed.horizontal()

    -- All of this is added in a fixed widget
    l:fill_space(true)
    l:add(ibm)
    l:add(tbm)

    -- And all of this gets a background
    bgb:set_widget(l)

    return {
        ib      = ib,
        tb      = tb,
        bgb     = bgb,
        tbm     = tbm,
        ibm     = ibm,
        primary = bgb,
    }
end

local function custom_template(args)
    local l = base.make_widget_from_value(args.widget_template)

    -- The template system requires being able to get children elements by ids.
    -- This is not optimal, but for now there is no way around it.
    assert(l.get_children_by_id,"The given widget template did not result in a"..
        "layout with a 'get_children_by_id' method")

    return {
        ib              = l:get_children_by_id( "icon_role"        )[1],
        tb              = l:get_children_by_id( "text_role"        )[1],
        bgb             = l:get_children_by_id( "background_role"  )[1],
        tbm             = l:get_children_by_id( "text_margin_role" )[1],
        ibm             = l:get_children_by_id( "icon_margin_role" )[1],
        primary         = l,
        update_callback = l.update_callback,
        create_callback = l.create_callback,
    }
end

--- Common update method.
-- @param w The widget.
-- @tab buttons
-- @func label Function to generate label parameters from an object.
--   The function gets passed an object from `objects`, and
--   has to return `text`, `bg`, `bg_image`, `icon`.
-- @tab data Current data/cache, indexed by objects.
-- @tab objects Objects to be displayed / updated.
-- @tparam[opt={}] table args
function common.list_update(w, buttons, label, data, objects, args)
    -- update the widgets, creating them if needed
    w:reset()
    for i, o in ipairs(objects) do
        local cache = data[o]

        if not cache then
            cache = (args and args.widget_template) and
                custom_template(args) or default_template()

            cache.primary:buttons(common.create_buttons(buttons, o))

            if cache.create_callback then
                cache.create_callback(cache.primary, o, i, objects)
            end

            data[o] = cache
        elseif cache.update_callback then
            cache.update_callback(cache.primary, o, i, objects)
        end

        local text, bg, bg_image, icon, item_args = label(o, cache.tb)
        item_args = item_args or {}

        -- The text might be invalid, so use pcall.
        if cache.tbm and (text == nil or text == "") then
            cache.tbm:set_margins(0)
        elseif cache.tb then
            if not cache.tb:set_markup_silently(text) then
                cache.tb:set_markup("<i>&lt;Invalid text&gt;</i>")
            end
        end

        if cache.bgb then
            cache.bgb:set_bg(bg)

            --TODO v5 remove this if, it existed only for a removed and
            -- undocumented API
            if type(bg_image) ~= "function" then
                cache.bgb:set_bgimage(bg_image)
            else
                gdebug.deprecate("If you read this, you used an undocumented API"..
                    " which has been replaced by the new awful.widget.common "..
                    "templating system, please migrate now. This feature is "..
                    "already staged for removal", {
                    deprecated_in = 4
                })
            end

            cache.bgb.shape              = item_args.shape
            cache.bgb.shape_border_width = item_args.shape_border_width
            cache.bgb.shape_border_color = item_args.shape_border_color

        end

        if cache.ib and icon then
            cache.ib:set_image(icon)
        elseif cache.ibm then
            cache.ibm:set_margins(0)
        end

        w:add(cache.primary)
   end
end

return common

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
