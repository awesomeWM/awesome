---------------------------------------------------------------------------
--- A specialised of the conditional container.
--
-- It adds:
--
-- * Automatic reload when the client count changes
--
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2018 Emmanuel Lepage Vallee
-- @module awful.layout.dynamic.base_conditional
---------------------------------------------------------------------------
local conditional = require("wibox.container.conditional")
local gtable      = require("gears.table")
local aclient     = require("awful.client")
local capi = {
    client = client,
    tag    = tag,
}

local module = {}

-- Ignore calls before `arrange` is called. Nothing good will come out of them.
function module:eval()
    if not self._private.is_woken_up then return end
    if not self._private.screen then return end

    return self._private.eval(self)
end

-- `:eval()` is called from an async calls.
-- This means `:layout()` will always get called first and thus we can use it
-- to cache the screen without adding weight elsewhere in the stack.
-- It is not very clean, but the alternatives are much worst.
function module:layout(context, width, height)
    local old_screen = self._private.screen
    self._private.screen = context.screen

    if old_screen and old_screen ~= context.screen then
        self._private.eval(self)
    end

    return self._private.layout(self, context, width, height)
end

-- Do not call eval when the layout isn't visible
function module:wake_up()
    self._private.is_woken_up = true
    self:eval()
    self:emit_signal("widget::layout_changed")
    for _, v in ipairs(self.children) do
        if v.suspend then
            v:suspend()
        end
    end
end

function module:suspend()
    if not self._private.is_woken_up then return end

    self._private.is_woken_up = false

    for _, v in ipairs(self.children) do
        if v.suspend then
            v:suspend()
        end
    end
end

local function template_changed_digger(w, ret)
    for _, c in ipairs(w:get_children()) do
        if c._client then
            table.insert(ret, c)
        else
            template_changed_digger(c, ret)
        end
    end
end

-- Not very efficient, but unless the hierarchy ever learns to know that a
-- widget containing a client was removed, this is better than nothing
local function template_changed(self, _, _, widget, old)
    -- min_elements and max_elements mean something different, their work is
    -- done, they need to be removed to avoid being misinterpreted by the
    -- `manual` layout.
    if widget then
        widget.max_elements = nil
        widget.min_elements = nil
    end

    if old then
        local children = {}
        template_changed_digger(old, children)

        -- Notify the manual layout that those widgets need a new home
        self:emit_signal_recursive("reparent::widgets", children)
    end

end

local function arg_callback(self)
    assert(self._private.screen)
    local t = self._private.screen.selected_tag

    -- In case there is multiple selected tags
    local count = #aclient.tiled(self._private.screen)

    return capi.client.focus, t, count
end

-- function module:set_minimum_clients(number)
--     self._private.minimum_clients = number
--     self:eval()
-- end

-- The problem is that most manual layout use containers and containers lack
-- the widget managment methods of layouts. To hack around this, look upward
-- until a layout is found.
function module:remove_widgets(w, ...)
    if not self._private.widget then return false end

    local other = {...}
    if other[#other] ~= true then return false end

    -- Digg until a proper widget is found
    local container = self._private.widget
    while container and not container.remove_widgets do
        container = container.widget
    end

    local ret = container:remove_widgets(w, ...)

    self:eval()

    return ret
end

local function create(...)
    local ret = conditional(...)

    ret._private.eval   = ret.eval
    ret._private.layout = ret.layout

    ret:set_argument_callback(arg_callback)

    ret:set_default_when(function(w, _, _, count)
        local min, max = w.min_elements or 0, w.max_elements or math.huge
        return count >= min and count <= max
    end)

    ret:connect_signal("property::selected_template", template_changed)

    ret:set_connected_global_signals {
--         {capi.client , "focus"               }, --TODO add a track focus property
        {capi.tag    , "property::selected"  },
        {capi.tag    , "property::activated" },
    }

    gtable.crush(ret, module, true)

    return ret
end

return create


-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
