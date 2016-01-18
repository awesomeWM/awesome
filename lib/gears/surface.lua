---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2012 Uli Schlachter
-- @release @AWESOME_VERSION@
-- @module gears.surface
---------------------------------------------------------------------------

local setmetatable = setmetatable
local type = type
local capi = { awesome = awesome }
local cairo = require("lgi").cairo
local gdebug = require("gears.debug")

-- Keep this in sync with build-utils/lgi-check.sh!
local ver_major, ver_minor, ver_patch = string.match(require('lgi.version'), '(%d)%.(%d)%.(%d)')
if tonumber(ver_major) <= 0 and (tonumber(ver_minor) < 7 or (tonumber(ver_minor) == 7 and tonumber(ver_patch) < 1)) then
    error("lgi too old, need at least version 0.7.1")
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
    local file
    -- On nil, return some sane default
    if not _surface then
        return get_default(default)
    end
    -- Remove from cache if it was cached
    surface_cache[_surface] = nil
    -- lgi cairo surfaces don't get changed either
    if cairo.Surface:is_type_of(_surface) then
        return _surface
    end
    -- Strings are assumed to be file names and get loaded
    if type(_surface) == "string" then
        local err
        file = _surface
        _surface, err = capi.awesome.load_image(file)
        if not _surface then
            return get_default(default), err
        end
    end
    -- Everything else gets forced into a surface
    _surface = cairo.Surface(_surface, true)
    -- If we loaded a file, cache it
    if file then
        surface_cache[file] = _surface
    end
    return _surface
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
    gdebug.print_error("Failed to load '" .. tostring(_surface) .. "': " .. tostring(err))
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

function surface.mt:__call(...)
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
end

return setmetatable(surface, surface.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
