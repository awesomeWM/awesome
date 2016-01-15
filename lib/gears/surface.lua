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

-- Keep this in sync with build-utils/lgi-check.sh!
local ver_major, ver_minor, ver_patch = string.match(require('lgi.version'), '(%d)%.(%d)%.(%d)')
if tonumber(ver_major) <= 0 and (tonumber(ver_minor) < 7 or (tonumber(ver_minor) == 7 and tonumber(ver_patch) < 1)) then
    error("lgi too old, need at least version 0.7.1")
end

local surface = { mt = {} }
local surface_cache = setmetatable({}, { __mode = 'v' })

local dummy_surface = nil

--- Try to convert the argument into an lgi cairo surface.
-- This is usually needed for loading images by file name.
function surface.load_uncached(_surface)
    local file
    -- Nil is not changed
    if not _surface then
        return nil
    end
    -- Remove from cache if it was cached
    surface_cache[_surface] = nil
    -- lgi cairo surfaces don't get changed either
    if cairo.Surface:is_type_of(_surface) then
        return _surface
    end
    -- Strings are assumed to be file names and get loaded
    if type(_surface) == "string" then
        file = _surface
        _surface = capi.awesome.load_image(file)
        if not _surface then
            io.stderr:write("W: The requested surface failed to load\n", debug.traceback())

            -- Avoid calling cairo.Surface with nil, it will crash Awesome
            if not dummy_surface then
                dummy_surface = cairo.ImageSurface(cairo.Format.ARGB32, 1, 1)
            end

            return dummy_surface
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

function surface.load(_surface)
    if type(_surface) == "string" then
        local cache = surface_cache[_surface]
        if cache then
            return cache
        end
    end
    return surface.load_uncached(_surface)
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


return setmetatable(surface, surface.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
