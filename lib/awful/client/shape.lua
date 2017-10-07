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
    local _shape = c._shape
    if not (shape_img or _shape) then return end

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

    -- Fill everything (this paints the titlebars and border).
    -- The `cr:paint()` below will have painted the whole surface, so
    -- everything inside the client is currently meant to be visible
    cr:paint()

    if shape_img then
        -- Draw the client's shape in the middle
        cr:set_operator(cairo.Operator.SOURCE)
        cr:set_source_surface(shape_img, border + l, border + t)
        cr:rectangle(border + l, border + t, geom.width - l - r, geom.height - t - b)
        cr:fill()

        shape_img:finish()
    end

    if _shape then
        -- Draw the shape to an intermediate surface
        cr:push_group()
        -- Intersect what is drawn so far with the shape set by Lua.
        if shape_name == "clip" then
            -- Correct for the border offset
            cr:translate(-c.border_width, -c.border_width)
        end
        -- Always call the shape with the size of the bounding shape
        _shape(cr, geom.width + 2*c.border_width, geom.height + 2*c.border_width)
        -- Now fill the "selected" part
        cr:set_operator(cairo.Operator.SOURCE)
        cr:set_source_rgba(1, 1, 1, 1)
        cr:fill_preserve()
        if shape_name == "clip" then
            -- Remove an area of size c.border_width again (We use 2*bw since
            -- half of that is on the outside)
            cr:set_source_rgba(0, 0, 0, 0)
            cr:set_line_width(2*c.border_width)
            cr:stroke()
        end
        -- Combine the result with what we already have
        cr:pop_group_to_source()
        cr:set_operator(cairo.Operator.IN)
        cr:paint()

        -- 'cr' is kept alive until Lua's GC frees it. Make sure it does not
        -- keep the group alive since that's a large image surface.
        cr:set_source_rgba(0, 0, 0, 0)
    end

    return result
end

--- Update all of a client's shapes from the shapes the client set itself.
-- @function awful.client.shape.update.all
-- @client c The client to act on
function shape.update.all(c)
    shape.update.bounding(c)
    shape.update.clip(c)
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
capi.client.connect_signal("property::size", shape.update.all)
capi.client.connect_signal("property::border_width", shape.update.all)

return shape

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
