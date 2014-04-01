---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2012 Uli Schlachter
-- @release @AWESOME_VERSION@
---------------------------------------------------------------------------

local drawable = {}
local capi = {
    awesome = awesome,
    root = root
}
local beautiful = require("beautiful")
local cairo = require("lgi").cairo
local color = require("gears.color")
local debug = require("gears.debug")
local object = require("gears.object")
local sort = require("gears.sort")
local surface = require("gears.surface")

local drawables = setmetatable({}, { __mode = 'k' })
local wallpaper = nil

local function do_redraw(self)
    local surf = surface(self.drawable.surface)
    -- The surface can be nil if the drawable's parent was already finalized
    if not surf then return end
    local cr = cairo.Context(surf)
    local geom = self.drawable:geometry();
    local x, y, width, height = geom.x, geom.y, geom.width, geom.height

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
    self._widget_geometries = {}
    if self.widget then
        cr:set_source(self.foreground_color)
        self.widget:draw(self.widget_arg, cr, width, height)
        self:widget_at(self.widget, 0, 0, width, height)
    end

    self.drawable:refresh()

    debug.assert(cr.status == "SUCCESS", "Cairo context entered error state: " .. cr.status)
end

--- Register a widget's position.
-- This is internal, don't call it yourself! Only wibox.layout.base.draw_widget
-- is allowed to call this.
function drawable:widget_at(widget, x, y, width, height)
    local t = {
        widget = widget,
        x = x, y = y,drawable = self,
        width = width, height = height
    }
    table.insert(self._widget_geometries, t)
end

--- Find a widget by a point.
-- The drawable must have drawn itself at least once for this to work.
-- @param x X coordinate of the point
-- @param y Y coordinate of the point
-- @return A sorted table with all widgets that contain the given point. The
--         widgets are sorted by relevance.
function drawable:find_widgets(x, y)
    local matches = {}
    -- Find all widgets that contain the point
    for k, v in pairs(self._widget_geometries) do
        local match = true
        if v.x > x or v.x + v.width <= x then match = false end
        if v.y > y or v.y + v.height <= y then match = false end
        if match then
            table.insert(matches, v)
        end
    end

    -- Sort the matches by area, the assumption here is that widgets don't
    -- overlap and so smaller widgets are "more specific".
    local function cmp(a, b)
        local area_a = a.width * a.height
        local area_b = b.width * b.height
        return area_a < area_b
    end
    sort(matches, cmp)

    return matches
end


--- Set the widget that the drawable displays
function drawable:set_widget(widget)
    if self.widget then
        -- Disconnect from the old widget so that we aren't updated due to it
        self.widget:disconnect_signal("widget::updated", self.draw)
    end

    self.widget = widget
    if widget then
        widget:connect_signal("widget::updated", self.draw)
    end

    -- Make sure the widget gets drawn
    self.draw()
end

--- Set the background of the drawable
-- @param drawable The drawable to use
-- @param c The background to use. This must either be a cairo pattern object,
--          nil or a string that gears.color() understands.
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
            self.drawable:connect_signal("property::x", self.draw)
            self.drawable:connect_signal("property::y", self.draw)
        else
            self.drawable:disconnect_signal("property::x", self.draw)
            self.drawable:disconnect_signal("property::y", self.draw)
        end
    end

    self.background_color = c
    self.draw()
end

--- Set the foreground of the drawable
-- @param drawable The drawable to use
-- @param c The foreground to use. This must either be a cairo pattern object,
--          nil or a string that gears.color() understands.
function drawable:set_fg(c)
    local c = c or "#FFFFFF"
    if type(c) == "string" or type(c) == "table" then
        c = color(c)
    end
    self.foreground_color = c
    self.draw()
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

function drawable.new(d, widget_arg)
    local ret = object()
    ret.drawable = d
    ret.widget_arg = widget_arg or ret
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
        capi.awesome.disconnect_signal("refresh", ret._do_redraw)
        do_redraw(ret)
    end

    -- Connect our signal when we need a redraw
    ret.draw = function()
        if not ret._redraw_pending then
            capi.awesome.connect_signal("refresh", ret._do_redraw)
            ret._redraw_pending = true
        end
    end
    drawables[ret.draw] = true
    d:connect_signal("property::surface", ret.draw)

    -- Currently we aren't redrawing on move (signals not connected).
    -- :set_bg() will later recompute this.
    ret._redraw_on_move = false

    -- Set the default background
    ret:set_bg(beautiful.bg_normal)
    ret:set_fg(beautiful.fg_normal)

    -- Initialize internals
    ret._widget_geometries = {}
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

    -- Make sure the drawable is drawn at least once
    ret.draw()

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

--- Handling of drawables. A drawable is something that can be drawn to.
-- @class table
-- @name drawable

return setmetatable(drawable, { __call = function(_, ...) return drawable.new(...) end })

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
