---------------------------------------------------------------------------
--
--@DOC_wibox_widget_defaults_imagebox_EXAMPLE@
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @classmod wibox.widget.imagebox
---------------------------------------------------------------------------

local base = require("wibox.widget.base")
local surface = require("gears.surface")
local gtable = require("gears.table")
local setmetatable = setmetatable
local type = type
local cairo = require("lgi").cairo
local print = print
local math = math
local string = string

local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)

local imagebox = { mt = {} }

-- Draw an imagebox with the given cairo context in the given geometry.
function imagebox:draw(_, cr, width, height)
    if width == 0 or height == 0 or not self._private.default then return end
    local _surface = self._private.image

    if not self._private.resize_forbidden then
        -- Let's scale the image so that it fits into (width, height)
        local w, h = self._private.default.width, self._private.default.height
        local aspect = math.min(width / w, height / h)

        if self._private.pixbuf_scale then
            -- advanced scaling with pixbuf
            local requested_size = { height = math.floor(w * aspect), width = math.floor(h * aspect) }
            _surface = self:_generate_surface(nil, requested_size)
            if not _surface then return end
        else
            --regular raster scaling with cairo
            cr:scale(aspect, aspect)
        end
    end

    -- Set the clip
    if self._private.clip_shape then
        cr:clip(self._private.clip_shape(cr, width, height, unpack(self._private.clip_args)))
    end

    cr:set_source_surface(_surface, 0, 0)
    cr:paint()
end

-- Fit the imagebox into the given geometry
function imagebox:fit(_, width, height)
    if not self._private.default then return 0, 0 end
    local w, h = self._private.default.width, self._private.default.height

    if not self._private.resize_forbidden or w > width or h > height then
        local aspect = math.min(width / w, height / h)
        return w * aspect, h * aspect
    end

    return w, h
end


---@local
---Generate cairo surface with requested size
---@tparam[opt] string source Custom file name, if not defined then current image file settled to imagebox
---  will be used.
---@tparam[opt] table size Requested surface size, *width* and *height* keys should be included both.
---@treturn cairo.surface the new surface
function imagebox:_generate_surface(source, size)
    source = source or self._private.pixbuf_source
    if not source then return end

    -- try to generate surface from image file
    local _surface = surface.load(source, size)
    if not _surface then
        print(debug.traceback())
        return
    elseif _surface.width <= 0 or _surface.height <= 0 then
        return
    end

    -- image cache implemented on 'surface' module side
    -- leave link to last used surface here to prevent it be garbage collected
    self._private.image = _surface
    return _surface
end

--- Set an imagebox' image
-- @property image
-- @param image Either a string or a cairo image surface. A string is
--   interpreted as the path to a png image file.
-- @return true on success, false if the image cannot be used
function imagebox:set_image(image)
    if type(image) == "string" and image ~= self._private.pixbuf_source then
        -- process image file
        local default_surface = self:_generate_surface(image)
        if default_surface then
            self._private.pixbuf_source = image
            self._private.pixbuf_scale = string.match(image, "%.svg$") ~= nil
        else
            return false
        end
    else
        -- clean up all pixbuf related things
        self._private.pixbuf_scale = false
        self._private.pixbuf_source = nil

        -- surface given as argument
        if image and cairo.Surface:is_type_of(image) and image.width > 0 and image.height > 0 then
            self._private.image = image
        else
            self._private.image = nil
        end
    end

    local _surface = self._private.image
    self._private.default = _surface and { width = _surface.width, height = _surface.height }

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
