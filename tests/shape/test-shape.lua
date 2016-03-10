-- Test if shape crash when called
-- Also generate some SVG to be used by the documentation
-- it also "prove" that the code examples are all working
local cairo = require( "lgi"         ).cairo
local shape = require( "gears.shape" )
local filepath, svgpath = ...

-- This is normal when running test, it will run twice
if (not filepath) or (not svgpath) or key then
    os.exit(0)
end

local function get_surface(p)
    local img = cairo.SvgSurface.create(p, 288, 76)
    return cairo.Context(img)
end

local function show(cr, skip_fill)
    if not skip_fill then
        cr:set_source_rgba(0.380392156863,0.505882352941,1,0.5)
        cr:fill_preserve()
    end

    cr:set_source_rgba(0.380392156863,0.505882352941,1,1)
    cr:stroke()

    cr:translate(96, 0)
    cr:reset_clip()
    cr:rectangle(-3,-3,76,76)
    cr:clip()
end

local cr = get_surface(svgpath)
cr:translate(3,3)

loadfile(filepath)(shape, cr, show)
