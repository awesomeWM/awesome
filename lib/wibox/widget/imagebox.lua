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
local gfs = require("gears.filesystem")
local setmetatable = setmetatable
local type = type
local math = math

local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)

-- Placeholder table to represent an emty stylesheet.
-- It has to be defined here to avoid being GCed
local empty_stylesheet = {}

local policies_to_extents = {
    ["pad"]     = cairo.Extend.PAD,
    ["repeat"]  = cairo.Extend.REPEAT,
    ["reflect"] = cairo.Extend.REFLECT,
}

-- Safe load for optional Rsvg module
local Rsvg = nil
do
    local success, err = pcall(function() Rsvg = lgi.Rsvg end)
    if not success then
        gdebug.print_warning(debug.traceback("Could not load Rsvg: " .. tostring(err)))
    end
end

local imagebox = { mt = {} }

local rsvg_handle_cache = setmetatable({}, { __mode = 'k' })
local stylesheet_cache = {}

--Load rsvg handle form image file
-- @tparam string file Path to svg file.
-- @return Rsvg handle
-- @treturn table A table where cached data can be stored.
function imagebox._load_rsvg_handle(file, style)
    -- Make sure this is called in the right order.
    assert((not style) or (style and stylesheet_cache[style]))

    local style_ref = style and stylesheet_cache[style] or empty_stylesheet

    if not Rsvg then return end

    local bucket = rsvg_handle_cache[file] or {}
    local cache = (bucket[style_ref] or {})["handle"]

    if cache then
        return cache, bucket[style_ref]
    end

    local handle, err

    if file:match("<[?]?xml") or file:match("<svg") then
        handle, err = Rsvg.Handle.new_from_data(file)
    else
        handle, err = Rsvg.Handle.new_from_file(file)
    end

    if not err then
        rsvg_handle_cache[file] = rsvg_handle_cache[file] or setmetatable({}, {__mode = "k"})
        rsvg_handle_cache[file][style_ref] = rsvg_handle_cache[file][style_ref] or {}
        rsvg_handle_cache[file][style_ref]["handle"] = handle
        return handle, rsvg_handle_cache[file][style_ref]
    end
end

---Apply cairo surface for given imagebox widget
local function set_surface(ib, surf)
    local is_surf_valid = surf.width > 0 and surf.height > 0
    if not is_surf_valid then return false end

    ib._private.default = { width = surf.width, height = surf.height }
    ib._private.handle = nil
    ib._private.image = surf
    return true
end

---Apply RsvgHandle for given imagebox widget
local function set_handle(ib, handle, cache)
    local dim = handle:get_dimensions()
    local is_handle_valid = dim.width > 0 and dim.height > 0
    if not is_handle_valid then return false end

    ib._private.default = { width = dim.width, height = dim.height }
    ib._private.handle = handle
    ib._private.cache = cache
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
    local object, cache = image_loader(file, ib._private.stylesheet_og)

    if object then
        image_applied = image_setter(ib, object, cache)
    end
    return image_applied
end

--- Support both CSS data and filepath for the stylsheet.
function imagebox._get_stylesheet(self, content_or_path)
    if not content_or_path then return nil end

    -- Always set the entry because the image cache uses it.
    stylesheet_cache[content_or_path] = stylesheet_cache[content_or_path]
        or setmetatable({}, {__mode = "v"})

    if gfs.file_readable(content_or_path) then
        local ret

        local _, obj = next(stylesheet_cache[content_or_path])

        if obj then
            ret = obj._private.stylesheet
            table.insert(stylesheet_cache[content_or_path], self)
        else
            local f = io.open(content_or_path, 'r')

            if not f then return nil end

            ret = f:read("*all")
            f:close()
            table.insert(stylesheet_cache[content_or_path], self)
        end

        return ret
    else
        return content_or_path
    end
end

---Update the cached size depending on the stylesheet and dpi.
--
-- It's necessary because a single RSVG handle can be used by
-- many imageboxes. So DPI and Stylesheet need to be set each time.
local function update_dpi(self, ctx)
    if not self._private.handle then return end

    local dpi = self._private.auto_dpi and
        ctx.dpi or
        self._private.dpi or
        nil

    local need_dpi = dpi and
        self._private.last_dpi ~= dpi

    local need_style = self._private.handle.set_stylesheet and
        self._private.stylesheet

    local old_size = self._private.default and self._private.default.width

    if dpi and dpi ~= self._private.cache.dpi then
        if type(dpi) == "table" then
            self._private.handle:set_dpi_x_y(dpi.x, dpi.y)
        else
            self._private.handle:set_dpi(dpi)
        end
    end

    if need_style and self._private.cache.stylesheet ~= self._private.stylesheet then
        self._private.handle:set_stylesheet(self._private.stylesheet)
    end

    -- Reload the size.
    if need_dpi or (need_style and self._private.stylesheet ~= self._private.last_stylesheet) then
        set_handle(self, self._private.handle, self._private.cache)
    end

    self._private.last_dpi = dpi
    self._private.cache.dpi = dpi
    self._private.last_stylesheet = self._private.stylesheet
    self._private.cache.stylesheet = self._private.stylesheet

    -- This can happen in the constructor when `dpi` is set after `image`.
    if old_size and old_size ~= self._private.default.width then
        self:emit_signal("widget::redraw_needed")
        self:emit_signal("widget::layout_changed")
    end
end

-- Draw an imagebox with the given cairo context in the given geometry.
function imagebox:draw(ctx, cr, width, height)
    if width == 0 or height == 0 or not self._private.default then return end

    -- For valign = "top" and halign = "left"
    local translate = {
        x = 0,
        y = 0,
    }

    update_dpi(self, ctx)

    local w, h = self._private.default.width, self._private.default.height

    local policy = {
        w = self._private.horizontal_fit_policy or "auto",
        h = self._private.vertical_fit_policy or "auto"
    }

    if self._private.resize then
        -- That's for the "fit" policy.
        local aspects = {
            w = width / w,
            h = height / h
        }

        for _, aspect in ipairs {"w", "h"} do
            if self._private.upscale == false and (w < width and h < height) then
                aspects[aspect] = 1
            elseif self._private.downscale == false and (w >= width and h >= height) then
                aspects[aspect] = 1
            elseif policy[aspect] == "none" or policies_to_extents[policy[aspect]] then
                aspects[aspect] = 1
            elseif policy[aspect] == "auto" then
                aspects[aspect] = math.min(width / w, height / h)
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
        -- Yes, it is possible that the vertical or horizontal policies both
        -- have extends, but Cairo doesn't support this. So be it.
        local pol = policies_to_extents[policy.w]
        pol = pol or policies_to_extents[policy.h]

        if pol then
            local pat = cairo.Pattern.create_for_surface(self._private.image)
            pat:set_extend(pol)
            cr:set_source(pat)
        else
            cr:set_source_surface(self._private.image, 0, 0)
        end

        local filter = self._private.scaling_quality

        if filter then
            cr:get_source():set_filter(cairo.Filter[filter:upper()])
        end

        cr:paint()
    end
end

-- Fit the imagebox into the given geometry
function imagebox:fit(ctx, width, height)
    if not self._private.default then return 0, 0 end

    update_dpi(self, ctx)

    local w, h = self._private.default.width, self._private.default.height

    if w <= width and h <= height and self._private.upscale == false then
        return w, h
    end

    if (w < width or h < height) and self._private.downscale == false then
        return w, h
    end

    if self._private.resize or w > width or h > height then
        local aspect = math.min(width / w, height / h)
        return w * aspect, h * aspect
    end

    return w, h
end

--- The image rendered by the `imagebox`.
--
-- @property image
-- @tparam[opt=nil] image|nil image
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

    -- Keep the original to prevent the cache from being GCed.
    self._private.original_image = image

    if type(image) == "userdata" and not (Rsvg and Rsvg.Handle:is_type_of(image)) then
        -- This function is not documented to handle userdata objects, but
        -- historically it did, and it did by just assuming they refer to a
        -- cairo surface.
        image = surface.load(image)
    end

    if type(image) == "string" then
        -- try to load rsvg handle from file
        setup_succeed = load_and_apply(self, image, imagebox._load_rsvg_handle, set_handle)

        if not setup_succeed then
            -- rsvg handle failed, try to load cairo surface with pixbuf
            setup_succeed = load_and_apply(self, image, surface.load, set_surface)
        end
    elseif Rsvg and Rsvg.Handle:is_type_of(image) then
        -- try to apply given rsvg handle
        rsvg_handle_cache[image] = rsvg_handle_cache[image] or {}
        setup_succeed = set_handle(self, image, rsvg_handle_cache[image])
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
-- @tparam[opt=gears.shape.rectangle] shape clip_shape A `gears.shape` compatible shape function.
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
--
-- Note that `upscale` and `downscale` can affect the value of `resize`.
-- If conflicting values are passed to the constructor, then the result
-- is undefined.
--
-- @DOC_wibox_widget_imagebox_resize_EXAMPLE@
-- @property resize
-- @propemits true false
-- @tparam[opt=true] boolean resize

--- Allow the image to be upscaled (made bigger).
--
-- Note that `upscale` and `downscale` can affect the value of `resize`.
-- If conflicting values are passed to the constructor, then the result
-- is undefined.
--
-- @DOC_wibox_widget_imagebox_upscale_EXAMPLE@
-- @property upscale
-- @tparam[opt=self.resize] boolean upscale
-- @see downscale
-- @see resize

--- Allow the image to be downscaled (made smaller).
--
-- Note that `upscale` and `downscale` can affect the value of `resize`.
-- If conflicting values are passed to the constructor, then the result
-- is undefined.
--
-- @DOC_wibox_widget_imagebox_downscale_EXAMPLE@
-- @property downscale
-- @tparam[opt=self.resize] boolean downscale
-- @see upscale
-- @see resize

--- Set the SVG CSS stylesheet.
--
-- If the image is an SVG (vector graphics), this property allows to set
-- a CSS stylesheet. It can be used to set colors and much more.
--
-- The value can be either CSS data or a file path.
--
--@DOC_wibox_widget_imagebox_stylesheet_EXAMPLE@
--
-- @property stylesheet
-- @tparam[opt=""] string stylesheet
-- @propemits true false

--- Set the SVG DPI (dot per inch).
--
-- Force a specific DPI when rendering the `.svg`. For other file formats,
-- this does nothing.
--
-- It can either be a number of a table containing the `x` and `y` keys.
--
-- Please note that DPI and `resize` can "fight" each other and end up
-- making the image smaller instead of bigger.
--
--@DOC_wibox_widget_imagebox_dpi_EXAMPLE@
--
-- @property dpi
-- @tparam[opt=96] number|table dpi
-- @negativeallowed false
-- @propemits true false
-- @see auto_dpi

--- Use the object DPI when rendering the SVG.
--
-- By default, the SVG are interpreted as-is. When this property is set,
-- the screen DPI will be passed to the SVG renderer. Depending on which
-- tool was used to create the `.svg`, this may do nothing at all. However,
-- for example, if the `.svg` uses `<text>` elements and doesn't have an
-- hardcoded stylesheet, the result will differ.
--
-- @property auto_dpi
-- @tparam[opt=false] boolean auto_dpi
-- @propemits true false
-- @see dpi

for _, prop in ipairs {"dpi", "auto_dpi"} do
    imagebox["set_" .. prop] = function(self, value)
        local old = self._private[prop]

        -- It will be set in :fit and :draw. The handle is shared
        -- by multiple imagebox, so it cannot be set just once.
        self._private[prop] = value

        self:emit_signal("widget::redraw_needed")
        self:emit_signal("widget::layout_changed")
        self:emit_signal("property::" .. prop, value, old)
    end
end

function imagebox:set_stylesheet(value)
    if value == self._private.stylesheet_og then return end

    local old = self._private.stylesheet_og

    if old and stylesheet_cache[old] then
        for k, v in ipairs(stylesheet_cache[old]) do
            if self == v then
                table.remove(stylesheet_cache[old], k)
                break
            end
        end
    end

    local content = imagebox._get_stylesheet(self, value)

    self._private.stylesheet    = content
    self._private.stylesheet_og = value

    -- Refresh the pixmap.
    self.image = self._private.original_image

    self:emit_signal("widget::redraw_needed")
    self:emit_signal("widget::layout_changed")
    self:emit_signal("property::stylesheet", value)
end

function imagebox:set_resize(allowed)
    self._private.resize = allowed

    if allowed then
        self._private.downscale = true
        self._private.upscale = true
        self:emit_signal("property::downscale", allowed)
        self:emit_signal("property::upscale", allowed)
    end

    self:emit_signal("widget::redraw_needed")
    self:emit_signal("widget::layout_changed")
    self:emit_signal("property::resize", allowed)
end

for _, prop in ipairs {"downscale", "upscale" } do
    imagebox["set_" .. prop] = function(self, allowed)
        self._private[prop] = allowed

        if self._private.resize ~= (self._private.upscale or self._private.downscale) then
            self._private.resize = self._private.upscale or self._private.downscale
            self:emit_signal("property::resize", self._private.resize)
        end

        self:emit_signal("widget::redraw_needed")
        self:emit_signal("widget::layout_changed")
        self:emit_signal("property::"..prop, allowed)
    end
end

--- Set the horizontal fit policy.
--
-- Note that `repeat`, `reflect` and `pad` cannot be mixed across
-- the vertical and horizontal axis.
--
-- Here is the result for a 22x32 image:
--
-- @DOC_wibox_widget_imagebox_horizontal_fit_policy_EXAMPLE@
--
-- @property horizontal_fit_policy
-- @tparam[opt="auto"] string horizontal_fit_policy
-- @propertyvalue "auto" Honor the `resize` variable and preserve the aspect ratio.
-- @propertyvalue "none" Do not resize at all.
-- @propertyvalue "fit" Resize to the widget width.
-- @propertyvalue "repeat"` Repeat the image side by side.
-- @propertyvalue "reflect"` Like `repeat`, but alternate the reflection.
-- @propertyvalue "pad"` Take the last column of pixels and repeat them.
-- @propemits true false
-- @see vertical_fit_policy
-- @see resize

--- Set the vertical fit policy.
--
-- Valid values are:
--
-- Note that `repeat`, `reflect` and `pad` cannot be mixed across
-- the vertical and horizontal axis.
--
-- Here is the result for a 32x22 image:
--
-- @DOC_wibox_widget_imagebox_vertical_fit_policy_EXAMPLE@
--
-- @property vertical_fit_policy
-- @tparam[opt="auto"] string vertical_fit_policy
-- @propertyvalue "auto" Honor the `resize` variable and preserve the aspect ratio.
-- @propertyvalue "none" Do not resize at all.
-- @propertyvalue "fit" Resize to the widget height.
-- @propertyvalue "fit" Resize to the widget width.
-- @propertyvalue "repeat"` Repeat the image side by side.
-- @propertyvalue "reflect"` Like `repeat`, but alternate the reflection.
-- @propertyvalue "pad"` Take the last column of pixels and repeat them.
-- @propemits true false
-- @see horizontal_fit_policy
-- @see resize


--- The vertical alignment.
--
-- @DOC_wibox_widget_imagebox_valign_EXAMPLE@
--
-- @property valign
-- @tparam[opt="center"] string valign
-- @propertyvalue "top"
-- @propertyvalue "center"
-- @propertyvalue "bottom"
-- @propemits true false
-- @see wibox.container.place
-- @see halign

--- The horizontal alignment.
--
-- @DOC_wibox_widget_imagebox_halign_EXAMPLE@
--
-- @property halign
-- @tparam[opt="center"] string halign
-- @propertyvalue "left"
-- @propertyvalue "center"
-- @propertyvalue "right"
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
-- @tparam[opt=0] number max_scaling_factor Use `0` for "no limit".
-- @negativeallowed false
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
-- @tparam[opt="good"] string scaling_quality
-- @propertyvalue "fast" A high-performance filter.
-- @propertyvalue "good" A reasonable-performance filter.
-- @propertyvalue "best" The highest-quality available.
-- @propertyvalue "nearest" Nearest-neighbor filtering (blocky).
-- @propertyvalue "bilinear" Linear interpolation in two dimensions.
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
    ret._private.resize = true

    if image then
        ret:set_image(image)
    end

    if resize_allowed ~= nil then
        ret.resize = resize_allowed
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
