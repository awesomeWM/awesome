---------------------------------------------------------------------------
-- This module simplifies the creation and modification of colors.
--
-- A color can be created from a normal hexadecimal value such as 0x33CCFF, a
-- table of 3 or 4 values which describe a color in either RGB[A] or HSL[A], or
-- a string which will take any form a CSS color can be described with.
--
-- The easiest way to create a color is with a string:
--     colorful.color("#FFCC99")
--     colorful.color("rgb(255, 204, 153, 1)")
--     colorful.color("rgb(100%, 80%, 60%)")
--     colorful.color("hsl(30, 100%, 80%, 100%)")
--
-- Compatible CSS color strings. Spaces between values and commas are optional.
-- For the whitespace functional notations (no commas), at least one space is
-- required between values.
--     -- Hex codes
--     #rgb
--     #rgba
--     #rrggbb
--     #rrggbbaa
--     -- RGB[A]
--     rgb([0,255], [0,255], [0,255])
--     rgb([0,255], [0,255], [0,255], [0,1])
--     rgb([0,255], [0,255], [0,255], [0,100]%)
--     rgb([0,255] [0,255] [0,255])
--     rgb([0,255] [0,255] [0,255] / [0,1])
--     rgb([0,255] [0,255] [0,255] / [0,100]%)
--     rgb([0,100]%, [0,100]%, [0,100]%)
--     rgb([0,100]%, [0,100]%, [0,100]%, [0,1])
--     rgb([0,100]%, [0,100]%, [0,100]%, [0,100]%)
--     rgb([0,100]% [0,100]% [0,100]%)
--     rgb([0,100]% [0,100]% [0,100]% / [0,1])
--     rgb([0,100]% [0,100]% [0,100]% / [0,100]%)
--     rgba([0,255], [0,255], [0,255], [0,1])
--     rgba([0,255], [0,255], [0,255], [0,100]%)
--     rgba([0,255] [0,255] [0,255] / [0,1])
--     rgba([0,255] [0,255] [0,255] / [0,100]%)
--     rgba([0,100]%, [0,100]%, [0,100]%, [0,1])
--     rgba([0,100]%, [0,100]%, [0,100]%, [0,100]%)
--     rgba([0,100]% [0,100]% [0,100]% / [0,1])
--     rgba([0,100]% [0,100]% [0,100]% / [0,100]%)
--     -- HSL[A]
--     hsl([0,360], [0,100]%, [0,100]%)
--     hsl([0,360], [0,100]%, [0,100]%, [0,1])
--     hsl([0,360], [0,100]%, [0,100]%, [0,100]%)
--     hsl([0,360] [0,100]% [0,100]%)
--     hsl([0,360] [0,100]% [0,100]% / [0,1])
--     hsl([0,360] [0,100]% [0,100]% / [0,100]%)
--     hsla([0,360], [0,100]%, [0,100]%, [0,1])
--     hsla([0,360], [0,100]%, [0,100]%, [0,100]%)
--     hsla([0,360] [0,100]% [0,100]% / [0,1])
--     hsla([0,360] [0,100]% [0,100]% / [0,100]%)
--
-- Additionally you can create a color from a hexadecimal value:
--     colorful.color(0xFFCC99)
--     colorful.color(0x336699AA)
--
-- Or you can create it from a table:
--     colorful.color({30, 1, 0.8})
--     colorful.color({0xff, 0xcc, 0x99})
--     colorful.color({red=255, green=204, blue=153})
--     colorful.color({hue=30, saturation=1, lightness=0.8})
--
-- One note about creating from a table, the new function will try to determine
-- what values you're passing in by checking the range of the values, or if the
-- table has named fields. It first checks for red, green, blue fields (optional
-- alpha field); then for hue, saturation, lightness; then if it has 3 or 4
-- fields. If it gets to a 3 or 4 field table, it will check the first for HSL
-- values, then for RGB.
--
-- After a color is created you can access it's individual elements as needed.
--     c = colorful.color.new("#ffcc99")
--     c.red        -- 1.0
--     c.green      -- 0.8
--     c.blue       -- 0.6
--     c.hue        -- 30
--     c.saturation -- 80%
--     c.lightness  -- 60%
--     c.alpha      -- 1.0 Note this defaults to no transparency
--
-- Once a color is created you can do transformations on it. Note that they do
-- not modify the original color object, but instead return a new one.
--     c2 = colorful.color("#99ccff")
--     c3 = c:mix(c2, 0.4) -- mix c with c2, giving c 40% and c2 60%
--     c:darken(0.25) -- darken 25%
--     c:lighten(0.25) -- lighten 25%
--     c:saturate(0.1) -- saturate 10%
--     c:desaturate(0.1) -- desaturate 10%
--     c:grayscale() -- desaturate color completely to grayscale
--     c:invert() -- inverts color
--     c:complement() -- the compliment of the color (180deg hue shift)
--
-- If you want to create a color scheme you can use the color harmony functions:
--     local left, right = c:analogous()
--     local left, right = c:split_complementary()
--     local left, right = c:triadic()
--     local complement, analogous, analogous_complement = c:tetradic()
--
-- @author Kevin Zander
-- @copyright 2019 Kevin Zander
-- @module beautiful.colorful
---------------------------------------------------------------------------

local colorful = {}

local color = {
    -- set the from and as tables
    from = {},
}

-- Metatable for color instances. This is set up near the end of the file.
local color_mt = {}

-- {{{ Helper functions

-- Lua 5.1 does not support bit shifting operators
local function lshift(x, disp)
    return (x * 2^disp) % 2^32
end

local function rshift(x, disp)
    return math.floor(x % 2^32 / 2^disp)
end

-- Do we want to pull in gears.math.round?
local function round(x)
    local d,_ = math.modf(x + (x >= 0 and 1 or -1) * 0.5)
    return d
end

-- Clamp a number between a low and high
local function clamp(v, l, h)
    if v < l then return l end
    if v > h then return h end
    return v
end

-- Return true/false if a number is between a low and high
local function between(v, l, h)
    if l <= v and v <= h then return true end
    return false
end

-- The formulas in hsl_to_rgb and rgb_to_hsl are described at the
-- Wikipedia page https://en.wikipedia.org/wiki/HSL_and_HSV

-- Expects Hue [0,360], Saturation [0,1], Lightness [0,1]
-- Does NOT check bounds
local function hsl_to_rgb(hue, saturation, lightness)
    -- If s = 0 then it's a grey
    if saturation == 0 then
        return lightness, lightness, lightness
    end
    local C = (1 - math.abs(2 * lightness - 1)) * saturation
    local X = C * (1 - math.abs((hue / 60) % 2 - 1))
    local m = lightness - (C / 2)
    local r,g,b = 0,0,0
    if 0 <= hue and hue <= 60 then
        r,g,b = C,X,0
    elseif 60 <= hue and hue <= 120 then
        r,g,b = X,C,0
    elseif 120 <= hue and hue <= 180 then
        r,g,b = 0,C,X
    elseif 180 <= hue and hue <= 240 then
        r,g,b = 0,X,C
    elseif 240 <= hue and hue <= 300 then
        r,g,b = X,0,C
    elseif 300 <= hue and hue <= 360 then
        r,g,b = C,0,X
    end
    return r+m, g+m, b+m
end

-- Expects Red, Blue, and Green [0,1]
-- Does NOT check bounds
local function rgb_to_hsl(red, green, blue)
    local h,s,l
    local M = math.max(red, green, blue)
    local m = math.min(red, green, blue)
    local C = M - m
    -- get Hue
    if C == 0 then
        h = 0 -- undefined technically
    elseif M == red then
        h = ((green - blue) / C) % 6
    elseif M == green then
        h = ((blue - red) / C) + 2
    elseif M == blue then
        h = ((red - green) / C) + 4
    end
    h = 60 * h
    -- Lightness/Luminosity
    l = (M + m) / 2
    -- Saturation
    if l == 1 or l == 0 then
        s = 0
    else
        s = C / (1 - math.abs((2 * l) - 1))
    end
    return h, s, l
end

-- Create a color object
local function create_color_object(r,g,b,h,s,l,a)
    local o = setmetatable({
        red = r, green = g, blue = b,
        hue = h, saturation = s, lightness = l,
        alpha = a
    }, color_mt)
    return o
end

-- }}} Helper functions

-- {{{ New Color functions

--- Color object creation functions
-- @section color_creation

-- {{{ color.new()

--- Create a new color instance
-- @param val A CSS color compatible string, decimal color value, or a table
-- containing 3 or 4 values.
-- @return A new color object, or nil if invalid
-- @usage local c = colorful.color.new({30, 0.8, 0.6})
-- -- For more examples see the top of this page
function color.new(val)
    if type(val) == "string" then return color.from.string(val) end
    if type(val) == "number" then
        if val > 0xFFFFFF then return color.from.hex4(val) end
        return color.from.hex3(val)
    end
    if type(val) == "table" then
        -- do we have red/green/blue/alpha fields?
        if val.red ~= nil and val.green ~= nil and val.blue ~= nil then
            local alpha = val.alpha or 255
            return color.from.rgba(val.red, val.green, val.blue, alpha)
        -- do we have hue/saturation/lightness/alpha fields?
        elseif val.hue ~= nil and val.saturation ~= nil and val.lightness ~= nil then
            local alpha = val.alpha or 1
            return color.from.hsla(val.hue, val.saturation, val.lightness, alpha)
        -- do we have a table of 3 or 4 values?
        elseif #val == 3 or #val == 4 then
            -- if val[1] [0,360] and val[2] and val[3] [0,1] we deduce HSL
            if (between(val[1], 0, 360) and
                between(val[2], 0, 1)   and
                between(val[3], 0, 1)      )
            then
                local alpha = val[4] and clamp(val[4], 0, 1) or 1
                return color.from.hsla(
                    val[1],
                    val[2],
                    val[3],
                    alpha
                )
            end
            if (between(val[1], 0, 255) and
                between(val[2], 0, 255) and
                between(val[3], 0, 255)    )
            then
                local alpha = val[4] and clamp(val[4], 0, 255) or 255
                return color.from.rgba(
                    val[1],
                    val[2],
                    val[3],
                    alpha
                )
            end
            -- if values aren't in range, escape
        end
        -- don't know the table type, escape
    end
    -- check for C userdata (GdkRGBA) with values
    if type(val) == "userdata" then
        if val.red ~= nil and val.green ~= nil and val.blue ~= nil then
            local alpha = val.alpha or 1
            return color.from.rgba(
                val.red * 255,
                val.green * 255,
                val.blue * 255,
                alpha * 255
            )
        end
    end
    -- not a valid type, quit
    return nil
end

-- }}} color.new()

-- {{{ String
-- {{{ Helper tables and functions

local css = {
    integer = "%d?%d?%d",   -- 0-999 (but 0-255 for values)
    float   = "%d?%.?%d+",  -- this is _very_ crude, but probably fast
    comma   = "%s*,%s*",    -- whitespace optional comma
    space   = "%s+",        -- whitespace required
    slash   = "%s*/%s*",    -- whitespace optional slash
    rgb     = "^rgb%(%s*",  -- rgb functional header
    rgba    = "^rgba%(%s*", -- rgba functional header
    hsl     = "^hsl%(%s*",  -- hsl functional header
    hsla    = "^hsla%(%s*", -- hsla functional header
    eol     = "%s*%)$",     -- functional EOL
}
-- capture a string
local function capt(s)
    return "(" .. s .. ")"
end
-- match string followed by %
local function perc(s)
    return s .. "%%"
end

-- }}} Helper tables and functions
-- {{{ Color patterns

-- identifier string = 3 char type, value type, alpha type (if present)
local color_patterns = {
    -- {{{ Hexadecimal patterns
    -- shorthand #rgb
    { "hexs", "^#(%x)(%x)(%x)$" },
    -- shorthand #rgba
    { "hexsa", "^#(%x)(%x)(%x)(%x)$" },
    -- longform #rrggbb
    { "hexd", "%#(%x%x)(%x%x)(%x%x)$" },
    -- longform #rrggbbaa
    { "hexda", "%#(%x%x)(%x%x)(%x%x)(%x%x)$" },
    -- }}}
    -- {{{ Functional patterns
    -- {{{ rgb(d, d, d) integers 0-255
    { "rgbi",
        css.rgb .. -- functional header
        capt(css.integer) .. -- red
        css.comma .. -- separator
        capt(css.integer) .. -- green
        css.comma .. -- separator
        capt(css.integer) .. -- blue
        css.eol -- EOL
    },
    -- }}}
    -- {{{ rgb(d, d, d, f) integers 0-255, float 0-1
    { "rgbif",
        css.rgb .. -- functional header
        capt(css.integer) .. -- red
        css.comma .. -- separator
        capt(css.integer) .. -- green
        css.comma .. -- separator
        capt(css.integer) .. -- blue
        css.comma .. -- separator
        capt(css.float) .. -- alpha
        css.eol -- EOL
    },
    -- }}}
    -- {{{ rgb(d, d, d, %) integers 0-255, percentage 0-100
    { "rgbip",
        css.rgb .. -- functional header
        capt(css.integer) .. -- red
        css.comma .. -- separator
        capt(css.integer) .. -- green
        css.comma .. -- separator
        capt(css.integer) .. -- blue
        css.comma ..
        perc(capt(css.integer)) .. -- alpha
        css.eol -- EOL
    },
    -- }}}
    -- {{{ rgb(d d d) integers 0-255
    { "rgbi",
        css.rgb .. -- functional header
        capt(css.integer) .. -- red
        css.space .. -- separator
        capt(css.integer) .. -- green
        css.space .. -- separator
        capt(css.integer) .. -- blue
        css.eol -- EOL
    },
    -- }}}
    -- {{{ rgb(d d d / f) integers 0-255, float 0-1
    { "rgbif",
        css.rgb .. -- functional header
        capt(css.integer) .. -- red
        css.space .. -- separator
        capt(css.integer) .. -- green
        css.space .. -- separator
        capt(css.integer) .. -- blue
        css.slash ..
        capt(css.float) .. -- alpha
        css.eol -- EOL
    },
    -- }}}
    -- {{{ rgb(d d d / %) integers 0-255, percentage 0-100
    { "rgbip",
        css.rgb .. -- functional header
        capt(css.integer) .. -- red
        css.space .. -- separator
        capt(css.integer) .. -- green
        css.space .. -- separator
        capt(css.integer) .. -- blue
        css.slash ..
        perc(capt(css.integer)) .. -- alpha
        css.eol -- EOL
    },
    -- }}}
    -- {{{ rgb(%, %, %) percentage 0-100
    { "rgbp",
        css.rgb .. -- functional header
        perc(capt(css.integer)) .. -- red
        css.comma .. -- separator
        perc(capt(css.integer)) .. -- green
        css.comma .. -- separator
        perc(capt(css.integer)) .. -- blue
        css.eol -- EOL
    },
    -- }}}
    -- {{{ rgb(%, %, %, f) percentage 0-100, float 0-1
    { "rgbpf",
        css.rgb .. -- functional header
        perc(capt(css.integer)) .. -- red
        css.comma .. -- separator
        perc(capt(css.integer)) .. -- green
        css.comma .. -- separator
        perc(capt(css.integer)) .. -- blue
        css.comma ..
        capt(css.float) .. -- alpha
        css.eol -- EOL
    },
    -- }}}
    -- {{{ rgb(%, %, %, %) percentage 0-100
    { "rgbpp",
        css.rgb .. -- functional header
        perc(capt(css.integer)) .. -- red
        css.comma .. -- separator
        perc(capt(css.integer)) .. -- green
        css.comma .. -- separator
        perc(capt(css.integer)) .. -- blue
        css.comma .. -- separator
        perc(capt(css.integer)) .. -- alpha
        css.eol -- EOL
    },
    -- }}}
    -- {{{ rgb(% % %) percentage 0-100
    { "rgbp",
        css.rgb .. -- functional header
        perc(capt(css.integer)) .. -- red
        css.space .. -- separator
        perc(capt(css.integer)) .. -- green
        css.space .. -- separator
        perc(capt(css.integer)) .. -- blue
        css.eol -- EOL
    },
    -- }}}
    -- {{{ rgb(% % % / f) percentage 0-100, float 0-1
    { "rgbpf",
        css.rgb .. -- functional header
        perc(capt(css.integer)) .. -- red
        css.space .. -- separator
        perc(capt(css.integer)) .. -- green
        css.space .. -- separator
        perc(capt(css.integer)) .. -- blue
        css.slash ..
        capt(css.float) .. -- alpha
        css.eol -- EOL
    },
    -- }}}
    -- {{{ rgb(% % % / %) percentage 0-100
    { "rgbpp",
        css.rgb .. -- functional header
        perc(capt(css.integer)) .. -- red
        css.space .. -- separator
        perc(capt(css.integer)) .. -- green
        css.space .. -- separator
        perc(capt(css.integer)) .. -- blue
        css.slash ..
        perc(capt(css.integer)) .. -- alpha
        css.eol -- EOL
    },
    -- }}}
    -- {{{ rgba(d, d, d, f) integers 0-255, float 0-1
    { "rgbif",
        css.rgba .. -- functional header
        capt(css.integer) .. -- red
        css.comma .. -- separator
        capt(css.integer) .. -- green
        css.comma .. -- separator
        capt(css.integer) .. -- blue
        css.comma .. -- separator
        capt(css.float) .. -- alpha
        css.eol -- EOL
    },
    -- }}}
    -- {{{ rgba(d, d, d, %) integers 0-255, percentage 0-100
    { "rgbip",
        css.rgba .. -- functional header
        capt(css.integer) .. -- red
        css.comma .. -- separator
        capt(css.integer) .. -- green
        css.comma .. -- separator
        capt(css.integer) .. -- blue
        css.comma ..
        perc(capt(css.integer)) .. -- alpha
        css.eol -- EOL
    },
    -- }}}
    -- {{{ rgba(d d d / f) integers 0-255, float 0-1
    { "rgbif",
        css.rgba .. -- functional header
        capt(css.integer) .. -- red
        css.space .. -- separator
        capt(css.integer) .. -- green
        css.space .. -- separator
        capt(css.integer) .. -- blue
        css.slash ..
        capt(css.float) .. -- alpha
        css.eol -- EOL
    },
    -- }}}
    -- {{{ rgba(d d d / %) integers 0-255, percentage 0-100
    { "rgbip",
        css.rgba .. -- functional header
        capt(css.integer) .. -- red
        css.space .. -- separator
        capt(css.integer) .. -- green
        css.space .. -- separator
        capt(css.integer) .. -- blue
        css.slash ..
        perc(capt(css.integer)) .. -- alpha
        css.eol -- EOL
    },
    -- }}}
    -- {{{ rgba(%, %, %, f) percentage 0-100, float 0-1
    { "rgbpf",
        css.rgba .. -- functional header
        perc(capt(css.integer)) .. -- red
        css.comma .. -- separator
        perc(capt(css.integer)) .. -- green
        css.comma .. -- separator
        perc(capt(css.integer)) .. -- blue
        css.comma ..
        capt(css.float) .. -- alpha
        css.eol -- EOL
    },
    -- }}}
    -- {{{ rgba(%, %, %, %) percentage 0-100
    { "rgbpp",
        css.rgba .. -- functional header
        perc(capt(css.integer)) .. -- red
        css.comma .. -- separator
        perc(capt(css.integer)) .. -- green
        css.comma .. -- separator
        perc(capt(css.integer)) .. -- blue
        css.comma .. -- separator
        perc(capt(css.integer)) .. -- alpha
        css.eol -- EOL
    },
    -- }}}
    -- {{{ rgba(% % % / f) percentage 0-100, float 0-1
    { "rgbpf",
        css.rgba .. -- functional header
        perc(capt(css.integer)) .. -- red
        css.space .. -- separator
        perc(capt(css.integer)) .. -- green
        css.space .. -- separator
        perc(capt(css.integer)) .. -- blue
        css.slash ..
        capt(css.float) .. -- alpha
        css.eol -- EOL
    },
    -- }}}
    -- {{{ rgba(% % % / %) percentage 0-100
    { "rgbpp",
        css.rgba .. -- functional header
        perc(capt(css.integer)) .. -- red
        css.space .. -- separator
        perc(capt(css.integer)) .. -- green
        css.space .. -- separator
        perc(capt(css.integer)) .. -- blue
        css.slash ..
        perc(capt(css.integer)) .. -- alpha
        css.eol -- EOL
    },
    -- }}}
    -- {{{ hsl(d, %, %) integers 0-360, percentage 0-100
    { "hsl",
        css.hsl ..
        capt(css.integer) .. -- hue
        css.comma ..
        perc(capt(css.integer)) .. -- saturation
        css.comma ..
        perc(capt(css.integer)) .. -- lightness
        css.eol
    },
    -- }}}
    -- {{{ hsl(d, %, %, f) integers 0-360, percentage 0-100, float 0-1
    { "hslxf",
        css.hsl ..
        capt(css.integer) .. -- hue
        css.comma ..
        perc(capt(css.integer)) .. -- saturation
        css.comma ..
        perc(capt(css.integer)) .. -- lightness
        css.comma ..
        capt(css.float) .. -- alpha
        css.eol
    },
    -- }}}
    -- {{{ hsl(d, %, %, %) integers 0-360, percentage 0-100
    { "hslxp",
        css.hsl ..
        capt(css.integer) .. -- hue
        css.comma ..
        perc(capt(css.integer)) .. -- saturation
        css.comma ..
        perc(capt(css.integer)) .. -- lightness
        css.comma ..
        perc(capt(css.integer)) .. -- alpha
        css.eol
    },
    -- }}}
    -- {{{ hsl(d % %) integers 0-360, percentage 0-100
    { "hsl",
        css.hsl ..
        capt(css.integer) .. -- hue
        css.space ..
        perc(capt(css.integer)) .. -- saturation
        css.space ..
        perc(capt(css.integer)) .. -- lightness
        css.eol
    },
    -- }}}
    -- {{{ hsl(d % % / f) integers 0-360, percentage 0-100, float 0-1
    { "hslxf",
        css.hsl ..
        capt(css.integer) .. -- hue
        css.space ..
        perc(capt(css.integer)) .. -- saturation
        css.space ..
        perc(capt(css.integer)) .. -- lightness
        css.slash ..
        capt(css.float) .. -- alpha
        css.eol
    },
    -- }}}
    -- {{{ hsl(d % % / %) integers 0-360, percentage 0-100
    { "hslxp",
        css.hsl ..
        capt(css.integer) .. -- hue
        css.space ..
        perc(capt(css.integer)) .. -- saturation
        css.space ..
        perc(capt(css.integer)) .. -- lightness
        css.slash ..
        perc(capt(css.integer)) .. -- alpha
        css.eol
    },
    -- }}}
    -- {{{ hsla(d, %, %, f) integers 0-360, percentage 0-100, float 0-1
    { "hslxf",
        css.hsla ..
        capt(css.integer) .. -- hue
        css.comma ..
        perc(capt(css.integer)) .. -- saturation
        css.comma ..
        perc(capt(css.integer)) .. -- lightness
        css.comma ..
        capt(css.float) .. -- alpha
        css.eol
    },
    -- }}}
    -- {{{ hsla(d, %, %, %) integers 0-360, percentage 0-100
    { "hslxp",
        css.hsla ..
        capt(css.integer) .. -- hue
        css.comma ..
        perc(capt(css.integer)) .. -- saturation
        css.comma ..
        perc(capt(css.integer)) .. -- lightness
        css.comma ..
        perc(capt(css.integer)) .. -- alpha
        css.eol
    },
    -- }}}
    -- {{{ hsla(d % % / f) integers 0-360, percentage 0-100, float 0-1
    { "hslxf",
        css.hsla ..
        capt(css.integer) .. -- hue
        css.space ..
        perc(capt(css.integer)) .. -- saturation
        css.space ..
        perc(capt(css.integer)) .. -- lightness
        css.slash ..
        capt(css.float) .. -- alpha
        css.eol
    },
    -- }}}
    -- {{{ hsla(d % % / %) integers 0-360, percentage 0-100
    { "hslxp",
        css.hsla ..
        capt(css.integer) .. -- hue
        css.space ..
        perc(capt(css.integer)) .. -- saturation
        css.space ..
        perc(capt(css.integer)) .. -- lightness
        css.slash ..
        perc(capt(css.integer)) .. -- alpha
        css.eol
    },
    -- }}}
    -- }}}
}
-- }}} Color patterns

--- Create a new color instance from a string
-- @tparam string str A CSS color compatible string.
-- @return A new color object, or nil if invalid
-- @usage local c = colorful.color.from.string("rgb(255, 204, 153)")
function color.from.string(str)
    for _,pat in pairs(color_patterns) do
        local t = pat[1]
        local m = {str:match(pat[2])}
        if m[1] ~= nil then
            local th = t:sub(1,3)
            local tv = t:sub(4,4)
            local ta = #t == 5 and t:sub(5,5) or nil
            if th == "hex" then
                -- r,g,b convert to number
                local r = tonumber(m[1], 16)
                local g = tonumber(m[2], 16)
                local b = tonumber(m[3], 16)
                local a = 0xFF
                -- if alpha present
                if ta ~= nil then
                    a = tonumber(m[4], 16)
                end
                if r == nil or g == nil or b == nil or a == nil then return nil end
                -- if type is shorthand
                if tv == "s" then
                    r = r * 16 + r
                    g = g * 16 + g
                    b = b * 16 + b
                    a = a * 16 + a
                end
                return color.from.rgba(r,g,b,a)
            elseif th == "rgb" then
                local r = tonumber(m[1])
                local g = tonumber(m[2])
                local b = tonumber(m[3])
                if r == nil or g == nil or b == nil then return nil end
                -- if values percentages
                if tv == "p" then
                    r,g,b = r/100 * 255, g/100 * 255, b/100 * 255
                end
                -- check for alpha
                local a = 255
                if ta ~= nil then
                    a = tonumber(m[4])
                    if a == nil then return nil end
                    if ta == "p" then
                        a = a/100
                    end
                    a = a * 255 -- both float and percentage need this
                end
                return color.from.rgba(r,g,b,a)
            elseif th == "hsl" then
                local h = tonumber(m[1])
                local s = tonumber(m[2]) / 100 -- always percentage
                local l = tonumber(m[3]) / 100 -- always percentage
                local a = 1
                if h == nil or s == nil or l == nil then return nil end
                if ta ~= nil then
                    a = tonumber(m[4])
                    if a == nil then return nil end
                    if ta == "p" then
                        a = a / 100
                    end
                end
                return color.from.hsla(h,s,l,a)
            end
        end
    end
    return nil
end

-- }}} String

-- {{{ Decimal

--- Create a new color instance from an RGB decimal value.
-- Will bitshift decimal 2 bytes left and call `color.from.hex4`
-- @tparam number hex A decimal describing an RGB color
-- @return A new color object, or nil if invalid
-- @usage local c = colorful.color.from.hex3(0xffcc99)
function color.from.hex3(hex)
    return color.from.hex4(lshift(hex, 8) + 0xff)
end

--- Create a new color instance from an RGBA decimal value.
-- @tparam number hex A decimal describing an RGBA color
-- @return A new color object, or nil if invalid
-- @usage local c = colorful.color.from.hex4(0xffcc99ff)
function color.from.hex4(hex)
    -- shift right 24 bits, 0s shifted in
    local r = rshift(hex, 24)
    -- clear upper bits as needed, and shift remaining bits down
    local g = rshift(lshift(hex, 8), 24)
    local b = rshift(lshift(hex, 16), 24)
    local a = rshift(lshift(hex, 24), 24)
    return color.from.rgba(r,g,b,a)
end

-- }}} Decimal

-- {{{ RGB

--- Create a new color instance from RGBA values.
-- @tparam number r Red decimal value 0-255
-- @tparam number g Green decimal value 0-255
-- @tparam number b Blue decimal value 0-255
-- @tparam number a Alpha decimal value 0-255
-- @return A new color object, or nil if invalid
-- @usage local c = colorful.color.from.rgba(255, 204, 153, 255)
function color.from.rgba(r,g,b,a)
    -- put r,g,b,a into [0,1]
    r = clamp(r, 0, 255) / 255
    g = clamp(g, 0, 255) / 255
    b = clamp(b, 0, 255) / 255
    a = clamp(a, 0, 255) / 255
    -- hsl function expects RGB in range [0,1]
    local h,s,l = rgb_to_hsl(r,g,b)
    return create_color_object(r,g,b,h,s,l,a)
end

--- Create a new color instance from RGB values.
-- Will call `color.from.rgba` with an Alpha value of 255
-- @tparam number r Red decimal value 0-255
-- @tparam number g Green decimal value 0-255
-- @tparam number b Blue decimal value 0-255
-- @return A new color object, or nil if invalid
-- @usage local c = colorful.color.from.rgb(255, 204, 153)
function color.from.rgb(r,g,b)
    return color.from.rgba(r,g,b,255)
end

-- }}} RGB

-- {{{ HSL

--- Create a new color instance from HSLA values.
-- @tparam number h Hue decimal value 0-360
-- @tparam number s Saturation float value 0-1
-- @tparam number l Lightness float value 0-1
-- @tparam number a Alpha float value 0-1
-- @return A new color object, or nil if invalid
-- @usage local c = colorful.color.from.hsla(30, 0.8, 0.6, 1)
function color.from.hsla(h,s,l,a)
    -- hue is adjustable as it's degrees
    if not between(h, 0, 360) then
        while h < 0 do
            h = h + 360
        end
        while h > 360 do
            h = h - 360
        end
    end
    s = clamp(s, 0.0, 1.0)
    l = clamp(l, 0.0, 1.0)
    local r,g,b = hsl_to_rgb(h,s,l)
    return create_color_object(r,g,b,h,s,l,a)
end

--- Create a new color instance from HSL values.
-- Will call `color.from.hsla` with an Alpha value of 1
-- @tparam number h Hue decimal value 0-360
-- @tparam number s Saturation float value 0-1
-- @tparam number l Lightness float value 0-1
-- @return A new color object, or nil if invalid
-- @usage local c = colorful.color.from.hsl(30, 0.8, 0.6)
function color.from.hsl(h,s,l)
    return color.from.hsla(h,s,l,1)
end

-- }}} HSL

-- @section end

-- }}} New Color functions

-- {{{ Color transformation functions

--- Color object transformation functions
-- @section color_change

--- Update the HSL values after editing any of the RGB values
function color:hsl_update()
    self.hue, self.saturation, self.lightness = rgb_to_hsl(self.red, self.green, self.blue)
end

--- Update the RGB values after editing any of the HSL values
function color:rgb_update()
    self.red, self.green, self.blue = hsl_to_rgb(self.hue, self.saturation, self.lightness)
end

-- {{{ RGB-adjusting functions

-- Helper function for color:mix
local function calcval(v,s,e)
    local w = 1 - v
    return s * v + e * w
end

--- Returns the mix of a color with another by a specified amount
-- @tparam beautiful.colorful.color c A color object
-- @tparam[opt=0.5] number v A number [0,1] of how much of the origin color
-- should be mixed with c.
-- @return A new color object representing the mix of the two colors, or nil if invalid
-- @usage local mixed_color = c:mix(c2, 0.5)
function color:mix(c, v)
    if v == nil then v = 0.5 end
    if v < 0 or v > 1 then return nil end
    local red = calcval(v, self.red, c.red) * 255
    local green = calcval(v, self.green, c.green) * 255
    local blue = calcval(v, self.blue, c.blue) * 255
    local alpha = calcval(v, self.alpha, c.alpha) * 255
    return color.from.rgba(red, green, blue, alpha)
end

--- Returns the inverse of a color
-- @return A new color object representing the inverted color, or nil if invalid
-- @usage local inverted_color = c:invert()
function color:invert()
    local red = clamp(1 - self.red, 0, 1) * 255
    local green = clamp(1 - self.green, 0, 1) * 255
    local blue = clamp(1 - self.blue, 0, 1) * 255
    return color.from.rgba(red, green, blue, self.alpha * 255)
end

-- }}} RGB-adjusting functions

-- {{{ HSL-adjusting functions

--- Returns a color darkened by a specified amount
-- @tparam number v A number [0,1] of how much to decrease Lightness
-- @return A new color object representing the darkened color, or nil if invalid
-- @usage local darker_color = c:darken(0.25)
function color:darken(v)
    local lightness = clamp(self.lightness - v, 0, 1)
    return color.from.hsla(self.hue, self.saturation, lightness, self.alpha)
end

--- Returns a color lightened by a specified amount
-- @tparam number v A number [0,1] of how much to increase Lightness
-- @return A new color object representing the lightened color, or nil if invalid
-- @usage local lighter_color = c:lighten(0.25)
function color:lighten(v)
    local lightness = clamp(self.lightness + v, 0, 1)
    return color.from.hsla(self.hue, self.saturation, lightness, self.alpha)
end

--- Returns a color saturated by a specified amount
-- @tparam number v A number [0,1] of how much to increase Saturation
-- @return A new color object representing the saturated color, or nil if invalid
-- @usage local saturated_color = c:saturate(0.25)
function color:saturate(v)
    local saturation = clamp(self.saturation + v, 0, 1)
    return color.from.hsla(self.hue, saturation, self.lightness, self.alpha)
end

--- Returns a color desaturated by a specified amount
-- @tparam number v A number [0,1] of how much to decrease Saturation
-- @return A new color object representing the desaturated color, or nil if invalid
-- @usage local desaturated_color = c:desaturate(0.25)
function color:desaturate(v)
    local saturation = clamp(self.saturation - v, 0, 1)
    return color.from.hsla(self.hue, saturation, self.lightness, self.alpha)
end

--- Returns the grayscale of a color
-- Reduces Saturation to 0
-- @return A new color object representing the grayscaled color, or nil if invalid
-- @usage local grayscaled_color = c:grayscale()
function color:grayscale()
    return color.from.hsla(self.hue, 0, self.lightness, self.alpha)
end

-- }}} HSL-adjusting functions

-- {{{ Color harmony functions

--- Returns the complement of a color (+180deg hue)
-- Adds 180 to Hue (hue will self-adjust to be within [0,360])
-- @return A new color object representing the complementary color, or nil if invalid
-- @usage local complemented_color = c:complement()
function color:complement()
    local hue = self.hue + 180
    return color.from.hsla(hue, self.saturation, self.lightness, self.alpha)
end

--- Returns the left and right analogous colors (+30deg, -30deg hue)
-- @return A new color object representing a -30 degree hue shift of the color, or nil if invalid
-- @return A new color object representing a +30 degree hue shift of the color, or nil if invalid
-- @usage local left, right = c:analogous()
function color:analogous()
    local left = self.hue + 30
    local right = self.hue - 30
    return color.from.hsla(
            left,
            self.saturation,
            self.lightness,
            self.alpha),
        color.from.hsla(
            right,
            self.saturation,
            self.lightness,
            self.alpha)
end

--- Returns the left and right complementary colors (+210deg, +150deg hue)
-- @return A new color object representing the +210 degree hue shift (180 + 30)
-- of the color, or nil if invalid
-- @return A new color object representing the +150 degree hue shift (180 - 30)
-- of the color, or nil if invalid
-- @usage local left, right = c:split_complementary()
function color:split_complementary()
    local left = self.hue + 180 + 30
    local right = self.hue + 180 - 30
    return color.from.hsla(
            left,
            self.saturation,
            self.lightness,
            self.alpha),
        color.from.hsla(
            right,
            self.saturation,
            self.lightness,
            self.alpha)
end

--- Returns the left and right triadic colors (+120deg, -120deg hue)
-- @return A new color object representing a +120 degree hue shift of the color, or nil if invalid
-- @return A new color object representing a -120 degree hue shift of the color, or nil if invalid
-- @usage local left, right = c:triadic()
function color:triadic()
    local left = self.hue + 120
    local right = self.hue - 120
    return color.from.hsla(
            left,
            self.saturation,
            self.lightness,
            self.alpha),
        color.from.hsla(
            right,
            self.saturation,
            self.lightness,
            self.alpha)
end

--- Returns three colors making up the tetradic colors (+180deg, +60deg, +240deg hue)
-- @return A new color object representing the complementary color, or nil if
-- invalid
-- @return A new color object representing a +60 degree shift of the color, or
-- nil if invalid
-- @return A new color object representing the complement of the previous
-- returned color, or nil if invalid
-- @usage local complement, analogous, analogous_complement = c:tetradic()
function color:tetradic()
    local complement = self:complement()
    local base_left = color.from.hsla(self.hue + 60, self.saturation, self.lightness, self.alpha)
    return complement, base_left, base_left:complement()
end

-- @section end

-- }}} Color harmony functions

-- }}} Color transformation functions

-- {{{ Color output functions

--- Color object output functions
-- @section color_output

-- {{{ String output

--- Return the string representation of the color in #RRGGBBAA format
-- @treturn string A string represention of the color in #RRGGBBAA format
-- @usage local hexa_str = c:hexa()
function color:hexa()
    local r = round(self.red   * 255)
    local g = round(self.green * 255)
    local b = round(self.blue  * 255)
    local a = round(self.alpha * 255)
    return string.format("#%02x%02x%02x%02x", r, g, b, a)
end

--- Return the string representation of the color in #RRGGBB format
-- @treturn string A string represention of the color in #RRGGBB format
-- @usage local hex_str = c:hex()
function color:hex()
    local r = round(self.red   * 255)
    local g = round(self.green * 255)
    local b = round(self.blue  * 255)
    return string.format("#%02x%02x%02x", r, g, b)
end

-- }}} String output

-- {{{ Decimal output

--- Return the decimal representation of the color including the alpha channel
-- @treturn number The decimal represention of the color including the alpha
-- channel
-- @usage local deca_color = c:deca()
function color:deca()
    return math.floor(
          lshift(round(self.red   * 0xFF), 24)
        + lshift(round(self.green * 0xFF), 16)
        + lshift(round(self.blue  * 0xFF), 8)
        +        round(self.alpha * 0xFF)
    )
end

--- Return the decimal representation of the color
-- @treturn number The decimal represention of the color not including the
-- alpha channel
-- @usage local dec_color = c:dec()
function color:dec()
    return math.floor(
          lshift(round(self.red   * 0xFF), 16)
        + lshift(round(self.green * 0xFF), 8)
        +        round(self.blue  * 0xFF)
    )
end

-- }}} Decimal output

-- {{{ Multi-return output

--- Return the Hue, Saturation, Lightness, and Alpha
-- @treturn number Hue [0,360]
-- @treturn number Saturation [0,1]
-- @treturn number Lightness [0,1]
-- @treturn number Alpha [0,1]
-- @usage local h,s,l,a = c:hsla()
function color:hsla()
    return self.hue,
        self.saturation,
        self.lightness,
        self.alpha
end

--- Return the Hue, Saturation, and Lightness
-- @treturn number Hue [0,360]
-- @treturn number Saturation [0,1]
-- @treturn number Lightness [0,1]
-- @usage local h,s,l = c:hsl()
function color:hsl()
    return self.hue,
        self.saturation,
        self.lightness
end

--- Return the Red, Green, Blue, and Alpha
-- @treturn number Red [0,1]
-- @treturn number Green [0,1]
-- @treturn number Blue [0,1]
-- @treturn number Alpha [0,1]
-- @usage local r,g,b,a = c:rgba()
function color:rgba()
    return self.red,
        self.green,
        self.blue,
        self.alpha
end

--- Return the Red, Green, and Blue
-- @treturn number Red [0,1]
-- @treturn number Green [0,1]
-- @treturn number Blue [0,1]
-- @usage local r,g,b = c:rgb()
function color:rgb()
    return self.red,
        self.green,
        self.blue
end

-- }}} Multi-return output

-- @section end

-- }}} Color output functions

color_mt.__index = color
color_mt.__tostring = color.hex

--- A color object
-- @type color
colorful.color = color

setmetatable(color, {
    __call = function(_,...) return color.new(...) end,
})

-- {{{ Color changing wrapper functions

--- Wrappers for Color object functions
-- @section wrappers

--- Mix two colors with optional mix amount
-- @param color1 A CSS color compatible string, decimal color value, or a table
-- containing 3 or 4 values.
-- @param color2 A CSS color compatible string, decimal color value, or a table
-- containing 3 or 4 values.
-- @tparam number amount A number [0,1] of how much of color1 should be mixed
-- with color2.
-- @treturn string The string representation of the mixed color in `#rrggbbaa`
-- format.
-- @see beautiful.colorful.color.mix
-- @usage local mix_str = colorful.mix("#ffcc99", "#99ccff", 0.5)
function colorful.mix(color1, color2, amount)
    local c1 = color.new(color1)
    local c2 = color.new(color2)
    if c1 == nil or c2 == nil then return nil end
    return tostring(c1:mix(c2, amount))
end

--- Darken a color by an amount
-- @param _color A CSS color compatible string, decimal color value, or a table
-- containing 3 or 4 values.
-- @tparam number amount A number [0,1] of how much to darken color.
-- @treturn string The string representation of the mixed color in `#rrggbbaa`
-- format.
-- @see beautiful.colorful.color.darken
-- @usage local dark_str = colorful.darken("#ffcc99", 0.25)
function colorful.darken(_color, amount)
    local c1 = color.new(_color)
    if c1 == nil then return nil end
    return tostring(c1:darken(amount))
end

--- Lighten a color by an amount
-- @param _color A CSS color compatible string, decimal color value, or a table
-- containing 3 or 4 values.
-- @tparam number amount A number [0,1] of how much to lighten color.
-- @treturn string The string representation of the mixed color in `#rrggbbaa`
-- format.
-- @see beautiful.colorful.color.lighten
-- @usage local light_str = colorful.lighten("#ffcc99", 0.25)
function colorful.lighten(_color, amount)
    local c1 = color.new(_color)
    if c1 == nil then return nil end
    return tostring(c1:lighten(amount))
end

--- Invert a color
-- @param _color A CSS color compatible string, decimal color value, or a table
-- containing 3 or 4 values.
-- @treturn string The string representation of the mixed color in `#rrggbbaa`
-- format.
-- @see beautiful.colorful.color.invert
-- @usage = local invert_str = colorful.invert("#ffcc99")
function colorful.invert(_color)
    local c1 = color.new(_color)
    if c1 == nil then return nil end
    return tostring(c1:invert())
end

--- Grayscale a color
-- @param _color A CSS color compatible string, decimal color value, or a table
-- containing 3 or 4 values.
-- @treturn string The string representation of the mixed color in `#rrggbbaa`
-- format.
-- @see beautiful.colorful.color.grayscale
-- @usage local grayed_str = colorful.grayscale("#ffcc99")
function colorful.grayscale(_color)
    local c1 = color.new(_color)
    if c1 == nil then return nil end
    return tostring(c1:grayscale())
end

-- @section end

-- }}} Color changing wrapper functions

return colorful

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
