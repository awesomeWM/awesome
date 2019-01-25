---------------------------------------------------------------------------
-- A container capable of changing the background color, foreground color
-- widget shape.
--
--@DOC_wibox_container_defaults_background_EXAMPLE@
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @classmod wibox.container.background
---------------------------------------------------------------------------

local base = require("wibox.widget.base")
local color = require("gears.color")
local surface = require("gears.surface")
local beautiful = require("beautiful")
local cairo = require("lgi").cairo
local gtable = require("gears.table")
local setmetatable = setmetatable
local type = type
local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)

local background = { mt = {} }

-- Make sure a surface pattern is freed *now*
local function dispose_pattern(pattern)
    local status, s = pattern:get_surface()
    if status == "SUCCESS" then
        s:finish()
    end
end

-- Prepare drawing the children of this widget
function background:before_draw_children(context, cr, width, height)
    local source = self._private.foreground or cr:get_source()

    -- Redirect drawing to a temporary surface if there is a shape
    if self._private.shape then
        cr:push_group_with_content(cairo.Content.COLOR_ALPHA)
    end

    -- Draw the background
    if self._private.background then
        cr:set_source(self._private.background)
        cr:rectangle(0, 0, width, height)
        cr:fill()
    end
    if self._private.bgimage then
        if type(self._private.bgimage) == "function" then
            self._private.bgimage(context, cr, width, height,unpack(self._private.bgimage_args))
        else
            local pattern = cairo.Pattern.create_for_surface(self._private.bgimage)
            cr:set_source(pattern)
            cr:rectangle(0, 0, width, height)
            cr:fill()
        end
    end

    cr:set_source(source)
end

-- Draw the border
function background:after_draw_children(_, cr, width, height)
    if not self._private.shape then
        return
    end

    -- Okay, there is a shape. Get it as a path.
    local bw = self._private.shape_border_width or 0

    cr:translate(bw, bw)
    self._private.shape(cr, width - 2*bw, height - 2*bw, unpack(self._private.shape_args or {}))
    cr:translate(-bw, -bw)

    if bw > 0 then
        -- Now we need to do a border, somehow. We begin with another
        -- temporary surface.
        cr:push_group_with_content(cairo.Content.ALPHA)

        -- Mark everything as "this is border"
        cr:set_source_rgba(0, 0, 0, 1)
        cr:paint()

        -- Now remove the inside of the shape to get just the border
        cr:set_operator(cairo.Operator.SOURCE)
        cr:set_source_rgba(0, 0, 0, 0)
        cr:fill_preserve()

        local mask = cr:pop_group()
        -- Now actually draw the border via the mask we just created.
        cr:set_source(color(self._private.shape_border_color or self._private.foreground or beautiful.fg_normal))
        cr:set_operator(cairo.Operator.SOURCE)
        cr:mask(mask)

        dispose_pattern(mask)
    end

    -- We now have the right content in a temporary surface. Copy it to the
    -- target surface. For this, we need another mask
    cr:push_group_with_content(cairo.Content.ALPHA)

    -- Draw the border with 2 * border width (this draws both
    -- inside and outside, only half of it is outside)
    cr.line_width = 2 * bw
    cr:set_source_rgba(0, 0, 0, 1)
    cr:stroke_preserve()

    -- Now fill the whole inside so that it is also include in the mask
    cr:fill()

    local mask = cr:pop_group()
    local source = cr:pop_group() -- This pops what was pushed in before_draw_children

    -- This now draws the content of the background widget to the actual
    -- target, but only the part that is inside the mask
    cr:set_operator(cairo.Operator.OVER)
    cr:set_source(source)
    cr:mask(mask)

    dispose_pattern(mask)
    dispose_pattern(source)
end

-- Layout this widget
function background:layout(_, width, height)
    if self._private.widget then
        return { base.place_widget_at(self._private.widget, 0, 0, width, height) }
    end
end

-- Fit this widget into the given area
function background:fit(context, width, height)
    if not self._private.widget then
        return 0, 0
    end

    return base.fit_widget(self, context, self._private.widget, width, height)
end

--- The widget displayed in the background widget.
-- @property widget
-- @tparam widget widget The widget to be disaplayed inside of the background
--  area

function background:set_widget(widget)
    if widget then
        base.check_widget(widget)
    end
    self._private.widget = widget
    self:emit_signal("widget::layout_changed")
end

function background:get_widget()
    return self._private.widget
end

-- Get children element
-- @treturn table The children
function background:get_children()
    return {self._private.widget}
end

-- Replace the layout children
-- This layout only accept one children, all others will be ignored
-- @tparam table children A table composed of valid widgets
function background:set_children(children)
    self:set_widget(children[1])
end

--- The background color/pattern/gradient to use.
--@DOC_wibox_container_background_bg_EXAMPLE@
-- @property bg
-- @param bg A color string, pattern or gradient
-- @see gears.color

function background:set_bg(bg)
    if bg then
        self._private.background = color(bg)
    else
        self._private.background = nil
    end
    self:emit_signal("widget::redraw_needed")
end

function background:get_bg()
    return self._private.background
end

--- The foreground (text) color/pattern/gradient to use.
--@DOC_wibox_container_background_fg_EXAMPLE@
-- @property fg
-- @param fg A color string, pattern or gradient
-- @see gears.color

function background:set_fg(fg)
    if fg then
        self._private.foreground = color(fg)
    else
        self._private.foreground = nil
    end
    self:emit_signal("widget::redraw_needed")
end

function background:get_fg()
    return self._private.foreground
end

--- The background shap     e.
--
-- Use `set_shape` to set additional shape paramaters.
--
--@DOC_wibox_container_background_shape_EXAMPLE@
-- @property shape
-- @param shape A function taking a context, width and height as arguments
-- @see gears.shape
-- @see set_shape

--- Set the background shape.
--
-- Any other arguments will be passed to the shape function
-- @param shape A function taking a context, width and height as arguments
-- @see gears.shape
-- @see shape
function background:set_shape(shape, ...)
    local args = {...}

    if shape == self._private.shape and #args == 0 then return end

    self._private.shape = shape
    self._private.shape_args = {...}
    self:emit_signal("widget::redraw_needed")
end

function background:get_shape()
    return self._private.shape
end

--- When a `shape` is set, also draw a border.
--
-- See `wibox.container.background.shape` for an usage example.
-- @property shape_border_width
-- @tparam number width The border width

function background:set_shape_border_width(width)
    if self._private.shape_border_width == width then return end

    self._private.shape_border_width = width
    self:emit_signal("widget::redraw_needed")
end

function background:get_shape_border_width()
    return self._private.shape_border_width
end

--- When a `shape` is set, also draw a border.
--
-- See `wibox.container.background.shape` for an usage example.
-- @property shape_border_color
-- @param[opt=self._private.foreground] fg The border color, pattern or gradient
-- @see gears.color

function background:set_shape_border_color(fg)
    if self._private.shape_border_color == fg then return end

    self._private.shape_border_color = fg
    self:emit_signal("widget::redraw_needed")
end

function background:get_shape_border_color()
    return self._private.shape_border_color
end

--- When a `shape` is set, make sure nothing is drawn outside of it.
--@DOC_wibox_container_background_clip_EXAMPLE@
-- @property shape_clip
-- @tparam boolean value If the shape clip is enable

function background:set_shape_clip(value)
    if self._private.shape_clip == value then return end

    self._private.shape_clip = value
    self:emit_signal("widget::redraw_needed")
end

function background:get_shape_clip()
    return self._private.shape_clip or false
end

--- The background image to use
-- If `image` is a function, it will be called with `(context, cr, width, height)`
-- as arguments. Any other arguments passed to this method will be appended.
-- @property bgimage
-- @param image A background image or a function
-- @see gears.surface

function background:set_bgimage(image, ...)
    self._private.bgimage = type(image) == "function" and image or surface.load(image)
    self._private.bgimage_args = {...}
    self:emit_signal("widget::redraw_needed")
end

function background:get_bgimage()
    return self._private.bgimage
end

--- Returns a new background container.
--
-- A background container applies a background and foreground color
-- to another widget.
-- @param[opt] widget The widget to display.
-- @param[opt] bg The background to use for that widget.
-- @param[opt] shape A `gears.shape` compatible shape function
-- @function wibox.container.background
local function new(widget, bg, shape)
    local ret = base.make_widget(nil, nil, {
        enable_properties = true,
    })

    gtable.crush(ret, background, true)

    ret._private.shape = shape

    ret:set_widget(widget)
    ret:set_bg(bg)

    return ret
end

function background.mt:__call(...)
    return new(...)
end

--@DOC_widget_COMMON@

--@DOC_object_COMMON@

return setmetatable(background, background.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
