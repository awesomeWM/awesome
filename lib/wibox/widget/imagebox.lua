---------------------------------------------------------------------------
--
--@DOC_wibox_widget_defaults_imagebox_EXAMPLE@
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @widgetmod wibox.widget.imagebox
---------------------------------------------------------------------------

local lgi = require("lgi")
local cairo = lgi.cairo

local base = require("wibox.widget.base")
local surface = require("gears.surface")
local gtable = require("gears.table")
local gdebug = require("gears.debug")
local setmetatable = setmetatable
local type = type
local math = math

local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)

-- Safe load for optional Rsvg module
local Rsvg = nil
do
    local success, err = pcall(function() Rsvg = lgi.Rsvg end)
    if not success then
        gdebug.print_warning(debug.traceback("Could not load Rsvg: " .. tostring(err)))
    end
end

local imagebox = { mt = {} }

local rsvg_handle_cache = setmetatable({}, { __mode = 'v' })

---Load rsvg handle form image file
---@tparam string file Path to svg file.
---@return Rsvg handle
local function load_rsvg_handle(file)
    if not Rsvg then return end

    local cache = rsvg_handle_cache[file]
    if cache then
        return cache
    end

    local handle, err = Rsvg.Handle.new_from_file(file)
    if not err then
        rsvg_handle_cache[file] = handle
        return handle
    end
end

-- Draw an imagebox with the given cairo context in the given geometry.
function imagebox:draw(_, cr, width, height)
    if width == 0 or height == 0 or not self._private.default then return end

    -- Set the clip
    if self._private.clip_shape then
        cr:clip(self._private.clip_shape(cr, width, height, unpack(self._private.clip_args)))
    end

    if not self._private.resize_forbidden then
        -- Let's scale the image so that it fits into (width, height)
        local w, h = self._private.default.width, self._private.default.height
        local aspect = math.min(width / w, height / h)
        cr:scale(aspect, aspect)
    end

    if self._private.handle then
        self._private.handle:render_cairo(cr)
    else
        cr:set_source_surface(self._private.image, 0, 0)
        cr:paint()
    end
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

---Apply cairo surface for given imagebox widget
local function set_surface(ib, surf)
    local is_surd_valid = surf.width > 0 and surf.height > 0
    if not is_surd_valid then return end

    ib._private.default = { width = surf.width, height = surf.height }
    ib._private.handle = nil
    ib._private.image = surf
    return true
end

---Apply RsvgHandle for given imagebox widget
local function set_handle(ib, handle)
    local dim = handle:get_dimensions()
    local is_handle_valid = dim.width > 0 and dim.height > 0
    if not is_handle_valid then return end

    ib._private.default = { width = dim.width, height = dim.height }
    ib._private.handle = handle
    ib._private.image = nil
    return true
end

---Try to load some image object from file then apply it to imagebox.
---@tparam table ib Imagebox
---@tparam string file Image file name
---@tparam function image_loader Function to load image object from file
---@tparam function image_setter Function to set image object to imagebox
---@treturn boolean True if image was successfully applied
local function load_and_apply(ib, file, image_loader, image_setter)
    local image_applied
    local object = image_loader(file)
    if object then
        image_applied = image_setter(ib, object)
    end
    return image_applied
end

--- Set an imagebox' image
-- @property image
-- @param image This can can be string, cairo image surface, rsvg handle object or nil. A string is
--   interpreted as the path to an image file. Nil will deny previously set image.
-- @return true on success, false if the image cannot be used
function imagebox:set_image(image)
    local setup_succeed

    if type(image) == "string" then
        -- try to load rsvg handle from file
        setup_succeed = load_and_apply(self, image, load_rsvg_handle, set_handle)

        if not setup_succeed then
            -- rsvg handle failed, try to load cairo surface with pixbuf
            setup_succeed = load_and_apply(self, image, surface.load, set_surface)
        end
    elseif Rsvg and Rsvg.Handle:is_type_of(image) then
        -- try to apply given rsvg handle
        setup_succeed = set_handle(self, image)
    elseif cairo.Surface:is_type_of(image) then
        -- try to apply given cairo surface
        setup_succeed = set_surface(self, image)
    elseif not image then
        -- nil as argument mean full imagebox reset
        setup_succeed = true
        self._private.handle = nil
        self._private.image = nil
        self._private.default = nil
    end

    if not setup_succeed then return false end

    self:emit_signal("widget::redraw_needed")
    self:emit_signal("widget::layout_changed")
    return true
end

--- Set a clip shape for this imagebox
-- A clip shape define an area where the content is displayed and one where it
-- is trimmed.
--
-- @property clip_shape
-- @tparam gears.shape clip_shape A `gears_shape` compatible shape function
-- @see gears.shape
-- @see set_clip_shape

--- Set a clip shape for this imagebox
-- A clip shape define an area where the content is displayed and one where it
-- is trimmed.
--
-- Any other parameters will be passed to the clip shape function
--
-- @tparam function clip_shape A `gears_shape` compatible shape function.
-- @method set_clip_shape
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
-- @constructorfct wibox.widget.imagebox
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
