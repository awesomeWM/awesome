---------------------------------------------------------------------------
--- Display the current client layout (`awful.layout`) icon or name.
--
-- @DOC_awful_widget_layoutbox_default_EXAMPLE@
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Julien Danjou
-- @widgetmod awful.widget.layoutbox
-- @supermodule wibox.widget.template
---------------------------------------------------------------------------

local setmetatable = setmetatable
local capi = { screen = screen, tag = tag }
local layout = require("awful.layout")
local tooltip = require("awful.tooltip")
local beautiful = require("beautiful")
local wibox = require("wibox")
local surface = require("gears.surface")
local gdebug = require("gears.debug")
local gtable = require("gears.table")

local function get_screen(s)
    return s and capi.screen[s] or 1
end

local default_template = {
    {
        {
            id = "icon_role",
            widget = wibox.widget.imagebox,
        },
        {
            id = "text_role",
            widget = wibox.widget.imagebox,
        },
        fill_space = true,
        layout = wibox.layout.fixed.horizontal,
    },
    id = "background_role",
    widget = wibox.container.background,
}

local function default_update_callback(template_widget, args)
    screen = get_screen(args.screen)
    local w = template_widget.widget
    local name = layout.getname(layout.get(screen))

    template_widget._layoutbox_tooltip:set_text(name or "[no name]")

    local img = surface.load_silently(beautiful["layout_" .. name], false)
    w:get_children_by_id("icon_role")[1].image = img
    w:get_children_by_id("text_role")[1].text = img and "" or name
end

local function build_default_widget()
    local widget = wibox.widget.template {
        template = default_template,
        update_callback = default_update_callback,
        update_now = true,
    }

    widget._layoutbox_tooltip = tooltip {
        objects = { widget },
        delay_show = 1,
    }

    return widget
end

local layoutbox = { mt = {} }

local boxes = nil

local function update_from_tag(t)
    local screen = get_screen(t.screen)
    local w = boxes[screen]
    if w then
        w:update { screen = screen }
    end
end

--- Create a layoutbox widget. It draws a picture with the current layout
-- symbol of the current tag.
-- @tparam table args The arguments.
-- @tparam screen args.screen The screen number that the layout will be represented for.
-- @tparam table args.buttons The `awful.button`s for this layoutbox.
-- @tparam[opt] table args.widget_template Widget template arguments.
-- @tparam[opt] widget args.widget_template.template Base widget for the
--   template.
-- @tparam[opt] function args.widget_template.update_callback Update function
--   for the widget template.
-- @return The layoutbox.
function layoutbox.new(args)
    args = args or {}
    local screen = args.screen

    if type(args) == "number" or type(args) == "screen" or args.fake_remove then
        screen, args = args, {}

        gdebug.deprecate(
            "Use awful.widget.layoutbox{screen=s} instead of awful.widget.layoutbox(screen)",
            {deprecated_in=5}
        )
    end

    assert(type(args) == "table")

    screen = get_screen(screen or 1)

    -- Do we already have the update callbacks registered?
    if boxes == nil then
        boxes = setmetatable({}, { __mode = "kv" })
        capi.tag.connect_signal("property::selected", update_from_tag)
        capi.tag.connect_signal("property::layout", update_from_tag)
        capi.tag.connect_signal("property::screen", function()
            for s, w in pairs(boxes) do
                if s.valid then
                    w:update { screen = s }
                end
            end
        end)
        layoutbox.boxes = boxes
    end

    -- Do we already have a layoutbox for this screen?
    local w = boxes[screen]
    if not w then
        w = args.widget_template and wibox.widget.template(args.widget_template)
            or build_default_widget()

        -- Apply the buttons, visible, forced_width and so on
        gtable.crush(w, args)

        w:update { screen = screen }
        boxes[screen] = w
    end

    return w
end

function layoutbox.mt:__call(...)
    return layoutbox.new(...)
end

--@DOC_widget_COMMON@

--@DOC_object_COMMON@

return setmetatable(layoutbox, layoutbox.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
