---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @release @AWESOME_VERSION@
---------------------------------------------------------------------------

local setmetatable = setmetatable
local string = string
local table = table
local unpack = unpack or table.unpack -- v5.1: unpack, v5.2: table.unpack
local tonumber = tonumber
local ipairs = ipairs
local pairs = pairs
local type = type
local cairo = require("lgi").cairo
local surface = require("gears.surface")

local color = { mt = {} }
local pattern_cache = setmetatable({}, { __mode = 'v' })

--- Parse a HTML-color.
-- This function can parse colors like #rrggbb and #rrggbbaa.
-- For example, parse_color("#00ff00ff") would return 0, 1, 0, 1.
-- Thanks to #lua for this. :)
-- @param col The color to parse
-- @return 4 values which each are in the range [0, 1].
function color.parse_color(col)
    local rgb = {}
    for pair in string.gmatch(col, "[^#].") do
        local i = tonumber(pair, 16)
        if i then
            table.insert(rgb, i / 255)
        end
    end
    while #rgb < 4 do
        table.insert(rgb, 1)
    end
    return unpack(rgb)
end

--- Find all numbers in a string
-- @param s The string to parse
-- @return Each number found as a separate value
local function parse_numbers(s)
    local res = {}
    for k in string.gmatch(s, "-?[0-9]+[.]?[0-9]*") do
        table.insert(res, tonumber(k))
    end
    return unpack(res)
end

--- Create a solid pattern
-- @param col The color for the pattern
-- @return A cairo pattern object
function color.create_solid_pattern(col)
    local col = col
    if col == nil then
        col = "#000000"
    elseif type(col) == "table" then
        col = col.color
    end
    return cairo.Pattern.create_rgba(color.parse_color(col))
end

--- Create an image pattern from a png file
-- @param file The filename of the file
-- @return a cairo pattern object
function color.create_png_pattern(file)
    local file = file
    if type(file) == "table" then
        file = file.file
    end
    local image = surface.load(file)
    local pattern = cairo.Pattern.create_for_surface(image)
    pattern:set_extend(cairo.Extend.REPEAT)
    return pattern
end

-- Add stops to the given pattern.
-- @param p The cairo pattern to add stops to
-- @param iterator An iterator that returns strings. Each of those strings
--                 should be in the form place,color where place is in [0, 1].
local function add_iterator_stops(p, iterator)
    for k in iterator do
        local sub = string.gmatch(k, "[^,]+")
        local point, clr = sub(), sub()
        p:add_color_stop_rgba(point, color.parse_color(clr))
    end
end

-- Add a list of stops to a given pattern
local function add_stops_table(pat, arg)
    for _, stop in ipairs(arg) do
        pat:add_color_stop_rgba(stop[1], color.parse_color(stop[2]))
    end
end

-- Create a pattern from a string
local function string_pattern(creator, arg)
    local iterator = string.gmatch(arg, "[^:]+")
    -- Create a table where each entry is a number from the original string
    local args = { parse_numbers(iterator()) }
    local to = { parse_numbers(iterator()) }
    -- Now merge those two tables
    for k, v in pairs(to) do
        table.insert(args, v)
    end
    -- And call our creator function with the values
    local p = creator(unpack(args))

    add_iterator_stops(p, iterator)
    return p
end

--- Create a linear pattern object.
-- The pattern is created from a string. This string should have the following
-- form: "x0,y0:x1,y1:&#60;stops&#62;"
-- Alternatively, the pattern can be specified as a table:
-- { type = "linear", from = { x0, y0 }, to = { x1, y1 },
-- stops = { &#60stops&#62 } }
-- x0,y0 and x1,y1 are the start and stop point of the pattern.
-- For the explanation of "&#60;stops&#62;", see create_pattern().
-- @param arg The argument describing the pattern
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
-- form: "x0,y0,r0:x1,y1,r1:&#60stops&#62"
-- Alternatively, the pattern can be specified as a table:
-- { type = "radial", from = { x0, y0, r0 }, to = { x1, y1, r1 },
-- stops = { &#60stops&#62 } }
-- x0,y0 and x1,y1 are the start and stop point of the pattern.
-- r0 and r1 are the radii of the start / stop circle.
-- For the explanation of "&#60;stops&#62;", see create_pattern().
-- @param arg The argument describing the pattern
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
-- This function can create solid, linear, radial and png patterns. In general,
-- patterns are specified as strings formatted as"type:arguments". "arguments"
-- is specific to the pattern used. For example, one can use
-- "radial:50,50,10:55,55,30:0,#ff0000:0.5,#00ff00:1,#0000ff"
-- Alternatively, patterns can be specified via tables. In this case, the
-- table's 'type' member specifies the type. For example:
-- { type = "radial", from = { 50, 50, 10 }, to = { 55, 55, 30 },
--   stops = { { 0, "#ff0000" }, { 0.5, "#00ff00" }, { 1, "#0000ff" } } }
-- Any argument that cannot be understood is passed to create_solid_pattern().
-- @see create_solid_pattern, create_png_pattern, create_linear_pattern,
--      create_radial_pattern
-- @param col The string describing the pattern.
-- @return a cairo pattern object
function color.create_pattern(col)
    -- If it already is a cairo pattern, just leave it as that
    if cairo.Pattern:is_type_of(col) then
        return col
    end
    local col = col or "#000000"
    local result = pattern_cache[col]
    if result then
        return result
    end
    if type(col) == "string" then
        local t = string.match(col, "[^:]+")
        if color.types[t] then
            local pos = string.len(t)
            local arg = string.sub(col, pos + 2)
            result = color.types[t](arg)
        end
    elseif type(col) == "table" then
        local t = col.type
        if color.types[t] then
            result = color.types[t](col)
        end
    end
    if not result then
        result = color.create_solid_pattern(col)
    end
    pattern_cache[col] = result
    return result
end

--- Check if a pattern is opaque.
-- A pattern is transparent if the background on which it gets drawn (with
-- operator OVER) doesn't influence the visual result.
-- @param col An argument that create_pattern() accepts
-- @return The pattern if it is surely opaque, else nil
function color.create_opaque_pattern(col)
    local pattern = color.create_pattern(col)
    local type = pattern:get_type()
    local extend = pattern:get_extend()

    if type == "SOLID" then
        local status, r, g, b, a = pattern:get_rgba()
        if a ~= 1 then
            return
        end
        return pattern
    elseif type == "SURFACE" then
        local status, surface = pattern:get_surface()
        if status ~= "SUCCESS" or surface.content ~= "COLOR" then
            -- The surface has an alpha channel which *might* be non-opaque
            return
        end

        -- Only the "NONE" extend mode is forbidden, everything else doesn't
        -- introduce transparent parts
        if pattern:get_extend() == "NONE" then
            return
        end

        return pattern
    elseif type == "LINEAR" then
        local status, stops = pattern:get_color_stop_count()

        -- No color stops or extend NONE -> pattern *might* contain transparency
        if stops == 0 or pattern:get_extend() == "NONE" then
            return
        end

        -- Now check if any of the color stops contain transparency
        for i = 0, stops - 1 do
            local status, offset, r, g, b, a = pattern:get_color_stop_rgba(i)
            if a ~= 1 then
                return
            end
        end
        return pattern
    end

    -- Unknown type, e.g. mesh or raster source or unsupported type (radial
    -- gradients can do weird self-intersections)
end

function color.mt:__call(...)
    return color.create_pattern(...)
end

return setmetatable(color, color.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
