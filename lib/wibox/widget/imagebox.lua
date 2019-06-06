---------------------------------------------------------------------------
--
--@DOC_wibox_widget_defaults_imagebox_EXAMPLE@
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @widgetmod wibox.widget.imagebox
---------------------------------------------------------------------------

local base = require("wibox.widget.base")
local surface = require("gears.surface")
local gtable = require("gears.table")
local setmetatable = setmetatable
local type = type
local print = print
local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)

local imagebox = { mt = {} }

-- Draw an imagebox with the given cairo context in the given geometry.
function imagebox:draw(_, cr, width, height)
    if not self._private.image then return end
    if width == 0 or height == 0 then return end

    if not self._private.resize_forbidden then
        -- Let's scale the image so that it fits into (width, height)
        local w = self._private.image:get_width()
        local h = self._private.image:get_height()
        local aspect = width / w
        local aspect_h = height / h
        if aspect > aspect_h then aspect = aspect_h end

        cr:scale(aspect, aspect)
    end

    -- Set the clip
    if self._private.clip_shape then
        cr:clip(self._private.clip_shape(cr, width, height, unpack(self._private.clip_args)))
    end

    cr:set_source_surface(self._private.image, 0, 0)
    cr:paint()
end

-- Fit the imagebox into the given geometry
function imagebox:fit(_, width, height)
    if not self._private.image then
        return 0, 0
    end

    local w = self._private.image:get_width()
    local h = self._private.image:get_height()

    if w > width then
        h = h * width / w
        w = width
    end
    if h > height then
        w = w * height / h
        h = height
    end

    if h == 0 or w == 0 then
        return 0, 0
    end

    if not self._private.resize_forbidden then
        local aspect = width / w
        local aspect_h = height / h

        -- Use the smaller one of the two aspect ratios.
        if aspect > aspect_h then aspect = aspect_h end

        w, h = w * aspect, h * aspect
    end

    return w, h
end

--- Set an imagebox' image
-- @property image
-- @param image Either a string or a cairo image surface. A string is
--   interpreted as the path to a png image file.
-- @return true on success, false if the image cannot be used

function imagebox:set_image(image)
    if type(image) == "string" then
        image = surface.load(image)
        if not image then
            print(debug.traceback())
            return false
        end
    end

    image = image and surface.load(image)

    if image then
        local w = image.width
        local h = image.height
        if w <= 0 or h <= 0 then
            return false
        end
    end

    if self._private.image == image then
        -- The image could have been modified, so better redraw
        self:emit_signal("widget::redraw_needed")
        return true
    end

    self._private.image = image

    self:emit_signal("widget::redraw_needed")
    self:emit_signal("widget::layout_changed")
    return true
end

--- Set a clip shape for this imagebox
-- A clip shape define an area where the content is displayed and one where it
-- is trimmed.
--
-- @property clip_shape
-- @param clip_shape A `gears_shape` compatible shape function
-- @see gears.shape
-- @see set_clip_shape

--- Set a clip shape for this imagebox
-- A clip shape define an area where the content is displayed and one where it
-- is trimmed.
--
-- Any other parameters will be passed to the clip shape function
--
-- @param clip_shape A `gears_shape` compatible shape function
-- @see gears.shape
-- @see clip_shape
function imagebox:set_clip_shape(clip_shape, ...)
    self._private.clip_shape = clip_shape
    self._private.clip_args = {...}
    self:emit_signal("widget::redraw_needed")
end

--- Should the image be resized to fit into the available space?
-- @property resize
-- @param allowed If false, the image will be clipped, else it will be resized
--   to fit into the available space.

function imagebox:set_resize(allowed)
    self._private.resize_forbidden = not allowed
    self:emit_signal("widget::redraw_needed")
    self:emit_signal("widget::layout_changed")
end

--- Returns a new imagebox.
-- Any other arguments will be passed to the clip shape function
-- @param image the image to display, may be nil
-- @param resize_allowed If false, the image will be clipped, else it will be resized
--   to fit into the available space.
-- @param clip_shape A `gears.shape` compatible function
-- @treturn table A new `imagebox`
-- @function wibox.widget.imagebox
local function new(image, resize_allowed, clip_shape)
    local ret = base.make_widget(nil, nil, {enable_properties = true})

    gtable.crush(ret, imagebox, true)

    if image then
        ret:set_image(image)
    end
    if resize_allowed ~= nil then
        ret:set_resize(resize_allowed)
    end

    ret._private.clip_shape = clip_shape
    ret._private.clip_args = {}

    return ret
end

function imagebox.mt:__call(...)
    return new(...)
end

--@DOC_widget_COMMON@

--@DOC_object_COMMON@

return setmetatable(imagebox, imagebox.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
