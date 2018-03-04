---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2012 Uli Schlachter
-- @module gears.surface
---------------------------------------------------------------------------

local setmetatable = setmetatable
local type = type
local capi = { awesome = awesome }
local cairo = require("lgi").cairo
local GdkPixbuf = require("lgi").GdkPixbuf
local color = nil
local gdebug = require("gears.debug")
local hierarchy = require("wibox.hierarchy")

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
-- @param _surface The surface to load or nil
-- @param default The default value to return on error; when nil, then a surface
-- in an error state is returned.
-- @return The loaded surface, or the replacement default
-- @return An error message, or nil on success
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
-- @param _surface The surface to load or nil
-- @param default The default value to return on error; when nil, then a surface
-- in an error state is returned.
-- @return The loaded surface, or the replacement default, or nil if called with
-- nil.
-- @return An error message, or nil on success
function surface.load_silently(_surface, default)
    if type(_surface) == "string" then
        local cache = surface_cache[_surface]
        if cache then
            return cache
        end
        local result, err = surface.load_uncached_silently(_surface, default)
        if not err then
            -- Cache the file
            surface_cache[_surface] = result
        end
        return result, err
    end
    return surface.load_uncached_silently(_surface, default)
end

local function do_load_and_handle_errors(_surface, func)
    if type(_surface) == 'nil' then
        return get_default()
    end
    local result, err = func(_surface, false)
    if result then
        return result
    end
    gdebug.print_error(debug.traceback(
        "Failed to load '" .. tostring(_surface) .. "': " .. tostring(err)))
    return get_default()
end

--- Try to convert the argument into an lgi cairo surface.
-- This is usually needed for loading images by file name. Errors are handled
-- via `gears.debug.print_error`.
-- @param _surface The surface to load or nil
-- @return The loaded surface, or nil
function surface.load_uncached(_surface)
    return do_load_and_handle_errors(_surface, surface.load_uncached_silently)
end

--- Try to convert the argument into an lgi cairo surface.
-- This is usually needed for loading images by file name. Errors are handled
-- via `gears.debug.print_error`.
-- @param _surface The surface to load or nil
-- @return The loaded surface, or nil
function surface.load(_surface)
    return do_load_and_handle_errors(_surface, surface.load_silently)
end

function surface.mt.__call(_, ...)
    return surface.load(...)
end

--- Get the size of a cairo surface
-- @param surf The surface you are interested in
-- @return The surface's width and height
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
-- @param[opt=white] shape_color The shape color or pattern
-- @param[opt=transparent] bg_color The surface background color
-- @treturn cairo.surface the new surface
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
-- @param draw A wibox or a client
-- @param shape or gears.shape function or a custom function with a context,
--   width and height as parameter.
-- @param[opt] Any additional parameters will be passed to the shape function
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
-- @deprecated wibox.widget.draw_to_svg_file
-- @tparam widget widget A widget
-- @tparam string path The output file path
-- @tparam number width The surface width
-- @tparam number height The surface height
-- @return The cairo surface
-- @return The hierarchy
function surface.widget_to_svg(widget, path, width, height)
    gdebug.deprecate("Use wibox.widget.draw_to_svg_file instead of "..
        "gears.surface.render_to_svg", {deprecated_in=5})
    local img = cairo.SvgSurface.create(path, width, height)
    local cr = cairo.Context(img)

    return img, run_in_hierarchy(widget, cr, width, height)
end

--- Create a cairo surface with this widget content.
-- This is dynamic, so the SVG will be updated along with the widget content.
-- because of this, the painting may happen hover multiple event loop cycles.
-- @deprecated wibox.widget.draw_to_image_surface
-- @tparam widget widget A widget
-- @tparam number width The surface width
-- @tparam number height The surface height
-- @param[opt=cairo.Format.ARGB32] format The surface format
-- @return The cairo surface
-- @return The hierarchy
function surface.widget_to_surface(widget, width, height, format)
    gdebug.deprecate("Use wibox.widget.draw_to_image_surface instead of "..
        "gears.surface.render_to_surface", {deprecated_in=5})
    local img = cairo.ImageSurface(format or cairo.Format.ARGB32, width, height)
    local cr = cairo.Context(img)

    return img, run_in_hierarchy(widget, cr, width, height)
end

return setmetatable(surface, surface.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
