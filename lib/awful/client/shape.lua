---------------------------------------------------------------------------
--- Handle client shapes.
--
-- @author Uli Schlachter &lt;psychon@znc.in&gt;
-- @copyright 2014 Uli Schlachter
-- @submodule client
---------------------------------------------------------------------------

-- Grab environment we need
local surface = require("gears.surface")
local cairo = require("lgi").cairo
local capi =
{
    client = client,
}

local shape = {}
shape.update = {}

--- Get one of a client's shapes and transform it to include window decorations.
-- @function awful.client.shape.get_transformed
-- @client c The client whose shape should be retrieved
-- @tparam string shape_name Either "bounding" or "clip"
function shape.get_transformed(c, shape_name)
    local border = shape_name == "bounding" and c.border_width or 0
    local shape_img = surface.load_silently(c["client_shape_" .. shape_name], false)
    if not shape_img then return end

    -- Get information about various sizes on the client
    local geom = c:geometry()
    local _, t = c:titlebar_top()
    local _, b = c:titlebar_bottom()
    local _, l = c:titlebar_left()
    local _, r = c:titlebar_right()

    -- Figure out the size of the shape that we need
    local img_width = geom.width + 2*border
    local img_height = geom.height + 2*border
    local result = cairo.ImageSurface(cairo.Format.A1, img_width, img_height)
    local cr = cairo.Context(result)

    -- Fill everything (this paints the titlebars and border)
    cr:paint()

    -- Draw the client's shape in the middle
    cr:set_operator(cairo.Operator.SOURCE)
    cr:set_source_surface(shape_img, border + l, border + t)
    cr:rectangle(border + l, border + t, geom.width - l - r, geom.height - t - b)
    cr:fill()

    return result
end

--- Update all of a client's shapes from the shapes the client set itself.
-- @function awful.client.shape.update.all
-- @client c The client to act on
function shape.update.all(c)
    local shape_fun = c._shape

    if not shape_fun then
        c.shape_bounding = nil
        c.shape_clip = nil
        shape.update.bounding(c)
        shape.update.clip(c)
        return
    end

    local geo = c:geometry()
    local bw = c.border_width

    -- First handle the bounding shape (things including the border)
    local img = cairo.ImageSurface(cairo.Format.A1, geo.width + 2*bw, geo.height + 2*bw)
    local cr = cairo.Context(img)

    -- We just draw the shape in its full size
    shape_fun(cr, geo.width + 2*bw, geo.height + 2*bw)
    cr:set_operator(cairo.Operator.SOURCE)
    cr:fill()
    c.shape_bounding = img._native
    img:finish()

    -- Now handle the clip shape (things excluding the border)
    img = cairo.ImageSurface(cairo.Format.A1, geo.width, geo.height)
    cr = cairo.Context(img)

    -- We give the shape the same arguments as for the bounding shape and draw
    -- it in its full size (the translate is to compensate for the smaller
    -- surface)
    cr:translate(-bw, -bw)
    shape_fun(cr, geo.width + 2*bw, geo.height + 2*bw)
    cr:set_operator(cairo.Operator.SOURCE)
    cr:fill_preserve()
    -- Now we remove an area of width 'bw' again around the shape (We use 2*bw
    -- since half of that is on the outside and only half on the inside)
    cr:set_source_rgba(0, 0, 0, 0)
    cr:set_line_width(2*bw)
    cr:stroke()
    c.shape_clip = img._native
    img:finish()
end

--- Update a client's bounding shape from the shape the client set itself.
-- @function awful.client.shape.update.bounding
-- @client c The client to act on
function shape.update.bounding(c)
    local res = shape.get_transformed(c, "bounding")
    c.shape_bounding = res and res._native
    -- Free memory
    if res then
        res:finish()
    end
end

--- Update a client's clip shape from the shape the client set itself.
-- @function awful.client.shape.update.clip
-- @client c The client to act on
function shape.update.clip(c)
    local res = shape.get_transformed(c, "clip")
    c.shape_clip = res and res._native
    -- Free memory
    if res then
        res:finish()
    end
end

capi.client.connect_signal("property::shape_client_bounding", shape.update.bounding)
capi.client.connect_signal("property::shape_client_clip", shape.update.clip)
capi.client.connect_signal("property::width", shape.update.all)
capi.client.connect_signal("property::height", shape.update.all)

return shape

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
