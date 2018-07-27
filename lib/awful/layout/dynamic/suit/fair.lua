---------------------------------------------------------------------------
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2016 Emmanuel Lepage Vallee
-- @module awful.layout

--- Replace the stateless fair.
--
-- This is not a perfect clone, as the stateful property of this layout
-- allow to minimize the number of clients being moved. If a splot of left
-- empty, then it will be used next time a client is added rather than
-- "pop" a client from the next column/row and move everything. This is
-- intended, if you really wish to see the old behavior, a new layout will
-- be created.
--
-- This version also support resizing, the older one did not--
--
--@DOC_awful_layout_dynamic_suit_fair_fair_EXAMPLE@
--
-- **Client count scaling**:
--
-- The first row is the `fair` layout and the second one `fair.horizontal`
--
--@DOC_awful_layout_dynamic_suit_fair_scaling_EXAMPLE@
--
-- **master_count effect**:
--
-- Unused
--
-- **column_count effect**:
--
-- Unused
--
-- **master_width_factor effect**:
--
-- Unused
--
-- **gap effect**:
--
-- The "useless" gap tag property will change the spacing between clients.
--@DOC_awful_layout_dynamic_suit_fair_gap_EXAMPLE@
-- See `awful.tag.setgap`
-- See `awful.tag.getgap`
-- See `awful.tag.incgap`
--
-- **screen padding effect**:
--
--@DOC_awful_layout_dynamic_suit_fair_padding_EXAMPLE@
-- See `awful.screen.padding`
-- @clientlayout awful.layout.suit.dynamic.fair

local dynamic     = require("awful.layout.dynamic.base")
local base_layout = require( "awful.layout.dynamic.base_layout" )

local function get_bounds(self)
    local lowest_idx, highest_count, lowest_count, col_count = -1, 0, 9999, 0

    for i, col in ipairs(self:get_children()) do
        if col.is_fair_col then
            local count = #col:get_children()

            if count < lowest_count then
                lowest_idx, lowest_count = i, count
            end

            lowest_count  = count < lowest_count  and count or lowest_count
            highest_count = count > highest_count and count or highest_count

            col_count = col_count + 1
        end
    end

    return lowest_count, highest_count, lowest_idx, col_count
end

local function add(self, widget)
    if not widget then return end

    local lowest_count, highest_count, lowest_idx, ncols = get_bounds(self)

    if ncols > 0 and (highest_count == 0 or (lowest_count == highest_count and highest_count <= ncols)) then
        -- Add to the first existing row
        self:get_children()[lowest_count]:add(widget)
    elseif lowest_count == highest_count or ncols == 0 then
        -- Add a row
        local l = self._col_layout()
        l.volatile, l.is_fair_col = true, true
        self._private.add(self, l)
        l:add(widget)
    elseif lowest_idx ~= -1 then
        -- Add to the row with the least clients
        self:get_children()[lowest_idx]:add(widget)
    else
        -- There is an internal corruption
        assert(false)
    end
end

local function ctr(_, direction)
    local main_layout = base_layout[
        (direction == "left" or direction == "right")
            and "horizontal" or "vertical"
    ]()

    main_layout._col_layout = base_layout[
        (direction == "left" or direction == "right")
            and "vertical" or "horizontal"
    ]

    main_layout._private.add = main_layout.add
    main_layout.add = add

    return main_layout
end

local module = dynamic("fair", function(t) return ctr(t, "right") end)

module.horizontal = dynamic("fairh", function(t) return ctr(t, "top"  ) end)

--- A fair layout prioritizing horizontal space.
-- @clientlayout awful.layout.suit.dynamic.fair.horizontal
-- @see awful.layout.suit.dynamic.fair

return module
-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
