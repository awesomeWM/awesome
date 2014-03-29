---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2012 Uli Schlachter
-- @release @AWESOME_VERSION@
---------------------------------------------------------------------------

local cairo = require("lgi").cairo
local color = require("gears.color")
local surface = require("gears.surface")

-- gears.wallpaper
local wallpaper = { mt = {} }

-- The size of the root window
local root_geom
do
    local geom = screen[1].geometry
    root_geom = {
        x = 0, y = 0,
        width = geom.x + geom.width,
        height = geom.y + geom.height
    }
    for s = 1, screen.count() do
        local g = screen[s].geometry
        root_geom.width = math.max(root_geom.width, g.x + g.width)
        root_geom.height = math.max(root_geom.height, g.y + g.height)
    end
end

--- Prepare the needed state for setting a wallpaper
-- @param s The screen to set the wallpaper on or nil for all screens
-- @return The available geometry (table with entries width and height), a
--         that should be used for setting the wallpaper and a cairo context
--         for drawing to this surface
local function prepare_wallpaper(s)
    local geom = s and screen[s].geometry or root_geom
    local img = surface(root.wallpaper())

    if not img then
        -- No wallpaper yet, create an image surface for just the part we need
        img = cairo.ImageSurface(cairo.Format.RGB24, geom.width, geom.height)
        img:set_device_offset(-geom.x, -geom.y)
    end

    local cr = cairo.Context(img)

    -- Only draw to the selected area
    cr:translate(geom.x, geom.y)
    cr:rectangle(0, 0, geom.width, geom.height)
    cr:clip()

    return geom, img, cr
end

--- Set the current wallpaper.
-- @param pattern The wallpaper that should be set. This can be a cairo surface,
--                a description for gears.color or a cairo pattern.
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
--          all screens are set.
-- @param background The background color that should be used. Gets handled via
--                   gears.color. The default is black.
function wallpaper.centered(surf, s, background)
    local geom, img, cr = prepare_wallpaper(s)
    local surf = surface(surf)
    local background = color(background)

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

    wallpaper.set(img)
end

--- Set a tiled wallpaper.
-- @param surf The wallpaper to tile. Either a cairo surface or a file name.
-- @param s The screen whose wallpaper should be set. Can be nil, in which case
--          all screens are set.
-- @param offset This can be set to a table with entries x and y.
function wallpaper.tiled(surf, s, offset)
    local geom, img, cr = prepare_wallpaper(s)

    if offset then
        cr:translate(offset.x, offset.y)
    end

    local pattern = cairo.Pattern.create_for_surface(surface(surf))
    pattern.extend = cairo.Extend.REPEAT
    cr.source = pattern
    cr.operator = cairo.Operator.SOURCE
    cr:paint()

    wallpaper.set(img)
end

--- Set a maximized wallpaper.
-- @param surf The wallpaper to set. Either a cairo surface or a file name.
-- @param s The screen whose wallpaper should be set. Can be nil, in which case
--          all screens are set.
-- @param ignore_aspect If this is true, the image's aspect ratio is ignored.
--                      The default is to honor the aspect ratio.
-- @param offset This can be set to a table with entries x and y.
function wallpaper.maximized(surf, s, ignore_aspect, offset)
    local geom, img, cr = prepare_wallpaper(s)
    local surf = surface(surf)
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
    end

    cr:set_source_surface(surf, 0, 0)
    cr.operator = cairo.Operator.SOURCE
    cr:paint()

    wallpaper.set(img)
end

--- Set a fitting wallpaper.
-- @param surf The wallpaper to set. Either a cairo surface or a file name.
-- @param s The screen whose wallpaper should be set. Can be nil, in which case
--          all screens are set.
-- @param background The background color that should be used. Gets handled via
--                   gears.color. The default is black.
function wallpaper.fit(surf, s, background)
    local geom, img, cr = prepare_wallpaper(s)
    local surf = surface(surf)
    local background = color(background)

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

    wallpaper.set(img)
end

return wallpaper

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
