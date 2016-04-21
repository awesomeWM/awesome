---------------------------------------------------------------------------
--- Algorithms used to place various drawables.
--
-- The functions provided by this module all follow the same arguments
-- conventions. This allow:
--
-- * To use them in various other module as
--   [visitor objects](https://en.wikipedia.org/wiki/Visitor_pattern)
-- * Turn each function into an API with various common customization parameters.
-- * Re-use the same functions for the `mouse`, `client`s, `screen`s and `wibox`es
-- 
-- 
--
-- It is possible to compose placement function using the `+` or `*` operator:
--
--    local f = (awful.placement.right + awful.placement.left)
--    f(client.focus)
--
-- ### Common arguments
--
-- **pretend** (*boolean*):
--
-- Do not apply the new geometry. This is useful if only the return values is
-- necessary.
--
-- **honor_workarea** (*boolean*):
--
--  Take workarea into account when placing the drawable (default: false)
--
-- **honor_padding** (*boolean*):
--
--  Take the screen padding into account (see `screen.padding`)
--
-- **tag** (*tag*):
--
--  Use a tag geometry
--
-- **margins** (*number* or *table*):
--
--  A table with left, right, top, bottom keys or a number
--
-- **parent** (client, wibox, mouse or screen):
--
--  A parent drawable to use a base geometry
--
-- **bounding_rect** (table):
--
-- A bounding rectangle
--
-- **attach** (*boolean*):
--
-- **store_geometry** (*boolean*):
--
-- Keep a single history of each type of placement. It can be restored using
-- `awful.placement.restore` by setting the right `context` argument.
--
-- When either the parent or the screen geometry change, call the placement
-- function again.
--
-- **update_workarea** (*boolean*):
--
-- If *attach* is true, also update the screen workarea.
--
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008 Julien Danjou, Emmanuel Lepage Vallee 2016
-- @release @AWESOME_VERSION@
-- @module awful.placement
---------------------------------------------------------------------------

-- Grab environment we need
local ipairs = ipairs
local pairs = pairs
local math = math
local table = table
local capi =
{
    screen = screen,
    mouse = mouse,
    client = client
}
local client = require("awful.client")
local layout = require("awful.layout")
local a_screen = require("awful.screen")
local dpi = require("beautiful").xresources.apply_dpi

local function get_screen(s)
    return s and capi.screen[s]
end

local wrap_client = nil

local function compose(w1, w2)
    return wrap_client(function(...)
        w1(...)
        w2(...)
        return --It make no sense to keep a return value
    end)
end

wrap_client = function(f)
    return setmetatable({is_placement=true}, {
        __call = function(_,...) return f(...) end,
        __add  = compose, -- Composition is usually defined as +
        __mul  = compose  -- Make sense if you think of the functions as matrices
    })
end

local placement_private = {}

-- The module is a proxy in front of the "real" functions.
-- This allow syntax like:
--
--    (awful.placement.no_overlap + awful.placement.no_offscreen)(c)
--
local placement = setmetatable({}, {
    __index = placement_private,
    __newindex = function(_, k, f)
        placement_private[k] = wrap_client(f)
    end
})

-- 3x3 matrix of the valid sides and corners
local corners3x3 = {{"top_left"   ,   "top"   , "top_right"   },
                    {"left"       ,    nil    , "right"       },
                    {"bottom_left",  "bottom" , "bottom_right"}}

-- 2x2 matrix of the valid sides and corners
local corners2x2 = {{"top_left"   ,            "top_right"   },
                    {"bottom_left",            "bottom_right"}}

-- Compute the new `x` and `y`.
-- The workarea position need to be applied by the caller
local align_map = {
    top_left          = function(_ , _ , _ , _ ) return {x=0        , y=0        } end,
    top_right         = function(sw, _ , dw, _ ) return {x=sw-dw    , y=0        } end,
    bottom_left       = function(_ , sh, _ , dh) return {x=0        , y=sh-dh    } end,
    bottom_right      = function(sw, sh, dw, dh) return {x=sw-dw    , y=sh-dh    } end,
    left              = function(_ , sh, _ , dh) return {x=0        , y=sh/2-dh/2} end,
    right             = function(sw, sh, dw, dh) return {x=sw-dw    , y=sh/2-dh/2} end,
    top               = function(sw, _ , dw, _ ) return {x=sw/2-dw/2, y=0        } end,
    bottom            = function(sw, sh, dw, dh) return {x=sw/2-dw/2, y=sh-dh    } end,
    centered          = function(sw, sh, dw, dh) return {x=sw/2-dw/2, y=sh/2-dh/2} end,
    center_vertical   = function(_ , sh, _ , dh) return {x= nil     , y=sh-dh    } end,
    center_horizontal = function(sw, _ , dw, _ ) return {x=sw/2-dw/2, y= nil     } end,
}

-- Store function -> keys
local reverse_align_map = {}

-- Some parameters to correctly compute the final size
local resize_to_point_map = {
    -- Corners
    top_left     = {p1= nil  , p2={1,1}, x_only=false, y_only=false, align="bottom_right"},
    top_right    = {p1={0,1} , p2= nil , x_only=false, y_only=false, align="bottom_left" },
    bottom_left  = {p1= nil  , p2={1,0}, x_only=false, y_only=false, align="top_right"   },
    bottom_right = {p1={0,0} , p2= nil , x_only=false, y_only=false, align="top_left"    },

    -- Sides
    left         = {p1= nil  , p2={1,1}, x_only=true , y_only=false, align="top_right"   },
    right        = {p1={0,0} , p2= nil , x_only=true , y_only=false, align="top_left"    },
    top          = {p1= nil  , p2={1,1}, x_only=false, y_only=true , align="bottom_left" },
    bottom       = {p1={0,0} , p2= nil , x_only=false, y_only=true , align="top_left"    },
}

--- Add a context to the arguments.
-- This function extend the argument table. The context is used by some
-- internal helper methods. If there already is a context, it has priority and
-- is kept.
local function add_context(args, context)
    return setmetatable({context = (args or {}).context or context }, {__index=args})
end

local data = setmetatable({}, { __mode = 'k' })

--- Store a drawable geometry (per context) in a weak table.
-- @param d The drawin
-- @tparam string reqtype The context.
local function store_geometry(d, reqtype)
    if not data[d] then data[d] = {} end
    if not data[d][reqtype] then data[d][reqtype] = {} end
    data[d][reqtype] = d:geometry()
    data[d][reqtype].screen = d.screen
end

--- Get the area covered by a drawin.
-- @param d The drawin
-- @tparam[opt=nil] table new_geo A new geometry
-- @tparam[opt=false] boolean ignore_border_width Ignore the border
-- @treturn The drawin's area.
local function area_common(d, new_geo, ignore_border_width)
    -- The C side expect no arguments, nil isn't valid
    local geometry = new_geo and d:geometry(new_geo) or d:geometry()
    local border = ignore_border_width and 0 or d.border_width or 0
    geometry.width = geometry.width + 2 * border
    geometry.height = geometry.height + 2 * border
    return geometry
end

--- Get (and optionally set) an object geometry.
-- Some elements, such as `mouse` and `screen` don't have a `:geometry()`
-- methods.
-- @param obj An object
-- @tparam table args the method arguments
-- @tparam[opt=nil] table new_geo A new geometry to replace the existing one
-- @tparam[opt=false] boolean ignore_border_width Ignore the border
-- @treturn table A table with *x*, *y*, *width* and *height*.
local function geometry_common(obj, args, new_geo, ignore_border_width)

    -- Store the current geometry in a singleton-memento
    if args.store_geometry and new_geo and args.context then
        store_geometry(obj, args.context)
    end

    -- It's a mouse
    if obj.coords then
        local coords = (not args.pretend and new_geo)
            and obj.coords(new_geo) or obj.coords()
        return {x=coords.x, y=coords.y, width=0, height=0}
    elseif obj.geometry then
        local geo = obj.geometry

        -- It is either a drawable or something that implement its API
        if type(geo) == "function" then
            local dgeo = area_common(
                obj, (not args.pretend) and new_geo or nil, ignore_border_width
            )

            -- Apply the margins
            if args.margins then
                local delta = type(args.margins) == "table" and args.margins or {
                    left = args.margins , right  = args.margins,
                    top  = args.margins , bottom = args.margins
                }

                return {
                    x      = dgeo.x      + (delta.left or 0),
                    y      = dgeo.y      + (delta.top  or 0),
                    width  = dgeo.width  - (delta.left or 0) - (delta.right  or 0),
                    height = dgeo.height - (delta.top  or 0) - (delta.bottom or 0),
                }
            end

            return dgeo
        end

        -- It is a screen, it doesn't support setting new sizes.
        return obj:get_bounding_geometry(args)
    else
        assert(false, "Invalid object")
    end
end

--- Get the parent geometry from the standardized arguments API shared by all
-- `awful.placement` methods.
-- @param obj A screen or a drawable
-- @tparam table args the method arguments
-- @treturn table A table with *x*, *y*, *width* and *height*.
local function get_parent_geometry(obj, args)
    if args.bounding_rect then
        return args.bounding_rect
    elseif args.parent then
        return geometry_common(args.parent, args)
    elseif obj.screen then
        return geometry_common(obj.screen, args)
    else
        return geometry_common(capi.screen[capi.mouse.screen], args)
    end
end

--- Move a point into an area.
-- This doesn't change the *width* and *height* values, allowing the target
-- area to be smaller than the source one.
-- @tparam table source The (larger) geometry to move `target` into
-- @tparam table target The area to move into `source`
-- @treturn table A table with *x* and *y* keys
local function move_into_geometry(source, target)
    local ret = {x = target.x, y = target.y}

    -- Horizontally
    if ret.x < source.x then
        ret.x = source.x
    elseif ret.x > source.x + source.width then
        ret.x = source.x + source.width - 1
    end

    -- Vertically
    if ret.y < source.y then
        ret.y = source.y
    elseif ret.y > source.y + source.height then
        ret.y = source.y + source.height - 1
    end

    return ret
end

-- Update the workarea
local function wibox_update_strut(d, position)
    -- If the drawable isn't visible, remove the struts
    if not d.visible then
        d:struts { left = 0, right = 0, bottom = 0, top = 0 }
        return
    end

    -- Detect horizontal or vertical drawables
    local geo      = area_common(d)
    local vertical = geo.width < geo.height

    -- Look into the `position` string to find the relevants sides to crop from
    -- the workarea
    local struts = { left = 0, right = 0, bottom = 0, top = 0 }

    if vertical then
        for _, v in ipairs {"right", "left"} do
            if (not position) or position:match(v) then
                struts[v] = geo.width
            end
        end
    else
        for _, v in ipairs {"top", "bottom"} do
            if (not position) or position:match(v) then
                struts[v] = geo.height
            end
        end
    end

    -- Update the workarea
    d:struts(struts)
end

--- Pin a drawable to a placement function.
-- Automatically update the position when the size change.
-- All other arguments will be passed to the `position` function (if any)
-- @tparam[opt=client.focus] drawable d A drawable (like `client`, `mouse`
--   or `wibox`)
-- @param position_f A position name (see `align`) or a position function
-- @tparam[opt={}] table args Other arguments
local function attach(d, position_f, args)
    args = args or {}

    if not args.attach then return end

    d = d or capi.client.focus
    if not d then return end

    if type(position_f) == "string" then
        position_f = placement[position_f]
    end

    if not position_f then return end

    local function tracker()
        position_f(d, args)
    end

    d:connect_signal("property::width" , tracker)
    d:connect_signal("property::height", tracker)

    tracker()

    if args.update_workarea then
        local function tracker_struts()
            --TODO this is too fragile and doesn't work with all methods.
            wibox_update_strut(d, reverse_align_map[position_f])
        end

        d:connect_signal("property::geometry" , tracker_struts)
        d:connect_signal("property::visible"  , tracker_struts)

        tracker_struts()
    end

    -- If there is a parent drawable, screen or mouse, also track it
    local parent = args.parent or d.screen
    if parent then
        args.parent:connect_signal("property::geometry" , tracker)
    end
end

--- Check if an area intersect another area.
-- @param a The area.
-- @param b The other area.
-- @return True if they intersect, false otherwise.
local function area_intersect_area(a, b)
    return (b.x < a.x + a.width
            and b.x + b.width > a.x
            and b.y < a.y + a.height
            and b.y + b.height > a.y)
end

--- Get the intersect area between a and b.
-- @param a The area.
-- @param b The other area.
-- @return The intersect area.
local function area_intersect_area_get(a, b)
    local g = {}
    g.x = math.max(a.x, b.x)
    g.y = math.max(a.y, b.y)
    g.width = math.min(a.x + a.width, b.x + b.width) - g.x
    g.height = math.min(a.y + a.height, b.y + b.height) - g.y
    return g
end

--- Remove an area from a list, splitting the space between several area that
-- can overlap.
-- @param areas Table of areas.
-- @param elem Area to remove.
-- @return The new area list.
local function area_remove(areas, elem)
    for i = #areas, 1, -1 do
        -- Check if the 'elem' intersect
        if area_intersect_area(areas[i], elem) then
            -- It does? remove it
            local r = table.remove(areas, i)
            local inter = area_intersect_area_get(r, elem)

            if inter.x > r.x then
                table.insert(areas, {
                    x = r.x,
                    y = r.y,
                    width = inter.x - r.x,
                    height = r.height
                })
            end

            if inter.y > r.y then
                table.insert(areas, {
                    x = r.x,
                    y = r.y,
                    width = r.width,
                    height = inter.y - r.y
                })
            end

            if inter.x + inter.width < r.x + r.width then
                table.insert(areas, {
                    x = inter.x + inter.width,
                    y = r.y,
                    width = (r.x + r.width) - (inter.x + inter.width),
                    height = r.height
                })
            end

            if inter.y + inter.height < r.y + r.height then
                table.insert(areas, {
                    x = r.x,
                    y = inter.y + inter.height,
                    width = r.width,
                    height = (r.y + r.height) - (inter.y + inter.height)
                })
            end
        end
    end

    return areas
end

-- Convert 2 points into a rectangle
local function rect_from_points(p1x, p1y, p2x, p2y)
    return {
        x      = p1x,
        y      = p1y,
        width  = p2x - p1x,
        height = p2y - p1y,
    }
end

-- Convert a rectangle and matrix info into a point
local function rect_to_point(rect, corner_i, corner_j)
    return {
        x = rect.x + corner_i * math.floor(rect.width ),
        y = rect.y + corner_j * math.floor(rect.height),
    }
end

--- Move a drawable to the closest corner of the parent geometry (such as the
-- screen).
--
-- Valid arguments include the common ones and:
--
-- * **include_sides**: Also include the left, right, top and bottom positions
--
--@DOC_awful_placement_closest_mouse_EXAMPLE@
-- @tparam[opt=client.focus] drawable d A drawable (like `client`, `mouse`
--   or `wibox`)
-- @tparam[opt={}] table args The arguments
-- @treturn table The new geometry
-- @treturn string The corner name
function placement.closest_corner(d, args)
    args = add_context(args, "closest_corner")
    d = d or capi.client.focus

    local sgeo = get_parent_geometry(d, args)
    local dgeo = geometry_common(d, args)

    local pos  = move_into_geometry(sgeo, dgeo)

    local corner_i, corner_j, n

    -- Use the product of 3 to get the closest point in a NxN matrix
    local function f(_n, mat)
        n        = _n
        corner_i = -math.ceil( ( (sgeo.x - pos.x) * n) / sgeo.width  )
        corner_j = -math.ceil( ( (sgeo.y - pos.y) * n) / sgeo.height )
        return mat[corner_j + 1][corner_i + 1]
    end

    -- Turn the area into a grid and snap to the cloest point. This size of the
    -- grid will increase the accuracy. A 2x2 matrix only include the corners,
    -- at 3x3, this include the sides too technically, a random size would work,
    -- but without corner names.
    local grid_size = args.include_sides and 3 or 2

    -- If the point is in the center, use the closest corner
    local corner = grid_size == 3 and f(3, corners3x3) or f(2, corners2x2)

    -- Transpose the corner back to the original size
    local new_args = setmetatable({position = corner}, {__index=args})
    local ngeo = placement_private.align(d, new_args)

    return ngeo, corner
end

--- Place the client so no part of it will be outside the screen (workarea).
--@DOC_awful_placement_no_offscreen_EXAMPLE@
-- @client c The client.
-- @tparam[opt=client's screen] integer screen The screen.
-- @treturn table The new client geometry.
function placement.no_offscreen(c, screen)
    --HACK necessary for composition to work. The API will be changed soon
    if type(screen) == "table" then
        screen = nil
    end

    c = c or capi.client.focus
    local geometry = area_common(c)
    screen = get_screen(screen or c.screen or a_screen.getbycoord(geometry.x, geometry.y))
    local screen_geometry = screen.workarea

    if geometry.x + geometry.width > screen_geometry.x + screen_geometry.width then
        geometry.x = screen_geometry.x + screen_geometry.width - geometry.width
    end
    if geometry.x < screen_geometry.x then
        geometry.x = screen_geometry.x
    end

    if geometry.y + geometry.height > screen_geometry.y + screen_geometry.height then
        geometry.y = screen_geometry.y + screen_geometry.height - geometry.height
    end
    if geometry.y < screen_geometry.y then
        geometry.y = screen_geometry.y
    end

    return c:geometry {
        x = geometry.x,
        y = geometry.y
    }
end

--- Place the client where there's place available with minimum overlap.
--@DOC_awful_placement_no_overlap_EXAMPLE@
-- @param c The client.
-- @treturn table The new geometry
function placement.no_overlap(c)
    c = c or capi.client.focus
    local geometry = area_common(c)
    local screen   = get_screen(c.screen or a_screen.getbycoord(geometry.x, geometry.y))
    local cls = client.visible(screen)
    local curlay = layout.get()
    local areas = { screen.workarea }
    for _, cl in pairs(cls) do
        if cl ~= c and cl.type ~= "desktop" and (cl.floating or curlay == layout.suit.floating) then
            areas = area_remove(areas, area_common(cl))
        end
    end

    -- Look for available space
    local found = false
    local new = { x = geometry.x, y = geometry.y, width = 0, height = 0 }
    for _, r in ipairs(areas) do
        if r.width >= geometry.width
           and r.height >= geometry.height
           and r.width * r.height > new.width * new.height then
            found = true
            new = r
            -- Check if the client's current position is available
            -- and prefer that one (why move it around pointlessly?)
            if     geometry.x >= r.x
               and geometry.y >= r.y
               and geometry.x + geometry.width <= r.x + r.width
               and geometry.y + geometry.height <= r.y + r.height then
                new.x = geometry.x
                new.y = geometry.y
            end
        end
    end

    -- We did not find an area with enough space for our size:
    -- just take the biggest available one and go in
    if not found then
        for _, r in ipairs(areas) do
            if r.width * r.height > new.width * new.height then
                new = r
            end
        end
    end

    -- Restore height and width
    new.width = geometry.width
    new.height = geometry.height

    return c:geometry({ x = new.x, y = new.y })
end

--- Place the client under the mouse.
--@DOC_awful_placement_under_mouse_EXAMPLE@
-- @param c The client.
-- @treturn table The new geometry
function placement.under_mouse(c)
    c = c or capi.client.focus
    local c_geometry = area_common(c)
    local m_coords = capi.mouse.coords()
    return c:geometry({ x = m_coords.x - c_geometry.width / 2,
                        y = m_coords.y - c_geometry.height / 2 })
end

--- Place the client next to the mouse.
--
-- It will place `c` next to the mouse pointer, trying the following positions
-- in this order: right, left, above and below.
--@DOC_awful_placement_next_to_mouse_EXAMPLE@
-- @client[opt=focused] c The client.
-- @tparam[opt=apply_dpi(5)] integer offset The offset from the mouse position.
-- @treturn table The new geometry
function placement.next_to_mouse(c, offset)
    c = c or capi.client.focus
    offset = offset or dpi(5)
    local c_geometry = area_common(c)
    local c_width = c_geometry.width
    local c_height = c_geometry.height
    local m_coords = capi.mouse.coords()
    local screen_geometry = capi.screen[capi.mouse.screen].workarea

    local x, y

    -- Prefer it to be on the right.
    x = m_coords.x + offset
    if x + c_width > screen_geometry.width then
        -- Then to the left.
        x = m_coords.x - c_width - offset
    end
    if x < screen_geometry.x then
        -- Then above.
        x = m_coords.x - math.ceil(c_width / 2)
        y = m_coords.y - c_height - offset
        if y < screen_geometry.y then
            -- Finally below.
            y = m_coords.y + offset
        end
    else
        y = m_coords.y - math.ceil(c_height / 2)
    end
    return c:geometry({ x = x, y = y })
end

--- Resize the drawable to the cursor.
--
-- Valid args:
--
-- * *axis*: The axis (vertical or horizontal). If none is
--    specified, then the drawable will be resized on both axis.
--
--@DOC_awful_placement_resize_to_mouse_EXAMPLE@
-- @tparam drawable d A drawable (like `client`, `mouse` or `wibox`)
-- @tparam[opt={}] table args Other arguments
-- @treturn table The new geometry
function placement.resize_to_mouse(d, args)
    d    = d or capi.client.focus
    args = add_context(args, "resize_to_mouse")

    local coords = capi.mouse.coords()
    local ngeo   = geometry_common(d, args)
    local h_only = args.axis == "horizontal"
    local v_only = args.axis == "vertical"

    -- To support both growing and shrinking the drawable, it is necessary
    -- to decide to use either "north or south" and "east or west" directions.
    -- Otherwise, the result will always be 1x1
    local _, closest_corner = placement.closest_corner(capi.mouse, {
        parent        = d,
        pretend       = true,
        include_sides = args.include_sides or false,
    })

    -- Given "include_sides" wasn't set, it will always return a name
    -- with the 2 axis. If only one axis is needed, adjust the result
    if h_only then
        closest_corner = closest_corner:match("left") or closest_corner:match("right")
    elseif v_only then
        closest_corner = closest_corner:match("top")  or closest_corner:match("bottom")
    end

    -- Use p0 (mouse), p1 and p2 to create a rectangle
    local pts = resize_to_point_map[closest_corner]
    local p1  = pts.p1 and rect_to_point(ngeo, pts.p1[1], pts.p1[2]) or coords
    local p2  = pts.p2 and rect_to_point(ngeo, pts.p2[1], pts.p2[2]) or coords

    -- Create top_left and bottom_right points, convert to rectangle
    ngeo = rect_from_points(
        pts.y_only and ngeo.x               or math.min(p1.x, p2.x),
        pts.x_only and ngeo.y               or math.min(p1.y, p2.y),
        pts.y_only and ngeo.x + ngeo.width  or math.max(p2.x, p1.x),
        pts.x_only and ngeo.y + ngeo.height or math.max(p2.y, p1.y)
    )

    local bw = d.border_width or 0

    for _, a in ipairs {"width", "height"} do
        ngeo[a] = ngeo[a] - 2*bw
    end

    -- Now, correct the geometry by the given size_hints offset
    if d.apply_size_hints then
        local w, h = d:apply_size_hints(
            ngeo.width,
            ngeo.height
        )
        local offset = align_map[pts.align](w, h, ngeo.width, ngeo.height)
        ngeo.x = ngeo.x - offset.x
        ngeo.y = ngeo.y - offset.y
    end

    geometry_common(d, args, ngeo)

    return ngeo
end

--- Move the drawable (client or wibox) `d` to a screen position or side.
--
-- Supported args.positions are:
--
-- * top_left
-- * top_right
-- * bottom_left
-- * bottom_right
-- * left
-- * right
-- * top
-- * bottom
-- * centered
-- * center_vertical
-- * center_horizontal
--
--@DOC_awful_placement_align_EXAMPLE@
-- @tparam drawable d A drawable (like `client`, `mouse` or `wibox`)
-- @tparam[opt={}] table args Other arguments
-- @treturn table The new geometry
function placement.align(d, args)
    args = add_context(args, "align")
    d    = d or capi.client.focus

    if not d or not args.position then return end

    local sgeo = get_parent_geometry(d, args)
    local dgeo = geometry_common(d, args)
    local bw   = d.border_width or 0

    local pos  = align_map[args.position](
        sgeo.width ,
        sgeo.height,
        dgeo.width ,
        dgeo.height
    )

    local ngeo = {
        x      = (pos.x and math.ceil(sgeo.x + pos.x) or dgeo.x)       ,
        y      = (pos.y and math.ceil(sgeo.y + pos.y) or dgeo.y)       ,
        width  =            math.ceil(dgeo.width    )            - 2*bw,
        height =            math.ceil(dgeo.height   )            - 2*bw,
    }

    geometry_common(d, args, ngeo)

    attach(d, placement[args.position], args)

    return ngeo
end

-- Add the alias functions
for k in pairs(align_map) do
    placement[k] = function(d, args)
        args = add_context(args, k)
        args.position = k
        return placement_private.align(d, args)
    end
    reverse_align_map[placement[k]] = k
end

-- Add the documentation for align alias

---@DOC_awful_placement_top_left_EXAMPLE@

---@DOC_awful_placement_top_right_EXAMPLE@

---@DOC_awful_placement_bottom_left_EXAMPLE@

---@DOC_awful_placement_bottom_right_EXAMPLE@

---@DOC_awful_placement_left_EXAMPLE@

---@DOC_awful_placement_right_EXAMPLE@

---@DOC_awful_placement_top_EXAMPLE@

---@DOC_awful_placement_bottom_EXAMPLE@

---@DOC_awful_placement_centered_EXAMPLE@

---@DOC_awful_placement_center_vertical_EXAMPLE@

---@DOC_awful_placement_center_horizontal_EXAMPLE@

--- Stretch a drawable in a specific direction.
-- Valid args:
--
-- * **direction**: The stretch direction (*left*, *right*, *up*, *down*) or
--  a table with multiple directions.
--
--@DOC_awful_placement_stretch_EXAMPLE@
-- @tparam[opt=client.focus] drawable d A drawable (like `client` or `wibox`)
-- @tparam[opt={}] table args The arguments
-- @treturn table The new geometry
function placement.stretch(d, args)
    args = add_context(args, "stretch")

    d    = d or capi.client.focus
    if not d or not args.direction then return end

    -- In case there is multiple directions, call `stretch` for each of them
    if type(args.direction) == "table" then
        for _, dir in ipairs(args.direction) do
            args.direction = dir
            placement_private.stretch(dir, args)
        end
        return
    end

    local sgeo = get_parent_geometry(d, args)
    local dgeo = geometry_common(d, args)
    local ngeo = geometry_common(d, args, nil, true)
    local bw   = d.border_width or 0

    if args.direction == "left" then
        ngeo.x      = sgeo.x
        ngeo.width  = dgeo.width + (dgeo.x - ngeo.x)
    elseif args.direction == "right" then
        ngeo.width  = sgeo.width - ngeo.x - 2*bw
    elseif args.direction == "up" then
        ngeo.y      = sgeo.y
        ngeo.height = dgeo.height + (dgeo.y - ngeo.y)
    elseif args.direction == "down" then
        ngeo.height = sgeo.height - dgeo.y - 2*bw
    else
        assert(false)
    end

    -- Avoid negative sizes if args.parent isn't compatible
    ngeo.width  = math.max(args.minimim_width  or 1, ngeo.width )
    ngeo.height = math.max(args.minimim_height or 1, ngeo.height)

    geometry_common(d, args, ngeo)

    attach(d, placement["stretch_"..args.direction], args)

    return ngeo
end

-- Add the alias functions
for _,v in ipairs {"left", "right", "up", "down"} do
    placement["stretch_"..v] =  function(d, args)
        args = add_context(args, "stretch_"..v)
        args.direction = v
        return placement_private.stretch(d, args)
    end
end

---@DOC_awful_placement_stretch_left_EXAMPLE@

---@DOC_awful_placement_stretch_right_EXAMPLE@

---@DOC_awful_placement_stretch_up_EXAMPLE@

---@DOC_awful_placement_stretch_down_EXAMPLE@

--- Maximize a drawable horizontally, vertically or both.
-- Valid args:
--
-- * *axis*:The axis (vertical or horizontal). If none is
--    specified, then the drawable will be maximized on both axis.
--
--@DOC_awful_placement_maximize_EXAMPLE@
-- @tparam[opt=client.focus] drawable d A drawable (like `client` or `wibox`)
-- @tparam[opt={}] table args The arguments
-- @treturn table The new geometry
function placement.maximize(d, args)
    args = add_context(args, "maximize")
    d    = d or capi.client.focus

    if not d then return end

    local sgeo = get_parent_geometry(d, args)
    local ngeo = geometry_common(d, args, nil, true)
    local bw   = d.border_width or 0

    if (not args.axis) or args.axis :match "vertical" then
        ngeo.y      = sgeo.y
        ngeo.height = sgeo.height - 2*bw
    end

    if (not args.axis) or args.axis :match "horizontal" then
        ngeo.x      = sgeo.x
        ngeo.width  = sgeo.width - 2*bw
    end

    geometry_common(d, args, ngeo)

    attach(d, placement.maximize, args)

    return ngeo
end

-- Add the alias functions
for _, v in ipairs {"vertically", "horizontally"} do
    placement["maximize_"..v] = function(d2, args)
        args = add_context(args, "maximize_"..v)
        args.axis = v
        return placement_private.maximize(d2, args)
    end
end

---@DOC_awful_placement_maximize_vertically_EXAMPLE@

---@DOC_awful_placement_maximize_horizontally_EXAMPLE@

--- Restore the geometry.
-- @tparam[opt=client.focus] drawable d A drawable (like `client` or `wibox`)
-- @tparam[opt={}] table args The arguments
-- @treturn boolean If the geometry was restored
function placement.restore(d, args)
    if not args or not args.context then return false end
    d = d or capi.client.focus

    if not data[d] then return false end

    local memento = data[d][args.context]

    if not memento then return false end

    memento.screen = nil --TODO use it
    d:geometry(memento)
    return true
end

return placement

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
