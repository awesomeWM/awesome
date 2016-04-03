---------------------------------------------------------------------------
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2016 Emmanuel Lepage Vallee
-- @module awful.layout
---------------------------------------------------------------------------

--- A layout with columns and rows.
--
-- **Variants** :
--
-- The `tile` layout is available in 4 variants: `tile`, `tile.left`, `tile.top`
-- and `tile.bottom`. The name indicate the position of the "slave" columns.
--@DOC_awful_layout_dynamic_suit_tile_directions_EXAMPLE@
--
-- **Client count scaling**:
--
-- In the image below, the first row represent the `tile` layout, the second the
-- `tile.left` folowed by `tile.top` and `tile.bottom`. The columns indicate how
-- the layout change when new clients are added.
--@DOC_awful_layout_dynamic_suit_tile_scaling_EXAMPLE@
--
-- **master_count effect**:
--
-- The master_count property will affect number of rows in the master columns.
--@DOC_awful_layout_dynamic_suit_tile_nmaster_EXAMPLE@
-- See `tag.master_count`
-- See `awful.tag.incnmaster`
--
-- **column_count effect**:
--
-- When columns are added, this layout will maximize the space available to each
-- clients in the slaves columns.
--@DOC_awful_layout_dynamic_suit_tile_ncol_EXAMPLE@
-- See `tag.column_count`
-- See `awful.tag.incncol`
--
-- **master_width_factor effect**:
--
-- The master width factor is the ratio between the "master" column and the
-- "slave" ones.
--@DOC_awful_layout_dynamic_suit_tile_mwfact_EXAMPLE@
-- See `tag.master_width_factor`
-- See `awful.tag.incmwfact`
--
-- **gap effect**:
--
-- The "useless" gap tag property will change the spacing between clients.
--@DOC_awful_layout_dynamic_suit_tile_gap_EXAMPLE@
-- See `tag.gap`
-- See `awful.tag.incgap`
--
-- **screen padding effect**:
--
--@DOC_awful_layout_dynamic_suit_tile_padding_EXAMPLE@
-- See `awful.screen.padding`
--
-- **Other properties**:
--
-- This layout also check the client `master` and `slave` properties. If none
-- is set, then `master` is used and the new clients will replace the existing
-- master one.
-- @clientlayout awful.layout.suit.dynamic.tile

local dynamic = require( "awful.layout.dynamic.base" )
local util    = require( "awful.util"                )
local base_layout = require( "awful.layout.dynamic.base_layout" )

-- Add some columns
local function init_col(self)

    -- This can change during initialization, but before the tag is selected
    self._private.mwfact  =          self._private.tag.master_width_factor
    self._private.ncol    = math.max(self._private.tag.column_count, 1)
    self._private.nmaster = math.max(self._private.tag.master_count, 1)

    if #self._private.cols_priority == 0 then

        -- Create the master column
        self:add_column()

        -- Apply the default width factor to the master
        self:set_widget_ratio(self._private.cols_priority[1], self._private.mwfact)
    end
end

--- When a tag is not visible, it must stop trying to mess with the content
local function wake_up(self)
    if self.is_woken_up then return end

    -- If the number of column changed while inactive, this layout doesn't care.
    -- It has its own state, so it only act on the delta while visible
    self._private.ncol    = self._private.tag.column_count
    self._private.mwfact  = self._private.tag.master_width_factor
    self._private.nmaster = self._private.tag.master_count

    -- Connect the signals
    self._private.tag:connect_signal("property::master_width_factor", self._private.conn_mwfact )
    self._private.tag:connect_signal("property::column_count"       , self._private.conn_ncol   )
    self._private.tag:connect_signal("property::master_count"       , self._private.conn_nmaster)

    self.is_woken_up = true

    if self._private.wake_up then
        self._private.wake_up(self)
    end
end

local function suspend(self)
    if not self.is_woken_up then
        return
    end

    -- Disconnect the signals
    self._private.tag:disconnect_signal("property::master_width_factor", self._private.conn_mwfact )
    self._private.tag:disconnect_signal("property::column_count"       , self._private.conn_ncol   )
    self._private.tag:disconnect_signal("property::master_count"       , self._private.conn_nmaster)

    self.is_woken_up = false

    if self._private.suspend then
        self._private.suspend(self)
    end
end

-- Clean empty columns
local function remove_empty_columns(self)
    for k, v in ipairs(self._private.cols_priority) do
        if k ~= 1 and #v:get_children() == 0 then
            self:remove_column()
        end
    end
end

--- When the number of column change, re-balance the elements
local function balance(self, additional_widgets)
    local elems = {}

    -- Get only the top level, this will preserve tabs and manual split points
    for _, v in ipairs(self._private.cols_priority) do
        util.table.merge(elems, v:get_children())
        v:reset()
    end

    util.table.merge(elems, additional_widgets or {})

    for i = #elems, 1 , -1 do
        self:add(elems[i])
    end

    remove_empty_columns(self)
end

local function get_least_used_col(self)
    local candidate_i, candidate_c = 0, 999

    -- Get the column with the least members, this is a break from the old
    -- behavior, but I think it make sense, as it optimize space
    for i = 2, math.max(#self._private.cols_priority, self._private.ncol) do
        local col = self._private.cols_priority[i]

        -- Create new columns on demand, nothing can have less members than a new col
        if not col then
            self:add_column()
            candidate_i = #self._private.cols_priority
            break
        end

        local count = #col:get_children()
        if count <= candidate_c then
            candidate_c = count
            candidate_i = i
        end
    end

    return candidate_i, candidate_c
end

--- Support 'n' column and 'm' number of master per column
local function add(self, widget)
    if not widget then return end

    -- By default, there is no column, so nowhere to add the widget, fix this
    init_col(self)

    -- Make sure there is enough masters
    if #self._private.cols_priority[1]:get_children() < self._private.nmaster then
        self._private.cols_priority[1]:add(widget)
        return
    elseif #self._private.cols_priority == 1 then
        self:add_column()
    end

    -- For legacy reason, the new clients are added as primary master
    -- Technically, I should foreach all masters and push them, but
    -- until someone complain, lets do something a little bit simpler
    local to_pop = self._private.cols_priority[1]:get_children()[1]
    if (not widget.slave) and to_pop then
        self:replace_widget(to_pop, widget, true)
        return self:insert(2 , to_pop)
    end

    local candidate_i = get_least_used_col(self)

    self._private.cols_priority[candidate_i]:add(widget)
end

local function insert(self, index, widget) --luacheck: no unused args
    -- TODO handle the master column

    -- Pop one widget per column until the least used column has been pushed
    local candidate_i = get_least_used_col(self)

    --local current_idx = #self._private.cols_priority[1]:get_children()
    --TODO use index it its set

    for i = 2, candidate_i do
        local col = self._private.cols_priority[i]
        if col.insert then
            col:insert(1, widget)

            -- Pop from this column and push_front in the next
            if i < candidate_i then
                local children = col:get_children()
                widget = children[#children]
                col:remove(#children)
            end
        end
    end
end

--- Remove a widget
local function remove_widgets(self, widget)
    local _, parent, path = self:index(widget, true)

    -- Avoid and infinite loop
    local f = parent == self and parent._private.remove_widgets or parent.remove_widgets
    local ret = f(parent, widget)

    -- A master is required
    if #self._private.cols_priority[1]:get_children() == 0 then
        self:balance()
    elseif #path < 3 then
        -- Indicate a column might be empty
        remove_empty_columns(self)
    end

    return ret
end

--- Add a new column
local function add_column(main_layout, idx)
    idx = idx or #main_layout._private.cols + 1
    local l = main_layout._private.col_layout()
    table.insert(main_layout._private.cols, idx, l)
    main_layout._private.add(main_layout, l)

    if main_layout._private.primary_col > 0 then
        table.insert(main_layout._private.cols_priority,l)
    else
        table.insert(main_layout._private.cols_priority, 1, l)
    end

    if #main_layout._private.cols_priority == 2 then
        -- Apply the default width factor to the master
        main_layout:set_widget_ratio(main_layout._private.cols_priority[1], main_layout._private.mwfact)
    end

    return l
end

-- Remove a column
local function remove_column(main_layout, idx)
    idx = idx or #main_layout._private.cols
    local wdg = main_layout._private.cols[idx] --TODO this is the only use of _cols[], remove it

    if main_layout._private.primary_col > 0 then
        table.remove(main_layout._private.cols_priority, idx)
    else
        table.remove(main_layout._private.cols_priority, 1) --TODO wrong
    end

    main_layout._private.cols[idx] = nil

    main_layout:remove_widgets(wdg, true)

    return wdg:get_children()
end

-- React when the number of column changes
local function col_count_changed(self, t)
    local diff = t.column_count - self._private.ncol

    if diff > 0 then
        for _ = 1, diff do
            self:add_column()
        end

        self:balance()
    elseif diff < 0 then
        local orphans = {}

        for _ = 1, -diff do
            util.table.merge(orphans, self:remove_column())
        end

        self:balance(orphans)
    end

    self._private.ncol = t.column_count
end

-- Widget factor changed
local function wfact_changed(self, t)
    local diff = (t.master_width_factor - self._private.mwfact) * 2

    self:inc_widget_ratio(self._private.cols_priority[1], diff)

    self._private.mwfact = t.master_width_factor

    self:emit_signal("widget::layout_changed")
end

-- The number of master clients changed
local function nmaster_changed(self, t)
    self._private.nmaster = t.master_count

    self:balance()
end

local function ctr(t, direction)
    -- Decide the right layout
    local main_layout = base_layout[
        (direction == "left" or direction == "right")
            and "horizontal" or "vertical"
    ]()

    main_layout._private.col_layout = base_layout[
        (direction == "left" or direction == "right")
            and "vertical" or "horizontal"
    ]

    -- Declare the signal handlers
    main_layout._private.conn_mwfact  = function(t2) wfact_changed    (main_layout, t2) end
    main_layout._private.conn_ncol    = function(t2) col_count_changed(main_layout, t2) end
    main_layout._private.conn_nmaster = function(t2) nmaster_changed  (main_layout, t2) end

    -- Cache
    main_layout._private.cols           = {}
    main_layout._private.cols_priority  = {}
    main_layout._private.primary_col    = (direction == "right" or direction == "bottom") and 1 or -1
    main_layout._private.add            = main_layout.add
    main_layout._private.remove_widgets = main_layout.remove_widgets
    main_layout._private.tag            = t
    main_layout._private.wake_up        = main_layout.wake_up
    main_layout._private.suspend        = main_layout.suspend

    -- Methods
    rawset(main_layout, "add"            , add            )
    rawset(main_layout, "remove_widgets" , remove_widgets )
    rawset(main_layout, "insert"         , insert         )
    rawset(main_layout, "balance"        , balance        )
    rawset(main_layout, "wake_up"        , wake_up        )
    rawset(main_layout, "suspend"        , suspend        )
    rawset(main_layout, "remove_column"  , remove_column  )
    rawset(main_layout, "add_column"     , add_column     )

    return main_layout
end

-- FIXME IDEA, I could also use the rotation widget

local module = dynamic("tile", function(t) return ctr(t, "right") end)

module.left   = dynamic("tileleft",   function(t) return ctr(t, "left"  ) end)
module.top    = dynamic("tiletop",    function(t) return ctr(t, "top"   ) end)
module.bottom = dynamic("tilebottom", function(t) return ctr(t, "bottom") end)

--- A tile layout with the slave clients on the left.
-- @clientlayout awful.layout.suit.dynamic.tile.left
-- @see awful.layout.suit.dynamic.tile

--- A tile layout with the slave clients on the top.
-- @clientlayout awful.layout.suit.dynamic.tile.top
-- @see awful.layout.suit.dynamic.tile

--- A tile layout with the slave clients on the bottom.
-- @clientlayout awful.layout.suit.dynamic.tile.bottom
-- @see awful.layout.suit.dynamic.tile

return module
