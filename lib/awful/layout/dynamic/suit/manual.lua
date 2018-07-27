---------------------------------------------------------------------------
--- Declare custom layouts.
--
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2017 Emmanuel Lepage Vallee
-- @module awful.layout
---------------------------------------------------------------------------

local dynamic = require( "awful.layout.dynamic.base" )
local gdebug  = require( "gears.debug"               )
local wibox   = require( "wibox"                     )
local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)

--- The (soft) limit for the number of children.
--
-- This is an hint used by the automatic placement fallback algorithm in case
-- the layout doesn't provide its own.
--
-- It also allows rather complex layout to be defined without an imperative
-- implementation.
--
-- If no `priority` is set, the layout with the highest number of free spot
-- will be chosen.
--
-- @property max_elements
-- @tparam[opt=0] number|function The maximum number of elements contained by
--  the layout or a function returning the value.

--- The priority of the layout compared to its peers.
--
-- A "full" layout is composed of many smaller layouts, containers and widgets.
-- When time comes to choose where to add a new client, the "full" layout can
-- either implement its own logic tree or fallback to the default on.
--
-- In that case, the `priority` index will be taken into account. The higher
-- the number is, the higher priority it has. The highest priority layout
-- will "win" as long as the number of elements is below `max_elements`.
--
-- @property priority
-- @tparam[opt=0] number|function The priority or a function returning the
--  priority.

--- Define how to distribute the empty space.
--
-- This property is only available on the tiled sections.
--
-- @see wibox.layout.ratio.inner_fill_strategy
-- @property inner_fill_strategy

--- Manually declare a layout for the clients.
--
-- The following properties are honored in the blocks:
--
-- * *name*: The layout name
-- * *priority*: The block priority when adding new clients
-- * *reflow*: When a client is removed, should it take one away from this
--   and add it to the one with an higher priority.
-- * *max_elements*: The maximium number of client for a block
-- * *ratio*: The ratio for the block FIXME
--
-- **Simple tiling layout**:
--
--@DOC_awful_layout_dynamic_suit_manual_scaling1_EXAMPLE@
--
-- **A column and a tabbed stack**:
--
--@DOC_awful_layout_dynamic_suit_manual_scaling2_EXAMPLE@
--
-- **Using master width factor and alignment**:
--
--@DOC_awful_layout_dynamic_suit_manual_scaling3_EXAMPLE@
--
-- **Adding margins:**:
--
--@DOC_awful_layout_dynamic_suit_manual_scaling_margin_EXAMPLE@
--
-- **Apply a reflection:**:
--
--@DOC_awful_layout_dynamic_suit_manual_mirror_EXAMPLE@
--
-- **Use different layouts depending on the number of clients:**:
--
-- A basic use case with 2 type of layouts depending only on the number of
-- client.
--
--@DOC_awful_layout_dynamic_suit_manual_conditional_EXAMPLE@
--
-- A more useful example using "real" layout suites:
--
--@DOC_awful_layout_dynamic_suit_manual_conditional2_EXAMPLE@
--
-- @clientlayout awful.layout.suit.dynamic.manual

--- When an element is removed, take one away from a lower priority layout.
--
-- @property reflow
-- @param boolean

--TODO insert_at_end (boolean)

-- Find the lowest priority client wrapper and remove it from the layout.
local function extract_wrapper(self, below_priority)
    local loser_priority, loser = below_priority

    for w, _, _, parent in self:by_client() do
        if parent and parent.priority and parent.priority < loser_priority then
            loser_priority, loser = parent.priority, w
        end
    end

    if not loser then return end

    if self._private._remove_widgets then
        self._private._remove_widgets(self, loser, true)
    end

    return loser
end

local function max_elements(w, self)
    -- Avoid using templates by accident
    if not self.is_widget then
        return nil
    end

    return type(w.max_elements) == "function"
        and w.max_elements(self) or w.max_elements
end

local function priority(w, self)
    return type(w.priority) == "function"
        and w.priority(self) or w.priority
end

local function select_target(self)
    local p_best, r_best, p_best_w, r_best_w = -math.huge, 0

    for _, w in ipairs(self:get_all_children()) do
        if w.add then
            local avail_space = (not w.max_elements)
                and math.huge or (max_elements(w, self) - #w:get_children())

            local has_priority = w.priority ~= nil
                and avail_space > 0 and p_best < priority(w, self)

            if has_priority then
                p_best, p_best_w = priority(w, self), w
            elseif avail_space > r_best then
                r_best, r_best_w = avail_space, w
            end
        end
    end

    return p_best_w or r_best_w or self
end

--TODO implement SWAP_WIDGETS

local function ctr(args)
    local function create()
        local main_layout = wibox.widget(args)

        main_layout._private._remove_widgets = main_layout.remove_widgets
        main_layout._private._add = main_layout.add

        function main_layout:add(w, ...)
            if not w then return end

            local t = select_target(self)

            local f = t == self and main_layout._private._add or t.add

            -- This is a pseudo-bug, but at least it wont burst the stack and
            -- hopefully has no consequences.
            if f ~= main_layout.add then
                f(t, w)
            else
                gdebug.print_warning("A manual client layout was created without"..
                    " an `:add()` method. The result is unpredictable. If you"..
                    " think there is a valid use case, please report a bug"
                )
            end

            main_layout:add(...)
        end

        -- Some layouts like the conditional can't know where to move the clients
        -- once the layout changed, so they emit this signal.
        main_layout:connect_signal("reparent::widgets", function(_,widgets)
            main_layout:add(unpack(widgets))
        end)

        -- Check if something has to be done afterward
        function main_layout:remove_widgets(w, ...)
            if not w then return false end
            assert(type(w) ~= "boolean")
            local other = {...}
            local _, parent, _ = self:index(w, true)
            local recurse = other[#other] == true

            -- Call the real function
            if main_layout._private._remove_widgets then
                main_layout._private._remove_widgets(self, w, recurse)
            end

            if parent.reflow then
                local replacement = extract_wrapper(self, parent.priority or 0)
                if replacement then
                    local f = parent == self and self._private._add or parent.add
                    f(parent, replacement)
                end
                --TODO
            end

            -- Do not call remove_widgets with a single boolean
            if #other > 1 or not recurse then
                self:remove_widgets(...)
            end

            return true
        end

        return main_layout
    end

    -- Pick a name for this layout
    local name = type(args.name) == "function" and args.name(args) or args.name or "manual"

    return dynamic(name, create)
end

return ctr
-- kate: space-indent on; indent-width 4; replace-tabs on;
