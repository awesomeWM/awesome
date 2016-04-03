---------------------------------------------------------------------------
--- A specialised stack layout with dynamic client layout features
--
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2016 Emmanuel Lepage Vallee
-- @release @AWESOME_VERSION@
-- @module awful.layout.dynamic.base_stack
---------------------------------------------------------------------------

local wibox = require( "wibox"      )
local util  = require( "awful.util" )

--- As all widgets are on the top of each other, it is necessary to add the
-- groups elsewhere
local function splitting_points(self, geometry)
    local pts = {}

    local top_level_widgets = self:get_children()

    for _, w in ipairs(top_level_widgets) do
        if w._client and w.splitting_points then
            local pt = w:splitting_points({x=1,y=1,width=1,height=1})
            for _, v2 in ipairs(pt) do
                if v2.type  == "internal" then
                    table.insert(pts, v2)
                end
            end
        end
    end

    -- There is nothing stacked, let the default point do its job
    if #pts < 2 then return end

    local _, dx = geometry.y+geometry.height*0.75, geometry.width / (#pts+1)
    local x = dx

    for _, v in ipairs(pts) do
        v.position = "centered"
        v.bounding_rect = geometry
        v.label = v.client and v.client.name or "N/A"

        -- Raise both widgets
        local cb = v.callback
        v.callback = function(w, context)
            cb(w, context)
            if v.widget then
                self:raise_widget(context.client_widget)
                self:raise_widget(v.widget)
            end
        end

        x = x + dx
    end

    return pts
end

--- When a tag is not visible, it must stop trying to mess with the content
local function wake_up(self)
    if self.is_woken_up then return end

    self.is_woken_up = true

    for _, v in ipairs(self.children) do
        if v.wake_up then
            v:wake_up()
        end
    end
end

local function suspend(self)
    if not self.is_woken_up then return end

    self.is_woken_up = false

    for _, v in ipairs(self.children) do
        if v.suspend then
            v:suspend()
        end
    end
end

-- Not necessary for "dump" max layout, but if the user add some splitter
-- then the whole sub-layout have to be raised too
local function raise_widget(self, widget)

    local all_widgets, old_l = self:get_children(true), nil

    -- If widget is not a top level of self, then get all of its siblings
    local _, l = self:index(widget, true)
    while l ~= self do
        old_l = l
        _, l = self:index(l, true)
    end

    local siblings = util.table.join({widget},(old_l and old_l.get_children and old_l:get_children(true)) or {})

    local by_c = {}

    for _, w in ipairs(all_widgets) do
        if w._client then
            by_c[w._client] = w
        end
    end

    for _, w in ipairs(siblings) do
        if w._client then
            by_c[w._client] = nil
        end
    end

    self._private.raise_widget(self, widget, true)

    for c in pairs(by_c) do
        if c.valid then
            c:lower()
        end
    end
end

local function add(self, w1, ...)
    self._private.add (self, w1, ...)
    self:raise_widget(w1)
end

local function ctr()
    -- Decide the right layout
    local main_layout = wibox.layout.stack()

    -- It need to be false of some clients wont be moved
    main_layout:set_top_only(false)
    main_layout._private.raise_widget  = main_layout.raise_widget
    main_layout._private.add    = main_layout.add

    -- Notify extensions that any elements further down is overlapping
    main_layout.is_stack = true

    rawset(main_layout, "suspend"         , suspend         )
    rawset(main_layout, "wake_up"         , wake_up         )
    rawset(main_layout, "raise_widget"    , raise_widget    )
    rawset(main_layout, "add"             , add             )
    rawset(main_layout, "splitting_points", splitting_points)

    -- Forward all changes.
    for _, s in ipairs {
        "widget::swapped",
        "widget::inserted",
        "widget::replaced",
        "widget::removed",
        "widget::added",
        "widget::reseted",
    } do
        main_layout:connect_signal(s, function(_, ...)
            main_layout:emit_signal_recursive(s.."_forward", ...)
        end)
    end

    return main_layout
end

return setmetatable({}, {__call = function(_, ...) return ctr(...) end})
