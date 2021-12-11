----------------------------------------------------------------------------
--- Display the available client layouts for a screen.
--
-- This is what the layoutlist looks like by default with a vertical layout:
--
--@DOC_awful_widget_layoutlist_default_EXAMPLE@
--
-- In the second example, it is shown how to create a popup in the center of
-- the screen:
--
--@DOC_awful_widget_layoutlist_popup_EXAMPLE@
--
-- This example extends `awful.widget.layoutbox` to show a layout list popup:
--
--@DOC_awful_widget_layoutlist_bar_EXAMPLE@
--
-- This example shows how to add a layout subset to the default wibar:
--
--@DOC_awful_widget_layoutlist_wibar_EXAMPLE@
--
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2010, 2018 Emmanuel Lepage Vallee
-- @widgetmod awful.widget.layoutlist
-- @supermodule wibox.widget.base
----------------------------------------------------------------------------

local capi     = {screen = screen, tag = tag}
local wibox    = require("wibox")
local awcommon = require("awful.widget.common")
local abutton  = require("awful.button")
local ascreen  = require("awful.screen")
local gtable   = require("gears.table")
local beautiful= require("beautiful")
local alayout  = require("awful.layout")
local surface  = require("gears.surface")
local gcolor   = require("gears.color")

local module = {}

local default_buttons = {
    abutton({ }, 1, function(a) a.callback(  ) end),
    abutton({ }, 4, function() alayout.inc(-1) end),
	abutton({ }, 5, function() alayout.inc( 1) end),
}

local function wb_label(item, _, textbox)
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

    if textbox and item.style.align or beautiful.layoutlist_align then
        textbox:set_align(item.style.align or beautiful.layoutlist_align)
    end

    local text = ""

    if item.name then
        text = "<span color='"..gcolor.ensure_pango_color(fg, "#000000").."'>"..item.name..'</span>'
    end

    return text, bg, nil, item.icon, {
        shape               = shape,
        shape_border_width  = shape_bw,
        shape_border_color  = shape_bc,
    }
end

module.source = {}

--- The layout list for the first selected tag of a screen.
-- @tparam screen s The screen.
-- @sourcefunction awful.widget.layoutlist.source.for_screen
function module.source.for_screen(s)
    s = capi.screen[s or ascreen.focused() or 1]
    assert(s)

    local t = s.selected_tag or #s.tags > 0 and s.tags[1]

    return t and t.layouts or {}
end

--- The layouts available for the first selected tag of `awful.screen.focused()`.
-- @sourcefunction awful.widget.layoutlist.source.current_screen
function module.source.current_screen()
    return module.source.for_screen()
end

--- The default layout list.
-- @see awful.layout.layouts
-- @sourcefunction awful.widget.layoutlist.source.default_layouts
function module.source.default_layouts()
    return alayout.layouts
end

-- Keep the object for each layout. This avoid creating little new tables in
-- each update, which is painful for the GC.
local l_cache = setmetatable({}, {__mode = "k"})

local function reload_cache(self)
    self._private.cache = setmetatable({}, {__mode = "v"})

    local show_text = (not self._private.style.disable_name) and
        (not beautiful.layoutlist_disable_name)

    local show_icon = (not self._private.style.disable_icon) and
        (not beautiful.layoutlist_disable_icon)

    local s = self.screen or ascreen.focused()

    -- Get the list.
    local ls = self:get_layouts()

    for _, l in ipairs(ls or {}) do
        local icn_path, icon = beautiful["layout_" .. (l.name or "")]
        if icn_path and show_icon then
            icon = surface.load(icn_path)
        end

        l_cache[l] = l_cache[l] or {
            layout       = l,
            callback     = function() alayout.set(l) end,
            style        = self._private.style,
        }

        -- Update the entry.
        l_cache[l].icon   = show_icon and icon   or nil
        l_cache[l].name   = show_text and l.name or nil
        l_cache[l].screen = s

        table.insert(self._private.cache, l_cache[l])
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
-- @propemits true false
-- @see wibox.layout.fixed.vertical
-- @see base_layout

--- The delegate widget template.
-- @property widget_template
-- @param table
-- @propemits true false

--- The layoutlist screen.
-- @property screen
-- @param screen

--- A function that returns the list of layout to display.
--
-- @property source
-- @param[opt=awful.widget.layoutlist.source.for_screen] function

--- The layoutlist filter function.
-- @property filter
-- @param[opt=awful.widget.layoutlist.source.for_screen] function

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

--- The selected layout title font.
-- @beautiful beautiful.layoutlist_font_selected
-- @tparam[opt=nil] string font_selected

--- The space between the layouts.
-- @beautiful beautiful.layoutlist_spacing
-- @tparam[opt=0] number spacing The spacing between layouts.

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

--- The currenly displayed layouts.
-- @property layouts
-- @param table

--- The currently selected layout.
-- @property current_layout
-- @param layout

--- The current number of layouts.
--
-- @property count
-- @readonly
-- @tparam number The number of layouts.
-- @propemits true false

function layoutlist:get_layouts()
    local f = self.source or self._private.source or module.source.for_screen

    local ret = f(self.screen)

    if self._private.last_count ~= #ret then
        self:emit_signal("property::count", ret, self._private.last_count)

        self._private.last_count = ret
    end

    return ret
end

function layoutlist:get_current_layout()
    -- Eventually the entire lookup could be removed. All it does is allowing
    -- `nil` to be returned when there is no selection.
    local ls = self:get_layouts()
    local l = alayout.get(self.screen or ascreen.focused())
    local selected_k = gtable.hasitem(ls, l, true)

    return selected_k and ls[selected_k] or nil
end

function layoutlist:set_buttons(buttons)
    self._private.buttons = buttons

    if self._private.layout then
        update(self)
    end
end

function layoutlist:get_buttons()
    return self._private.buttons
end

function layoutlist:set_base_layout(layout)
    self._private.layout = wibox.widget.base.make_widget_from_value(
        layout or wibox.layout.fixed.horizontal
    )

    local spacing = self._private.style.spacing or beautiful.tasklist_spacing

    if self._private.layout.set_spacing and spacing then
        self._private.layout:set_spacing(spacing)
    end

    assert(self._private.layout.is_widget)

    update(self)

    self:emit_signal("widget::layout_changed")
    self:emit_signal("widget::redraw_needed")
    self:emit_signal("property::base_layout", layout)
end

function layoutlist:get_count()
    if not self._private.last_count then
        self._do_()
    end

    return self._private.last_count
end

function layoutlist:set_widget_template(widget_template)
    self._private.widget_template = widget_template

    -- Remove the existing instances
    self._private.data = setmetatable({}, { __mode = 'k' })

    -- Prevent a race condition when the constructor loop to initialize the
    -- arguments.
    if self._private.layout then
        update(self)
    end

    self:emit_signal("widget::layout_changed")
    self:emit_signal("widget::redraw_needed")
    self:emit_signal("property::widget_template", widget_template)
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

--- Create a layout list.
--
-- @tparam table args
-- @tparam widget args.base_layout The widget layout (not to be confused with client
--  layout).
-- @tparam table args.buttons A table with buttons binding to set.
-- @tparam[opt=awful.widget.layoutlist.source.for_screen] function args.source A
--  function to generate the list of layouts.
-- @tparam[opt] table args.widget_template A custom widget to be used for each action.
-- @tparam[opt=ascreen.focused()] screen args.screen A screen
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
-- @constructorfct awful.widget.layoutlist

local is_connected, instances = false, setmetatable({}, {__mode = "v"})

local function update_common()
    for _, ll in ipairs(instances) do
        reload_cache(ll)
        update(ll)
    end
end

local function new(_, args)
    args = args or {}

    local ret = wibox.widget.base.make_widget(nil, nil, {
        enable_properties = true,
    })

    gtable.crush(ret, layoutlist, true)

    ret._private.style   = args.style or {}
    ret._private.buttons = args.buttons
    ret._private.source  = args.source
    ret._private.data = setmetatable({}, { __mode = 'k' })

    reload_cache(ret)

    -- Apply all args properties. Make sure "set_layout" doesn't override
    -- the widget `layout` method.
    local l = args.layout
    args.layout = nil
    gtable.crush(ret, args, false)
    args.layout = l

    if not ret._private.layout then
        ret:set_base_layout(args.layout)
    end

    assert(ret._private.layout)

    -- Do not create a connection per instance, if used in a "volatile"
    -- popup, this will balloon.
    if not is_connected then
        for _, sig in ipairs {"layout", "layouts", "selected", "activated"} do
            capi.tag.connect_signal("property::"..sig, update_common)
        end
    end

    -- Add to the (weak) list of active instances.
    table.insert(instances, ret)

    update(ret)

    return ret
end

return setmetatable(module, {__call = new})
