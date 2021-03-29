---------------------------------------------------------------------------
-- A widget to display image.
--
-- The `wibox.widget.imagebox` is part of the Awesome WM's wiboxes system
-- (see @{03-declarative-layout.md}).
--
-- This widget displays an image. The image can be a file,
-- a cairo image surface, or an rsvg handle object (see the
-- [image property](#image)).
--
-- Use a `wibox.widget.imagebox`
-- ---
--
-- @DOC_wibox_widget_defaults_imagebox_EXAMPLE@
--
-- Alternatively, you can declare the `imagebox` widget using the
-- declarative pattern (Both codes are strictly equivalent):
--
-- @DOC_wibox_widget_declarative-pattern_imagebox_EXAMPLE@
--
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @widgetmod wibox.widget.imagebox
-- @supermodule wibox.widget.base
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

--- The image rendered by the `imagebox`.
--
-- It can can be any of the following:
--
-- * A `string` : Interpreted as the path to an image file,
-- * A cairo image surface : Directly used as is,
-- * An rsvg handle object : Directly used as is,
-- * `nil` : Unset the image.
--
-- @property image
-- @tparam image image The image to render.
-- @propemits false false
-- @see set_image

--- Set the `imagebox` image.
--
-- The image can be a file, a cairo image surface, or an rsvg handle object
-- (see the [image property](#image)).
-- @method set_image
-- @hidden
-- @tparam image image The image to render.
-- @treturn boolean `true` on success, `false` if the image cannot be used.
-- @usage my_imagebox:set_image(beautiful.awesome_icon)
-- @usage my_imagebox:set_image('/usr/share/icons/theme/my_icon.png')
-- @see image
function imagebox:set_image(image)
    local setup_succeed

    if type(image) == "userdata" then
        -- This function is not documented to handle userdata objects, but
        -- historically it did, and it did by just assuming they refer to a
        -- cairo surface.
        image = surface.load(image)
    end

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
    self:emit_signal("property::image")
    return true
end

--- Set a clip shape for this imagebox.
-- A clip shape define an area where the content is displayed and one where it
-- is trimmed.
--
-- @property clip_shape
-- @tparam function|gears.shape clip_shape A `gears.shape` compatible shape function.
-- @propemits true false
-- @see gears.shape
-- @see set_clip_shape

--- Set a clip shape for this imagebox.
-- A clip shape define an area where the content is displayed and one where it
-- is trimmed.
--
-- Any other parameters will be passed to the clip shape function.
--
-- @tparam function|gears.shape clip_shape A `gears_shape` compatible shape function.
-- @method set_clip_shape
-- @hidden
-- @see gears.shape
-- @see clip_shape
function imagebox:set_clip_shape(clip_shape, ...)
    self._private.clip_shape = clip_shape
    self._private.clip_args = {...}
    self:emit_signal("widget::redraw_needed")
    self:emit_signal("property::clip_shape", clip_shape)
end

--- Should the image be resized to fit into the available space?
-- @DOC_wibox_widget_imagebox_resize_EXAMPLE@
-- @property resize
-- @propemits true false
-- @tparam boolean resize

--- Should the image be resized to fit into the available space?
-- @tparam boolean allowed If `false`, the image will be clipped, else it will
--   be resized to fit into the available space.
-- @method set_resize
-- @hidden
function imagebox:set_resize(allowed)
    self._private.resize_forbidden = not allowed
    self:emit_signal("widget::redraw_needed")
    self:emit_signal("widget::layout_changed")
    self:emit_signal("property::resize", allowed)
end

--- Returns a new `wibox.widget.imagebox` instance.
--
-- This is the constructor of `wibox.widget.imagebox`. It creates a new
-- instance of imagebox widget.
--
-- Alternatively, the declarative layout syntax can handle
-- `wibox.widget.imagebox` instanciation.
--
-- The image can be a file, a cairo image surface, or an rsvg handle object
-- (see the [image property](#image)).
--
-- Any additional arguments will be passed to the clip shape function.
-- @tparam[opt] image image The image to display (may be `nil`).
-- @tparam[opt] boolean resize_allowed If `false`, the image will be
--   clipped, else it will be resized to fit into the available space.
-- @tparam[opt] function clip_shape A `gears.shape` compatible function.
-- @treturn wibox.widget.imagebox A new `wibox.widget.imagebox` widget instance.
-- @constructorfct wibox.widget.imagebox
local function new(image, resize_allowed, clip_shape, ...)
    local ret = base.make_widget(nil, nil, {enable_properties = true})

    gtable.crush(ret, imagebox, true)

    if image then
        ret:set_image(image)
    end
    if resize_allowed ~= nil then
        ret:set_resize(resize_allowed)
    end

    ret._private.clip_shape = clip_shape
    ret._private.clip_args = {...}

    return ret
end

function imagebox.mt:__call(...)
    return new(...)
end

return setmetatable(imagebox, imagebox.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
