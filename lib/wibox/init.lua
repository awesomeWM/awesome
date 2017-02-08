---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @classmod wibox
---------------------------------------------------------------------------

local capi = {
    drawin = drawin,
    root = root,
    awesome = awesome,
    screen = screen
}
local setmetatable = setmetatable
local pairs = pairs
local type = type
local object = require("gears.object")
local grect =  require("gears.geometry").rectangle
local beautiful = require("beautiful")
local base = require("wibox.widget.base")
local cairo = require("lgi").cairo

--- This provides widget box windows. Every wibox can also be used as if it were
-- a drawin. All drawin functions and properties are also available on wiboxes!
-- wibox
local wibox = { mt = {}, object = {} }
wibox.layout = require("wibox.layout")
wibox.container = require("wibox.container")
wibox.widget = require("wibox.widget")
wibox.drawable = require("wibox.drawable")
wibox.hierarchy = require("wibox.hierarchy")

local force_forward = {
    shape_bounding = true,
    shape_clip = true,
    shape_input = true,
}

--@DOC_wibox_COMMON@

function wibox:set_widget(widget)
    self._drawable:set_widget(widget)
end

function wibox:get_widget()
    return self._drawable.widget
end

wibox.setup = base.widget.setup

function wibox:set_bg(c)
    self._drawable:set_bg(c)
end

function wibox:set_bgimage(image, ...)
    self._drawable:set_bgimage(image, ...)
end

function wibox:set_fg(c)
    self._drawable:set_fg(c)
end

function wibox:find_widgets(x, y)
    return self._drawable:find_widgets(x, y)
end

function wibox:_apply_shape()
    local shape = self._shape

    if not shape then
        self.shape_bounding = nil
        self.shape_clip = nil
        return
    end

    local geo = self:geometry()
    local bw = self.border_width

    -- First handle the bounding shape (things including the border)
    local img = cairo.ImageSurface(cairo.Format.A1, geo.width + 2*bw, geo.height + 2*bw)
    local cr = cairo.Context(img)

    -- We just draw the shape in its full size
    shape(cr, geo.width + 2*bw, geo.height + 2*bw)
    cr:set_operator(cairo.Operator.SOURCE)
    cr:fill()
    self.shape_bounding = img._native
    img:finish()

    -- Now handle the clip shape (things excluding the border)
    img = cairo.ImageSurface(cairo.Format.A1, geo.width, geo.height)
    cr = cairo.Context(img)

    -- We give the shape the same arguments as for the bounding shape and draw
    -- it in its full size (the translate is to compensate for the smaller
    -- surface)
    cr:translate(-bw, -bw)
    shape(cr, geo.width + 2*bw, geo.height + 2*bw)
    cr:set_operator(cairo.Operator.SOURCE)
    cr:fill_preserve()
    -- Now we remove an area of width 'bw' again around the shape (We use 2*bw
    -- since half of that is on the outside and only half on the inside)
    cr:set_source_rgba(0, 0, 0, 0)
    cr:set_line_width(2*bw)
    cr:stroke()
    self.shape_clip = img._native
    img:finish()
end

--- Set the wibox shape.
-- @property shape
-- @tparam gears.shape A gears.shape compatible function.
-- @see gears.shape

function wibox:set_shape(shape)
    self._shape = shape
    self:_apply_shape()
end

function wibox:get_shape()
    return self._shape
end

function wibox:get_screen()
    if self.screen_assigned and self.screen_assigned.valid then
        return self.screen_assigned
    else
        self.screen_assigned = nil
    end
    local sgeos = {}

    for s in capi.screen do
        sgeos[s] = s.geometry
    end

    return grect.get_closest_by_coord(sgeos, self.x, self.y)
end

function wibox:set_screen(s)
    s = capi.screen[s or 1]
    if s ~= self:get_screen() then
        self.x = s.geometry.x
        self.y = s.geometry.y
    end

    -- Remember this screen so things work correctly if screens overlap and
    -- (x,y) is not enough to figure out the correct screen.
    self.screen_assigned = s
    self._drawable:_force_screen(s)
end

for _, k in pairs{ "buttons", "struts", "geometry", "get_xproperty", "set_xproperty" } do
    wibox[k] = function(self, ...)
        return self.drawin[k](self.drawin, ...)
    end
end

local function setup_signals(_wibox)
    local obj
    local function clone_signal(name)
        -- When "name" is emitted on wibox.drawin, also emit it on wibox
        obj:connect_signal(name, function(_, ...)
            _wibox:emit_signal(name, ...)
        end)
    end

    obj = _wibox.drawin
    clone_signal("property::border_color")
    clone_signal("property::border_width")
    clone_signal("property::buttons")
    clone_signal("property::cursor")
    clone_signal("property::height")
    clone_signal("property::ontop")
    clone_signal("property::opacity")
    clone_signal("property::struts")
    clone_signal("property::visible")
    clone_signal("property::width")
    clone_signal("property::x")
    clone_signal("property::y")
    clone_signal("property::geometry")
    clone_signal("property::shape_bounding")
    clone_signal("property::shape_clip")
    clone_signal("property::shape_input")

    obj = _wibox._drawable
    clone_signal("button::press")
    clone_signal("button::release")
    clone_signal("mouse::enter")
    clone_signal("mouse::leave")
    clone_signal("mouse::move")
    clone_signal("property::surface")
end

--- Create a wibox.
-- @tparam[opt=nil] table args
-- @tparam integer args.border_width Border width.
-- @tparam string args.border_color Border color.
-- @tparam boolean args.ontop On top of other windows.
-- @tparam string args.cursor The mouse cursor.
-- @tparam boolean args.visible Visibility.
-- @tparam number args.opacity The opacity of the wibox, between 0 and 1.
-- @tparam string args.type The window type (desktop, normal, dock, …).
-- @tparam integer args.x The x coordinates.
-- @tparam integer args.y The y coordinates.
-- @tparam integer args.width The width of the wibox.
-- @tparam integer args.height The height of the wibox.
-- @tparam screen args.screen The wibox screen.
-- @tparam wibox.widget args.widget The widget that the wibox displays.
-- @param args.shape_bounding The wibox’s bounding shape as a (native) cairo surface.
-- @param args.shape_clip The wibox’s clip shape as a (native) cairo surface.
-- @param args.shape_input The wibox’s input shape as a (native) cairo surface.
-- @tparam color args.bg The background of the wibox.
-- @tparam surface args.bgimage The background image of the drawable.
-- @tparam color args.fg The foreground (text) of the wibox.
-- @treturn wibox The new wibox
-- @function .wibox

local function new(args)
    args = args or {}
    local ret = object()
    local w = capi.drawin(args)

    function w.get_wibox()
        return ret
    end

    ret.drawin = w
    ret._drawable = wibox.drawable(w.drawable, { wibox = ret },
        "wibox drawable (" .. object.modulename(3) .. ")")

    ret._drawable:_inform_visible(w.visible)
    w:connect_signal("property::visible", function()
        ret._drawable:_inform_visible(w.visible)
    end)

    for k, v in pairs(wibox) do
        if type(v) == "function" then
            ret[k] = v
        end
    end

    setup_signals(ret)
    ret.draw = ret._drawable.draw

    -- Set the default background
    ret:set_bg(args.bg or beautiful.bg_normal)
    ret:set_fg(args.fg or beautiful.fg_normal)

    -- Add __tostring method to metatable.
    local mt = {}
    local orig_string = tostring(ret)
    mt.__tostring = function()
        return string.format("wibox: %s (%s)",
                             tostring(ret._drawable), orig_string)
    end
    ret = setmetatable(ret, mt)

    -- Make sure the wibox is drawn at least once
    ret.draw()

    ret:connect_signal("property::geometry", ret._apply_shape)
    ret:connect_signal("property::border_width", ret._apply_shape)

    -- If a value is not found, look in the drawin
    setmetatable(ret, {
        __index = function(self, k)
            if rawget(self, "get_"..k) then
                return self["get_"..k](self)
            else
                return w[k]
            end
        end,
        __newindex = function(self, k,v)
            if rawget(self, "set_"..k) then
                self["set_"..k](self, v)
            elseif w[k] ~= nil or force_forward[k] then
                w[k] = v
            else
                rawset(self, k, v)
            end
        end
    })

    -- Set other wibox specific arguments
    if args.bgimage then
        ret:set_bgimage( args.bgimage )
    end

    if args.widget then
        ret:set_widget ( args.widget  )
    end

    if args.screen then
        ret:set_screen ( args.screen  )
    end

    ret.shape = args.shape

    return ret
end

--- Redraw a wibox. You should never have to call this explicitely because it is
-- automatically called when needed.
-- @param wibox
-- @function draw

function wibox.mt:__call(...)
    return new(...)
end

-- Extend the luaobject
object.properties(capi.drawin, {
    getter_class = wibox.object,
    setter_class = wibox.object,
    auto_emit    = true,
})

return setmetatable(wibox, wibox.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
