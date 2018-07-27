---------------------------------------------------------------------------
--- Allow dynamic layouts to be created using wibox.layout composition
--
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2016 Emmanuel Lepage Vallee
-- @release @AWESOME_VERSION@
-- @module awful.layout.dynamic.base
---------------------------------------------------------------------------
local capi       = { client=client, tag=tag, screen=screen }
local matrix     = require( "gears.matrix"                 )
local screen     = require( "awful.screen"                 )
local hierarchy  = require( "wibox.hierarchy"              )
local aw_layout  = require( "awful.layout"                 )
local l_wrapper  = require( "awful.layout.dynamic.wrapper" )
local xresources = require( "beautiful.xresources"         )
local unpack     = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)

local internal = {}

local contexts = setmetatable({}, {__mode = "k"})

-- Create and return the screens context
local function get_context(s)
    s = capi.screen[s or 1]

    -- The screen is important because some layouts like `treesome` or anything
    -- using conditional elements need to generate contextual data from the
    -- screen state.
    contexts[s] = contexts[s] or {
        screen = s,
        dpi    = xresources.get_dpi(s)
    }

    return contexts[s]
end

-- Check if a widget should be added in the layout
-- Note that maximized/fullscreen/minimized clients should, as they do in the
-- stateless layout system. Adding and removing them only
-- cause information loss (where the client was placed, was it
-- tabbed and so on)
local function check_tiled(c)
    return (not c.floating)
        or c.maximized
        or c.maximized_horizontal
        or c.maximized_vertical
        or c.fullscreen
        or c.minimized
end

-- Add a wrapper to the handler and layout
local function insert_wrapper(handler, c, wrapper)
    local pos = #handler.wrappers+1
    handler.wrappers         [ pos ] = wrapper
    handler.client_to_wrapper[ c   ] = wrapper
    handler.client_to_index  [ c   ] = pos

    handler.widget:add(wrapper)
end

-- Traverse the hierarchy and notify every compatible widget of the state
-- change. This is necessary because some widget might connect to global
-- signals and must know when to connect or disconnect from them. If they
-- stay connected while the layout isn't in use, they will get corrupted.
local function wake_up_hierarchy(_hierarchy, counter)
    local widget = _hierarchy:get_widget()

    if widget.wake_up then
        widget:wake_up()
    end

--     assert(#_hierarchy:get_children() == #widget:get_children()) --TODO for debug, remove it

    for _, child in ipairs(_hierarchy:get_children()) do
        wake_up_hierarchy(child, counter+1)
    end
end

local function suspend_hierarchy(_hierarchy)
    local widget = _hierarchy:get_widget()

    if widget.wake_up then
        widget:suspend()
    end

    for _, child in ipairs(_hierarchy:get_children()) do
        suspend_hierarchy(child)
    end
end

-- Traverse the hierarchy and remove every empty parent.
local function hierarchy_remove_fallback(h, widget)
    -- The problem is that most manual layout use containers and containers lack
    -- the widget managment methods of layouts. To hack around this, look downward
    -- until a layout is found.
    while h do
        local w = h:get_widget()
        if w.remove_widgets then
            return w:remove_widgets(widget, true)
        elseif w.widget then
            h = h:get_children()[1]
        end
    end
end

-- Remove a wrapper from the handler and layout
local function remove_wrapper(handler, c, wrapper)
    local pos = handler.client_to_index[c]
    table.remove(handler.wrappers, pos)
    handler.client_to_index  [c] = nil
    handler.client_to_wrapper[c] = nil

    wrapper:suspend()

    -- If the widget defines a removal function, assume it works
    if (not handler.widget.remove_widgets) or
      not handler.widget:remove_widgets(wrapper, true) then
        hierarchy_remove_fallback(handler.hierarchy, wrapper)
    end

    -- Note that this point client_to_index is corrupted, it's fixed by the
    -- caller
end

--- Get the list of client that were added and removed
local function get_client_differential(self)
    local added, removed = {}, {}

    -- Get all clients visible on the tag screen, this include clients
    -- from other selected tags
    local clients, reverse = self._tag.screen.clients, {}

    for _,c in ipairs(clients) do
        if check_tiled(c) and not self.client_to_wrapper[c] then
            table.insert(added, c)
        end
        reverse[c] = true
    end

    for c,_ in pairs(self.client_to_wrapper) do
        -- Arrange is also called when clients are killed. This function must
        -- **never** assert as this will mess the whole tag state and Awesome
        -- will need to be reloaded.
        local is_valid = pcall(function() return c.valid end) and c.valid
        if (not is_valid) or (not reverse[c]) then
            table.insert(removed, c)
        end
    end

    return added, removed
end

-- When a tag is selected or the layout change for this one, activate the handler
local function wake_up(self)
    if (not self) or not self.is_dynamic then return end

    -- It is possible to misuse a layout so this happen. For example, trying to
    -- compute the layout of an invisible tag for a "pager" widget. Those widgets
    -- should use the "minimap" extension.
    if (not self._tag) or (not self._tag.selected) then return end

    -- Call wake_up on the top level independently from the tree to ensure
    -- top level containers work
    if self.widget.wake_up then
        self.widget:wake_up()
    end

    local added, removed = get_client_differential(self)

    -- Remove the old client first as they may be already invalid (in case of a
    -- resurected layout object)
    for _, c in ipairs(removed) do
        local wrapper = self.client_to_wrapper[c]
        remove_wrapper(self, c, wrapper)
    end

    -- Fix the index mapping
    for i = 1, #self.wrappers do
        local c = self.wrappers[i]._client

        -- Note that c `might` be invalid as this point if multiple clients
        -- are removed simultaneously
        assert(c)

        self.client_to_index[c] = i
    end

    for _, c in ipairs(added) do
        if not c.floating then
            if self.client_to_wrapper[c] then
                self.widget:add(self.client_to_wrapper[c])
            else
                local wrapper = l_wrapper(c)
                wrapper._handler = self

                insert_wrapper(self, c, wrapper)
            end
        end
    end

    -- Traverse the hierarchy and wake up every other component
    wake_up_hierarchy(self.hierarchy,1)

    self.active = true
end

-- When a tag is hidden or the layout isn't the handler, stop all processing
local function suspend(self)
    if not self.is_dynamic then return end

    -- Traverse the hierarchy and wake up every other component
    suspend_hierarchy(self.hierarchy)

    self.active = false
end

-- Emulate the main "layout" method of a hierarchy
local function main_layout(_, handler)

    if not handler.param then
        handler.param = aw_layout.parameters(handler._tag)
        handler.param.is_init = true
    end

    local workarea = handler.param.workarea

    handler.hierarchy:update(
        get_context(handler._tag.screen),
        handler.widget,
        workarea.width,
        workarea.height
    )

end

local context_index_miss = setmetatable({
    operator = 0,
    save = function(self)
        table.insert(self._memento, self._matrix)
    end,
    restore = function(self)
        local pop = self._memento[#self._memento]
        if pop then
            table.remove(self._memento, #self._memento)
            self._matrix = pop
        end
    end,
    get_matrix = function(self) return self._matrix end,
    transform = function(self, cmatrix)
        self._matrix = self._matrix:multiply(matrix.from_cairo_matrix(cmatrix))
        return self._matrix
    end,
    _memento = {},
    _matrix = matrix.identity*matrix.identity
},
{__index = function(self2, key)
    if matrix[key] then
        self2[key] = function(self,...)
            self._matrix = self._matrix[key](self._matrix,...)
        end
        return self2[key]
    end
end})

local function handle_hierarchy(context, cr, _hierarchy, wa)
    local widget = _hierarchy:get_widget()

    cr:transform(_hierarchy:get_matrix_to_parent():to_cairo_matrix())
    cr._matrix.x0, cr._matrix.y0 = _hierarchy:get_matrix_to_device():transform_point(0, 0)
    cr._matrix.x0, cr._matrix.y0 = cr._matrix.x0 + wa.x, cr._matrix.y0 + wa.y

    local w, h = _hierarchy:get_size()

    if widget.before_draw_children then
        widget:before_draw_children(context, cr, w, h)
    end

    if widget._client then
        widget:draw(context, cr, w, h)
    end

    for i, child in ipairs(_hierarchy:get_children()) do
        if widget.before_draw_child then
            assert(type(cr) == "table")
            widget:before_draw_child(context, i, child, cr, w, h)
        end
        handle_hierarchy(context, cr, child, wa)
        if widget.after_draw_child then
            widget:after_draw_child(context, i, child, cr, w, h)
        end
    end

    if widget.after_draw_children then
        widget:after_draw_children(context, cr, w, h)
    end
end

-- Place all the clients correctly
local function redraw(_, handler)
    if handler.active then

        -- Move to the work area
        local wa = handler.param.workarea

        -- Use a matrix to emulate a Cairo context. Only the transform methods
        -- are used anyway. Anything else make no sense and deserve to crash
        local m = setmetatable({}, {
           __index = context_index_miss,
        })

        handle_hierarchy(get_context(handler._tag.screen), m, handler.hierarchy, wa)
    end
end

-- Shared implementation by the iterators below.
local function hierarchy_iterator_common(self, lambda, wrapper)
    local handler = self._client_layout_handler
    local results = {}

    local function hh(_hierarchy, parent)
        local widget = _hierarchy:get_widget()
        local x, y = _hierarchy:get_matrix_to_device():transform_point(0, 0)
        local w, h = _hierarchy:get_size()

        if lambda and lambda(widget) then
            table.insert(results, {
                widget,
                {x=x,y=y,width=w,height=h},
                parent
            })
        elseif wrapper and widget._client then
            table.insert(results, {
                widget,
                widget._client,
                {x=x,y=y,width=w,height=h},
                parent
            })
        end

        for _, child in ipairs(_hierarchy:get_children()) do
            hh(child, widget)
        end
    end

    hh(handler.hierarchy, nil)

    local i = 0
    return function ()
        i = i + 1
        if not results[i] then return end -- Because of the multiple returns
        return unpack(results[i])
    end
end

-- Lua iterator for all client wrappers.
-- Note that the geometry is relative to the layout. That geometry should be
-- used and **not** `c:geometry()`. The client could still be in another tag
-- (and thus have a different geometry).
-- @treturn table w The wrapper widget.
-- @treturn client c The client.
-- @treturn table geo The geometry.
-- @treturn number geo.x The horizontal position *within the layout*.
-- @treturn number geo.y The vertical position *within the layout*.
-- @treturn number geo.width The width.
-- @treturn number geo.height The height.
-- @treturn table parent The parent widget.
local function iterate_by_client(self)
    return hierarchy_iterator_common(self, nil, true)
end

-- Lua iterator each layout elements using a "reduce" function.
-- Note that the geometry is relative to the layout.
-- Note that this function doesn't use the cached id table as it expects it
-- to be outdated by the time this function is called.
-- @tparam function lambda The "reduce" function. Has to return a boolean.
-- @treturn table w The wrapper widget.
-- @treturn table geo The geometry.
-- @treturn number geo.x The horizontal position *within the layout*.
-- @treturn number geo.y The vertical position *within the layout*.
-- @treturn number geo.width The width.
-- @treturn number geo.height The height.
-- @treturn table parent The parent widget.
local function iterate_by_lambda(self, id)
    if not id then return end
    return hierarchy_iterator_common(self, id, false)
end

-- Convert client into emulated widget
function internal.create_layout(t, l)

    local handler = {
        wrappers          = {},
        client_to_wrapper = {},
        client_to_index   = {},
        layout            = main_layout,
        widget            = l,
        swap_widgets      = internal.swap_widgets,
        active            = true,
        _tag              = t,
    }

    local context = get_context(t.screen)

    handler.hierarchy = hierarchy.new(
        context     ,
        l           ,
        0           ,
        0           ,
        redraw      ,
        main_layout ,
        handler
    )

    l._client_layout_handler = handler
    l.by_lambda              = iterate_by_lambda
    l.by_client              = iterate_by_client

    t:connect_signal("property::layout", function(t2)
        if t2.screen.selected_tag == t2 then
            if t2.layout ~= handler then
                suspend(handler)
            else
                wake_up(handler)
            end
        end
    end)

    --TODO once it no longer depends on `arrange`, this become necessary for
    -- the `conditional` elements
    --t:connect_signal("property::screen", function(t2)
    --    handler.hierarchy:_redraw()
    --end)

    function handler.arrange(param)
        handler.param = param

        -- The wrapper handle useless gap, remove it from the workarea
        local gap = not handler._tag and 0 or handler._tag.gap

        local screen_geo   = screen.object.get_bounding_geometry(nil, {
            honor_workarea = handler.honor_workarea,
            honor_padding  = handler.honor_padding,
            tag            = handler._tag,
            margins        = handler.honor_gap and gap/2 or 0,
        })

        handler.param.workarea = screen_geo

        -- Make sure to update the hierarchy size when necessary
        local w,h = handler.hierarchy:get_size()
        if w ~= screen_geo.width or h ~= screen_geo.height then
            main_layout(nil, handler)
        end

        wake_up(handler)

        handler.hierarchy:_redraw()

    end

    local function size_change()
        main_layout(handler.widget, handler)
    end

    t:connect_signal("property::geometry", size_change)

    -- Widgets, like empty columns, can request to be "garbage collected"
    l:connect_signal("request::removal", function(_, w)
        handler.widget:remove_widgets(w, true)
    end)

    return handler
end

-- This is inspired by wibox.fixed.layout, TODO port to the hierarchy
local function swap_widgets_fallback(self, widget1, widget2)
    local idx1, l1 = self:index(widget1, true)
    local idx2, l2 = self:index(widget2, true)

    if idx1 and l1 and idx2 and l2 and (l1.set or l1.set_widget) and (l2.set or l2.set_widget) then
        if l1.set then
            l1:set(idx1, widget2)
            if l1 == self then
                self:emit_signal("widget::swapped", widget1, widget2, idx2, idx1)
            end
        elseif l1.set_widget then
            l1:set_widget(widget2)
        end
        if l2.set then
            l2:set(idx2, widget1)
            if l2 == self then
                self:emit_signal("widget::swapped", widget1, widget2, idx2, idx1)
            end
        elseif l2.set_widget then
            l2:set_widget(widget1)
        end

        return true
    end

    return false
end

-- Swap the client of 2 wrappers
function internal.swap_widgets(handler, client1, client2)
    -- Handle case where the screens are different
    local handler1 = client1.screen.selected_tag.layout
    local handler2 = client2.screen.selected_tag.layout

    assert(handler2)
    assert(handler1)
    assert(handler1.client_to_wrapper)
    assert(handler2.client_to_wrapper)

    local w1 = handler1.client_to_wrapper[client1]
    local w2 = handler2.client_to_wrapper[client2]

    assert(w1 and w2)

    -- Try various ways of swapping the widgets
    if handler.widget.swap_widgets then
        handler.widget:swap_widgets(w1, w2, true)
    else
        swap_widgets_fallback(handler.widget, w1, w2)
    end

    w1._handler = handler2
    w2._handler = handler1
end

-- Suspend tags invisible tags
capi.tag.connect_signal("property::selected", function(t)
    if (not t.selected) or (not t.screen.selected_tag) then
        suspend(t.layout)
    end
end)

--- Register a new type of dynamic layout
-- Any other arguments will be passed to `bl`.
-- @tparam string name An unique name, duplicates are forbidden
-- @tparam function bl The layout constructor function. When called,
--   the first paramater is the tag.
-- @return A layout constructor metafunction.
local function register(name, bl, ...)
    local generator = {name = name}
    local args = {...}

    setmetatable(generator, {__call = function(_, t )
        -- If the tag isn't set, then create a normal widget. This is the case
        -- when the client layout is used a a "sub layout" by a "meta layout"
        -- such as the `maunal` layout.
        if not t then
            return bl()
        end

        local l =  bl(t , unpack(args))
        local l_obj          = internal.create_layout(t, l)
        l_obj._type          = generator
        l_obj.name           = name
        l_obj.is_dynamic     = true
        l_obj.arrange        = l_obj.arrange or function() end
        l_obj.honor_padding  = generator.honor_padding == nil
            and true or generator.honor_padding
        l_obj.honor_workarea = generator.honor_workarea == nil
            and true or generator.honor_workarea
        l_obj.honor_gap = generator.honor_gap == nil
            and true or generator.honor_gap

        --TODO implement :reset() here

        return l_obj
    end})

    -- The arrange method is necessary to pass the checks, the instance one is
    -- used. This one should never be called (but it wont hurt if it is)
    generator.arrange = function() end

    return generator
end

return setmetatable({}, { __call = function(_, ...) return register(...) end })
-- kate: space-indent on; indent-width 4; replace-tabs on;
