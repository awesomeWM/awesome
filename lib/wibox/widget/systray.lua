---------------------------------------------------------------------------
-- Container for the various system tray icons.
--
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @widgetmod wibox.widget.systray
-- @supermodule wibox.widget.base
---------------------------------------------------------------------------

local wbase = require("wibox.widget.base")
local drawable = require("wibox.drawable")
local cairo = require("lgi").cairo
local beautiful = require("beautiful")
local gcolor = require("gears.color")
local gtable = require("gears.table")
local capi = {
    awesome = awesome,
    screen = screen
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
-- @beautiful beautiful.systray_max_rows
-- @tparam[opt=1] integer systray_max_rows The positive number of rows.

--- The systray icon spacing.
--
-- @beautiful beautiful.systray_icon_spacing
-- @tparam[opt=0] integer systray_icon_spacing The icon spacing

--- Whether to skip drawing the systray background when compositing the systray icons.
--
-- @beautiful beautiful.systray_skip_bg
-- @tparam[opt=0] boolean Whether to skip drawing the systray background

local function should_display_on(s)
    if display_on_screen == "primary" then
        return s == capi.screen.primary
    end
    return s == display_on_screen
end

function systray:draw(context, cr, width, height)
    if not should_display_on(context.screen) then
        return
    end

    local x, y, _, _ = wbase.rect_to_device_geometry(cr, 0, 0, width, height)
    local num_entries = capi.awesome.systray()
    local max_rows = math.floor(tonumber(beautiful.systray_max_rows) or 1)
    local rows = math.max(math.min(num_entries, max_rows), 1)
    local cols = math.ceil(num_entries / rows)
    local bg = beautiful.bg_systray or beautiful.bg_normal or "#000000"
    local spacing = beautiful.systray_icon_spacing or 0

    if context and not context.wibox then
        error("The systray widget can only be placed inside a wibox.")
    end

    -- Figure out if the cairo context is rotated
    local dir_x, dir_y = cr:user_to_device_distance(1, 0)
    local is_rotated = abs(dir_x) < abs(dir_y)

    local in_dir, ortho, base
    if horizontal then
        in_dir, ortho = width, height
        is_rotated = not is_rotated
    else
        ortho, in_dir = width, height
    end
    -- The formula for a given base, spacing, and num_entries for the necessary
    -- space is (draw a picture to convince yourself; this assumes horizontal):
    --   height = (base + spacing) * rows - spacing
    --   width = (base + spacing) * cols - spacing
    -- Now, we check if we are limited by horizontal or vertical space: Which of
    -- the two limits the base size more?
    base = (ortho + spacing) / rows - spacing
    if (base + spacing) * cols - spacing > in_dir then
        -- Solving the "width" formula above for "base" (with width=in_dir):
        base = (in_dir + spacing) / cols - spacing
    end
    capi.awesome.systray(context.wibox.drawin, math.ceil(x), math.ceil(y),
                         base, is_rotated, bg, reverse, spacing, rows)

    local surf_width, surf_height =
        base * rows + spacing * (rows - 1),
        base * cols + spacing * (cols - 1)
    if is_rotated then
        surf_width, surf_height = surf_height, surf_width
    end
    local surf_raw = capi.awesome.systray_surface(surf_width, surf_height)
    if surf_raw then
        local surf = cairo.Surface(surf_raw, true)
        if not beautiful.systray_skip_bg then
            cr:set_source(gcolor(bg))
            cr:paint()
        end
        cr:set_source_surface(surf, 0, 0)
        cr:paint()
    end
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

    local num_entries = capi.awesome.systray()
    local max_rows = math.floor(tonumber(beautiful.systray_max_rows) or 1)
    local rows = math.max(math.min(num_entries, max_rows), 1)
    local cols = math.ceil(num_entries / rows)
    local base = base_size
    local spacing = beautiful.systray_icon_spacing or 0
    if num_entries == 0 then
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
