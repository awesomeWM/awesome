---------------------------------------------------------------------------
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2016 Emmanuel Lepage Vallee
-- @module awful.layout

--- Replace the stateless corner layout.
--
-- This layout has a master client and a row on the side and bottom of the
-- master client. They are resized so the two "slave" column and row are
-- aligned.
--
--@DOC_awful_layout_dynamic_suit_corner_corner_EXAMPLE@
--
-- **Client count scaling**:
--
-- The first row is the `corner` layout and the second one `corner.horizontal`
--
--@DOC_awful_layout_dynamic_suit_corner_scaling_EXAMPLE@
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
--@DOC_awful_layout_dynamic_suit_corner_gap_EXAMPLE@
-- See `awful.tag.setgap`
-- See `awful.tag.getgap`
-- See `awful.tag.incgap`
--
-- **screen padding effect**:
--
--@DOC_awful_layout_dynamic_suit_corner_padding_EXAMPLE@
-- See `awful.screen.padding`
-- @clientlayout awful.layout.dynamic.suit.corner


local dynamic     = require("awful.layout.dynamic.base")
local wibox       = require("wibox")
local base_layout = require( "awful.layout.dynamic.base_layout" )
local util        = require( "awful.util")

local corner = {}

-- A lua iterator closure
function corner.columns(self)
    local i = 0
    return function () i = i + 1 return self:get_children_by_id("column")[i] end
end

-- Boilerplate
local function main_sections(self)
    return {
        master_wdg  = self:get_children_by_id("column")[1].top_section,
        maincol_wdg = self:get_children_by_id("column")[2].top_section,
        corner_wdg  = self:get_children_by_id("column")[2].bottom_section,
        bottom_wdg  = self:get_children_by_id("column")[1].bottom_section
    }
end

-- Support 'n' column and 'm' number of master per column
function corner.add(self, widget)
    if not widget then return end

    local sections = main_sections(self)

    if #sections.master_wdg.children == 0 then
        sections.master_wdg:add(widget)
    elseif #sections.bottom_wdg.children == 0 then
        sections.bottom_wdg:add(widget)
    elseif #sections.maincol_wdg.children == 0 then
        sections.maincol_wdg:add(widget)
    elseif #sections.corner_wdg.children == 0 then
        sections.corner_wdg:add(widget)
    elseif #sections.maincol_wdg.children == 2
      and #sections.bottom_wdg.children < 2 then
        sections.bottom_wdg:add(widget)
    else
        sections.maincol_wdg:add(widget)
    end
end

--FIXME use the history and swap with the candidates
function corner.remove_widgets(self, widget, ...)
    if not widget then return end

    local ret = wibox.layout.fixed.remove_widgets(self, widget, ...)
    local sections = main_sections(self)

    -- Rebalance
    for _, sec in pairs(sections) do
        if #sec.children == 0 then
            local candidate = nil

            if #sections.bottom_wdg.children >= 2 then
                candidate = sections.bottom_wdg.children[1]
                sections.bottom_wdg:remove(1)
            elseif #sections.maincol_wdg.children >= 2 then
                candidate = sections.maincol_wdg.children[1]
                sections.maincol_wdg:remove(1)
            elseif #sections.corner_wdg.children >= 1 then
                candidate = sections.corner_wdg.children[1]
                sections.corner_wdg:remove(1)
            elseif #sections.maincol_wdg.children >= 1 then
                candidate = sections.maincol_wdg.children[1]
                sections.maincol_wdg:remove(1)
            elseif #sections.bottom_wdg.children >= 1 then
                candidate = sections.bottom_wdg.children[1]
                sections.bottom_wdg:remove(1)
            end

            if candidate then
                sec:add(candidate)
            end
        end
    end

    return ret
end

-- Synchronize all bottom columns
local function locked_adjust(self, index, before, itself, after)
    for col in self._private.corner_parent:columns() do
        wibox.layout.ratio.ajust_ratio(col, index, before, itself, after)
    end
end

-- Make sure all columns have the new resize
local function register_columns(self)
    util.table.crush(self, corner, true)

    for col in self:columns() do
        rawset(col, "ajust_ratio", locked_adjust)
        col._private.corner_parent = self
    end
end

local function column(strategy)
    local ret = {
        { -- top
            id     = "top_section",
            inner_fill_strategy = strategy,
            layout = base_layout.vertical
        },
        { --bottom
            id     = "bottom_section",
            inner_fill_strategy = strategy,
            layout = base_layout.horizontal
        },
        id     = "column",
        inner_fill_strategy = strategy,
        layout = base_layout.vertical --col1
    }

    return ret
end

local function ctr(t, mirror_v, mirror_h) --luacheck: no unused_args
    t.master_width_factor = 0.66

    local strategy = t.master_fill_policy == "master_width_factor" and
        "center" or "justify"

    local main_layout = wibox.widget {
        column(strategy),
        column(strategy),
        inner_fill_strategy = strategy,
        layout = base_layout.horizontal
    }

    register_columns(main_layout)

    main_layout:ajust_ratio(1, 0, t.master_width_factor, 1-t.master_width_factor)
    locked_adjust(
        main_layout:get_children_by_id("column")[1], 1, 0,
        t.master_width_factor, 1-t.master_width_factor
    )

    -- Implement directions
    if mirror_v or mirror_h then
        local l = wibox.container.mirror(main_layout, {
            horizontal = mirror_h,
            vertical   = mirror_v,
        })
        l.add            = function(_, ...) return main_layout:add(...) end
        l.remove_widgets = function(_, ...) return main_layout:remove_widgets(...) end
        return l
    end

    return main_layout
end

local module = dynamic("corner"  , function(t) return ctr(t, false, false) end)
module.nw    = dynamic("cornernw", function(t) return ctr(t, false, false) end)
module.sw    = dynamic("cornersw", function(t) return ctr(t, true , false) end)
module.ne    = dynamic("cornerne", function(t) return ctr(t, false, true ) end)
module.se    = dynamic("cornerse", function(t) return ctr(t, true , true ) end)

return module
-- kate: space-indent on; indent-width 4; replace-tabs on;
