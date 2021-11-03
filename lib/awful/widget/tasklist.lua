---------------------------------------------------------------------------
--- Tasklist widget module for awful.
--
-- <a name="status_icons"></a>
-- **Status icons:**
--
-- By default, the tasklist prepends some symbols in front of the client name.
-- This is used to notify that the client has some specific properties that are
-- currently enabled. This can be disabled using
-- `beautiful.tasklist_plain_task_name`=true in the theme.
--
-- <table class='widget_list' border=1>
-- <tr style='font-weight: bold;'>
--  <th align='center'>Icon</th>
--  <th align='center'>Client property</th>
-- </tr>
-- <tr><td>▪</td><td><a href="./client.html#client.sticky">sticky</a></td></tr>
-- <tr><td>⌃</td><td><a href="./client.html#client.ontop">ontop</a></td></tr>
-- <tr><td>▴</td><td><a href="./client.html#client.above">above</a></td></tr>
-- <tr><td>▾</td><td><a href="./client.html#client.below">below</a></td></tr>
-- <tr><td>✈</td><td><a href="./client.html#client.floating">floating</a></td></tr>
-- <tr><td>+</td><td><a href="./client.html#client.maximized">maximized</a></td></tr>
-- <tr><td>⬌</td><td><a href="./client.html#client.maximized_horizontal">maximized_horizontal</a></td></tr>
-- <tr><td>⬍</td><td><a href="./client.html#client.maximized_vertical">maximized_vertical</a></td></tr>
-- </table>
--
-- **Customizing the tasklist:**
--
-- The `tasklist` created by `rc.lua` uses the default values for almost
-- everything. However, it is possible to override each aspect to create a
-- very different widget. Here's an example that creates a tasklist similar to
-- the default one, but with an explicit layout and some spacing widgets:
--
--@DOC_wibox_awidget_tasklist_rounded_EXAMPLE@
--
-- As demonstrated in the example above, there are a few "shortcuts" to avoid
-- re-inventing the wheel. By setting the predefined roles as widget `id`s,
-- `awful.widget.common` will do most of the work to update the values
-- automatically. All of them are optional. The supported roles are:
--
-- * `icon_role`: A `wibox.widget.imagebox`
-- * `text_role`: A `wibox.widget.textbox`
-- * `background_role`: A `wibox.container.background`
-- * `text_margin_role`: A `wibox.container.margin`
-- * `icon_margin_role`: A `wibox.container.margin`
--
-- `awful.widget.common` also has 2 callbacks to give more control over the widget:
--
-- * `create_callback`: Called once after the widget instance is created
-- * `update_callback`: Called every time the data is refreshed
--
-- Both callback have the same parameters:
--
-- * `self`: The widget instance (*widget*).
-- * `c`: The client (*client*)
-- * `index`: The widget position in the list (*number*)
-- * `clients`: The list of client, in order (*table*)
--
-- It is also possible to omit some roles and create an icon only tasklist.
-- Notice that this example use the `awful.widget.clienticon` widget instead
-- of an `imagebox`. This allows higher resolution icons to be loaded. This
-- example reproduces the Windows 10 tasklist look and feel:
--
--@DOC_wibox_awidget_tasklist_windows10_EXAMPLE@
--
-- The tasklist can also be created in an `awful.popup` in case there is no
-- permanent `awful.wibar`:
--
--@DOC_awful_popup_alttab_EXAMPLE@
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008-2009 Julien Danjou
-- @widgetmod awful.widget.tasklist
---------------------------------------------------------------------------

-- Grab environment we need
local capi = { screen = screen,
               client = client }
local ipairs = ipairs
local setmetatable = setmetatable
local table = table
local common = require("awful.widget.common")
local beautiful = require("beautiful")
local tag = require("awful.tag")
local flex = require("wibox.layout.flex")
local timer = require("gears.timer")
local gcolor = require("gears.color")
local gstring = require("gears.string")
local gdebug = require("gears.debug")
local dpi = require("beautiful").xresources.apply_dpi
local base = require("wibox.widget.base")
local wfixed = require("wibox.layout.fixed")
local wmargin = require("wibox.container.margin")
local wtextbox = require("wibox.widget.textbox")
local clienticon = require("awful.widget.clienticon")
local wbackground = require("wibox.container.background")
local gtable = require("gears.table")

local function get_screen(s)
    return s and screen[s]
end

local tasklist = { mt = {} }

local instances

--- The default foreground (text) color.
-- @beautiful beautiful.tasklist_fg_normal
-- @tparam[opt=nil] string|pattern fg_normal
-- @see gears.color

--- The default background color.
-- @beautiful beautiful.tasklist_bg_normal
-- @tparam[opt=nil] string|pattern bg_normal
-- @see gears.color

--- The focused client foreground (text) color.
-- @beautiful beautiful.tasklist_fg_focus
-- @tparam[opt=nil] string|pattern fg_focus
-- @see gears.color

--- The focused client background color.
-- @beautiful beautiful.tasklist_bg_focus
-- @tparam[opt=nil] string|pattern bg_focus
-- @see gears.color

--- The urgent clients foreground (text) color.
-- @beautiful beautiful.tasklist_fg_urgent
-- @tparam[opt=nil] string|pattern fg_urgent
-- @see gears.color

--- The urgent clients background color.
-- @beautiful beautiful.tasklist_bg_urgent
-- @tparam[opt=nil] string|pattern bg_urgent
-- @see gears.color

--- The minimized clients foreground (text) color.
-- @beautiful beautiful.tasklist_fg_minimize
-- @tparam[opt=nil] string|pattern fg_minimize
-- @see gears.color

--- The minimized clients background color.
-- @beautiful beautiful.tasklist_bg_minimize
-- @tparam[opt=nil] string|pattern bg_minimize
-- @see gears.color

--- The elements default background image.
-- @beautiful beautiful.tasklist_bg_image_normal
-- @tparam[opt=nil] string bg_image_normal

--- The focused client background image.
-- @beautiful beautiful.tasklist_bg_image_focus
-- @tparam[opt=nil] string bg_image_focus

--- The urgent clients background image.
-- @beautiful beautiful.tasklist_bg_image_urgent
-- @tparam[opt=nil] string bg_image_urgent

--- The minimized clients background image.
-- @beautiful beautiful.tasklist_bg_image_minimize
-- @tparam[opt=nil] string bg_image_minimize

--- Disable the tasklist client icons.
-- @beautiful beautiful.tasklist_disable_icon
-- @tparam[opt=false] boolean tasklist_disable_icon

--- Disable the tasklist client titles.
-- @beautiful beautiful.tasklist_disable_task_name
-- @tparam[opt=false] boolean tasklist_disable_task_name

--- Disable the extra tasklist client property notification icons.
--
-- See the <a href="#status_icons">Status icons</a> section for more details.
--
-- @beautiful beautiful.tasklist_plain_task_name
-- @tparam[opt=false] boolean tasklist_plain_task_name

--- Extra tasklist client property notification icon for clients with the sticky property set.
-- @beautiful beautiful.tasklist_sticky
-- @tparam[opt=nil] string tasklist_sticky

--- Extra tasklist client property notification icon for clients with the ontop property set.
-- @beautiful beautiful.tasklist_ontop
-- @tparam[opt=nil] string tasklist_ontop

--- Extra tasklist client property notification icon for clients with the above property set.
-- @beautiful beautiful.tasklist_above
-- @tparam[opt=nil] string tasklist_above

--- Extra tasklist client property notification icon for clients with the below property set.
-- @beautiful beautiful.tasklist_below
-- @tparam[opt=nil] string tasklist_below

--- Extra tasklist client property notification icon for clients with the floating property set.
-- @beautiful beautiful.tasklist_floating
-- @tparam[opt=nil] string tasklist_floating

--- Extra tasklist client property notification icon for clients with the maximized property set.
-- @beautiful beautiful.tasklist_maximized
-- @tparam[opt=nil] string tasklist_maximized

--- Extra tasklist client property notification icon for clients with the maximized_horizontal property set.
-- @beautiful beautiful.tasklist_maximized_horizontal
-- @tparam[opt=nil] string maximized_horizontal

--- Extra tasklist client property notification icon for clients with the maximized_vertical property set.
-- @beautiful beautiful.tasklist_maximized_vertical
-- @tparam[opt=nil] string maximized_vertical

--- Extra tasklist client property notification icon for clients with the minimized property set.
-- @beautiful beautiful.tasklist_minimized
-- @tparam[opt=nil] string minimized

--- The tasklist font.
-- @beautiful beautiful.tasklist_font
-- @tparam[opt=nil] string font

--- The focused client alignment.
-- @beautiful beautiful.tasklist_align
-- @tparam[opt=left] string align *left*, *right* or *center*

--- The focused client title alignment.
-- @beautiful beautiful.tasklist_font_focus
-- @tparam[opt=nil] string font_focus

--- The minimized clients font.
-- @beautiful beautiful.tasklist_font_minimized
-- @tparam[opt=nil] string font_minimized

--- The urgent clients font.
-- @beautiful beautiful.tasklist_font_urgent
-- @tparam[opt=nil] string font_urgent

--- The space between the tasklist elements.
-- @beautiful beautiful.tasklist_spacing
-- @tparam[opt=0] number spacing The spacing between tasks.

--- The default tasklist elements shape.
-- @beautiful beautiful.tasklist_shape
-- @tparam[opt=nil] gears.shape shape

--- The default tasklist elements border width.
-- @beautiful beautiful.tasklist_shape_border_width
-- @tparam[opt=0] number shape_border_width

--- The default tasklist elements border color.
-- @beautiful beautiful.tasklist_shape_border_color
-- @tparam[opt=nil] string|color shape_border_color
-- @see gears.color

--- The focused client shape.
-- @beautiful beautiful.tasklist_shape_focus
-- @tparam[opt=nil] gears.shape shape_focus

--- The focused client border width.
-- @beautiful beautiful.tasklist_shape_border_width_focus
-- @tparam[opt=0] number shape_border_width_focus

--- The focused client border color.
-- @beautiful beautiful.tasklist_shape_border_color_focus
-- @tparam[opt=nil] string|color shape_border_color_focus
-- @see gears.color

--- The minimized clients shape.
-- @beautiful beautiful.tasklist_shape_minimized
-- @tparam[opt=nil] gears.shape shape_minimized

--- The minimized clients border width.
-- @beautiful beautiful.tasklist_shape_border_width_minimized
-- @tparam[opt=0] number shape_border_width_minimized

--- The minimized clients border color.
-- @beautiful beautiful.tasklist_shape_border_color_minimized
-- @tparam[opt=nil] string|color shape_border_color_minimized
-- @see gears.color

--- The urgent clients shape.
-- @beautiful beautiful.tasklist_shape_urgent
-- @tparam[opt=nil] gears.shape shape_urgent

--- The urgent clients border width.
-- @beautiful beautiful.tasklist_shape_border_width_urgent
-- @tparam[opt=0] number shape_border_width_urgent

--- The urgent clients border color.
-- @beautiful beautiful.tasklist_shape_border_color_urgent
-- @tparam[opt=nil] string|color shape_border_color_urgent
-- @see gears.color

-- Public structures
tasklist.filter, tasklist.source = {}, {}

-- This is the same template as awful.widget.common, but with an clienticon widget
local default_template = {
    {
        {
            clienticon,
            id     = "icon_margin_role",
            left   = dpi(4),
            widget = wmargin
        },
        {
            {
                id     = "text_role",
                widget = wtextbox,
            },
            id     = "text_margin_role",
            left   = dpi(4),
            right  = dpi(4),
            widget = wmargin
        },
        fill_space = true,
        layout     = wfixed.horizontal
    },
    id     = "background_role",
    widget = wbackground
}

local function tasklist_label(c, args, tb)
    if not args then args = {} end
    local theme = beautiful.get()
    local align = args.align or theme.tasklist_align or "left"
    local fg_normal = gcolor.ensure_pango_color(args.fg_normal or theme.tasklist_fg_normal or theme.fg_normal, "white")
    local bg_normal = args.bg_normal or theme.tasklist_bg_normal or theme.bg_normal or "#000000"
    local fg_focus = gcolor.ensure_pango_color(args.fg_focus or theme.tasklist_fg_focus or theme.fg_focus, fg_normal)
    local bg_focus = args.bg_focus or theme.tasklist_bg_focus or theme.bg_focus or bg_normal
    local fg_urgent = gcolor.ensure_pango_color(args.fg_urgent or theme.tasklist_fg_urgent or theme.fg_urgent,
                                                fg_normal)
    local bg_urgent = args.bg_urgent or theme.tasklist_bg_urgent or theme.bg_urgent or bg_normal
    local fg_minimize = gcolor.ensure_pango_color(args.fg_minimize or theme.tasklist_fg_minimize or theme.fg_minimize,
                                                  fg_normal)
    local bg_minimize = args.bg_minimize or theme.tasklist_bg_minimize or theme.bg_minimize or bg_normal
    -- FIXME v5, remove the fallback theme.bg_image_* variables, see GH#1403
    local bg_image_normal = args.bg_image_normal or theme.tasklist_bg_image_normal or theme.bg_image_normal
    local bg_image_focus = args.bg_image_focus or theme.tasklist_bg_image_focus or theme.bg_image_focus
    local bg_image_urgent = args.bg_image_urgent or theme.tasklist_bg_image_urgent or theme.bg_image_urgent
    local bg_image_minimize = args.bg_image_minimize or theme.tasklist_bg_image_minimize or theme.bg_image_minimize
    local tasklist_disable_icon = args.tasklist_disable_icon or theme.tasklist_disable_icon or false
    local disable_task_name = args.disable_task_name or theme.tasklist_disable_task_name or false
    local font = args.font or theme.tasklist_font or theme.font or ""
    local font_focus = args.font_focus or theme.tasklist_font_focus or theme.font_focus or font or ""
    local font_minimized = args.font_minimized or theme.tasklist_font_minimized or theme.font_minimized or font or ""
    local font_urgent = args.font_urgent or theme.tasklist_font_urgent or theme.font_urgent or font or ""
    local text = ""
    local name = ""
    local bg
    local bg_image
    local shape              = args.shape or theme.tasklist_shape
    local shape_border_width = args.shape_border_width or theme.tasklist_shape_border_width
    local shape_border_color = args.shape_border_color or theme.tasklist_shape_border_color
    local icon_size = args.icon_size or theme.tasklist_icon_size

    -- symbol to use to indicate certain client properties
    local sticky = args.sticky or theme.tasklist_sticky or "▪"
    local ontop = args.ontop or theme.tasklist_ontop or '⌃'
    local above = args.above or theme.tasklist_above or '▴'
    local below = args.below or theme.tasklist_below or '▾'
    local floating = args.floating or theme.tasklist_floating or '✈'
    local maximized = args.maximized or theme.tasklist_maximized or '<b>+</b>'
    local maximized_horizontal = args.maximized_horizontal or theme.tasklist_maximized_horizontal or '⬌'
    local maximized_vertical = args.maximized_vertical or theme.tasklist_maximized_vertical or '⬍'
    local minimized = args.minimized or theme.tasklist_minimized or '<b>_</b>'

    if tb then
        tb:set_align(align)
    end

    if not theme.tasklist_plain_task_name then
        if c.sticky then name = name .. sticky end

        if c.ontop then name = name .. ontop
        elseif c.above then name = name .. above
        elseif c.below then name = name .. below end

        if c.maximized then
            name = name .. maximized
        else
            if c.maximized_horizontal then name = name .. maximized_horizontal end
            if c.maximized_vertical then name = name .. maximized_vertical end
            if c.floating then name = name .. floating end
        end
        if c.minimized then name = name .. minimized end
    end

    if not disable_task_name then
        if c.minimized then
            name = name .. (gstring.xml_escape(c.icon_name) or gstring.xml_escape(c.name) or
                            gstring.xml_escape("<untitled>"))
        else
            name = name .. (gstring.xml_escape(c.name) or gstring.xml_escape("<untitled>"))
        end
    end

    local focused = c.active
    -- Handle transient_for: the first parent that does not skip the taskbar
    -- is considered to be focused, if the real client has skip_taskbar.
    if not focused and capi.client.focus and capi.client.focus.skip_taskbar
        and capi.client.focus:get_transient_for_matching(function(cl)
                                                             return not cl.skip_taskbar
                                                         end) == c then
        focused = true
    end

    if focused then
        bg = bg_focus
        text = text .. "<span color='"..fg_focus.."'>"..name.."</span>"
        bg_image = bg_image_focus
        font = font_focus

        if args.shape_focus or theme.tasklist_shape_focus then
            shape = args.shape_focus or theme.tasklist_shape_focus
        end

        if args.shape_border_width_focus or theme.tasklist_shape_border_width_focus then
            shape_border_width = args.shape_border_width_focus or theme.tasklist_shape_border_width_focus
        end

        if args.shape_border_color_focus or theme.tasklist_shape_border_color_focus then
            shape_border_color = args.shape_border_color_focus or theme.tasklist_shape_border_color_focus
        end
    elseif c.urgent then
        bg = bg_urgent
        text = text .. "<span color='"..fg_urgent.."'>"..name.."</span>"
        bg_image = bg_image_urgent
        font = font_urgent

        if args.shape_urgent or theme.tasklist_shape_urgent then
            shape = args.shape_urgent or theme.tasklist_shape_urgent
        end

        if args.shape_border_width_urgent or theme.tasklist_shape_border_width_urgent then
            shape_border_width = args.shape_border_width_urgent or theme.tasklist_shape_border_width_urgent
        end

        if args.shape_border_color_urgent or theme.tasklist_shape_border_color_urgent then
            shape_border_color = args.shape_border_color_urgent or theme.tasklist_shape_border_color_urgent
        end
    elseif c.minimized then
        bg = bg_minimize
        text = text .. "<span color='"..fg_minimize.."'>"..name.."</span>"
        bg_image = bg_image_minimize
        font = font_minimized

        if args.shape_minimized or theme.tasklist_shape_minimized then
            shape = args.shape_minimized or theme.tasklist_shape_minimized
        end

        if args.shape_border_width_minimized or theme.tasklist_shape_border_width_minimized then
            shape_border_width = args.shape_border_width_minimized or theme.tasklist_shape_border_width_minimized
        end

        if args.shape_border_color_minimized or theme.tasklist_shape_border_color_minimized then
            shape_border_color = args.shape_border_color_minimized or theme.tasklist_shape_border_color_minimized
        end
    else
        bg = bg_normal
        text = text .. "<span color='"..fg_normal.."'>"..name.."</span>"
        bg_image = bg_image_normal
    end

    if tb then
        tb:set_font(font)
    end

    local other_args = {
        shape              = shape,
        shape_border_width = shape_border_width,
        shape_border_color = shape_border_color,
        icon_size          = icon_size,
    }

    return text, bg, bg_image, not tasklist_disable_icon and c.icon or nil, other_args
end

-- Remove some callback boilerplate from the user provided templates.
local function create_callback(w, t)
    common._set_common_property(w, "client", t)
end

local function tasklist_update(s, w, buttons, filter, data, style, update_function, args)
    local clients = {}

    local source = self.source or tasklist.source.all_clients or nil
    local list   = source and source(s, args) or capi.client.get()

    for _, c in ipairs(list) do
        if not (c.skip_taskbar or c.hidden
            or c.type == "splash" or c.type == "dock" or c.type == "desktop")
            and filter(c, s) then
            table.insert(clients, c)
        end
    end

    local function label(c, tb) return tasklist_label(c, style, tb) end

    update_function(w, buttons, label, data, clients, {
        widget_template = args.widget_template or default_template,
        create_callback = create_callback,
    })
end

--- Set the tasklist layout.
--
-- @property base_layout
-- @tparam[opt=wibox.layout.flex.horizontal] wibox.layout base_layout
-- @see wibox.layout.flex.horizontal

function tasklist:set_base_layout(layout)
    self._private.base_layout = base.make_widget_from_value(
        layout or flex.horizontal
    )

    local spacing = self._private.style.spacing or beautiful.tasklist_spacing

    if self._private.base_layout.set_spacing and spacing then
        self._private.base_layout:set_spacing(spacing)
    end

    assert(self._private.base_layout.is_widget)

    self._do_tasklist_update()

    self:emit_signal("widget::layout_changed")
    self:emit_signal("widget::redraw_needed")
    self:emit_signal("property::base_layout", layout)
end

function tasklist:layout(_, width, height)
    if self._private.base_layout then
        return { base.place_widget_at(self._private.base_layout, 0, 0, width, height) }
    end
end

function tasklist:fit(context, width, height)
    if not self._private.base_layout then
        return 0, 0
    end

    return base.fit_widget(self, context, self._private.base_layout, width, height)
end

for _, prop in ipairs { "screen", "filter", "update_function", "widget_template", "source"} do
    tasklist["set_"..prop] = function(self, value)
        if value == self._private[prop] then return end

        self._private[prop] = value

        self._do_tasklist_update()

        self:emit_signal("widget::layout_changed")
        self:emit_signal("widget::redraw_needed")
        self:emit_signal("property::"..prop, value)
    end

    tasklist["get_"..prop] = function(self)
        return self._private[prop]
    end
end

local function update_screen(self, screen, old)
    if not instances then return end

    if old and instances[old] then
        for k, w in ipairs(instances[old]) do
            if w == self then
                table.remove(instances[old], k)
                break
            end
        end
    end

    local list = instances[screen]

    if not list then
        list = setmetatable({}, { __mode = "v" })
        instances[screen] = list
    end

    table.insert(list, self)
end

function tasklist:set_screen(value)
    value = get_screen(value)

    if value == self._private.screen then return end

    local old = self._private.screen

    self._private.screen = value

    update_screen(self, screen, old)

    self._do_tasklist_update()

    self:emit_signal("widget::layout_changed")
    self:emit_signal("widget::redraw_needed")
    self:emit_signal("property::screen", value)
end

function tasklist:set_widget_template(widget_template)
    self._private.widget_template = widget_template

    -- Remove the existing instances
    self._private.data = setmetatable({}, { __mode = 'k' })

    self._do_tasklist_update()

    self:emit_signal("widget::layout_changed")
    self:emit_signal("widget::redraw_needed")
    self:emit_signal("property::widget_template", widget_template)
end

--- Create a new tasklist widget.
-- The last two arguments (update_function
-- and layout) serve to customize the layout of the tasklist (eg. to
-- make it vertical). For that, you will need to copy the
-- awful.widget.common.list_update function, make your changes to it
-- and pass it as update_function here. Also change the layout if the
-- default is not what you want.
--
-- @tparam table args
-- @tparam screen args.screen The screen to draw tasklist for.
-- @tparam function args.filter Filter function to define what clients will be listed.
-- @tparam table args.buttons A table with buttons binding to set.
-- @tparam[opt] function args.update_function Function to create a tag widget on each
--   update. See `awful.widget.common.list_update`.
-- @tparam[opt] table args.layout Container widget for tag widgets. Default
--   is `wibox.layout.flex.horizontal`.
-- @tparam[opt=awful.widget.tasklist.source.all_clients] function args.source The
--  function used to generate the list of client.
-- @tparam[opt] table args.widget_template A custom widget to be used for each client
-- @tparam[opt={}] table args.style The style overrides default theme.
-- @tparam[opt=nil] string|pattern args.style.fg_normal
-- @tparam[opt=nil] string|pattern args.style.bg_normal
-- @tparam[opt=nil] string|pattern args.style.fg_focus
-- @tparam[opt=nil] string|pattern args.style.bg_focus
-- @tparam[opt=nil] string|pattern args.style.fg_urgent
-- @tparam[opt=nil] string|pattern args.style.bg_urgent
-- @tparam[opt=nil] string|pattern args.style.fg_minimize
-- @tparam[opt=nil] string|pattern args.style.bg_minimize
-- @tparam[opt=nil] string args.style.bg_image_normal
-- @tparam[opt=nil] string args.style.bg_image_focus
-- @tparam[opt=nil] string args.style.bg_image_urgent
-- @tparam[opt=nil] string args.style.bg_image_minimize
-- @tparam[opt=nil] boolean args.style.disable_icon
-- @tparam[opt=nil] number args.style.icon_size The size of the icon
-- @tparam[opt='▪'] string args.style.sticky Extra icon when client is sticky
-- @tparam[opt='⌃'] string args.style.ontop Extra icon when client is ontop
-- @tparam[opt='▴'] string args.style.above Extra icon when client is above
-- @tparam[opt='▾'] string args.style.below Extra icon when client is below
-- @tparam[opt='✈'] string args.style.floating Extra icon when client is floating
-- @tparam[opt='<b>+</b>'] string args.style.maximized Extra icon when client is maximized
-- @tparam[opt='⬌'] string args.style.maximized_horizontal Extra icon when client is maximized_horizontal
-- @tparam[opt='⬍'] string args.style.maximized_vertical Extra icon when client is maximized_vertical
-- @tparam[opt=false] boolean args.style.disable_task_name
-- @tparam[opt=nil] string args.style.font
-- @tparam[opt=left] string args.style.align *left*, *right* or *center*
-- @tparam[opt=nil] string args.style.font_focus
-- @tparam[opt=nil] string args.style.font_minimized
-- @tparam[opt=nil] string args.style.font_urgent
-- @tparam[opt=nil] number args.style.spacing The spacing between tags.
-- @tparam[opt=nil] gears.shape args.style.shape
-- @tparam[opt=nil] number args.style.shape_border_width
-- @tparam[opt=nil] string|color args.style.shape_border_color
-- @tparam[opt=nil] gears.shape args.style.shape_focus
-- @tparam[opt=nil] number args.style.shape_border_width_focus
-- @tparam[opt=nil] string|color args.style.shape_border_color_focus
-- @tparam[opt=nil] gears.shape args.style.shape_minimized
-- @tparam[opt=nil] number args.style.shape_border_width_minimized
-- @tparam[opt=nil] string|color args.style.shape_border_color_minimized
-- @tparam[opt=nil] gears.shape args.style.shape_urgent
-- @tparam[opt=nil] number args.style.shape_border_width_urgent
-- @tparam[opt=nil] string|color args.style.shape_border_color_urgent
-- @param filter **DEPRECATED** use args.filter
-- @param buttons **DEPRECATED** use args.buttons
-- @param style **DEPRECATED** use args.style
-- @param update_function **DEPRECATED** use args.update_function
-- @param base_widget **DEPRECATED** use args.base_layout
-- @constructorfct awful.widget.tasklist
function tasklist.new(args, filter, buttons, style, update_function, base_widget)
    local screen = nil

    local argstype = type(args)

    -- Detect the old function signature
    if argstype == "number" or argstype == "screen" or
      (argstype == "table" and args.index and args == capi.screen[args.index]) then
        gdebug.deprecate("The `screen` parameter is deprecated, use `args.screen`.",
            {deprecated_in=5})

        screen = get_screen(args)
        args = {}
    end

    assert(type(args) == "table")

    for k, v in pairs { filter          = filter,
                        buttons         = buttons,
                        style           = style,
                        update_function = update_function,
                        layout          = base_widget
    } do
        gdebug.deprecate("The `awful.widget.tasklist()` `"..k
            .."` parameter is deprecated, use `args."..k.."`.",
        {deprecated_in=5})
        args[k] = v
    end

    screen = screen or get_screen(args.screen)
    local uf = args.update_function or common.list_update

    local w = base.make_widget(nil, nil, {
        enable_properties = true,
    })

    gtable.crush(w._private, {
        disable_task_name = args.disable_task_name,
        disable_icon      = args.disable_icon,
        update_function   = args.update_function,
        filter            = args.filter,
        buttons           = args.buttons,
        style             = args.style or {},
        screen            = screen,
        widget_template   = args.widget_template,
        source            = args.source,
        data              = setmetatable({}, { __mode = 'k' })
    })

    gtable.crush(w, tasklist, true)
    rawset(w, "filter", nil)
    rawset(w, "source", nil)

    local queued_update = false

    -- For the tests
    function w._do_tasklist_update_now()
        queued_update = false
        if w._private.screen.valid then
            tasklist_update(
                w._private.screen, w, w._private.buttons, w._private.filter, w._private.data, args.style, uf, args
            )
        end
    end

    function w._do_tasklist_update()
        -- Add a delayed callback for the first update.
        if not queued_update then
            timer.delayed_call(w._do_tasklist_update_now)
            queued_update = true
        end
    end
    function w._unmanage(c)
        w._private.data[c] = nil
    end
    if instances == nil then
        instances = setmetatable({}, { __mode = "k" })
        local function us(s)
            local i = instances[get_screen(s)]
            if i then
                for _, tlist in pairs(i) do
                    tlist._do_tasklist_update()
                end
            end
        end
        local function u()
            for s in pairs(instances) do
                if s.valid then
                    us(s)
                end
            end
        end

        tag.attached_connect_signal(nil, "property::selected", u)
        tag.attached_connect_signal(nil, "property::activated", u)
        capi.client.connect_signal("property::urgent", u)
        capi.client.connect_signal("property::sticky", u)
        capi.client.connect_signal("property::ontop", u)
        capi.client.connect_signal("property::above", u)
        capi.client.connect_signal("property::below", u)
        capi.client.connect_signal("property::floating", u)
        capi.client.connect_signal("property::maximized_horizontal", u)
        capi.client.connect_signal("property::maximized_vertical", u)
        capi.client.connect_signal("property::maximized", u)
        capi.client.connect_signal("property::minimized", u)
        capi.client.connect_signal("property::name", u)
        capi.client.connect_signal("property::icon_name", u)
        capi.client.connect_signal("property::icon", u)
        capi.client.connect_signal("property::skip_taskbar", u)
        capi.client.connect_signal("property::screen", function(c, old_screen)
            us(c.screen)
            us(old_screen)
        end)
        capi.client.connect_signal("property::hidden", u)
        capi.client.connect_signal("tagged", u)
        capi.client.connect_signal("untagged", u)
        capi.client.connect_signal("request::unmanage", function(c)
            u(c)
            for _, i in pairs(instances) do
                for _, tlist in pairs(i) do
                    tlist._unmanage(c)
                end
            end
        end)
        capi.client.connect_signal("list", u)
        capi.client.connect_signal("property::active", u)
        capi.screen.connect_signal("removed", function(s)
            instances[get_screen(s)] = nil
        end)
    end

    tasklist.set_base_layout(w, args.layout or args.base_layout)

    w._do_tasklist_update()

    update_screen(w, screen)

    return w
end

--- Filtering function to include all clients.
--
--@DOC_sequences_client_tasklist_filter_allscreen1_EXAMPLE@
--
-- @return true
-- @filterfunction awful.widget.tasklist.filter.allscreen
function tasklist.filter.allscreen()
    return true
end

--- Filtering function to include the clients from all tags on the screen.
--
--@DOC_sequences_client_tasklist_filter_alltags1_EXAMPLE@
--
-- @tparam client c The client.
-- @tparam screen screen The screen we are drawing on.
-- @return true if c is on screen, false otherwise
-- @filterfunction awful.widget.tasklist.filter.alltags
function tasklist.filter.alltags(c, screen)
    -- Only print client on the same screen as this widget
    return get_screen(c.screen) == get_screen(screen)
end

--- Filtering function to include only the clients from currently selected tags.
--
-- This is the filter used in the default `rc.lua`.
--
--@DOC_sequences_client_tasklist_filter_currenttags1_EXAMPLE@
--
-- @tparam client c The client.
-- @tparam screen screen The screen we are drawing on.
-- @return true if c is in a selected tag on screen, false otherwise
-- @filterfunction awful.widget.tasklist.filter.currenttags
function tasklist.filter.currenttags(c, screen)
    screen = get_screen(screen)
    -- Only print client on the same screen as this widget
    if get_screen(c.screen) ~= screen then return false end
    -- Include sticky client too
    if c.sticky then return true end
    local tags = screen.tags
    for _, t in ipairs(tags) do
        if t.selected then
            local ctags = c:tags()
            for _, v in ipairs(ctags) do
                if v == t then
                    return true
                end
            end
        end
    end
    return false
end

--- Filtering function to include only the minimized clients from currently selected tags.
-- @param c The client.
-- @param screen The screen we are drawing on.
-- @return true if c is in a selected tag on screen and is minimized, false otherwise
-- @filterfunction awful.widget.tasklist.filter.minimizedcurrenttags
function tasklist.filter.minimizedcurrenttags(c, screen)
    screen = get_screen(screen)
    -- Only print client on the same screen as this widget
    if get_screen(c.screen) ~= screen then return false end
    -- Check client is minimized
    if not c.minimized then return false end
    -- Include sticky client
    if c.sticky then return true end
    local tags = screen.tags
    for _, t in ipairs(tags) do
        -- Select only minimized clients
        if t.selected then
            local ctags = c:tags()
            for _, v in ipairs(ctags) do
                if v == t then
                    return true
                end
            end
        end
    end
    return false
end

--- Filtering function to include only the currently focused client.
-- @param c The client.
-- @param screen The screen we are drawing on.
-- @return true if c is focused on screen, false otherwise
-- @filterfunction awful.widget.tasklist.filter.focused
function tasklist.filter.focused(c, screen)
    -- Only print client on the same screen as this widget
    return get_screen(c.screen) == get_screen(screen) and c.active
end

--- Get all the clients in an undefined order.
--
-- This is the default source.
--
-- @sourcefunction awful.widget.tasklist.source.all_clients
function tasklist.source.all_clients()
    return capi.client.get()
end

function tasklist.mt:__call(...)
    return tasklist.new(...)
end

--@DOC_widget_COMMON@

--@DOC_object_COMMON@

return setmetatable(tasklist, tasklist.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
