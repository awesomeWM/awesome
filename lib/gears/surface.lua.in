---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2012 Uli Schlachter
-- @release @AWESOME_VERSION@
---------------------------------------------------------------------------

local setmetatable = setmetatable
local type = type
local capi = { awesome = awesome }
local cairo = require("lgi").cairo

-- Keep this in sync with build-utils/lgi-check.sh!
local ver_major, ver_minor = string.match(require('lgi.version'), '(%d)%.(%d)')
if tonumber(ver_major) <= 0 and tonumber(ver_minor) < 7 then
    error("lgi too old, need at least version 0.7.0")
end

-- gears.surface
local surface = { mt = {} }
local surface_cache = setmetatable({}, { __mode = 'v' })

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

return setmetatable(surface, surface.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
