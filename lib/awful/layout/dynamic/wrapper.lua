---------------------------------------------------------------------------
--- A compatibility wrapper around clients to conform to the widget API
--
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2016 Emmanuel Lepage Vallee
-- @release @AWESOME_VERSION@
-- @module awful.layout.dynamic.wrapper
---------------------------------------------------------------------------
local object      = require( "gears.object"                     )
local resize      = require( "awful.layout.dynamic.resize"      )
local base_layout = require( "awful.layout.dynamic.base_layout" )
local stack_l     = require( "awful.layout.dynamic.tabbed"      )

local module = {}

-- Callback called when a split point is activated
local function split(wrapper, context, direction)
    if not context.client_widget or not context.source_root then return end
    local t = (direction == "left" or direction == "right")

    local l = t and base_layout.horizontal() or base_layout.vertical()

    local f = context.source_root._private.remove_widgets or context.source_root.remove_widgets
    f(context.source_root, context.client_widget, true)

    t = (direction == "left" or direction == "top")
    l:add(t and context.client_widget or wrapper)
    l:add(t and wrapper or context.client_widget)

    context.source_root:replace_widget(wrapper, l, true)

    context.source_root:raise_widget(context.client_widget)

    context.source_root:emit_signal("widget::redraw_needed")
end

-- Callback called when a stack point is activated
local function stack(wrapper, context)

    local f = context.source_root._remove_widgets or context.source_root.remove_widgets
    f(context.source_root, context.client_widget, true)

    local l = stack_l()

    l:add(context.client_widget)
    l:add(wrapper              )

    context.source_root:replace_widget(wrapper, l, true)

    context.source_root:emit_signal("widget::redraw_needed")

    l:raise_widget(context.client_widget)
end

--- Allow the wrapper to be splited in each of the 4 directions
-- @param self A wrapper
-- @param[opt] geometry A modified geometry preferred by the handler
--
-- This function can be overloaded to add new points
--
-- @return A table of potential split points
function module.splitting_points(self, geometry)
    geometry = geometry or (self._client and self._client:geometry())

    -- Add a group of split point for the UX handler
    local ret = {
        position      = "centered",
        bounding_rect = geometry,
        type          = "internal",
        client        = self._client,
        points        = {}
    }

    for _, v in ipairs {"left", "right", "top", "bottom", false} do
        table.insert(ret.points, {
            direction = v or "stack",
            callback  = function(_, ct) (v and split or stack)(self, ct, v) end
        })
    end

    return ret
end

-- Equivalent of wibox.widget.draw, simply move and resize the client
local function draw(self, _, cr, width, height)
    local c = self._client

    -- There is a few race condition where the client is invalidated during a
    -- pending redraw. This will cause a visual glitch, but the layouts should
    -- be repainted in the next iteration.
    if not c.valid then return end

    -- This will be true for maximized and fullscreen clients too
    if c.floating then return end

    local matrix = cr:get_matrix()
    local gap    = (not self._handler._tag and 0 or self._handler._tag.gap)/2

    -- If a mirror or rotate container was used, the size might be negative.
    local x2, y2, w2, h2 = matrix:transform_rectangle( 0, 0, width, height )

    -- Remove the border and gap from the final size
    w2 = w2 - 2*gap - 2*c.border_width
    h2 = h2 - 2*gap - 2*c.border_width

    c:geometry {
        x      = x2 + gap,
        y      = y2 + gap,
        width  = w2      ,
        height = h2      ,
    }
end

--- Callback when a new geometry is requested
local function on_geometry(wrapper, c, reasons, hints)
    assert(wrapper._is_connected)
    local handler = wrapper._handler
    if handler.active  and reasons == "mouse.resize" then
        local gap = c.screen.selected_tag.gap/2

        hints = setmetatable({
            width  = (hints.width  or c.width ) + 2*c.border_width + 2*gap,
            height = (hints.height or c.height) + 2*c.border_width + 2*gap,
            x      = (hints.x      or c.x     ) - gap,
            y      = (hints.y      or c.y     ) - gap,
            gap    = gap,
        }, {__index = hints or {}})

        resize(handler, c, wrapper, hints)
    end
end

--- Callback when the client is raised
local function on_raise(wrapper, _)
    assert(wrapper._is_connected)
    if wrapper._handler.active and wrapper._handler.widget.raise then
        wrapper._handler.widget:raise_widget(wrapper, true)
    end
end

--- Callback when two client indices are swapped
local function on_swap(wrapper, self,other_c,is_source)
    assert(wrapper._is_connected)
    if wrapper._handler.active and is_source then
        if wrapper._handler then
            wrapper._handler:swap_widgets(self, other_c, true)
            wrapper._handler.widget:emit_signal("widget::redraw_needed")
        end
    end
end

--- Avoid flickering by disconnecting signals when the wrapper is not in use
local function suspend(wrapper)
    if not wrapper._is_connected then return end

    wrapper._client:disconnect_signal("request::geometry" , wrapper.on_geometry )
    wrapper._client:disconnect_signal("swapped"           , wrapper.on_swap     )
    wrapper._client:disconnect_signal("raised"            , wrapper.on_raise    )
    wrapper._is_connected = false
end

--- Re-connect the signals when the wrapper is activated
local function wake_up(wrapper)
    if wrapper._is_connected then return end

    wrapper._client:connect_signal("request::geometry" , wrapper.on_geometry )
    wrapper._client:connect_signal("swapped"           , wrapper.on_swap     )
    wrapper._client:connect_signal("raised"            , wrapper.on_raise    )
    wrapper._is_connected = true
end

-- Create the wrapper
-- @param c_w A client or a wibox
local function wrap_client(c_w)
    local wrapper = object()
    wrapper.index = function() return nil, wrapper end

    rawset(wrapper, "_client"          , c_w                      )
    rawset(wrapper, "draw"             , draw                     )
    rawset(wrapper, "suspend"          , suspend                  )
    rawset(wrapper, "wake_up"          , wake_up                  )
    rawset(wrapper, "splitting_points" , module.splitting_points  )
    rawset(wrapper, "is_widget"        , true                     )
    rawset(wrapper, "get_opacity"      , function() return 1  end )
    rawset(wrapper, "get_children"     , function() return {} end )
    rawset(wrapper, "_private"         , {
        visible       = true,
        widget_caches = {}
    })

    -- Always return the minimum client size (1x1 or size_hints) as 0x0 is
    -- treated as invisible and anything larger would be random.
    --FIXME query the size_hints
    rawset(wrapper, "fit", function() return 1, 1 end)

    -- Hints as to where to place the new wrapper
    wrapper.slave  = c_w.slave
    wrapper.master = c_w.master

    wrapper.on_geometry = function(...) on_geometry(wrapper, ... ) end
    wrapper.on_swap     = function(...) on_swap    (wrapper, ... ) end
    wrapper.on_raise    = function(...) on_raise   (wrapper, ... ) end

    wake_up(wrapper)

    wrapper:connect_signal("widget::layout_changed", function()
        wrapper._private.widget_caches = {}
    end)

    return wrapper
end

return setmetatable(module, {__call = function(_,...) return wrap_client(...) end})
-- kate: space-indent on; indent-width 4; replace-tabs on;
