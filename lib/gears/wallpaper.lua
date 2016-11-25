---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2012 Uli Schlachter
-- @module gears.wallpaper
---------------------------------------------------------------------------

local cairo = require("lgi").cairo
local color = require("gears.color")
local surface = require("gears.surface")
local timer = require("gears.timer")
local root = root

local wallpaper = { mt = {} }

local function root_geometry()
    local width, height = root.size()
    return { x = 0, y = 0, width = width, height = height }
end

-- Information about a pending wallpaper change, see prepare_context()
local pending_wallpaper = nil

local function get_screen(s)
    return s and screen[s]
end

--- Prepare the needed state for setting a wallpaper.
-- This function returns a cairo context through which a wallpaper can be drawn.
-- The context is only valid for a short time and should not be saved in a
-- global variable.
-- @param s The screen to set the wallpaper on or nil for all screens
-- @return[1] The available geometry (table with entries width and height)
-- @return[1] A cairo context that the wallpaper should be drawn to
function wallpaper.prepare_context(s)
    s = get_screen(s)

    local root_width, root_height = root.size()
    local geom = s and s.geometry or root_geometry()
    local source, target, cr

    if not pending_wallpaper then
        -- Prepare a pending wallpaper
        source = surface(root.wallpaper())
        target = source:create_similar(cairo.Content.COLOR, root_width, root_height)

        -- Set the wallpaper (delayed)
        timer.delayed_call(function()
            local paper = pending_wallpaper
            pending_wallpaper = nil
            wallpaper.set(paper.surface)
            paper.surface:finish()
        end)
    elseif root_width > pending_wallpaper.width or root_height > pending_wallpaper.height then
        -- The root window was resized while a wallpaper is pending
        source = pending_wallpaper.surface
        target = source:create_similar(cairo.Content.COLOR, root_width, root_height)
    else
        -- Draw to the already-pending wallpaper
        source = nil
        target = pending_wallpaper.surface
    end

    cr = cairo.Context(target)

    if source then
        -- Copy the old wallpaper to the new one
        cr:save()
        cr.operator = cairo.Operator.SOURCE
        cr:set_source_surface(source, 0, 0)
        cr:paint()
        cr:restore()
    end

    pending_wallpaper = {
        surface = target,
        width = root_width,
        height = root_height
    }

    -- Only draw to the selected area
    cr:translate(geom.x, geom.y)
    cr:rectangle(0, 0, geom.width, geom.height)
    cr:clip()

    return geom, cr
end

--- Set the current wallpaper.
-- @param pattern The wallpaper that should be set. This can be a cairo surface,
--   a description for gears.color or a cairo pattern.
-- @see gears.color
function wallpaper.set(pattern)
    if cairo.Surface:is_type_of(pattern) then
        pattern = cairo.Pattern.create_for_surface(pattern)
    end
    if type(pattern) == "string" or type(pattern) == "table" then
        pattern = color(pattern)
    end
    if not cairo.Pattern:is_type_of(pattern) then
        error("wallpaper.set() called with an invalid argument")
    end
    root.wallpaper(pattern._native)
end

--- Set a centered wallpaper.
-- @param surf The wallpaper to center. Either a cairo surface or a file name.
-- @param s The screen whose wallpaper should be set. Can be nil, in which case
--   all screens are set.
-- @param background The background color that should be used. Gets handled via
--   gears.color. The default is black.
-- @see gears.color
function wallpaper.centered(surf, s, background)
    local geom, cr = wallpaper.prepare_context(s)
    surf = surface.load_uncached(surf)
    background = color(background)

    -- Fill the area with the background
    cr.operator = cairo.Operator.SOURCE
    cr.source = background
    cr:paint()

    -- Now center the surface
    local w, h = surface.get_size(surf)
    cr:translate((geom.width - w) / 2, (geom.height - h) / 2)
    cr:rectangle(0, 0, w, h)
    cr:clip()
    cr:set_source_surface(surf, 0, 0)
    cr:paint()
    surf:finish()
end

--- Set a tiled wallpaper.
-- @param surf The wallpaper to tile. Either a cairo surface or a file name.
-- @param s The screen whose wallpaper should be set. Can be nil, in which case
--   all screens are set.
-- @param offset This can be set to a table with entries x and y.
function wallpaper.tiled(surf, s, offset)
    local _, cr = wallpaper.prepare_context(s)

    if offset then
        cr:translate(offset.x, offset.y)
    end

    surf = surface.load_uncached(surf)
    local pattern = cairo.Pattern.create_for_surface(surf)
    pattern.extend = cairo.Extend.REPEAT
    cr.source = pattern
    cr.operator = cairo.Operator.SOURCE
    cr:paint()
    surf:finish()
end

--- Set a maximized wallpaper.
-- @param surf The wallpaper to set. Either a cairo surface or a file name.
-- @param s The screen whose wallpaper should be set. Can be nil, in which case
--   all screens are set.
-- @param ignore_aspect If this is true, the image's aspect ratio is ignored.
--   The default is to honor the aspect ratio.
-- @param offset This can be set to a table with entries x and y.
function wallpaper.maximized(surf, s, ignore_aspect, offset)
    local geom, cr = wallpaper.prepare_context(s)
    surf = surface.load_uncached(surf)
    local w, h = surface.get_size(surf)
    local aspect_w = geom.width / w
    local aspect_h = geom.height / h

    if not ignore_aspect then
        aspect_h = math.max(aspect_w, aspect_h)
        aspect_w = math.max(aspect_w, aspect_h)
    end
    cr:scale(aspect_w, aspect_h)

    if offset then
        cr:translate(offset.x, offset.y)
    elseif not ignore_aspect then
        local scaled_width = geom.width / aspect_w
        local scaled_height = geom.height / aspect_h
        cr:translate((scaled_width - w) / 2, (scaled_height - h) / 2)
    end

    cr:set_source_surface(surf, 0, 0)
    cr.operator = cairo.Operator.SOURCE
    cr:paint()
    surf:finish()
end

--- Set a fitting wallpaper.
-- @param surf The wallpaper to set. Either a cairo surface or a file name.
-- @param s The screen whose wallpaper should be set. Can be nil, in which case
--   all screens are set.
-- @param background The background color that should be used. Gets handled via
--   gears.color. The default is black.
-- @see gears.color
function wallpaper.fit(surf, s, background)
    local geom, cr = wallpaper.prepare_context(s)
    surf = surface.load_uncached(surf)
    background = color(background)

    -- Fill the area with the background
    cr.operator = cairo.Operator.SOURCE
    cr.source = background
    cr:paint()

    -- Now fit the surface
    local w, h = surface.get_size(surf)
    local scale = geom.width / w
    if h * scale > geom.height then
       scale = geom.height / h
    end
    cr:translate((geom.width - (w * scale)) / 2, (geom.height - (h * scale)) / 2)
    cr:rectangle(0, 0, w * scale, h * scale)
    cr:clip()
    cr:scale(scale, scale)
    cr:set_source_surface(surf, 0, 0)
    cr:paint()
    surf:finish()
end

return wallpaper

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
