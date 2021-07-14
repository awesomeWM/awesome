---------------------------------------------------------------------------
-- A widget to display an image.
--
-- The `wibox.widget.imagebox` is part of the Awesome WM's widget system
-- (see @{03-declarative-layout.md}).
--
-- This widget displays an image. The image can be a file,
-- a cairo image surface, or an rsvg handle object (see the
-- [image property](#image)).
--
-- Examples using a `wibox.widget.imagebox`:
-- ---
--
-- @DOC_wibox_widget_defaults_imagebox_EXAMPLE@
--
-- Alternatively, you can declare the `imagebox` widget using the
-- declarative pattern (both variants are strictly equivalent):
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

    -- For valign = "top" and halign = "left"
    local translate = {
        x = 0,
        y = 0,
    }

    local w, h = self._private.default.width, self._private.default.height

    if not self._private.resize_forbidden then
        -- That's for the "fit" policy.
        local aspects = {
            w = width / w,
            h = height / h
        }

        local policy = {
            w = self._private.horizontal_fit_policy or "auto",
            h = self._private.vertical_fit_policy or "auto"
        }

        for _, aspect in ipairs {"w", "h"} do
            if policy[aspect] == "auto" then
                aspects[aspect] = math.min(width / w, height / h)
            elseif policy[aspect] == "none" then
                aspects[aspect] = 1
            end
        end

        if self._private.halign == "center" then
            translate.x = math.floor((width - w*aspects.w)/2)
        elseif self._private.halign == "right" then
            translate.x = math.floor(width - (w*aspects.w))
        end

        if self._private.valign == "center" then
            translate.y = math.floor((height - h*aspects.h)/2)
        elseif self._private.valign == "bottom" then
            translate.y = math.floor(height - (h*aspects.h))
        end

        cr:translate(translate.x, translate.y)

        -- Before using the scale, make sure it is below the threshold.
        local threshold, max_factor = self._private.max_scaling_factor, math.max(aspects.w, aspects.h)

        if threshold and threshold > 0 and threshold < max_factor then
            aspects.w = (aspects.w*threshold)/max_factor
            aspects.h = (aspects.h*threshold)/max_factor
        end

        -- Set the clip
        if self._private.clip_shape then
            cr:clip(self._private.clip_shape(cr, w*aspects.w, h*aspects.h, unpack(self._private.clip_args)))
        end

        cr:scale(aspects.w, aspects.h)
    else
        if self._private.halign == "center" then
            translate.x = math.floor((width - w)/2)
        elseif self._private.halign == "right" then
            translate.x = math.floor(width - w)
        end

        if self._private.valign == "center" then
            translate.y = math.floor((height - h)/2)
        elseif self._private.valign == "bottom" then
            translate.y = math.floor(height - h)
        end

        cr:translate(translate.x, translate.y)

        -- Set the clip
        if self._private.clip_shape then
            cr:clip(self._private.clip_shape(cr, w, h, unpack(self._private.clip_args)))
        end
    end

    if self._private.handle then
        self._private.handle:render_cairo(cr)
    else
        cr:set_source_surface(self._private.image, 0, 0)

        local filter = self._private.scaling_quality

        if filter then
            cr:get_source():set_filter(cairo.Filter[filter:upper()])
        end

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
-- * A `string`: Interpreted as a path to an image file
-- * A cairo image surface: Directly used as-is
-- * A librsvg handle object: Directly used as-is
-- * `nil`: Unset the image.
--
-- @property image
-- @tparam image image The image to render.
-- @propemits false false

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
--
-- A clip shape defines an area and dimension to which the content should be
-- trimmed.
--
-- @DOC_wibox_widget_imagebox_clip_shape_EXAMPLE@
--
-- @property clip_shape
-- @tparam function|gears.shape clip_shape A `gears.shape` compatible shape function.
-- @propemits true false
-- @see gears.shape

--- Set a clip shape for this imagebox.
--
-- A clip shape defines an area and dimensions to which the content should be
-- trimmed.
--
-- Additional parameters will be passed to the clip shape function.
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

--- Set the horizontal fit policy.
--
-- Valid values are:
--
--  * `"auto"`: Honor the `resize` variable and preserve the aspect ratio.
--   This is the default behaviour.
--  * `"none"`: Do not resize at all.
--  * `"fit"`: Resize to the widget width.
--
-- Here is the result for a 22x32 image:
--
-- @DOC_wibox_widget_imagebox_horizontal_fit_policy_EXAMPLE@
--
-- @property horizontal_fit_policy
-- @tparam[opt="auto"] string horizontal_fit_policy
-- @propemits true false
-- @see vertical_fit_policy
-- @see resize

--- Set the vertical fit policy.
--
-- Valid values are:
--
--  * `"auto"`: Honor the `resize` varible and preserve the aspect ratio.
--   This is the default behaviour.
--  * `"none"`: Do not resize at all.
--  * `"fit"`: Resize to the widget height.
--
-- Here is the result for a 32x22 image:
--
-- @DOC_wibox_widget_imagebox_vertical_fit_policy_EXAMPLE@
--
-- @property vertical_fit_policy
-- @tparam[opt="auto"] string horizontal_fit_policy
-- @propemits true false
-- @see horizontal_fit_policy
-- @see resize


--- The vertical alignment.
--
-- Possible values are:
--
-- * `"top"`
-- * `"center"` (default)
-- * `"bottom"`
--
-- @DOC_wibox_widget_imagebox_valign_EXAMPLE@
--
-- @property valign
-- @tparam[opt="center"] string valign
-- @propemits true false
-- @see wibox.container.place
-- @see halign

--- The horizontal alignment.
--
-- Possible values are:
--
-- * `"left"`
-- * `"center"` (default)
-- * `"right"`
--
-- @DOC_wibox_widget_imagebox_halign_EXAMPLE@
--
-- @property halign
-- @tparam[opt="center"] string halign
-- @propemits true false
-- @see wibox.container.place
-- @see valign

--- The maximum scaling factor.
--
-- If an image is scaled too much, it gets very blurry. This
-- property allows to limit the scaling.
-- Use the properties `valign` and `halign` to control how the image will be
-- aligned.
--
-- In the example below, the original size is 22x22
--
-- @DOC_wibox_widget_imagebox_max_scaling_factor_EXAMPLE@
--
-- @property max_scaling_factor
-- @tparam number max_scaling_factor
-- @propemits true false
-- @see valign
-- @see halign
-- @see scaling_quality

--- Set the scaling aligorithm.
--
-- Depending on how the image is used, what is the "correct" way to
-- scale can change. For example, upscaling a pixel art image should
-- not make it blurry. However, scaling up a photo should not make it
-- blocky.
--
--<table class='widget_list' border=1>
-- <tr style='font-weight: bold;'>
--  <th align='center'>Value</th>
--  <th align='center'>Description</th>
-- </tr>
-- <tr><td>fast</td><td>A high-performance filter</td></tr>
-- <tr><td>good</td><td>A reasonable-performance filter</td></tr>
-- <tr><td>best</td><td>The highest-quality available</td></tr>
-- <tr><td>nearest</td><td>Nearest-neighbor filtering (blocky)</td></tr>
-- <tr><td>bilinear</td><td>Linear interpolation in two dimensions</td></tr>
--</table>
--
-- The image used in the example below has a resolution of 32x22 and is
-- intentionally blocky to highlight the difference.
-- It is zoomed by a factor of 3.
--
-- @DOC_wibox_widget_imagebox_scaling_quality_EXAMPLE@
--
-- @property scaling_quality
-- @tparam string scaling_quality Either `"fast"`, `"good"`, `"best"`,
--   `"nearest"` or `"bilinear"`.
-- @propemits true false
-- @see resize
-- @see horizontal_fit_policy
-- @see vertical_fit_policy
-- @see max_scaling_factor

local defaults = {
    halign                = "left",
    valign                = "top",
    horizontal_fit_policy = "auto",
    vertical_fit_policy   = "auto",
    max_scaling_factor    = 0,
    scaling_quality       = "good"
}

local function get_default(prop, value)
    if value == nil then return defaults[prop] end

    return value
end

for prop in pairs(defaults) do
    imagebox["set_"..prop] = function(self, value)
        if value == self._private[prop] then return end

        self._private[prop] = get_default(prop, value)
        self:emit_signal("widget::redraw_needed")
        self:emit_signal("property::"..prop, self._private[prop])
    end

    imagebox["get_"..prop] = function(self)
        if self._private[prop] == nil then
            return defaults[prop]
        end

        return self._private[prop]
    end
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
