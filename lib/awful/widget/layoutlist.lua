----------------------------------------------------------------------------
--- Display the available client layouts for a screen.
--
-- This is what the layoutlist looks like by default with a vertical layout:
--
--@DOC_awful_widget_layoutlist_default_EXAMPLE@
--
-- In the second example, it is shown how to create a popup in centered in
-- the screen:
--
--@DOC_awful_widget_layoutlist_popup_EXAMPLE@
--
-- This example show how to extend the `awful.widget.layoutbox` to show a layout
-- list popup:
--
--@DOC_awful_widget_layoutlist_bar_EXAMPLE@
--
-- This example show how to add a layout subset to the default wibar:
--
--@DOC_awful_widget_layoutlist_wibar_EXAMPLE@
--
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2010, 2018 Emmanuel Lepage Vallee
-- @classmod awful.widget.layoutlist
----------------------------------------------------------------------------

local wibox    = require("wibox")
local awcommon = require("awful.widget.common")
local abutton  = require("awful.button")
local gtable   = require("gears.table")
local beautiful= require("beautiful")
local alayout  = require("awful.layout")
local surface  = require("gears.surface")

local module = {}

local default_buttons = gtable.join(
    abutton({ }, 1, function(a) a.callback() end)
)

local function wb_label(item, _, tb)
    local selected = alayout.get(item.screen) == item.layout

    -- Apply the built-in customization
    local bg, fg, shape, shape_bw, shape_bc

    -- The layout have only 2 states: normal and selected
    if selected then
        bg = item.style.bg_selected or
            beautiful.layoutlist_bg_selected or
            beautiful.bg_focus
        fg = item.style.fg_selected or
            beautiful.layoutlist_fg_selected or
            beautiful.fg_focus or beautiful.fg_normal
        shape = item.style.shape_selected or
            beautiful.layoutlist_shape_selected
        shape_bw = item.style.shape_border_width_selected or
            beautiful.layoutlist_shape_border_width_selected
        shape_bc = item.style.shape_border_color_selected or
            beautiful.layoutlist_shape_border_color_selected
    else
        bg = item.style.bg_normal or
            beautiful.layoutlist_bg_normal or
            nil
        fg = item.style.fg_normal or
            beautiful.layoutlist_fg_normal or
            beautiful.fg_normal
        shape = item.style.shape or
            beautiful.layoutlist_shape
        shape_bw = item.style.shape_border_width or
            beautiful.layoutlist_shape_border_width
        shape_bc = item.style.shape_border_color or
            beautiful.layoutlist_shape_border_color
    end

    if tb and item.style.align or beautiful.layoutlist_align then
        tb:set_align(item.style.align or beautiful.layoutlist_align)
    end

    local text = ""

    if item.name then
        text = "<span color='"..fg.."'>"..item.name..'</span>'
    end

    return text, bg, nil, item.icon, {
        shape               = shape,
        shape_border_width  = shape_bw,
        shape_border_color  = shape_bc,
    }
end

local function reload_cache(self)
    self._private.cache = {}

    local show_text = (not self._private.style.disable_name) and
        (not beautiful.layoutlist_disable_name)

    local show_icon = (not self._private.style.disable_icon) and
        (not beautiful.layoutlist_disable_icon)

    for _, l in ipairs(self._private.layouts or alayout.layouts or {}) do
        local icn_path, icon = beautiful["layout_" .. (l.name or "")]
        if icn_path then
            icon = surface.load(icn_path)
        end

        table.insert(self._private.cache, {
            icon         = show_icon and icon   or nil,
            name         = show_text and l.name or nil,
            layout       = l,
            screen       = self.screen,
            callback     = function() alayout.set(l) end,
            style        = self._private.style,
        })
    end
end

local function update(self)
    assert(self._private.layout)
    awcommon.list_update(
        self._private.layout,
        self._private.buttons or default_buttons,
        wb_label,
        self._private.data,
        self._private.cache,
        {
            widget_template = self._private.widget_template
        }
    )
end

local layoutlist = {}

--- The layoutlist default widget layout.
-- If no layout is specified, a `wibox.layout.fixed.vertical` will be created
-- automatically.
-- @property base_layout
-- @param widget
-- @see wibox.layout.fixed.vertical
-- @see base_layout

--- The delegate widget template.
-- @property widget_template
-- @param table

--- The layoutlist screen.
-- @property screen
-- @param screen

--- The layoutlist buttons.
--
-- The default is:
--
--    gears.table.join(
--        awful.button({ }, 1, awful.layout.set)
--    )
--
-- @property buttons
-- @param table

--- The default foreground (text) color.
-- @beautiful beautiful.layoutlist_fg_normal
-- @tparam[opt=nil] string|pattern fg_normal
-- @see gears.color

--- The default background color.
-- @beautiful beautiful.layoutlist_bg_normal
-- @tparam[opt=nil] string|pattern bg_normal
-- @see gears.color

--- The selected layout foreground (text) color.
-- @beautiful beautiful.layoutlist_fg_selected
-- @tparam[opt=nil] string|pattern fg_selected
-- @see gears.color

--- The selected layout background color.
-- @beautiful beautiful.layoutlist_bg_selected
-- @tparam[opt=nil] string|pattern bg_selected
-- @see gears.color

--- Disable the layout icons (only show the name label).
-- @beautiful beautiful.layoutlist_disable_icon
-- @tparam[opt=false] boolean layoutlist_disable_icon

--- Disable the layout name label (only show the icon).
-- @beautiful beautiful.layoutlist_disable_name
-- @tparam[opt=false] boolean layoutlist_disable_name

--- The layoutlist font.
-- @beautiful beautiful.layoutlist_font
-- @tparam[opt=nil] string font

--- The selected layout alignment.
-- @beautiful beautiful.layoutlist_align
-- @tparam[opt=left] string align *left*, *right* or *center*

--- The selected layout title alignment.
-- @beautiful beautiful.layoutlist_font_selected
-- @tparam[opt=nil] string font_selected

--- The space between the layouts.
-- @beautiful beautiful.layoutlist_spacing
-- @tparam[opt=0] number spacing The spacing between tasks.

--- The default layoutlist elements shape.
-- @beautiful beautiful.layoutlist_shape
-- @tparam[opt=nil] gears.shape shape

--- The default layoutlist elements border width.
-- @beautiful beautiful.layoutlist_shape_border_width
-- @tparam[opt=0] number shape_border_width

--- The default layoutlist elements border color.
-- @beautiful beautiful.layoutlist_shape_border_color
-- @tparam[opt=nil] string|color shape_border_color
-- @see gears.color

--- The selected layout shape.
-- @beautiful beautiful.layoutlist_shape_selected
-- @tparam[opt=nil] gears.shape shape_selected

--- The selected layout border width.
-- @beautiful beautiful.layoutlist_shape_border_width_selected
-- @tparam[opt=0] number shape_border_width_selected

--- The selected layout border color.
-- @beautiful beautiful.layoutlist_shape_border_color_selected
-- @tparam[opt=nil] string|color shape_border_color_selected
-- @see gears.color

function layoutlist:set_buttons(b)
    self._private.buttons = b
    update(self)
end

function layoutlist:get_buttons()
    return self._private.buttons
end

function layoutlist:set_base_layout(layout)
    self._private.layout = wibox.widget.base.make_widget_from_value(
        layout or wibox.layout.fixed.horizontal
    )

    if self._private.layout.set_spacing then
        self._private.layout:set_spacing(
            self._private.style.spacing or beautiful.tasklist_spacing or 0
        )
    end

    assert(self._private.layout.is_widget)

    update(self)

    self:emit_signal("widget::layout_changed")
    self:emit_signal("widget::redraw_needed")
end

function layoutlist:set_widget_template(widget_template)
    self._private.widget_template = widget_template

    -- Remove the existing instances
    self._private.data = {}

    -- Prevent a race condition when the constructor loop to initialize the
    -- arguments.
    if self._private.layout then
        update(self)
    end

    self:emit_signal("widget::layout_changed")
    self:emit_signal("widget::redraw_needed")
end

function layoutlist:layout(_, width, height)
    if self._private.layout then
        return { wibox.widget.base.place_widget_at(self._private.layout, 0, 0, width, height) }
    end
end

function layoutlist:fit(context, width, height)
    if not self._private.layout then
        return 0, 0
    end

    return wibox.widget.base.fit_widget(self, context, self._private.layout, width, height)
end

--- Create an action list.
--
-- @tparam table args
-- @tparam widget args.layout The widget layout (not to be confused with client
--  layout).
-- @tparam table args.buttons A table with buttons binding to set.
-- @tparam table args.layouts A list of layouts to display.
-- @tparam[opt] table args.widget_template A custom widget to be used for each action.
-- @tparam screen args.screen A screen (mandatory)
-- @tparam[opt={}] table args.style Extra look and feel parameters
-- @tparam boolean args.style.disable_icon
-- @tparam boolean args.style.disable_name
-- @tparam string|pattern args.style.fg_normal
-- @tparam string|pattern args.style.bg_normal
-- @tparam string|pattern args.style.fg_selected
-- @tparam string|pattern args.style.bg_selected
-- @tparam string args.style.font
-- @tparam string args.style.font_selected
-- @tparam string args.style.align *left*, *right* or *center*
-- @tparam number args.style.spacing
-- @tparam gears.shape args.style.shape
-- @tparam number args.style.shape_border_width
-- @tparam string|pattern args.style.shape_border_color
-- @tparam gears.shape args.style.shape_selected
-- @tparam string|pattern args.style.shape_border_width_selected
-- @tparam string|pattern args.style.shape_border_color_selected
-- @treturn widget The action widget.
-- @function naughty.widget.layoutlist

local function new(_, args)
    assert(args and args.screen, "The screen argument is mandatory")

    local wdg = wibox.widget.base.make_widget(nil, nil, {
        enable_properties = true,
    })

    gtable.crush(wdg, layoutlist, true)

    wdg._private.style   = args.style or {}
    wdg._private.buttons = args.button
    wdg._private.layouts = args.layouts
    wdg._private.data = {}

    reload_cache(wdg)

    -- Apply all args properties
    gtable.crush(wdg, args)

    if not wdg._private.layout then
        wdg:set_base_layout()
    end

    assert(wdg._private.layout)

    tag.connect_signal("property::layout", function() reload_cache(wdg); update(wdg) end)
    update(wdg)

    return wdg
end

--@DOC_widget_COMMON@

--@DOC_object_COMMON@

return setmetatable(module, {__call = new})
