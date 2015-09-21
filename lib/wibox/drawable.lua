---------------------------------------------------------------------------
--- Handling of drawables. A drawable is something that can be drawn to.
--
-- @author Uli Schlachter
-- @copyright 2012 Uli Schlachter
-- @release @AWESOME_VERSION@
-- @classmod wibox.drawable
---------------------------------------------------------------------------

local drawable = {}
local capi = {
    awesome = awesome,
    root = root
}
local beautiful = require("beautiful")
local cairo = require("lgi").cairo
local color = require("gears.color")
local object = require("gears.object")
local sort = require("gears.sort")
local surface = require("gears.surface")
local timer = require("gears.timer")
local matrix = require("gears.matrix")
local hierarchy = require("wibox.hierarchy")
local base = require("wibox.widget.base")

local drawables = setmetatable({}, { __mode = 'k' })
local wallpaper = nil

-- This is awful.screen.getbycoord() which we sadly cannot use from here (cyclic
-- dependencies are bad!)
function screen_getbycoord(x, y)
    for i = 1, screen:count() do
        local geometry = screen[i].geometry
        if x >= geometry.x and x < geometry.x + geometry.width
           and y >= geometry.y and y < geometry.y + geometry.height then
            return i
        end
    end
    return 1
end

-- Get the widget context. This should always return the same table (if
-- possible), so that our draw and fit caches can work efficiently.
local function get_widget_context(self)
    local geom = self.drawable:geometry()
    local s = screen_getbycoord(geom.x, geom.y)
    local context = self._widget_context
    local dpi = beautiful.xresources.get_dpi(s)
    if (not context) or context.screen ~= s or context.dpi ~= dpi then
        context = {
            screen = s,
            dpi = dpi,
            drawable = self,
            widget_at = function(_, ...)
                self:widget_at(...)
            end
        }
        for k, v in pairs(self._widget_context_skeleton) do
            context[k] = v
        end
        self._widget_context = context
    end
    return context
end

local function do_redraw(self)
    local surf = surface(self.drawable.surface)
    -- The surface can be nil if the drawable's parent was already finalized
    if not surf then return end
    local cr = cairo.Context(surf)
    local geom = self.drawable:geometry();
    local x, y, width, height = geom.x, geom.y, geom.width, geom.height

    -- Relayout
    if self._need_relayout or self._need_complete_repaint then
        self._need_relayout = false
        if self._widget_hierarchy and self.widget then
            self._widget_hierarchy:update(get_widget_context(self),
                self.widget, width, height, self._dirty_area)
        else
            self._need_complete_repaint = true
            if self.widget then
                self._widget_hierarchy_callback_arg = {}
                self._widget_hierarchy = hierarchy.new(get_widget_context(self), self.widget, width, height,
                        self._redraw_callback, self._layout_callback, self._widget_hierarchy_callback_arg)
            else
                self._widget_hierarchy = nil
            end
        end

        if self._need_complete_repaint then
            self._need_complete_repaint = false
            self._dirty_area:union_rectangle(cairo.RectangleInt{
                x = 0, y = 0, width = width, height = height
            })
        end
    end

    -- Clip to the dirty area
    if self._dirty_area:is_empty() then
        return
    end
    for i = 0, self._dirty_area:num_rectangles() - 1 do
        local rect = self._dirty_area:get_rectangle(i)
        cr:rectangle(rect.x, rect.y, rect.width, rect.height)
    end
    self._dirty_area = cairo.Region.create()
    cr:clip()

    -- Draw the background
    cr:save()

    if not capi.awesome.composite_manager_running then
        -- This is pseudo-transparency: We draw the wallpaper in the background
        if not wallpaper then
            wallpaper = surface(capi.root.wallpaper())
        end
        if wallpaper then
            cr.operator = cairo.Operator.SOURCE
            cr:set_source_surface(wallpaper, -x, -y)
            cr:paint()
        end
        cr.operator = cairo.Operator.OVER
    else
        -- This is true transparency: We draw a translucent background
        cr.operator = cairo.Operator.SOURCE
    end

    cr:set_source(self.background_color)
    cr:paint()
    cr:restore()

    -- Draw the widget
    if self._widget_hierarchy then
        cr:set_source(self.foreground_color)
        self._widget_hierarchy:draw(get_widget_context(self), cr)
    end

    self.drawable:refresh()

    assert(cr.status == "SUCCESS", "Cairo context entered error state: " .. cr.status)
end

local function find_widgets(drawable, result, hierarchy, x, y)
    local m = hierarchy:get_matrix_from_device()

    -- Is (x,y) inside of this hierarchy or any child (aka the draw extents)
    local x1, y1 = m:transform_point(x, y)
    local x2, y2, width, height = hierarchy:get_draw_extents()
    if x1 < x2 or x1 >= x2 + width then
        return
    end
    if y1 < y2 or y1 >= y2 + height then
        return
    end

    -- Is (x,y) inside of this widget?
    local width, height = hierarchy:get_size()
    if x1 >= 0 and y1 >= 0 and x1 <= width and y1 <= height then
        -- Get the extents of this widget in the device space
        local x2, y2, w2, h2 = matrix.transform_rectangle(hierarchy:get_matrix_to_device(),
            0, 0, width, height)
        table.insert(result, {
            x = x2, y = y2, width = w2, height = h2,
            drawable = drawable, widget = hierarchy:get_widget()
        })
    end
    for _, child in ipairs(hierarchy:get_children()) do
        find_widgets(drawable, result, child, x, y)
    end
end

--- Find a widget by a point.
-- The drawable must have drawn itself at least once for this to work.
-- @param x X coordinate of the point
-- @param y Y coordinate of the point
-- @return A sorted table with all widgets that contain the given point. The
--   widgets are sorted by relevance.
function drawable:find_widgets(x, y)
    local result = {}
    if self._widget_hierarchy then
        find_widgets(self, result, self._widget_hierarchy, x, y)
    end
    return result
end


--- Set the widget that the drawable displays
function drawable:set_widget(widget)
    self.widget = widget

    -- Make sure the widget gets drawn
    self._need_relayout = true
    self.draw()
end

--- Set the background of the drawable
-- @param c The background to use. This must either be a cairo pattern object,
--   nil or a string that gears.color() understands.
function drawable:set_bg(c)
    local c = c or "#000000"
    if type(c) == "string" or type(c) == "table" then
        c = color(c)
    end

    -- If the background is completely opaque, we don't need to redraw when
    -- the drawable is moved
    -- XXX: This isn't needed when awesome.composite_manager_running is true,
    -- but a compositing manager could stop/start and we'd have to properly
    -- handle this. So for now we choose the lazy approach.
    local redraw_on_move = not color.create_opaque_pattern(c)
    if self._redraw_on_move ~= redraw_on_move then
        self._redraw_on_move = redraw_on_move
        if redraw_on_move then
            self.drawable:connect_signal("property::x", self._do_complete_repaint)
            self.drawable:connect_signal("property::y", self._do_complete_repaint)
        else
            self.drawable:disconnect_signal("property::x", self._do_complete_repaint)
            self.drawable:disconnect_signal("property::y", self._do_complete_repaint)
        end
    end

    self.background_color = c
    self._do_complete_repaint()
end

--- Set the foreground of the drawable
-- @param c The foreground to use. This must either be a cairo pattern object,
--   nil or a string that gears.color() understands.
function drawable:set_fg(c)
    local c = c or "#FFFFFF"
    if type(c) == "string" or type(c) == "table" then
        c = color(c)
    end
    self.foreground_color = c
    self._do_complete_repaint()
end

local function emit_difference(name, list, skip)
    local function in_table(table, val)
        for k, v in pairs(table) do
            if v.widget == val.widget then
                return true
            end
        end
        return false
    end

    for k, v in pairs(list) do
        if not in_table(skip, v) then
            v.widget:emit_signal(name,v)
        end
    end
end

local function handle_leave(_drawable)
    emit_difference("mouse::leave", _drawable._widgets_under_mouse, {})
    _drawable._widgets_under_mouse = {}
end

local function handle_motion(_drawable, x, y)
    if x < 0 or y < 0 or x > _drawable.drawable:geometry().width or y > _drawable.drawable:geometry().height then
        return handle_leave(_drawable)
    end

    -- Build a plain list of all widgets on that point
    local widgets_list = _drawable:find_widgets(x, y)

    -- First, "leave" all widgets that were left
    emit_difference("mouse::leave", _drawable._widgets_under_mouse, widgets_list)
    -- Then enter some widgets
    emit_difference("mouse::enter", widgets_list, _drawable._widgets_under_mouse)

    _drawable._widgets_under_mouse = widgets_list
end

local function setup_signals(_drawable)
    local d = _drawable.drawable

    local function clone_signal(name)
        _drawable:add_signal(name)
        -- When "name" is emitted on wibox.drawin, also emit it on wibox
        d:connect_signal(name, function(_, ...)
            _drawable:emit_signal(name, ...)
        end)
    end
    clone_signal("button::press")
    clone_signal("button::release")
    clone_signal("mouse::enter")
    clone_signal("mouse::leave")
    clone_signal("mouse::move")
    clone_signal("property::surface")
    clone_signal("property::width")
    clone_signal("property::height")
    clone_signal("property::x")
    clone_signal("property::y")
end

function drawable.new(d, widget_context_skeleton, drawable_name)
    local ret = object()
    ret.drawable = d
    ret._widget_context_skeleton = widget_context_skeleton
    ret._need_complete_repaint = true
    ret._need_relayout = true
    ret._dirty_area = cairo.Region.create()
    setup_signals(ret)

    for k, v in pairs(drawable) do
        if type(v) == "function" then
            ret[k] = v
        end
    end

    -- Only redraw a drawable once, even when we get told to do so multiple times.
    ret._redraw_pending = false
    ret._do_redraw = function()
        ret._redraw_pending = false
        do_redraw(ret)
    end

    -- Connect our signal when we need a redraw
    ret.draw = function()
        if not ret._redraw_pending then
            timer.delayed_call(ret._do_redraw)
            ret._redraw_pending = true
        end
    end
    ret._do_complete_repaint = function()
        ret._need_complete_repaint = true
        ret:draw()
    end
    drawables[ret._do_complete_repaint] = true
    d:connect_signal("property::surface", ret._do_complete_repaint)

    -- Currently we aren't redrawing on move (signals not connected).
    -- :set_bg() will later recompute this.
    ret._redraw_on_move = false

    -- Set the default background
    ret:set_bg(beautiful.bg_normal)
    ret:set_fg(beautiful.fg_normal)

    -- Initialize internals
    ret._widgets_under_mouse = {}

    local function button_signal(name)
        d:connect_signal(name, function(d, x, y, button, modifiers)
            local widgets = ret:find_widgets(x, y)
            for k, v in pairs(widgets) do
                -- Calculate x/y inside of the widget
                local lx = x - v.x
                local ly = y - v.y
                v.widget:emit_signal(name, lx, ly, button, modifiers,v)
            end
        end)
    end
    button_signal("button::press")
    button_signal("button::release")

    d:connect_signal("mouse::move", function(_, x, y) handle_motion(ret, x, y) end)
    d:connect_signal("mouse::leave", function() handle_leave(ret) end)

    -- Set up our callbacks for repaints
    ret._redraw_callback = function(hierarchy, arg)
        if ret._widget_hierarchy_callback_arg ~= arg then
            return
        end
        local m = hierarchy:get_matrix_to_device()
        local x, y, width, height = matrix.transform_rectangle(m, hierarchy:get_draw_extents())
        local x1, y1 = math.floor(x), math.floor(y)
        local x2, y2 = math.ceil(x + width), math.ceil(y + height)
        ret._dirty_area:union_rectangle(cairo.RectangleInt{
            x = x1, y = y1, width = x2 - x1, height = y2 - y1
        })
        ret:draw()
    end
    ret._layout_callback = function(hierarchy, arg)
        if ret._widget_hierarchy_callback_arg ~= arg then
            return
        end
        ret._need_relayout = true
        ret:draw()
    end

    -- Add __tostring method to metatable.
    ret.drawable_name = drawable_name or object.modulename(3)
    local mt = {}
    local orig_string = tostring(ret)
    mt.__tostring = function(o)
        return string.format("%s (%s)", ret.drawable_name, orig_string)
    end
    ret = setmetatable(ret, mt)

    -- Make sure the drawable is drawn at least once
    ret._do_complete_repaint()

    return ret
end

-- Redraw all drawables when the wallpaper changes
capi.awesome.connect_signal("wallpaper_changed", function()
    local k
    wallpaper = nil
    for k in pairs(drawables) do
        k()
    end
end)

return setmetatable(drawable, { __call = function(_, ...) return drawable.new(...) end })

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
