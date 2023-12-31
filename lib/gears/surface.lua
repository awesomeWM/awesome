---------------------------------------------------------------------------
-- Utilities to integrate and manipulate Cairo drawing surfaces.
--
-- @author Uli Schlachter
-- @copyright 2012 Uli Schlachter
-- @module gears.surface
---------------------------------------------------------------------------

local setmetatable = setmetatable
local type = type
local capi = { awesome = awesome }
local cairo = require("lgi").cairo
local GdkPixbuf = require("lgi").GdkPixbuf
local color, beautiful = nil, nil
local gdebug = require("gears.debug")
local hierarchy = require("wibox.hierarchy")
local ceil = math.ceil

-- Keep this in sync with build-utils/lgi-check.c!
local ver_major, ver_minor, ver_patch = string.match(require('lgi.version'), '(%d)%.(%d)%.(%d)')
if tonumber(ver_major) <= 0 and (tonumber(ver_minor) < 8 or (tonumber(ver_minor) == 8 and tonumber(ver_patch) < 0)) then
    error("lgi too old, need at least version 0.8.0")
end

local surface = { mt = {} }
local surface_cache = setmetatable({}, { __mode = 'v' })

local function get_default(arg)
    if type(arg) == 'nil' then
        return cairo.ImageSurface(cairo.Format.ARGB32, 0, 0)
    end
    return arg
end

--- Try to convert the argument into an lgi cairo surface.
-- This is usually needed for loading images by file name.
-- @param surface The surface to load or nil
-- @param default The default value to return on error; when nil, then a surface
-- in an error state is returned.
-- @return The loaded surface, or the replacement default
-- @return An error message, or nil on success.
-- @staticfct load_uncached_silently
function surface.load_uncached_silently(_surface, default)
    -- On nil, return some sane default
    if not _surface then
        return get_default(default)
    end
    -- lgi cairo surfaces don't get changed either
    if cairo.Surface:is_type_of(_surface) then
        return _surface
    end
    -- Strings are assumed to be file names and get loaded
    if type(_surface) == "string" then
        local pixbuf, err = GdkPixbuf.Pixbuf.new_from_file(_surface)
        if not pixbuf then
            return get_default(default), tostring(err)
        end
        _surface = capi.awesome.pixbuf_to_surface(pixbuf._native, _surface)

        -- The shims implement load_image() to return a surface directly,
        -- instead of a lightuserdatum.
        if cairo.Surface:is_type_of(_surface) then
            return _surface
        end
    end
    -- Everything else gets forced into a surface
    return cairo.Surface(_surface, true)
end

--- Try to convert the argument into an lgi cairo surface.
-- This is usually needed for loading images by file name and uses a cache.
-- In contrast to `load()`, errors are returned to the caller.
-- @param surface The surface to load or nil
-- @param default The default value to return on error; when nil, then a surface
-- in an error state is returned.
-- @return The loaded surface, or the replacement default, or nil if called with
-- nil.
-- @return An error message, or nil on success.
-- @staticfct load_silently
function surface.load_silently(self, default)
    if type(self) == "string" then
        local cache = surface_cache[self]
        if cache then
            return cache
        end
        local result, err = surface.load_uncached_silently(self, default)
        if not err then
            -- Cache the file
            surface_cache[self] = result
        end
        return result, err
    end
    return surface.load_uncached_silently(self, default)
end

local function do_load_and_handle_errors(self, func)
    if type(self) == 'nil' then
        return get_default()
    end
    local result, err = func(self, false)
    if result then
        return result
    end
    gdebug.print_error(debug.traceback(
        "Failed to load '" .. tostring(self) .. "': " .. tostring(err)))
    return get_default()
end

--- Try to convert the argument into an lgi cairo surface.
-- This is usually needed for loading images by file name. Errors are handled
-- via `gears.debug.print_error`.
-- @param surface The surface to load or nil
-- @return The loaded surface, or nil
-- @staticfct load_uncached
function surface.load_uncached(self)
    return do_load_and_handle_errors(self, surface.load_uncached_silently)
end

--- Try to convert the argument into an lgi cairo surface.
-- This is usually needed for loading images by file name. Errors are handled
-- via `gears.debug.print_error`.
-- @param surface The surface to load or nil
-- @return The loaded surface, or nil.
-- @staticfct gears.surface
function surface.load(self)
    return do_load_and_handle_errors(self, surface.load_silently)
end

function surface.mt.__call(_, ...)
    return surface.load(...)
end

--- Get the size of a cairo surface
-- @param surf The surface you are interested in
-- @return The surface's width and height.
-- @staticfct get_size
function surface.get_size(surf)
    local cr = cairo.Context(surf)
    local x, y, w, h = cr:clip_extents()
    return w - x, h - y
end

--- Create a copy of a cairo surface.
-- The surfaces returned by `surface.load` are cached and must not be
-- modified to avoid unintended side-effects. This function allows to create
-- a copy of a cairo surface. This copy can then be freely modified.
-- The surface returned will be as compatible as possible to the input
-- surface. For example, it will likely be of the same surface type as the
-- input. The details are explained in the `create_similar` function on a cairo
-- surface.
-- @param s Source surface.
-- @return The surface's duplicate.
-- @staticfct duplicate_surface
function surface.duplicate_surface(s)
    s = surface.load(s)

    -- Figure out surface size (this does NOT work for unbounded recording surfaces)
    local cr = cairo.Context(s)
    local x, y, w, h = cr:clip_extents()

    -- Create a copy
    local result = s:create_similar(s.content, w - x, h - y)
    cr = cairo.Context(result)
    cr:set_source_surface(s, 0, 0)
    cr.operator = cairo.Operator.SOURCE
    cr:paint()
    return result
end

--- Create a surface from a `gears.shape`
-- Any additional parameters will be passed to the shape function
-- @tparam number width The surface width
-- @tparam number height The surface height
-- @param shape A `gears.shape` compatible function
-- @param[opt="#000000"] shape_color The shape color or pattern
-- @param[opt="#00000000"] bg_color The surface background color
-- @treturn cairo.surface the new surface
-- @staticfct load_from_shape
function surface.load_from_shape(width, height, shape, shape_color, bg_color, ...)
    color = color or require("gears.color")

    local img = cairo.ImageSurface(cairo.Format.ARGB32, width, height)
    local cr = cairo.Context(img)

    cr:set_source(color(bg_color or "#00000000"))
    cr:paint()

    cr:set_source(color(shape_color or "#000000"))

    shape(cr, width, height, ...)

    cr:fill()

    return img
end

--- Apply a shape to a client or a wibox.
--
--  If the wibox or client size change, this function need to be called
--   again.
-- @tparam client|wibox draw A wibox or a client.
-- @tparam gears.shape|function shape The shape.
-- @param[opt] ... Any additional parameters will be passed to the shape function.
-- @staticfct apply_shape_bounding
-- @noreturn
function surface.apply_shape_bounding(draw, shape, ...)
  local geo = draw:geometry()

  local img = cairo.ImageSurface(cairo.Format.A1, geo.width, geo.height)
  local cr = cairo.Context(img)

  cr:set_operator(cairo.Operator.CLEAR)
  cr:set_source_rgba(0,0,0,1)
  cr:paint()
  cr:set_operator(cairo.Operator.SOURCE)
  cr:set_source_rgba(1,1,1,1)

  shape(cr, geo.width, geo.height, ...)

  cr:fill()

  draw.shape_bounding = img._native
  img:finish()
end

local function no_op() end

local function run_in_hierarchy(self, cr, width, height)
    local context = {dpi=96}
    local h = hierarchy.new(context, self, width, height, no_op, no_op, {})
    h:draw(context, cr)
    return h
end

--- Create an SVG file with this widget content.
-- This is dynamic, so the SVG will be updated along with the widget content.
-- because of this, the painting may happen hover multiple event loop cycles.
-- @deprecated widget_to_svg
-- @tparam widget widget A widget
-- @tparam string path The output file path
-- @tparam number width The surface width
-- @tparam number height The surface height
-- @return The cairo surface
-- @return The hierarchy.
-- @see wibox.widget.draw_to_svg_file
-- @see wibox.widget.draw_to_image_surface
function surface.widget_to_svg(widget, path, width, height)
    gdebug.deprecate("Use wibox.widget.draw_to_svg_file instead of "..
        "gears.surface.widget_to_svg", {deprecated_in=5})
    local img = cairo.SvgSurface.create(path, width, height)
    local cr = cairo.Context(img)

    -- Bad dependecy, but this is deprecated.
    beautiful = beautiful or require("beautiful")
    color = color or require("gears.color")
    cr:set_source(color(beautiful.fg_normal))

    return img, run_in_hierarchy(widget, cr, width, height)
end

--- Create a cairo surface with this widget content.
-- This is dynamic, so the SVG will be updated along with the widget content.
-- because of this, the painting may happen hover multiple event loop cycles.
-- @deprecated widget_to_surface
-- @tparam widget widget A widget
-- @tparam number width The surface width
-- @tparam number height The surface height
-- @param[opt=cairo.Format.ARGB32] format The surface format
-- @return The cairo surface
-- @return The hierarchy.
-- @see wibox.widget.draw_to_svg_file
-- @see wibox.widget.draw_to_image_surface
function surface.widget_to_surface(widget, width, height, format)
    gdebug.deprecate("Use wibox.widget.draw_to_image_surface instead of "..
        "gears.surface.render_to_surface", {deprecated_in=5})
    local img = cairo.ImageSurface(format or cairo.Format.ARGB32, width, height)
    local cr = cairo.Context(img)

    -- Bad dependecy, but this is deprecated.
    color = color or require("gears.color")
    beautiful = beautiful or require("beautiful")
    cr:set_source(color(beautiful.fg_normal))

    return img, run_in_hierarchy(widget, cr, width, height)
end

--- Crop a surface on its edges.
-- @tparam[opt=nil] table args
-- @tparam[opt=0] integer args.left Left cutoff, cannot be negative
-- @tparam[opt=0] integer args.right Right cutoff, cannot be negative
-- @tparam[opt=0] integer args.top Top cutoff, cannot be negative
-- @tparam[opt=0] integer args.bottom Bottom cutoff, cannot be negative
-- @tparam[opt=nil] number|nil args.ratio Ratio to crop the image to. If edge cutoffs and
-- ratio are given, the edge cutoffs are computed first. Using ratio will crop
-- the center out of an image, similar to what "zoomed-fill" does in wallpaper
-- setter programs. Cannot be negative
-- @tparam[opt=nil] surface args.surface The surface to crop
-- @return The cropped surface
-- @staticfct crop_surface
function surface.crop_surface(args)
    args = args or {}

    if not args.surface then
        error("No surface to crop_surface supplied")
        return nil
    end

    local surf = args.surface
    local target_ratio = args.ratio

    local w, h = surface.get_size(surf)
    local offset_w, offset_h =  0, 0

    if (args.top or args.right or args.bottom or args.left) then
        local left = args.left or 0
        local right = args.right or 0
        local top = args.top or 0
        local bottom = args.bottom or 0

        if (top < 0 or right < 0 or bottom < 0 or left < 0) then
            error("negative offsets are not supported for crop_surface")
        end

        w = w - left - right
        h = h - top - bottom

        -- the offset needs to be negative
        offset_w = - left
        offset_h = - top

        -- breaking stuff with cairo crashes awesome with no way to restart in place
        -- so here are checks for user error
        if w <= 0 or h <= 0 then
            error("Area to remove cannot be larger than the image size")
            return nil
        end
    end

    if target_ratio and target_ratio > 0 then
        local prev_ratio = w/h
        if prev_ratio ~= target_ratio then
            if (prev_ratio < target_ratio) then
                local old_h = h
                h = ceil(w * (1/target_ratio))
                offset_h = offset_h - ceil((old_h - h)/2)
            else
                local old_w = w
                w = ceil(h * target_ratio)
                offset_w = offset_w - ceil((old_w - w)/2)
            end
        end
    end

    local ret = cairo.ImageSurface(cairo.Format.ARGB32, w, h)
    local cr = cairo.Context(ret)
    cr:set_source_surface(surf, offset_w, offset_h)
    cr.operator = cairo.Operator.SOURCE
    cr:paint()

    return ret
end

return setmetatable(surface, surface.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
