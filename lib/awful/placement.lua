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
-- <h3>Compositing</h3>
--
-- It is possible to compose placement function using the `+` or `*` operator:
--
--@DOC_awful_placement_compose_EXAMPLE@
--
--@DOC_awful_placement_compose2_EXAMPLE@
--
-- <h3>Common arguments</h3>
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
-- When the parent geometry (like the screen) changes, re-apply the placement
-- function. This will add a `detach_callback` function to the drawable. Call
-- this to detach the function. This will be called automatically when a new
-- attached function is set.
--
-- **offset** (*table or number*):
--
-- The offset(s) to apply to the new geometry.
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
local floating = require("awful.layout.suit.floating")
local a_screen = require("awful.screen")
local grect = require("gears.geometry").rectangle
local gdebug = require("gears.debug")
local gmath = require("gears.math")
local gtable = require("gears.table")
local cairo = require( "lgi" ).cairo
local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)

local function get_screen(s)
    return s and capi.screen[s]
end

local wrap_client = nil
local placement

-- Store function -> keys
local reverse_align_map = {}

-- Forward declarations
local area_common
local wibox_update_strut
local attach

--- Allow multiple placement functions to be daisy chained.
-- This also allow the functions to be aware they are being chained and act
-- upon the previous nodes results to avoid unnecessary processing or deduce
-- extra parameters/arguments.
local function compose(...)
    local queue = {}

    local nodes = {...}

    -- Allow placement.foo + (var == 42 and placement.bar)
    if not nodes[2] then
        return nodes[1]
    end

    -- nodes[1] == self, nodes[2] == other
    for _, w in ipairs(nodes) do
        -- Build an execution queue
        if w.context and w.context == "compose" then
            for _, elem in ipairs(w.queue or {}) do
                table.insert(queue, elem)
            end
        else
            table.insert(queue, w)
        end
    end

    local ret
    ret = wrap_client(function(d, args, ...)
        local rets = {}
        local last_geo = nil

        -- As some functions may have to take into account results from
        -- previously executed ones, add the `composition_results` hint.
        args = setmetatable({composition_results=rets}, {__index=args})

        -- Only apply the geometry once, not once per chain node, to do this,
        -- Force the "pretend" argument and restore the original value for
        -- the last node.
        local attach_real = args.attach
        args.pretend      = true
        args.attach       = false
        args.offset       = {}

        for k, f in ipairs(queue) do
            if k == #queue then
                -- Let them fallback to the parent table
                args.pretend = nil
                args.offset  = nil
            end

            local r = {f(d, args, ...)}
            last_geo = r[1] or last_geo
            args.override_geometry = last_geo

            -- Keep the return value, store one per context
            if f.context then
                -- When 2 composition queue are executed, merge the return values
                if f.context == "compose" then
                    for k2,v in pairs(r) do
                        rets[k2] = v
                    end
                else
                    rets[f.context] = r
                end
            end
        end

        if attach_real then
            args.attach = true
            attach(d, ret, args)
        end

        -- Cleanup the override because otherwise it might leak into
        -- future calls.
        rawset(args, "override_geometry", nil)

        return last_geo, rets
    end, "compose")

    ret.queue = queue

    return ret
end

wrap_client = function(f, context)
    return setmetatable(
        {
            is_placement= true,
            context     = context,
        },
        {
            __call = function(_,...) return f(...) end,
            __add  = compose, -- Composition is usually defined as +
            __mul  = compose  -- Make sense if you think of the functions as matrices
        }
    )
end

local placement_private = {}

-- The module is a proxy in front of the "real" functions.
-- This allow syntax like:
--
--    (awful.placement.no_overlap + awful.placement.no_offscreen)(c)
--
placement = setmetatable({}, {
    __index = placement_private,
    __newindex = function(_, k, f)
        placement_private[k] = wrap_client(f, k)
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

-- Outer position matrix
local outer_positions = {
    left_front    = function(r, w, _) return {x=r.x-w            , y=r.y                }, "front"  end,
    left_back     = function(r, w, h) return {x=r.x-w            , y=r.y-h+r.height     }, "back"   end,
    left_middle   = function(r, w, h) return {x=r.x-w            , y=r.y-h/2+r.height/2 }, "middle" end,
    right_front   = function(r, _, _) return {x=r.x              , y=r.y                }, "front"  end,
    right_back    = function(r, _, h) return {x=r.x              , y=r.y-h+r.height     }, "back"   end,
    right_middle  = function(r, _, h) return {x=r.x              , y=r.y-h/2+r.height/2 }, "middle" end,
    top_front     = function(r, _, h) return {x=r.x              , y=r.y-h              }, "front"  end,
    top_back      = function(r, w, h) return {x=r.x-w+r.width    , y=r.y-h              }, "back"   end,
    top_middle    = function(r, w, h) return {x=r.x-w/2+r.width/2, y=r.y-h              }, "middle" end,
    bottom_front  = function(r, _, _) return {x=r.x              , y=r.y                }, "front"  end,
    bottom_back   = function(r, w, _) return {x=r.x-w+r.width    , y=r.y                }, "back"   end,
    bottom_middle = function(r, w, _) return {x=r.x-w/2+r.width/2, y=r.y                }, "middle" end,
}

-- Map the opposite side for a string
local opposites = {
    top    = "bottom",
    bottom = "top",
    left   = "right",
    right  = "left",
    width  = "height",
    height = "width",
    x      = "y",
    y      = "x",
}

-- List reletvant sides for each orientation.
local struts_orientation_to_sides = {
    horizontal = { "top" , "bottom" },
    vertical   = { "left", "right"  }
}

-- Map orientation to the length components (width/height).
local orientation_to_length = {
    horizontal = "width",
    vertical   = "height"
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
    data[d][reqtype].sgeo = d.screen and d.screen.geometry or nil
    data[d][reqtype].border_width = d.border_width
end

--- Get the margins and offset
-- @tparam table args The arguments
-- @treturn table The margins
-- @treturn table The offsets
local function get_decoration(args)
    local offset = args.offset

    -- Offset are "blind" values added to the output
    offset = type(offset) == "number" and {
        x      = offset,
        y      = offset,
        width  = offset,
        height = offset,
    } or args.offset or {}

    -- Margins are distances on each side to subtract from the area`
    local m = type(args.margins) == "table" and args.margins or {
        left = args.margins or 0 , right  = args.margins or 0,
        top  = args.margins or 0 , bottom = args.margins or 0
    }

    return m, offset
end

--- Apply some modifications before applying the new geometry.
-- @tparam table new_geo The new geometry
-- @tparam table args The common arguments
-- @tparam boolean force Always adjust the geometry, even in pretent mode. This
--  should only be used when returning the final geometry as it would otherwise
--  mess the pipeline.
-- @treturn table|nil The new geometry
local function fix_new_geometry(new_geo, args, force)
    if (args.pretend and not force) or not new_geo then return nil end

    local m, offset = get_decoration(args)

    return {
        x      = new_geo.x      and (new_geo.x + (offset.x or 0) + (m.left or 0) ),
        y      = new_geo.y      and (new_geo.y + (offset.y or 0) + (m.top  or 0) ),
        width  = new_geo.width  and math.max(
            1, (new_geo.width  + (offset.width  or 0) - (m.left or 0) - (m.right or 0)  )
        ),
        height = new_geo.height and math.max(
            1, (new_geo.height + (offset.height or 0) - (m.top  or 0) - (m.bottom or 0) )
        ),
    }
end

-- Get the area covered by a drawin.
-- @param d The drawin
-- @tparam[opt=nil] table new_geo A new geometry
-- @tparam[opt=false] boolean ignore_border_width Ignore the border
-- @tparam table args the method arguments
-- @treturn The drawin's area.
area_common = function(d, new_geo, ignore_border_width, args)
    -- The C side expect no arguments, nil isn't valid
    if new_geo and args.zap_border_width then
        d._border_width = 0
    end
    local geometry = new_geo and d:geometry(new_geo) or d:geometry()
    local border = ignore_border_width and 0 or d.border_width or 0

    -- When using the placement composition along with the "pretend"
    -- option, it is necessary to keep a "virtual" geometry.
    if args and args.override_geometry then
        geometry = gtable.clone(args.override_geometry)
    end

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
        local coords = fix_new_geometry(new_geo, args)
            and obj.coords(new_geo) or obj.coords()
        return {x=coords.x, y=coords.y, width=0, height=0}
    elseif obj.geometry then
        if obj.get_bounding_geometry then
            -- It is a screen, it doesn't support setting new sizes.
            return obj:get_bounding_geometry(args)
        end

        -- It is either a drawable or something that implement its API
        local dgeo = area_common(
            obj, fix_new_geometry(new_geo, args), ignore_border_width, args
        )

        -- Apply the margins
        if args.margins then
            local delta = get_decoration(args)

            return {
                x      = dgeo.x      - (delta.left or 0),
                y      = dgeo.y      - (delta.top  or 0),
                width  = dgeo.width  + (delta.left or 0) + (delta.right  or 0),
                height = dgeo.height + (delta.top  or 0) + (delta.bottom or 0),
            }
        end

        return dgeo
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
    -- Didable override_geometry, context and other to avoid mutating the state
    -- or using the wrong geo.

    if args.bounding_rect then
        return args.bounding_rect
    elseif args.parent then
        return geometry_common(args.parent, {})
    elseif obj.screen then
        return geometry_common(obj.screen, {
            honor_padding  = args.honor_padding,
            honor_workarea = args.honor_workarea
        })
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
wibox_update_strut = function(d, position, args)
    -- If the drawable isn't visible, remove the struts
    if not d.visible then
        d:struts { left = 0, right = 0, bottom = 0, top = 0 }
        return
    end

    -- Detect horizontal or vertical drawables
    local geo         = area_common(d)
    local orientation = geo.width < geo.height and "vertical" or "horizontal"

    -- Look into the `position` string to find the relevants sides to crop from
    -- the workarea
    local struts = { left = 0, right = 0, bottom = 0, top = 0 }

    local m = get_decoration(args)

    for _, v in ipairs(struts_orientation_to_sides[orientation]) do
        if (not position) or position:match(v) then
            -- Add the "short" rectangle length then the above and below margins.
            struts[v] = geo[opposites[orientation_to_length[orientation]]]
                + m[v]
                + m[opposites[v]]
        end
    end

    -- Update the workarea
    d:struts(struts)
end

-- Pin a drawable to a placement function.
-- Automatically update the position when the size change.
-- All other arguments will be passed to the `position` function (if any)
-- @tparam[opt=client.focus] drawable d A drawable (like `client`, `mouse`
--   or `wibox`)
-- @param position_f A position name (see `align`) or a position function
-- @tparam[opt={}] table args Other arguments
attach = function(d, position_f, args)
    args = args or {}

    if args.pretend then return end

    if not args.attach then return end

    -- Avoid a connection loop
    args = setmetatable({attach=false}, {__index=args})

    d = d or capi.client.focus
    if not d then return end

    if type(position_f) == "string" then
        position_f = placement[position_f]
    end

    if not position_f then return end

    -- If there is multiple attached function, there is an high risk of infinite
    -- loop. While some combinaisons are harmless, other are very hard to debug.
    --
    -- Use the placement composition to build explicit multi step attached
    -- placement functions.
    if d.detach_callback then
        d.detach_callback()
        d.detach_callback = nil
    end

    local function tracker()
        position_f(d, args)
    end

    d:connect_signal("property::width"       , tracker)
    d:connect_signal("property::height"      , tracker)
    d:connect_signal("property::border_width", tracker)

    local function tracker_struts()
        --TODO this is too fragile and doesn't work with all methods.
        wibox_update_strut(d, d.position or reverse_align_map[position_f], args)
    end

    local parent = args.parent or d.screen

    if args.update_workarea then
        d:connect_signal("property::geometry" , tracker_struts)
        d:connect_signal("property::visible"  , tracker_struts)
        capi.client.connect_signal("property::struts", tracker_struts)

        tracker_struts()
    elseif parent == d.screen then
        if args.honor_workarea then
            parent:connect_signal("property::workarea", tracker)
        end

        if args.honor_padding then
            parent:connect_signal("property::padding", tracker)
        end
    end

    -- If there is a parent drawable, screen, also track it.
    -- Note that tracking the mouse is not supported
    if parent and parent.connect_signal then
        parent:connect_signal("property::geometry" , tracker)
    end

    -- Create a way to detach a placement function
    function d.detach_callback()
        d:disconnect_signal("property::width"       , tracker)
        d:disconnect_signal("property::height"      , tracker)
        d:disconnect_signal("property::border_width", tracker)
        if parent then
            parent:disconnect_signal("property::geometry" , tracker)

            if parent == d.screen then
                if args.honor_workarea then
                    parent:disconnect_signal("property::workarea", tracker)
                end

                if args.honor_padding then
                    parent:disconnect_signal("property::padding", tracker)
                end
            end
        end

        if args.update_workarea then
            d:disconnect_signal("property::geometry" , tracker_struts)
            d:disconnect_signal("property::visible"  , tracker_struts)
            capi.client.disconnect_signal("property::struts", tracker_struts)
        end
    end
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

-- Create a pair of rectangles used to set the relative areas.
-- v=vertical, h=horizontal
local function get_cross_sections(abs_geo, mode)
    if not mode or mode == "cursor" then
        -- A 1px cross section centered around the mouse position
        local coords = capi.mouse.coords()
        return {
            h = {
                x      = abs_geo.drawable_geo.x     ,
                y      = coords.y                   ,
                width  = abs_geo.drawable_geo.width ,
                height = 1                          ,
            },
            v = {
                x      = coords.x                   ,
                y      = abs_geo.drawable_geo.y     ,
                width  = 1                          ,
                height = abs_geo.drawable_geo.height,
            }
        }
    elseif mode == "geometry" then
        -- The widget geometry extended to reach the end of the drawable
        return {
            h = {
                x      = abs_geo.drawable_geo.x     ,
                y      = abs_geo.y                  ,
                width  = abs_geo.drawable_geo.width ,
                height = abs_geo.height             ,
            },
            v = {
                x      = abs_geo.x                  ,
                y      = abs_geo.drawable_geo.y     ,
                width  = abs_geo.width              ,
                height = abs_geo.drawable_geo.height,
            }
        }
    elseif mode == "cursor_inside" then
        -- A 1x1 rectangle  centered around the mouse position

        local coords = capi.mouse.coords()
        coords.width,coords.height = 1,1
        return {h=coords, v=coords}
    elseif mode == "geometry_inside" then
        -- The widget absolute geometry, unchanged

        return {h=abs_geo, v=abs_geo}
    end
end

-- When a rectangle is embedded into a bigger one, get the regions around
-- the outline of the bigger rectangle closest to the smaller one (on each side)
local function get_relative_regions(geo, mode, is_absolute)

    -- Use the mouse position and the wibox/client under it
    if not geo then
        local draw   = capi.mouse.current_wibox
        geo          = draw and draw:geometry() or capi.mouse.coords()
        geo.drawable = draw
    elseif is_absolute then
        -- Some signals are a bit inconsistent in their arguments convention.
        -- This little hack tries to mitigate the issue.

        geo.drawable = geo -- is a wibox or client, geometry and object are one
                           -- and the same.
    elseif (not geo.drawable) and geo.x and geo.width then
        local coords = capi.mouse.coords()

        -- Check if the mouse is in the rect
        if coords.x > geo.x and coords.x < geo.x+geo.width and
          coords.y > geo.y and coords.y < geo.y+geo.height then
            geo.drawable = capi.mouse.current_wibox
        end

        -- Maybe there is a client
        if (not geo.drawable) and capi.mouse.current_client then
            geo.drawable = capi.mouse.current_client
        end
    end

    -- Get the parent geometry using one way or another depending on the object
    -- Type
    local bw, dgeo = 0, {x=0, y=0, width=1, height=1}

    -- Detect various types of geometry table and (try) to get rid of the
    -- differences so the code below don't have to care anymore.
    if geo.drawin then
        bw, dgeo = geo.drawin._border_width, geo.drawin:geometry()
    elseif geo.drawable and geo.drawable.get_wibox then
        bw   = geo.drawable.get_wibox().border_width
        dgeo = geo.drawable.get_wibox():geometry()
    elseif geo.drawable and geo.drawable.drawable then
        bw, dgeo = 0, geo.drawable.drawable:geometry()
    else
        -- The placement isn't done on an object at all, having no border is
        -- normal.
        assert(mode == "geometry")
    end

    -- Add the infamous border size
    dgeo.width  = dgeo.width  + 2*bw
    dgeo.height = dgeo.height + 2*bw

    -- Compute the absolute widget geometry
    local abs_widget_geo = is_absolute and dgeo or {
        x            = dgeo.x + geo.x + bw,
        y            = dgeo.y + geo.y + bw,
        width        = geo.width          ,
        height       = geo.height         ,
        drawable     = geo.drawable       ,
    }

    abs_widget_geo.drawable_geo = geo.drawable and dgeo or geo

    -- Get the point for comparison.
    local center_point = mode:match("cursor") and capi.mouse.coords() or {
        x = abs_widget_geo.x + abs_widget_geo.width  / 2,
        y = abs_widget_geo.y + abs_widget_geo.height / 2,
    }

    -- Get widget regions for both axis
    local cs = get_cross_sections(abs_widget_geo, mode)

    -- Get the 4 closest points from `center_point` around the wibox
    local regions = {
        left   = {x = cs.h.x           , y = cs.h.y            },
        right  = {x = cs.h.x+cs.h.width, y = cs.h.y            },
        top    = {x = cs.v.x           , y = cs.v.y            },
        bottom = {x = cs.v.x           , y = cs.v.y+cs.v.height},
    }

    -- Assume the section is part of a single screen until someone complains.
    -- It is much faster to compute and getting it wrong probably has no side
    -- effects.
    local s = geo.drawable and geo.drawable.screen or a_screen.getbycoord(
                                                        center_point.x,
                                                        center_point.y
                                                      )

    -- Compute the distance (dp) between the `center_point` and the sides.
    -- This is only relevant for "cursor" and "cursor_inside" modes.
    for _, v in pairs(regions) do
        local dx, dy = v.x - center_point.x, v.y - center_point.y

        v.distance = math.sqrt(dx*dx + dy*dy)
        v.width    = cs.v.width
        v.height   = cs.h.height
        v.screen   = capi.screen[s]
    end

    return regions
end

local function round_geometry(geo)
    local geo_keys = { "x", "y", "height", "width" }
    local rounded_geo = {}
    for _, key in pairs(geo_keys) do
        if geo[key] ~= nil then
            rounded_geo[key] = gmath.round(geo[key])
        end
    end
    return rounded_geo
end

-- Check if the proposed geometry fits the screen
local function fit_in_bounding(obj, geo, args)
    local round_sgeo = round_geometry(get_parent_geometry(obj, args))
    local region     = cairo.Region.create_rectangle(cairo.RectangleInt(round_sgeo))

    local round_geo = round_geometry(geo)
    region:intersect(cairo.Region.create_rectangle(
        cairo.RectangleInt(round_geo)
    ))

    local geo2 = region:get_rectangle(0)

    -- If the geometry is the same then it fits, otherwise it will be cropped.
    return geo2.width == round_geo.width and geo2.height == round_geo.height
end

-- Remove border from drawable geometry
local function remove_border(drawable, args, geo)
    local bw    = (not args.ignore_border_width) and drawable.border_width or 0
    geo.width  = geo.width  - 2*bw
    geo.height = geo.height - 2*bw
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
-- @treturn string The corner name.
-- @staticfct awful.placement.closest_corner
function placement.closest_corner(d, args)
    args = add_context(args, "closest_corner")
    d = d or capi.client.focus

    local sgeo = get_parent_geometry(d, args)
    local dgeo = geometry_common(d, args)

    local pos  = move_into_geometry(sgeo, dgeo)

    local corner_i, corner_j, n

    -- Use the product of 3 to get the closest point in a NxN matrix
    local function f(dim, mat)
        n        = dim
        -- The +1 is required to avoid a rounding error when
        --    pos.x == sgeo.x+sgeo.width
        corner_i = -math.ceil( ( (sgeo.x - pos.x) * n) / (sgeo.width  + 1))
        corner_j = -math.ceil( ( (sgeo.y - pos.y) * n) / (sgeo.height + 1))
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

    return fix_new_geometry(ngeo, args, true), corner
end

--- Place the client so no part of it will be outside the screen (workarea).
--@DOC_awful_placement_no_offscreen_EXAMPLE@
-- @tparam client c The client.
-- @tparam[opt={}] table args The arguments
-- @tparam[opt=client's screen] integer args.screen The screen.
-- @treturn table The new client geometry.
-- @staticfct awful.placement.no_offscreen
function placement.no_offscreen(c, args)

    --compatibility with the old API
    if type(args) == "number" or type(args) == "screen" then
        gdebug.deprecate(
            "awful.placement.no_offscreen screen argument is deprecated"..
            " use awful.placement.no_offscreen(c, {screen=...})",
            {deprecated_in=5}
        )
        args = { screen = args }
    end

    c = c or capi.client.focus
    args = add_context(args, "no_offscreen")
    local geometry = geometry_common(c, args)
    local screen = get_screen(args.screen or c.screen or a_screen.getbycoord(geometry.x, geometry.y))
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

    remove_border(c, args, geometry)
    geometry_common(c, args, geometry)
    return fix_new_geometry(geometry, args, true)
end

-- Check whether the client is on at least one of selected tags (similar to
-- `c:isvisible()`, but without checks for the hidden or minimized state).
local function client_on_selected_tags(c)
    if c.sticky then
        return true
    else
        for _, t in pairs(c:tags()) do
            if t.selected then
                return true
            end
        end
        return false
    end
end

-- Check whether the client would be visible if the specified set of tags is
-- selected (using the same set of conditions as `c:isvisible()`, but checking
-- against the specified tags instead of currently selected ones).
local function client_visible_on_tags(c, tags)
    if c.hidden or c.minimized then
        return false
    elseif c.sticky then
        return true
    else
        for _, t in pairs(c:tags()) do
            if gtable.hasitem(tags, t) then
                return true
            end
        end
        return false
    end
end

--- Place the client where there's place available with minimum overlap.
--@DOC_awful_placement_no_overlap_EXAMPLE@
-- @param c The client.
-- @tparam[opt={}] table args Other arguments
-- @treturn table The new geometry
-- @staticfct awful.placement.no_overlap
function placement.no_overlap(c, args)
    c = c or capi.client.focus
    args = add_context(args, "no_overlap")
    local geometry = geometry_common(c, args)
    local screen   = get_screen(c.screen or a_screen.getbycoord(geometry.x, geometry.y))
    local cls, curlay
    if client_on_selected_tags(c) then
        cls = screen:get_clients(false)
        local t = screen.selected_tag
        curlay = t.layout or floating
    else
        -- When placing a client on unselected tags, place it as if all tags of
        -- that client are selected.
        local tags = c:tags()
        cls = {}
        for _, other_c in pairs(capi.client.get(screen)) do
            if client_visible_on_tags(other_c, tags) then
                table.insert(cls, other_c)
            end
        end
        curlay = tags[1] and tags[1].layout
    end
    local areas = { screen.workarea }
    for _, cl in pairs(cls) do
        if cl ~= c
           and cl.type ~= "desktop"
           and (cl.floating or curlay == floating)
           and not (cl.maximized or cl.fullscreen) then
            areas = grect.area_remove(areas, area_common(cl))
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
    -- just take the biggest available one and go in.
    -- This makes sure to have the whole screen's area in case it has been
    -- removed.
    if not found then
        if #areas > 0 then
            for _, r in ipairs(areas) do
                if r.width * r.height > new.width * new.height then
                    new = r
                end
            end
        elseif grect.area_intersect_area(geometry, screen.workarea) then
            new.x = geometry.x
            new.y = geometry.y
        else
            new.x = screen.workarea.x
            new.y = screen.workarea.y
        end
    end

    -- Restore height and width
    new.width = geometry.width
    new.height = geometry.height

    remove_border(c, args, new)
    geometry_common(c, args, new)
    return fix_new_geometry(new, args, true)
end

--- Place the client under the mouse.
--@DOC_awful_placement_under_mouse_EXAMPLE@
-- @tparam drawable d A drawable (like `client`, `mouse` or `wibox`)
-- @tparam[opt={}] table args Other arguments
-- @treturn table The new geometry
-- @staticfct awful.placement.under_mouse
function placement.under_mouse(d, args)
    args = add_context(args, "under_mouse")
    d = d or capi.client.focus

    local m_coords = capi.mouse.coords()

    local ngeo = geometry_common(d, args)
    ngeo.x = math.floor(m_coords.x - ngeo.width  / 2)
    ngeo.y = math.floor(m_coords.y - ngeo.height / 2)

    remove_border(d, args, ngeo)
    geometry_common(d, args, ngeo)

    return fix_new_geometry(ngeo, args, true)
end

--- Place the client next to the mouse.
--
-- It will place `c` next to the mouse pointer, trying the following positions
-- in this order: right, left, above and below.
--@DOC_awful_placement_next_to_mouse_EXAMPLE@
-- @tparam drawable d A drawable (like `client`, `mouse` or `wibox`)
-- @tparam[opt={}] table args Other arguments
-- @treturn table The new geometry
-- @staticfct awful.placement.next_to_mouse
function placement.next_to_mouse(d, args)
    if type(args) == "number" then
        gdebug.deprecate(
            "awful.placement.next_to_mouse offset argument is deprecated"..
            " use awful.placement.next_to_mouse(c, {offset={x=...}})",
            {deprecated_in=4}
        )
        args = nil
    end

    local old_args = args or {}

    args = add_context(args, "next_to_mouse")
    d = d or capi.client.focus

    local sgeo = get_parent_geometry(d, args)

    args.pretend = true
    args.parent  = capi.mouse

    local ngeo = placement.left(d, args)

    if ngeo.x + ngeo.width > sgeo.x+sgeo.width then
        ngeo = placement.right(d, args)
    else
        -- It is _next_ to mouse, not under_mouse
        ngeo.x = ngeo.x+1
    end

    args.pretend = old_args.pretend

    geometry_common(d, args, ngeo)

    attach(d, placement.next_to_mouse, old_args)

    return fix_new_geometry(ngeo, args, true)
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
-- @staticfct awful.placement.resize_to_mouse
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

    remove_border(d, args, ngeo)

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

    return fix_new_geometry(ngeo, args, true)
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
-- @staticfct awful.placement.align
function placement.align(d, args)
    args = add_context(args, "align")
    d    = d or capi.client.focus

    if not d or not args.position then return end

    local sgeo = get_parent_geometry(d, args)
    local dgeo = geometry_common(d, args)

    local pos  = align_map[args.position](
        sgeo.width ,
        sgeo.height,
        dgeo.width ,
        dgeo.height
    )

    local ngeo = {
        x      = (pos.x and math.ceil(sgeo.x + pos.x) or dgeo.x)       ,
        y      = (pos.y and math.ceil(sgeo.y + pos.y) or dgeo.y)       ,
        width  =            math.ceil(dgeo.width    )                  ,
        height =            math.ceil(dgeo.height   )                  ,
    }
    remove_border(d, args, ngeo)
    geometry_common(d, args, ngeo)

    attach(d, placement[args.position], args)

    return fix_new_geometry(ngeo, args, true)
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
-- @staticfct awful.placement.stretch
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
    local bw   = (not args.ignore_border_width) and d.border_width or 0

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

    return fix_new_geometry(ngeo, args, true)
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
-- @staticfct awful.placement.maximize
function placement.maximize(d, args)
    args = add_context(args, "maximize")
    d    = d or capi.client.focus

    if not d then return end

    local sgeo = get_parent_geometry(d, args)
    local ngeo = geometry_common(d, args, nil, true)
    local bw   = (not args.ignore_border_width) and d.border_width or 0

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

    return fix_new_geometry(ngeo, args, true)
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

--- Scale the drawable by either a relative or absolute percent.
--
-- Valid args:
--
-- **to_percent** : A number between 0 and 1. It represent a percent related to
--  the parent geometry.
-- **by_percent** : A number between 0 and 1. It represent a percent related to
--  the current size.
-- **direction**: Nothing or "left", "right", "up", "down".
--
-- @tparam[opt=client.focus] drawable d A drawable (like `client` or `wibox`)
-- @tparam[opt={}] table args The arguments
-- @treturn table The new geometry
-- @staticfct awful.placement.scale
function placement.scale(d, args)
    args = add_context(args, "scale_to_percent")
    d    = d or capi.client.focus

    local to_percent = args.to_percent
    local by_percent = args.by_percent

    local percent = to_percent or by_percent

    local direction = args.direction

    local sgeo = get_parent_geometry(d, args)
    local ngeo = geometry_common(d, args, nil)

    local old_area = {width = ngeo.width, height = ngeo.height}

    if (not direction) or direction == "left" or direction == "right" then
        ngeo.width = (to_percent and sgeo or ngeo).width*percent

        if direction == "left" then
            ngeo.x = ngeo.x - (ngeo.width - old_area.width)
        end
    end

    if (not direction) or direction == "up" or direction == "down" then
        ngeo.height = (to_percent and sgeo or ngeo).height*percent

        if direction == "up" then
            ngeo.y = ngeo.y - (ngeo.height - old_area.height)
        end
    end

    remove_border(d, args, ngeo)
    geometry_common(d, args, ngeo)

    attach(d, placement.maximize, args)

    return fix_new_geometry(ngeo, args, true)
end

--- Move a drawable to a relative position next to another one.
--
-- This placement function offers two additional settings to align the drawable
-- alongside the parent geometry. The first one, the position, sets the side
-- relative to the parent. The second one, the anchor, set the alignment within
-- the side selected by the `preferred_positions`. Both settings are tables of
-- priorities. The first available slot will be used. If there isn't enough
-- space, then it will fallback to the next until it is possible to fit the
-- drawable. This is meant to avoid going offscreen.
--
-- The `args.preferred_positions` look like this:
--
--    {"top", "right", "left", "bottom"}
--
-- The `args.preferred_anchors` are:
--
-- * "front": The closest to the origin (0,0)
-- * "middle": Centered aligned with the parent
-- * "back": The opposite side compared to `front`
--
-- In that case, if there is room on the top of the geometry, then it will have
-- priority, followed by all the others, in order.
--
--@DOC_awful_placement_next_to_EXAMPLE@
--
-- The `args.mode` parameters allows to control from which `next_to` takes its
-- source object from. The valid values are:
--
-- * geometry: Next to this geometry, `args.geometry` has to be set.
-- * cursor: Next to the mouse.
-- * cursor_inside
-- * geometry_inside
--
-- @tparam drawable d A wibox or client
-- @tparam table args
-- @tparam string args.mode The mode
-- @tparam string|table args.preferred_positions The preferred positions (in order)
-- @tparam string|table args.preferred_anchors The preferred anchor(s) (in order)
-- @tparam string args.geometry A geometry inside the other drawable
-- @treturn table The new geometry
-- @treturn string The chosen position ("left", "right", "top" or "bottom")
-- @treturn string The chosen anchor ("front", "middle" or "back")
-- @staticfct awful.placement.next_to
function placement.next_to(d, args)
    args = add_context(args, "next_to")
    d    = d or capi.client.focus

    local osize = type(d.geometry) == "function"  and d:geometry() or d.geometry
    local original_pos, original_anchors = args.preferred_positions, args.preferred_anchors

    if type(original_pos) == "string" then
        original_pos = {original_pos}
    end

    if type(original_anchors) == "string" then
        original_anchors = {original_anchors}
    end

    local preferred_positions = {}
    local preferred_anchors = #(original_anchors or {}) > 0 and
        original_anchors or {"front", "back", "middle"}

    for k, v in ipairs(original_pos or {}) do
        preferred_positions[v] = k
    end

    local dgeo = geometry_common(d, args)
    local pref_idx, pref_name = 99, nil
    local mode,wgeo = args.mode

    if args.geometry then
        mode = "geometry"
        wgeo = args.geometry
    else
        local pos = capi.mouse.current_widget_geometry

        if pos then
            wgeo, mode = pos, "cursor"
        elseif capi.mouse.current_client then
            wgeo, mode = capi.mouse.current_client:geometry(), "cursor"
        end
    end

    if not wgeo then return end

    -- See get_relative_regions comments
    local is_absolute = wgeo.ontop ~= nil

    local regions = get_relative_regions(wgeo, mode, is_absolute)

    -- Order the regions with the preferred_positions, then the defaults
    local sorted_regions, default_positions = {}, {"left", "right", "bottom", "top"}

    for _, pos in ipairs(original_pos or {}) do
        for idx, def in ipairs(default_positions) do
            if def == pos then
                table.remove(default_positions, idx)
                break
            end
        end

        table.insert(sorted_regions, {name = pos, region = regions[pos]})
    end

    for _, pos in ipairs(default_positions) do
        table.insert(sorted_regions, {name = pos, region = regions[pos]})
    end

    -- Check each possible slot around the drawable (8 total), see what fits
    -- and order them by preferred_positions
    local does_fit = {}
    for _, pos in ipairs(sorted_regions) do
        local geo, dir, fit

        -- Try each anchor until one that fits is found
        for _, anchor in ipairs(preferred_anchors) do
            geo, dir = outer_positions[pos.name.."_"..anchor](pos.region, dgeo.width, dgeo.height)

            geo.width, geo.height = dgeo.width, dgeo.height

            fit = fit_in_bounding(pos.region, geo, args)

            if fit then break end
        end

        does_fit[pos.name] = fit and {geo, dir} or nil

        -- preferred_positions is optional
        local better_pos_idx = preferred_positions[pos.name]
            and preferred_positions[pos.name] < pref_idx or false

        if fit and (better_pos_idx or not pref_name) then
            pref_idx  = preferred_positions[pos.name]
            pref_name = pos.name
        end

        -- No need to continue
        if fit then break end
    end

    if not pref_name then return end

    assert(does_fit[pref_name])

    local ngeo, dir = unpack(does_fit[pref_name])

    -- The requested placement isn't possible due to the lack of space, better
    -- do nothing an try random things
    if not ngeo then return end

    remove_border(d, args, ngeo)

    geometry_common(d, args, ngeo)

    attach(d, placement.next_to, args)

    local ret = fix_new_geometry(ngeo, args, true)

    -- Make sure the geometry didn't change, it would indicate an
    -- "off by border" issue.
    assert((not osize.width) or ret.width == d.width)
    assert((not osize.height) or ret.height == d.height)

    return ret, pref_name, dir
end

--- Restore the geometry.
-- @tparam[opt=client.focus] drawable d A drawable (like `client` or `wibox`)
-- @tparam[opt={}] table args The arguments
-- @treturn boolean If the geometry was restored
-- @staticfct awful.placement.restore
function placement.restore(d, args)
    if not args or not args.context then return false end
    d = d or capi.client.focus

    if not data[d] then return false end

    local memento = data[d][args.context]

    if not memento then return false end

    local x, y = memento.x, memento.y

    -- Some people consider that once moved to another screen, then
    -- the memento needs to be upgraded. For now this is only true for
    -- maximization until someone complains.
    if memento.sgeo and memento.screen and memento.screen.valid
      and args.context == "maximize" and d.screen
      and get_screen(memento.screen) ~= get_screen(d.screen) then
        -- Use the absolute geometry as the memento also does
        local sgeo = get_screen(d.screen).geometry

        x = sgeo.x + (memento.x - memento.sgeo.x)
        y = sgeo.y + (memento.y - memento.sgeo.y)

    end

    d._border_width = memento.border_width

    -- Don't use the memento as it would be "destructive", since `x`, `y`
    -- and `screen` have to be modified.
    d:geometry {
        x      = x,
        y      = y,
        width  = memento.width,
        height = memento.height,
    }

    return true
end

--- Skip all preceding results of placement pipeline for fullscreen clients.
--@DOC_awful_placement_skip_fullscreen_EXAMPLE@
-- @tparam drawable d A drawable (like `client`, `mouse` or `wibox`)
-- @tparam[opt={}] table args Other arguments
-- @treturn table The new geometry
-- @staticfct awful.placement.skip_fullscreen
function placement.skip_fullscreen(d, args)
    args = add_context(args, "skip_fullscreen")
    d = d or capi.client.focus

    if d.fullscreen then
        return {get_screen(d.screen).geometry, {}, true}
    else
        local ngeo = geometry_common(d, args)
        remove_border(d, args, ngeo)
        geometry_common(d, args, ngeo)
        return fix_new_geometry(ngeo, args, true)
    end
end

return placement

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
