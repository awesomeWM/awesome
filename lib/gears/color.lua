---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @module gears.color
---------------------------------------------------------------------------

local setmetatable = setmetatable
local string = string
local table = table
local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)
local tonumber = tonumber
local ipairs = ipairs
local pairs = pairs
local type = type
local lgi = require("lgi")
local cairo = lgi.cairo
local Pango = lgi.Pango
local surface = require("gears.surface")

local color = { mt = {} }
local pattern_cache

--- Parse a HTML-color.
-- This function can parse colors like `#rrggbb` and `#rrggbbaa` and also `red`.
-- Thanks to #lua for this. :)
--
-- @param col The color to parse
-- @return 4 values which each are in the range [0, 1].
-- @usage -- This will return 0, 1, 0, 1
-- gears.color.parse_color("#00ff00ff")
function color.parse_color(col)
    local rgb = {}
    -- Is it a HTML-style color?
    if string.match(col, "^#%x+$") then
        -- Get all hex chars
        for char in string.gmatch(col, "[^#]") do
            table.insert(rgb, tonumber(char, 16) / 0xf)
        end
        -- Merge consecutive values until we have at most four groups (rgba)
        local factor = 0xf
        while #rgb > 4 do
            local merged = {}
            local key, value = next(rgb, nil)
            local next_factor = (factor + 1)*(factor + 1) - 1
            while key do
                local key2, value2 = next(rgb, key)
                local v1, v2 = value * factor, value2 * factor
                local new = v1 * (factor + 1) + v2
                table.insert(merged, new / next_factor)
                key, value = next(rgb, key2)
            end
            rgb = merged
            factor = next_factor
        end
    else
        -- Let's ask Pango for its opinion (but this doesn't support alpha!)
        local c = Pango.Color()
        if c:parse(col) then
            rgb = {
                c.red / 0xffff,
                c.green / 0xffff,
                c.blue / 0xffff,
            }
        end
    end
    -- Add missing groups (missing alpha)
    while #rgb < 4 do
        table.insert(rgb, 1)
    end
    return unpack(rgb)
end

--- Find all numbers in a string
--
-- @tparam string s The string to parse
-- @return Each number found as a separate value
local function parse_numbers(s)
    local res = {}
    for k in string.gmatch(s, "-?[0-9]+[.]?[0-9]*") do
        table.insert(res, tonumber(k))
    end
    return unpack(res)
end

--- Create a solid pattern
--
-- @param col The color for the pattern
-- @return A cairo pattern object
function color.create_solid_pattern(col)
    if col == nil then
        col = "#000000"
    elseif type(col) == "table" then
        col = col.color
    end
    return cairo.Pattern.create_rgba(color.parse_color(col))
end

--- Create an image pattern from a png file
--
-- @param file The filename of the file
-- @return a cairo pattern object
function color.create_png_pattern(file)
    if type(file) == "table" then
        file = file.file
    end
    local image = surface.load(file)
    local pattern = cairo.Pattern.create_for_surface(image)
    pattern:set_extend(cairo.Extend.REPEAT)
    return pattern
end

--- Add stops to the given pattern.
-- @param p The cairo pattern to add stops to
-- @param iterator An iterator that returns strings. Each of those strings
--   should be in the form place,color where place is in [0, 1].
local function add_iterator_stops(p, iterator)
    for k in iterator do
        local sub = string.gmatch(k, "[^,]+")
        local point, clr = sub(), sub()
        p:add_color_stop_rgba(point, color.parse_color(clr))
    end
end

--- Add a list of stops to a given pattern
local function add_stops_table(pat, arg)
    for _, stop in ipairs(arg) do
        pat:add_color_stop_rgba(stop[1], color.parse_color(stop[2]))
    end
end

--- Create a pattern from a string
local function string_pattern(creator, arg)
    local iterator = string.gmatch(arg, "[^:]+")
    -- Create a table where each entry is a number from the original string
    local args = { parse_numbers(iterator()) }
    local to = { parse_numbers(iterator()) }
    -- Now merge those two tables
    for _, v in pairs(to) do
        table.insert(args, v)
    end
    -- And call our creator function with the values
    local p = creator(unpack(args))

    add_iterator_stops(p, iterator)
    return p
end

--- Create a linear pattern object.
-- The pattern is created from a string. This string should have the following
-- form: `"x0, y0:x1, y1:<stops>"`
-- Alternatively, the pattern can be specified as a table:
--    { type = "linear", from = { x0, y0 }, to = { x1, y1 },
--      stops = { <stops> } }
-- `x0,y0` and `x1,y1` are the start and stop point of the pattern.
-- For the explanation of `<stops>`, see `color.create_pattern`.
-- @tparam string|table arg The argument describing the pattern.
-- @return a cairo pattern object
function color.create_linear_pattern(arg)
    local pat

    if type(arg) == "string" then
        return string_pattern(cairo.Pattern.create_linear, arg)
    elseif type(arg) ~= "table" then
        error("Wrong argument type: " .. type(arg))
    end

    pat = cairo.Pattern.create_linear(arg.from[1], arg.from[2], arg.to[1], arg.to[2])
    add_stops_table(pat, arg.stops)
    return pat
end

--- Create a radial pattern object.
-- The pattern is created from a string. This string should have the following
-- form: `"x0, y0, r0:x1, y1, r1:<stops>"`
-- Alternatively, the pattern can be specified as a table:
--    { type = "radial", from = { x0, y0, r0 }, to = { x1, y1, r1 },
--      stops = { <stops> } }
-- `x0,y0` and `x1,y1` are the start and stop point of the pattern.
-- `r0` and `r1` are the radii of the start / stop circle.
-- For the explanation of `<stops>`, see `color.create_pattern`.
-- @tparam string|table arg The argument describing the pattern
-- @return a cairo pattern object
function color.create_radial_pattern(arg)
    local pat

    if type(arg) == "string" then
        return string_pattern(cairo.Pattern.create_radial, arg)
    elseif type(arg) ~= "table" then
        error("Wrong argument type: " .. type(arg))
    end

    pat = cairo.Pattern.create_radial(arg.from[1], arg.from[2], arg.from[3],
            arg.to[1], arg.to[2], arg.to[3])
    add_stops_table(pat, arg.stops)
    return pat
end

--- Mapping of all supported color types. New entries can be added.
color.types = {
    solid = color.create_solid_pattern,
    png = color.create_png_pattern,
    linear = color.create_linear_pattern,
    radial = color.create_radial_pattern
}

--- Create a pattern from a given string.
-- For full documentation of this function, please refer to
-- `color.create_pattern`.  The difference between `color.create_pattern`
-- and this function is that this function does not insert the generated
-- objects into the pattern cache. Thus, you are allowed to modify the
-- returned object.
-- @see create_pattern
-- @param col The string describing the pattern.
-- @return a cairo pattern object
function color.create_pattern_uncached(col)
    -- If it already is a cairo pattern, just leave it as that
    if cairo.Pattern:is_type_of(col) then
        return col
    end
    col = col or "#000000"
    if type(col) == "string" then
        local t = string.match(col, "[^:]+")
        if color.types[t] then
            local pos = string.len(t)
            local arg = string.sub(col, pos + 2)
            return color.types[t](arg)
        end
    elseif type(col) == "table" then
        local t = col.type
        if color.types[t] then
            return color.types[t](col)
        end
    end
    return color.create_solid_pattern(col)
end

--- Create a pattern from a given string.
-- This function can create solid, linear, radial and png patterns. In general,
-- patterns are specified as strings formatted as"type:arguments". "arguments"
-- is specific to the pattern used. For example, one can use
-- "radial:50,50,10:55,55,30:0,#ff0000:0.5,#00ff00:1,#0000ff"
-- Alternatively, patterns can be specified via tables. In this case, the
-- table's 'type' member specifies the type. For example:
-- { type = "radial", from = { 50, 50, 10 }, to = { 55, 55, 30 },
--   stops = { { 0, "#ff0000" }, { 0.5, "#00ff00" }, { 1, "#0000ff" } } }
-- Any argument that cannot be understood is passed to @{create_solid_pattern}.
--
-- Please note that you MUST NOT modify the returned pattern, for example by
-- calling :set_matrix() on it, because this function uses a cache and your
-- changes could thus have unintended side effects. Use @{create_pattern_uncached}
-- if you need to modify the returned pattern.
-- @see create_pattern_uncached, create_solid_pattern, create_png_pattern,
--   create_linear_pattern, create_radial_pattern
-- @param col The string describing the pattern.
-- @return a cairo pattern object
function color.create_pattern(col)
    -- If it already is a cairo pattern, just leave it as that
    if cairo.Pattern:is_type_of(col) then
        return col
    end
    return pattern_cache:get(col or "#000000")
end

--- Check if a pattern is opaque.
-- A pattern is transparent if the background on which it gets drawn (with
-- operator OVER) doesn't influence the visual result.
-- @param col An argument that `create_pattern` accepts.
-- @return The pattern if it is surely opaque, else nil
function color.create_opaque_pattern(col)
    local pattern = color.create_pattern(col)
    local kind = pattern:get_type()

    if kind == "SOLID" then
        local _, _, _, _, alpha = pattern:get_rgba()
        if alpha ~= 1 then
            return
        end
        return pattern
    elseif kind == "SURFACE" then
        local status, surf = pattern:get_surface()
        if status ~= "SUCCESS" or surf.content ~= "COLOR" then
            -- The surface has an alpha channel which *might* be non-opaque
            return
        end

        -- Only the "NONE" extend mode is forbidden, everything else doesn't
        -- introduce transparent parts
        if pattern:get_extend() == "NONE" then
            return
        end

        return pattern
    elseif kind == "LINEAR" then
        local _, stops = pattern:get_color_stop_count()

        -- No color stops or extend NONE -> pattern *might* contain transparency
        if stops == 0 or pattern:get_extend() == "NONE" then
            return
        end

        -- Now check if any of the color stops contain transparency
        for i = 0, stops - 1 do
            local _, _, _, _, _, alpha = pattern:get_color_stop_rgba(i)
            if alpha ~= 1 then
                return
            end
        end
        return pattern
    end

    -- Unknown type, e.g. mesh or raster source or unsupported type (radial
    -- gradients can do weird self-intersections)
end

--- Fill non-transparent area of an image with a given color.
-- @param image Image or path to it.
-- @param new_color New color.
-- @return Recolored image.
function color.recolor_image(image, new_color)
    if type(image) == 'string' then
        image = surface.duplicate_surface(image)
    end
    local cr = cairo.Context.create(image)
    cr:set_source(color.create_pattern(new_color))
    cr:mask(cairo.Pattern.create_for_surface(image), 0, 0)
    return image
end

function color.mt.__call(_, ...)
    return color.create_pattern(...)
end

pattern_cache = require("gears.cache").new(color.create_pattern_uncached)

--- No color
color.transparent = color.create_pattern("#00000000")

return setmetatable(color, color.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
