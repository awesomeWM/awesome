---------------------------------------------------------------------------
-- Container for the various system tray icons.
--
-- Both kinds of system tray icons are displayed:
--
-- * The legacy XEmbed icons (`_NET_SYSTEM_TRAY`).
-- * The StatusNotifierItems, which many applications (Electron, Qt,
--   Chromium, libappindicator, KDE, ...) use to export their icon and menu over
--   D-Bus. For these, awesome provides the `StatusNotifierWatcher` D-Bus
--   service (both `org.kde.StatusNotifierWatcher` and
--   `org.freedesktop.StatusNotifierWatcher`), so programs such as `snixembed`
--   are not needed (and would display each icon twice). When another program
--   already provides it, that one is used instead.
--
-- For StatusNotifierItems:
--
-- * A left click activates the item. When the item has no such action, its
--   menu is shown instead.
-- * A middle click triggers the secondary action.
-- * A right click shows the item menu. The menu is a native GTK menu (like
--   the menus of the XEmbed icons). GTK 3 is required for the menus. Without
--   it, the application is asked to display its own menu.
-- * Scrolling is forwarded to the item.
--
-- The items registered with awesome are kept across `awesome.restart()`: they
-- are saved in `$XDG_RUNTIME_DIR` and adopted by the new instance, as are the
-- items owning a well-known `org.kde.StatusNotifierItem-*` name.
--
-- The StatusNotifierItem support can be disabled with the
-- `statusnotifier_enabled` property:
--
--    local tray = wibox.widget.systray()
--    tray.statusnotifier_enabled = false
--
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @widgetmod wibox.widget.systray
-- @supermodule wibox.widget.base
---------------------------------------------------------------------------

local wbase = require("wibox.widget.base")
local drawable = require("wibox.drawable")
local imagebox = require("wibox.widget.imagebox")
local beautiful = require("beautiful")
local gtable = require("gears.table")
local gtimer = require("gears.timer")
local gcolor = require("gears.color")
local gstring = require("gears.string")
local lgi = require("lgi")
local Pango = lgi.Pango
local capi = {
    awesome = awesome,
    screen = screen,
    mouse = mouse,
}
local setmetatable = setmetatable
local error = error
local abs = math.abs

local systray = { mt = {} }

local instance = nil
local horizontal = true
local base_size = nil
local reverse = false
local display_on_screen = "primary"
local statusnotifier_enabled = true
local show_passive = true

-- The StatusNotifierItem support (loaded on demand).
local statusnotifier, sni_icon = nil, nil

-- The displayed StatusNotifierItems and their widgets.
local sni_items = {}
local sni_widgets = setmetatable({}, { __mode = "k" })
local sni_icon_size = nil

--- The systray background color.
--
-- Please note that only solid+opaque colors are supported. It does **not**
-- support gradients, patterns or transparent colors.
--
-- @beautiful beautiful.bg_systray
-- @tparam string bg_systray The color (string like "#ff0000" only)

--- The maximum number of rows for systray icons. Icons will fill the
-- current column (orthogonally to the systray direction) before
-- filling the next column.
--
-- The StatusNotifierItems and the XEmbed icons are placed in separate columns.
-- Both use the same number of rows: the smallest of `systray_max_rows` and the
-- number of icons of the larger group.
--
-- @beautiful beautiful.systray_max_rows
-- @tparam[opt=1] integer systray_max_rows The positive number of rows.

--- The systray icon spacing.
--
-- @beautiful beautiful.systray_icon_spacing
-- @tparam[opt=0] integer systray_icon_spacing The icon spacing

--- The color used for symbolic StatusNotifierItem icons (`foo-symbolic`).
--
-- @beautiful beautiful.systray_symbolic_color
-- @tparam[opt=beautiful.fg_normal] color systray_symbolic_color

--- The icon theme used for the StatusNotifierItem icons.
--
-- When not set, `beautiful.icon_theme` is used, then the GTK icon theme
-- (`gtk-icon-theme-name` in `$XDG_CONFIG_HOME/gtk-4.0/settings.ini`, then
-- in `$XDG_CONFIG_HOME/gtk-3.0/settings.ini`) and finally `Adwaita`. Icons
-- missing from the theme (and the themes it inherits) are looked up in the
-- `hicolor` theme.
--
-- The menus are rendered by GTK and use GTK's own icon theme.
--
-- @beautiful beautiful.systray_icon_theme
-- @tparam[opt=nil] string|nil systray_icon_theme

local function should_display_on(s)
    if display_on_screen == "primary" then
        return s == capi.screen.primary
    end
    return s == display_on_screen
end

-- Compute the grid. The StatusNotifierItems and the XEmbed icons use their own
-- columns, so the XEmbed icons (handled by the C code) stay contiguous. Both
-- groups share the same number of rows (and icon size).
--
-- The number of rows is only limited by the larger group: using more rows than
-- that cannot save a column, it would only make the icons smaller. When there
-- are no StatusNotifierItems, this is the same as the XEmbed-only layout.
local function get_grid()
    local num_xembed = capi.awesome.systray()
    local num_sni = #sni_items
    local max_rows = math.floor(tonumber(beautiful.systray_max_rows) or 1)
    local rows = math.max(math.min(math.max(num_xembed, num_sni), max_rows), 1)
    local cols_xembed = math.ceil(num_xembed / rows)
    local cols_sni = math.ceil(num_sni / rows)

    return {
        num_xembed  = num_xembed,
        num_sni     = num_sni,
        rows        = rows,
        cols        = cols_xembed + cols_sni,
        cols_xembed = cols_xembed,
        cols_sni    = cols_sni,
        spacing     = beautiful.systray_icon_spacing or 0,
    }
end

local function get_base(grid, width, height)
    local in_dir, ortho

    if horizontal then
        in_dir, ortho = width, height
    else
        ortho, in_dir = width, height
    end

    local rows, cols, spacing = grid.rows, grid.cols, grid.spacing

    -- The formula for a given base, spacing, and num_entries for the necessary
    -- space is (draw a picture to convince yourself; this assumes horizontal):
    --   height = (base + spacing) * rows - spacing
    --   width = (base + spacing) * cols - spacing
    -- Now, we check if we are limited by horizontal or vertical space: Which of
    -- the two limits the base size more?
    local base = (ortho + spacing) / rows - spacing
    if (base + spacing) * cols - spacing > in_dir then
        -- Solving the "width" formula above for "base" (with width=in_dir):
        base = (in_dir + spacing) / cols - spacing
    end

    return base
end

-- The offset (in the systray direction) of the StatusNotifierItems and of the
-- XEmbed icons.
local function get_offsets(grid, base)
    local step = base + grid.spacing

    if reverse then
        return grid.cols_xembed * step, 0
    end

    return 0, grid.cols_sni * step
end

function systray:draw(context, cr, width, height)
    if not should_display_on(context.screen) then
        return
    end

    if context and not context.wibox then
        error("The systray widget can only be placed inside a wibox.")
    end

    local grid = get_grid()
    local bg = beautiful.bg_systray or beautiful.bg_normal or "#000000"
    local base = get_base(grid, width, height)
    local sni_offset, xembed_offset = get_offsets(grid, base)

    -- The XEmbed icons have a `bg_systray` background, use it for the
    -- StatusNotifierItems too.
    if grid.num_sni > 0 then
        local start = sni_offset
        local len = (base + grid.spacing) * grid.cols_sni - grid.spacing

        -- Also fill the space between the two kinds of icons.
        if grid.num_xembed > 0 then
            len = len + grid.spacing
            if reverse then start = start - grid.spacing end
        end

        cr:set_source(gcolor(bg))
        if horizontal then
            cr:rectangle(start, 0, len, height)
        else
            cr:rectangle(0, start, width, len)
        end
        cr:fill()
    end

    -- The XEmbed window is placed at the top-left corner of its block in device
    -- space, so the rectangle must be the XEmbed block (and not the whole
    -- widget) for this to be correct under rotations and mirroring.
    local xembed_width, xembed_height = width, height
    if grid.num_sni > 0 then
        local len = math.max(
            (base + grid.spacing) * grid.cols_xembed - grid.spacing, 0)
        if horizontal then
            xembed_width = len
        else
            xembed_height = len
        end
    end

    local x, y
    if horizontal then
        x, y = wbase.rect_to_device_geometry(cr, xembed_offset, 0,
                                             xembed_width, xembed_height)
    else
        x, y = wbase.rect_to_device_geometry(cr, 0, xembed_offset,
                                             xembed_width, xembed_height)
    end

    -- Figure out if the cairo context is rotated
    local dir_x, dir_y = cr:user_to_device_distance(1, 0)
    local is_rotated = abs(dir_x) < abs(dir_y)

    if horizontal then
        is_rotated = not is_rotated
    end

    capi.awesome.systray(context.wibox.drawin, math.ceil(x), math.ceil(y),
                         base, is_rotated, bg, reverse, grid.spacing, grid.rows)
end

-- Private API. Does not appear in LDoc on purpose. This function is called
-- some time after the systray is removed from some drawable. It's purpose is to
-- really remove the systray.
function systray:_kickout(context)
    capi.awesome.systray(context.wibox.drawin)
end

function systray:fit(context, width, height)
    if not should_display_on(context.screen) then
        return 0, 0
    end

    local grid = get_grid()
    local rows, cols, spacing = grid.rows, grid.cols, grid.spacing
    local base = base_size
    if cols == 0 then
        return 0, 0
    end
    if base == nil then
        if horizontal then
            base = math.min(math.floor((height + spacing) / rows) - spacing,
                            math.floor((width + spacing) / cols) - spacing)
        else
            base = math.min(math.floor((width + spacing) / rows) - spacing,
                            math.floor((height + spacing) / cols) - spacing)
        end
    end
    base = base + spacing
    if horizontal then
        return base * cols - spacing, base * rows - spacing
    end
    return base * rows - spacing, base * cols - spacing
end

-- Make sure the tooltip text is valid Pango markup.
local function sanitize_markup(text)
    if not text or text == "" then return nil end

    -- Qt rich text is commonly used by KDE applications.
    text = text:gsub("<[bB][rR]%s*/?>", "\n")

    local ok, res = pcall(Pango.parse_markup, text, -1, 0)

    if ok and res then return text end

    return gstring.xml_escape(text)
end

local function tooltip_markup(item)
    local tt = item.tooltip
    local title, text

    if tt then
        title = tt.title ~= "" and tt.title or nil
        text = tt.text ~= "" and tt.text or nil
    end

    title = title or (item.title ~= "" and item.title) or item.id

    if not title or title == "" then return nil end

    local ret = "<b>" .. gstring.xml_escape(title) .. "</b>"

    text = sanitize_markup(text)

    if text then
        ret = ret .. "\n" .. text
    end

    return ret
end

-- A minimal tooltip. `awful.tooltip` cannot be used here, but the same
-- `beautiful` variables are used.
local tooltip = { timer = nil, wibox = nil, textbox = nil }

function tooltip.hide()
    if tooltip.timer then
        tooltip.timer:stop()
        tooltip.timer = nil
    end

    if tooltip.wibox then
        tooltip.wibox.visible = false
    end
end

function tooltip.show(item)
    local markup = tooltip_markup(item)

    if not markup then return end

    if not tooltip.wibox then
        -- Delay loading wibox to avoid a cyclic dependency.
        local wibox = require("wibox")
        local dpi = beautiful.xresources.apply_dpi

        tooltip.textbox = wibox.widget.textbox()
        tooltip.wibox = wibox {
            ontop  = true,
            type   = "tooltip",
            widget = wibox.container.margin(tooltip.textbox, dpi(5), dpi(5),
                                            dpi(3), dpi(3)),
        }
    end

    local w, tb = tooltip.wibox, tooltip.textbox
    tb.font = beautiful.tooltip_font or beautiful.font
    tb.markup = markup
    w.bg = beautiful.tooltip_bg or beautiful.bg_focus or "#ffcb60"
    w.fg = beautiful.tooltip_fg or beautiful.fg_focus or "#000000"
    w.border_width = beautiful.tooltip_border_width or 1
    w.border_color = beautiful.tooltip_border_color or beautiful.border_color_normal
        or beautiful.border_normal or "#ffcb60"
    w.opacity = beautiful.tooltip_opacity or 1
    w.shape = beautiful.tooltip_shape

    local coords = capi.mouse.coords()
    local s = capi.mouse.screen
    local tw, th = tb:get_preferred_size(s)
    local dpi = beautiful.xresources.apply_dpi
    tw, th = tw + dpi(10), th + dpi(6)

    local geo = s.geometry
    local gap = 20
    local x = math.max(geo.x, math.min(coords.x - tw / 2, geo.x + geo.width - tw))
    local y = coords.y + gap

    -- Display it above the pointer when the bar is at the bottom.
    if coords.y > geo.y + geo.height / 2 then
        y = coords.y - gap - th
    end

    w:geometry { x = math.floor(x), y = math.floor(y), width = tw, height = th }
    w.visible = true
end

local function schedule_tooltip(item)
    tooltip.hide()

    tooltip.timer = gtimer.start_new(0.5, function()
        tooltip.timer = nil
        tooltip.show(item)
        return false
    end)
end

local function update_sni_widget(item, w)
    local size = math.max(math.ceil(sni_icon_size or 24), 16)
    local ic = item:get_icon(size)

    -- Do not let the item disappear when it has no usable icon.
    if not ic then
        local path = sni_icon.lookup("application-x-executable", nil, size)
            or sni_icon.lookup("image-missing", nil, size)
        ic = path and { path = path, symbolic = sni_icon.is_symbolic(path) }
    end

    if ic and ic.path then
        w.image = ic.path
    elseif ic and ic.surface then
        w.image = ic.surface
    else
        w.image = nil
    end

    if ic and ic.symbolic then
        local col = beautiful.systray_symbolic_color
            or beautiful.fg_normal
            or "#ffffff"
        w.stylesheet = sni_icon.symbolic_stylesheet(
            (gcolor.to_rgba_string(col, "#ffffff")):sub(1, 7)
        )
    else
        w.stylesheet = nil
    end
end

local function on_sni_button(item, button)
    tooltip.hide()

    local c = capi.mouse.coords()

    if button == 1 then
        item:activate(c.x, c.y)
    elseif button == 2 then
        item:secondary_activate(c.x, c.y)
    elseif button == 3 then
        item:show_menu(c.x, c.y, 3)
    elseif button == 4 then
        item:scroll(-1, "vertical")
    elseif button == 5 then
        item:scroll(1, "vertical")
    elseif button == 6 then
        item:scroll(-1, "horizontal")
    elseif button == 7 then
        item:scroll(1, "horizontal")
    end
end

local function get_sni_widget(item)
    local w = sni_widgets[item]

    if w then return w end

    w = imagebox()
    w.resize = true
    w._item = item

    w:connect_signal("button::press", function(_, _, _, button)
        on_sni_button(item, button)
    end)
    w:connect_signal("mouse::enter", function() schedule_tooltip(item) end)
    w:connect_signal("mouse::leave", tooltip.hide)

    sni_widgets[item] = w
    update_sni_widget(item, w)

    return w
end

function systray:layout(context, width, height)
    if #sni_items == 0 or not should_display_on(context.screen) then
        return {}
    end

    local grid = get_grid()
    local base = get_base(grid, width, height)
    local step = base + grid.spacing
    local offset = get_offsets(grid, base)

    if sni_icon_size ~= base then
        sni_icon_size = base
        for item, w in pairs(sni_widgets) do
            update_sni_widget(item, w)
        end
    end

    local ret = {}

    for i, item in ipairs(sni_items) do
        local idx = reverse and (#sni_items - i) or (i - 1)
        local col, row = math.floor(idx / grid.rows), idx % grid.rows
        local x, y

        if horizontal then
            x, y = offset + col * step, row * step
        else
            x, y = row * step, offset + col * step
        end

        table.insert(ret, wbase.place_widget_at(get_sni_widget(item), x, y,
            base, base))
    end

    return ret
end

local function update_sni_items()
    sni_items = {}

    local items = statusnotifier and statusnotifier.get_items() or {}

    for _, item in ipairs(items) do
        if show_passive or item.status ~= "Passive" then
            table.insert(sni_items, item)
        end
    end

    if instance then
        instance:emit_signal("widget::layout_changed")
        instance:emit_signal("widget::redraw_needed")
    end
end

local function start_statusnotifier()
    if statusnotifier then
        statusnotifier.start()
        return
    end

    statusnotifier = require("wibox.widget.systray.statusnotifier")
    sni_icon = require("wibox.widget.systray.icon")

    statusnotifier:connect_signal("item::added", update_sni_items)

    statusnotifier:connect_signal("item::removed", function(_, item)
        local w = sni_widgets[item]

        if w then
            sni_widgets[item] = nil
            tooltip.hide()
        end

        update_sni_items()
    end)

    statusnotifier:connect_signal("item::updated", function(_, item)
        local w = sni_widgets[item]

        if w then
            update_sni_widget(item, w)
        end

        update_sni_items()
    end)

    statusnotifier.start()
    update_sni_items()
end

-- Remove all StatusNotifierItems and release everything on the bus.
local function stop_statusnotifier()
    if not statusnotifier then return end

    tooltip.hide()
    statusnotifier.stop()
    update_sni_items()
end

--- Refresh the StatusNotifierItem icons (for example after changing the
-- icon theme).
-- @staticfct wibox.widget.systray.refresh_icons
-- @usebeautiful beautiful.systray_icon_theme
-- @noreturn
function systray.refresh_icons()
    if not statusnotifier then return end

    sni_icon.clear_cache()

    -- Also clear the hidden (Passive) items, they may be shown later.
    for _, item in ipairs(statusnotifier.get_items()) do
        item:_clear_icon_cache()
    end

    for item, w in pairs(sni_widgets) do
        update_sni_widget(item, w)
    end

    update_sni_items()
end

-- Check if the function was called like :foo() or .foo() and do the right thing
local function get_args(self, ...)
    if self == instance then
        return ...
    end
    return self, ...
end

--- Set the size of a single icon.
--
-- If this is set to nil, then the size is picked dynamically based on the
-- available space. Otherwise, any single icon has a size of `size`x`size`.
--
-- @property base_size
-- @tparam[opt=nil] integer|nil base_size
-- @propertytype integer The size.
-- @propertytype nil Select the size based on the availible space.
-- @propertyunit pixel
-- @negativeallowed false
-- @propemits true false

function systray:set_base_size(size)
    base_size = get_args(self, size)
    if instance then
        instance:emit_signal("widget::layout_changed")
        instance:emit_signal("property::base_size", size)
    end
end

--- Decide between horizontal or vertical display.
--
-- @property horizontal
-- @tparam[opt=true] boolean horizontal
-- @propemits true false

function systray:set_horizontal(horiz)
    horizontal = get_args(self, horiz)
    if instance then
        instance:emit_signal("widget::layout_changed")
        instance:emit_signal("property::horizontal", horiz)
    end
end

--- Should the systray icons be displayed in reverse order?
--
-- @property reverse
-- @tparam[opt=false] boolean reverse
-- @propemits true false

function systray:set_reverse(rev)
    reverse = get_args(self, rev)
    if instance then
        instance:emit_signal("widget::redraw_needed")
        instance:emit_signal("property::reverse", rev)
    end
end

--- Set the screen that the systray should be displayed on.
--
-- This can either be a screen, in which case the systray will be displayed on
-- exactly that screen, or the string `"primary"`, in which case it will be
-- visible on the primary screen. The default value is "primary".
--
-- @property screen
-- @tparam[opt=nil] screen|nil screen
-- @propertytype nil Valid as long as the `systray` widget is only being displayed
--  on a single screen.
-- @propemits true false

function systray:set_screen(s)
    display_on_screen = get_args(self, s)
    if instance then
        instance:emit_signal("widget::layout_changed")
        instance:emit_signal("property::screen", s)
    end
end

--- Display the StatusNotifierItems (D-Bus tray icons).
--
-- When this is `false`, awesome does not provide the StatusNotifierWatcher
-- and StatusNotifierHost D-Bus services and only the XEmbed icons are
-- displayed. The D-Bus services are started shortly after the widget is
-- created, so setting this to `false` right after creating the widget (or
-- before, using `wibox.widget.systray.set_statusnotifier_enabled(false)`)
-- prevents them from ever being started. Setting it to `false` later
-- removes the StatusNotifierItems and stops the services.
--
-- @property statusnotifier_enabled
-- @tparam[opt=true] boolean statusnotifier_enabled
-- @propemits true false

function systray:set_statusnotifier_enabled(enabled)
    statusnotifier_enabled = get_args(self, enabled)

    if not statusnotifier_enabled then
        stop_statusnotifier()
    elseif instance then
        start_statusnotifier()
    end

    if instance then
        instance:emit_signal("property::statusnotifier_enabled",
            statusnotifier_enabled)
    end
end

function systray:get_statusnotifier_enabled()
    return statusnotifier_enabled
end

--- Display the StatusNotifierItems with the `Passive` status.
--
-- Applications use this status when their icon is not important. This only
-- applies to the StatusNotifierItems, the XEmbed icons have no status.
--
-- @property show_passive
-- @tparam[opt=true] boolean show_passive
-- @propemits true false

function systray:set_show_passive(show)
    show_passive = get_args(self, show)
    update_sni_items()
    if instance then
        instance:emit_signal("property::show_passive", show_passive)
    end
end

function systray:get_show_passive()
    return show_passive
end

--- Create the systray widget.
--
-- Note that this widget can only exist once.
--
-- @tparam boolean reverse Show in the opposite direction
-- @treturn table The new `systray` widget
-- @constructorfct wibox.widget.systray
-- @usebeautiful beautiful.bg_systray
-- @usebeautiful beautiful.systray_icon_spacing
-- @usebeautiful beautiful.systray_max_rows
-- @usebeautiful beautiful.systray_symbolic_color
-- @usebeautiful beautiful.systray_icon_theme
-- @usebeautiful beautiful.icon_theme Fallback when
--  `beautiful.systray_icon_theme` isn't set.
-- @usebeautiful beautiful.tooltip_font For the StatusNotifierItem tooltips.
-- @usebeautiful beautiful.tooltip_bg For the StatusNotifierItem tooltips.
-- @usebeautiful beautiful.tooltip_fg For the StatusNotifierItem tooltips.
-- @usebeautiful beautiful.tooltip_border_width For the StatusNotifierItem
--  tooltips.
-- @usebeautiful beautiful.tooltip_border_color For the StatusNotifierItem
--  tooltips.
-- @usebeautiful beautiful.tooltip_opacity For the StatusNotifierItem tooltips.
-- @usebeautiful beautiful.tooltip_shape For the StatusNotifierItem tooltips.
-- @usebeautiful beautiful.bg_normal Fallback when `beautiful.bg_systray`
--  isn't set.
-- @usebeautiful beautiful.fg_normal Fallback when
--  `beautiful.systray_symbolic_color` isn't set.
-- @usebeautiful beautiful.font Fallback when `beautiful.tooltip_font`
--  isn't set.
-- @usebeautiful beautiful.bg_focus Fallback when `beautiful.tooltip_bg`
--  isn't set.
-- @usebeautiful beautiful.fg_focus Fallback when `beautiful.tooltip_fg`
--  isn't set.
-- @usebeautiful beautiful.border_color_normal Fallback when
--  `beautiful.tooltip_border_color` isn't set.

local function new(revers)
    local ret = wbase.make_widget(nil, nil, {enable_properties = true})

    gtable.crush(ret, systray, true)

    if revers then
        ret:set_reverse(true)
    end

    capi.awesome.connect_signal("systray::update", function()
        ret:emit_signal("widget::layout_changed")
        ret:emit_signal("widget::redraw_needed")
    end)
    capi.screen.connect_signal("primary_changed", function()
        if display_on_screen == "primary" then
            ret:emit_signal("widget::layout_changed")
        end
    end)

    drawable._set_systray_widget(ret)

    -- Leave some time to disable the StatusNotifierItems.
    gtimer.delayed_call(function()
        if statusnotifier_enabled then
            start_statusnotifier()
        end
    end)

    return ret
end

function systray.mt:__call(...)
    if not instance then
        instance = new(...)
    end
    return instance
end

return setmetatable(systray, systray.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
