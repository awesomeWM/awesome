---------------------------------------------------------------------------
--- Split the focused* client area in two.
--
-- The split will be either vertical or horizontal to maximize the space.
--
-- * Or, when there is none, the largest client
--
-- Based on https://github.com/RobSis/treesome
--
-- BUG, over very, very long period of time, this will create tons of empty
-- subdivision layout. They need to be GCed manually
--
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2016 Emmanuel Lepage Vallee
-- @release @AWESOME_VERSION@
-- @module awful.layout.dynamic.suit.treesome
---------------------------------------------------------------------------
local dynamic     = require( "awful.layout.dynamic.base"        )
local wibox       = require( "wibox"                            )
local base_layout = require( "awful.layout.dynamic.base_layout" )
local stack       = require( "awful.layout.dynamic.base_stack"  )

local function raise_widget(self, widget)
    base_layout.raise_widget(self, widget)
    self._private.current = widget
end

-- Select the client to split in half
local function find_current(self, force_c)
    local everything = self:get_all_children()
    local c, biggest, ret = force_c or client.focus, 0

    -- There is already one FIXME unreliable and contains invalid clients
    --[[if self._private.current then
        assert((not self._private.current._client)
            or self._private.current._client.valid
        )
        return self._private.current
    end]]

    -- Find the focused client
    -- FIXME this should also check the history for the last focused client
    if c then
        for _, w in ipairs(everything) do
            if w._client == c then
                assert(w._client.valid)
                return w
            end
        end
    end

    -- Find the biggest client
    for _, w in ipairs(everything) do
        if w._client then
            -- BUG this uses the **current** client size, but it can be already
            -- invalid.
            local geo = w._client:geometry()
            if geo.width*geo.height > biggest then
                assert(w._client.valid)
                biggest, ret = geo.width*geo.height, w
            end
        end
    end

    assert((not ret) or ret._client.valid)
    return ret
end

local function remove_widgets(self, w, ...)
    if not w then return end

    local _, parent, path = self:index(w, true)
    local to_rm, idx = nil, 0

    while idx < #path and w ~= self do
        local wdg = path[idx] or parent

        if #wdg:get_children() > 1 then break end

        to_rm = path[idx]

        if to_rm == self._private.current then
            self._private.current = nil
        end

        idx = idx + 1
    end

    to_rm = to_rm or w

    if to_rm then
        wibox.layout.fixed.remove_widgets(self, to_rm, true)
        if w == self._private.current then
            self._private.current = find_current(self)
        end
    end

    if not self._private.current then
        self._private.current = find_current(self)
    end

    self:remove_widgets(...)
end

local function add(self, w1, ...)
        if not w1 then return end
        local mstack = self:get_children_by_id("main_stack")[1]

        -- Add the first element
        if #mstack:get_children() == 0 then
            mstack:add(w1)
            self:add(...)
            return true
        end

        local target = find_current(self)

        if target then
            local geo = target._client:geometry()
            local l = wibox.layout {
                target,
                w1,
                layout = geo.width > geo.height and
                    base_layout.horizontal or base_layout.vertical
            }
            self:replace_widget(target, l, true)

            if w1._client and w1._client == client.focus then
                self._private.current = w1
            end
        else
            --??? no idea what else to do here
            mstack:add(w1)
        end

        self:add(...)
    end

local function ctr()
    local main = wibox.widget {
        {
            {
                id     = "main_stack",
                layout = stack,
            },
            layout = base_layout.vertical
        },
        layout = base_layout.horizontal
    }

    main.add = add
    main.raise_widget = raise_widget
    main.remove_widgets = remove_widgets

    return main
end

local module                     = dynamic("max"       , ctr)
module.fullscreen                = dynamic("fullscreen", ctr)
module.fullscreen.honor_workarea = false
module.fullscreen.honor_gap      = false

return module
-- kate: space-indent on; indent-width 4; replace-tabs on;
