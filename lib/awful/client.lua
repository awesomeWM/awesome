---------------------------------------------------------------------------
--- Useful client manipulation functions.
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008 Julien Danjou
-- @module client
---------------------------------------------------------------------------

-- Grab environment we need
local gdebug = require("gears.debug")
local spawn = nil --TODO v5 deprecate
local set_shape = require("awful.client.shape").update.all
local object = require("gears.object")
local grect = require("gears.geometry").rectangle
local gmath = require("gears.math")
local gtable = require("gears.table")
local amousec = require("awful.mouse.client")
local pcommon = require("awful.permissions._common")
local pairs = pairs
local type = type
local ipairs = ipairs
local table = table
local math = math
local setmetatable = setmetatable
local capi =
{
    client = client,
    mouse = mouse,
    screen = screen,
    awesome = awesome,
}

local function get_screen(s)
    return s and capi.screen[s]
end

-- We use a metatable to prevent circular dependency loops.
local screen
do
    screen = setmetatable({}, {
        __index = function(_, k)
            screen = require("awful.screen")
            return screen[k]
        end,
        __newindex = error -- Just to be sure in case anything ever does this
    })
end
local client = {object={}}

-- Private data
client.data = {}
client.data.marked = {}
client.data.persistent_properties_registered = {} -- keys are names of persistent properties, value always true

-- Functions
client.urgent = require("awful.client.urgent")
client.swap = {}
client.floating = {}
client.dockable = {}
client.property = {}
client.shape = require("awful.client.shape")
client.focus = require("awful.client.focus")

--- The client default placement on the screen.
--
-- The default config uses:
--
--    awful.placement.no_overlap+awful.placement.no_offscreen
--
-- @DOC_sequences_client_rules_placement_EXAMPLE@
--
-- @clientruleproperty placement
-- @see awful.placement

--- When applying the placement, honor the screen padding.
-- @clientruleproperty honor_padding
-- @tparam[opt=true] boolean honor_padding
-- @see awful.placement

--- When applying the placement, honor the screen work area.
--
-- The workarea is the part of the screen that excludes the bars and docks.
--
-- @clientruleproperty honor_workarea
-- @tparam[opt=true] boolean honor_workarea
-- @see awful.placement

--- The client default tag.
--
-- @DOC_sequences_client_rules_tags_EXAMPLE@
--
-- @clientruleproperty tag
-- @tparam tag tag
-- @see tag
-- @see new_tag
-- @see tags
-- @see switch_to_tags

--- The client default tags.
--
-- Avoid using the tag and tags properties at the same time, it will cause
-- issues.
--
-- @clientruleproperty tags
-- @tparam[opt={tag}] table tags
-- @see tag
-- @see new_tag
-- @see tags
-- @see switch_to_tags

--- Create a new tag for this client.
--
-- If the value is `true`, the new tag will be named after the client `class`.
-- If it is a string, it will be the tag name.
--
-- If a table is used, all of its properties will be passed to the tag
-- constructor:
--
-- @DOC_sequences_client_rules_new_tag_EXAMPLE@
--
-- @tparam[opt=false] table|string|boolean new_tag
-- @clientruleproperty new_tag
-- @see tag
-- @see tags
-- @see switch_to_tags

--- Unselect the current tags and select this client tags.
-- Note that this property was called `switchtotag` in previous Awesome versions.
--
-- @DOC_sequences_client_rules_switch_to_tags_EXAMPLE@
--
-- @clientruleproperty switch_to_tags
-- @tparam[opt=false] boolean switch_to_tags
-- @see tag.selected

--- Define if the client should grab focus by default.
--
-- The `request::activate` context for this call is `rules`.
--
-- @clientruleproperty focus
-- @tparam[opt=false] boolean focus

--- Should this client have a titlebar by default.
-- @clientruleproperty titlebars_enabled
-- @tparam[opt=false] boolean titlebars_enabled
-- @see awful.titlebar

--- A function to call when this client is ready.
--
-- It can be useful to set extra properties or perform actions.
--
-- @clientruleproperty callback
-- @see awful.spawn

--- Jump to the given client.
-- Takes care of focussing the screen, the right tag, etc.
--
-- @deprecated awful.client.jumpto
-- @see client.jump_to
-- @tparam client c the client to jump to
-- @tparam bool|function merge If true then merge tags (select the client's
--   first tag additionally) when the client is not visible.
--   If it is a function, it will be called with the client and its first
--   tag as arguments.
function client.jumpto(c, merge)
    gdebug.deprecate("Use c:jump_to(merge) instead of awful.client.jumpto", {deprecated_in=4})
    client.object.jump_to(c, merge)
end

--- Jump to the given client.
--
-- Takes care of focussing the screen, the right tag, etc.
--
-- @DOC_sequences_client_jump_to1_EXAMPLE@
--
-- @method jump_to
-- @tparam bool|function merge If true then merge tags (select the client's
--   first tag additionally) when the client is not visible.
--   If it is a function, it will be called with the client and its first
--   tag as arguments.
-- @request client activate client.jumpto granted When a client is activated
--  because `c:jump_to()` is called.
-- @see activate
-- @see active
function client.object.jump_to(self, merge)
    local s = get_screen(screen.focused())
    -- focus the screen
    if s ~= get_screen(self.screen) then
        screen.focus(self.screen)
    end

    self.minimized = false

    -- Try to make client visible, this also covers e.g. sticky.
    if not self:isvisible() then
        local t = self.first_tag
        if merge then
            if type(merge) == "function" then
                merge(self, t)
            elseif t then
                t.selected = true
            end
        elseif t then
            t:view_only()
        end
    end

    self:emit_signal("request::activate", "client.jumpto", {raise=true})
end

--- Get visible clients from a screen.
--
-- @deprecated awful.client.visible
-- @see screen.clients
-- @tparam[opt] integer|screen s The screen, or nil for all screens.
-- @tparam[opt=false] boolean stacked Use stacking order? (top to bottom)
-- @treturn table A table with all visible clients.
function client.visible(s, stacked)
    local cls = capi.client.get(s, stacked)
    local vcls = {}
    for _, c in pairs(cls) do
        if c:isvisible() then
            table.insert(vcls, c)
        end
    end
    return vcls
end

--- Get visible and tiled clients
--
-- @deprecated awful.client.tiled
-- @see screen.tiled_clients
-- @tparam integer|screen s The screen, or nil for all screens.
-- @tparam[opt=false] boolean stacked Use stacking order? (top to bottom)
-- @treturn table A table with all visible and tiled clients.
function client.tiled(s, stacked)
    local clients = client.visible(s, stacked)
    local tclients = {}
    -- Remove floating clients
    for _, c in pairs(clients) do
        if not client.object.get_floating(c)
            and not c.fullscreen
            and not c.maximized
            and not c.maximized_vertical
            and not c.maximized_horizontal then
            table.insert(tclients, c)
        end
    end
    return tclients
end

--- Get a client by its relative index to another client.
-- If no client is passed, the focused client will be used.
--
-- @staticfct awful.client.next
-- @tparam int i The index.  Use 1 to get the next, -1 to get the previous.
-- @tparam[opt] client sel The client.
-- @tparam[opt=false] boolean stacked Use stacking order? (top to bottom)
-- @treturn[opt] client|nil A client, or nil if no client is available.
-- @see client.get
--
-- @usage -- focus the next window in the index
-- awful.client.next(1)
-- -- focus the previous
-- awful.client.next(-1)
function client.next(i, sel, stacked)
    -- Get currently focused client
    sel = sel or capi.client.focus
    if sel then
        -- Get all visible clients
        local cls = client.visible(sel.screen, stacked)
        local fcls = {}
        -- Remove all non-normal clients
        for _, c in ipairs(cls) do
            if client.focus.filter(c) or c == sel then
                table.insert(fcls, c)
            end
        end
        cls = fcls
        -- Loop upon each client
        for idx, c in ipairs(cls) do
            if c == sel then
                -- Cycle
                return cls[gmath.cycle(#cls, idx + i)]
            end
        end
    end
end

--- Swap a client with another client in the given direction.
--
-- This will not cross the screen boundary. If you want this behavior, use
-- `awful.client.swap.global_bydirection`.
--
-- @DOC_sequences_client_swap_bydirection1_EXAMPLE@
--
-- @staticfct awful.client.swap.bydirection
-- @tparam string dir The direction, can be either "up", "down", "left" or "right".
-- @tparam[opt=focused] client c The client.
-- @tparam[opt=false] boolean stacked Use stacking order? (top to bottom)
-- @see swap
-- @see swapped
-- @see awful.client.swap.global_bydirection
-- @see awful.client.swap.byidx
-- @see awful.client.cycle
function client.swap.bydirection(dir, c, stacked)
    local sel = c or capi.client.focus
    if sel then
        local cltbl = client.visible(sel.screen, stacked)
        local geomtbl = {}
        for i,cl in ipairs(cltbl) do
            geomtbl[i] = cl:geometry()
        end
        local target = grect.get_in_direction(dir, geomtbl, sel:geometry())

        -- If we found a client to swap with, then go for it
        if target then
            cltbl[target]:swap(sel)
        end
    end
end

--- Swap a client with another client in the given direction.
--
-- Swaps across screens.
--
-- @DOC_sequences_client_swap_bydirection2_EXAMPLE@
--
-- @staticfct awful.client.swap.global_bydirection
-- @tparam string dir The direction, can be either "up", "down", "left" or "right".
-- @tparam[opt] client sel The client.
-- @request client activate client.swap.global_bydirection granted When a client
--  could be activated because `awful.client.swap.global_bydirection` was called.
-- @see swap
-- @see swapped
-- @see awful.client.swap.bydirection
-- @see awful.client.swap.byidx
-- @see awful.client.cycle
function client.swap.global_bydirection(dir, sel)
    sel = sel or capi.client.focus
    local scr = get_screen(sel and sel.screen or screen.focused())

    if sel then
        -- move focus
        client.focus.global_bydirection(dir, sel)
        local c = capi.client.focus

        -- swapping inside a screen
        if get_screen(sel.screen) == get_screen(c.screen) and sel ~= c then
            c:swap(sel)

        -- swapping to an empty screen
        elseif sel == c then
            sel:move_to_screen(screen.focused())

        -- swapping to a nonempty screen
        elseif get_screen(sel.screen) ~= get_screen(c.screen) and sel ~= c then
            sel:move_to_screen(c.screen)
            c:move_to_screen(scr)
        end

        screen.focus(sel.screen)
        sel:emit_signal("request::activate", "client.swap.global_bydirection",
                        {raise=false})
    end
end

--- Swap a client by its relative index.
--
-- @DOC_sequences_client_swap_byidx1_EXAMPLE@
--
-- @staticfct awful.client.swap.byidx
-- @tparam integer i The index.
-- @tparam[opt] client c The client, otherwise focused one is used.
-- @see swap
-- @see swapped
-- @see awful.client.swap.bydirection
-- @see awful.client.swap.global_bydirection
-- @see awful.client.cycle
function client.swap.byidx(i, c)
    local sel = c or capi.client.focus
    local target = client.next(i, sel)
    if target then
        target:swap(sel)
    end
end

--- Cycle through the clients to change the focus.
--
-- This will swap the client from one position to the next
-- in the layout.
--
-- @DOC_sequences_client_cycle1_EXAMPLE@
--
-- @staticfct awful.client.cycle
-- @tparam boolean clockwise True to cycle clients clockwise.
-- @tparam[opt] screen s The screen where to cycle clients.
-- @tparam[opt=false] boolean stacked Use stacking order? (top to bottom)
-- @see swap
-- @see swapped
-- @see awful.client.swap.bydirection
-- @see awful.client.swap.global_bydirection
-- @see awful.client.swap.byidx
function client.cycle(clockwise, s, stacked)
    s = s or screen.focused()
    local cls = client.visible(s, stacked)
    -- We can't rotate without at least 2 clients, buddy.
    if #cls >= 2 then
        local c = table.remove(cls, 1)
        if clockwise then
            for i = #cls, 1, -1 do
                c:swap(cls[i])
            end
        else
            for _, rc in pairs(cls) do
                c:swap(rc)
            end
        end
    end
end

--- Append a keybinding.
--
-- @method append_keybinding
-- @tparam awful.key key The key.
-- @see remove_keybinding
-- @see append_mousebinding
-- @see remove_mousebinding

--- Remove a keybinding.
--
-- @method remove_keybinding
-- @tparam awful.key key The key.

--- Append a mousebinding.
--
-- @method append_mousebinding
-- @tparam awful.button button The button.

--- Remove a mousebinding.
--
-- @method remove_mousebinding
-- @tparam awful.button button The button.

--- Get the master window.
--
-- @deprecated awful.client.getmaster
-- @tparam[opt=awful.screen.focused()] screen s The screen.
-- @treturn client The master client.
function client.getmaster(s)
    s = s or screen.focused()
    return client.visible(s)[1]
end

--- Set the client as master: put it at the beginning of other windows.
--
-- @deprecated awful.client.setmaster
-- @tparam client c The window to set as master.
function client.setmaster(c)
    c:to_primary_section()
end

--- Set the client as slave: put it at the end of other windows.
-- @deprecated awful.client.setslave
-- @tparam client c The window to set as slave.
function client.setslave(c)
    c:to_secondary_section()
end

--- Move the client to the most significant layout position.
--
-- This only affects tiled clients. It will shift all other
-- client to fill the gap caused to by the move.
--
-- @DOC_sequences_client_to_primary_EXAMPLE@
--
-- @method to_primary_section
-- @see swap
-- @see to_secondary_section
function client.object:to_primary_section()
    local cls = gtable.reverse(capi.client.get(self.screen))

    for _, v in pairs(cls) do
        self:swap(v)
    end
end

--- Move the client to the least significant layout position.
--
-- This only affects tiled clients. It will shift all other
-- client to fill the gap caused to by the move.
--
-- @DOC_sequences_client_to_secondary_EXAMPLE@
--
-- @method to_secondary_section
-- @see swap
-- @see to_primary_section

function client.object:to_secondary_section()
    local cls = capi.client.get(self.screen)

    for _, v in pairs(cls) do
        self:swap(v)
    end
end

--- Move/resize a client relative to current coordinates.
-- @deprecated awful.client.moveresize
-- @tparam integer x The relative x coordinate.
-- @tparam integer y The relative y coordinate.
-- @tparam integer w The relative width.
-- @tparam integer h The relative height.
-- @tparam[opt] client c The client, otherwise focused one is used.
-- @see client.relative_move
function client.moveresize(x, y, w, h, c)
    gdebug.deprecate("Use c:relative_move(x, y, w, h) instead of awful.client.moveresize", {deprecated_in=4})
    client.object.relative_move(c or capi.client.focus, x, y, w, h)
end

--- Move/resize a client relative to current coordinates.
--
-- @DOC_sequences_client_relative_move1_EXAMPLE@
--
-- @method relative_move
-- @see geometry
-- @tparam[opt=c.x] number x The relative x coordinate.
-- @tparam[opt=c.y] number y The relative y coordinate.
-- @tparam[opt=c.width] number w The relative width.
-- @tparam[opt=c.height] number h The relative height.
function client.object.relative_move(self, x, y, w, h)
    local geometry = self:geometry()
    geometry['x'] = geometry['x'] + (x or geometry.x)
    geometry['y'] = geometry['y'] + (y or geometry.y)
    geometry['width'] = geometry['width'] + (w or geometry.width)
    geometry['height'] = geometry['height'] + (h or geometry.height)
    self:geometry(geometry)
end

--- Move a client to a tag.
-- @deprecated awful.client.movetotag
-- @tparam tag target The tag to move the client to.
-- @tparam[opt] client c The client to move, otherwise the focused one is used.
-- @see client.move_to_tag
function client.movetotag(target, c)
    gdebug.deprecate("Use c:move_to_tag(target) instead of awful.client.movetotag", {deprecated_in=4})
    client.object.move_to_tag(c or capi.client.focus, target)
end

--- Move a client to a tag.
--
-- @DOC_sequences_client_move_to_tag1_EXAMPLE@
--
-- @method move_to_tag
-- @tparam tag target The tag to move the client to.
-- @request client activate client.movetotag granted When a client could be
--  activated because `c:move_to_tag()` was called.
-- @see tags
function client.object.move_to_tag(self, target)
    local s = target.screen
    if self and s then
        if self == capi.client.focus then
            self:emit_signal("request::activate", "client.movetotag", {raise=true})
        end
        -- Set client on the same screen as the tag.
        self.screen = s
        self:tags({ target })
    end
end

--- Toggle a tag on a client.
--
-- @deprecated awful.client.toggletag
-- @tparam tag target The tag to toggle.
-- @tparam[opt] client c The client to toggle, otherwise the focused one is used.
-- @see client.toggle_tag
-- @see tags
function client.toggletag(target, c)
    gdebug.deprecate("Use c:toggle_tag(target) instead of awful.client.toggletag", {deprecated_in=4})
    client.object.toggle_tag(c or capi.client.focus, target)
end

--- Toggle a tag on a client.
--
-- @DOC_sequences_client_toggle_tag1_EXAMPLE@
--
-- @method toggle_tag
-- @tparam tag target The tag to move the client to.
-- @see tags
function client.object.toggle_tag(self, target)
    -- Check that tag and client screen are identical
    if self and get_screen(self.screen) == get_screen(target.screen) then
        local tags = self:tags()
        local index = nil;
        for i, v in ipairs(tags) do
            if v == target then
                index = i
                break
            end
        end
        if index then
            -- If it's the only tag for the window, stop.
            if #tags == 1 then return end
            tags[index] = nil
        else
            tags[#tags + 1] = target
        end
        self:tags(tags)
    end
end

--- Move a client to a screen. Default is next screen, cycling.
-- @deprecated awful.client.movetoscreen
-- @tparam client c The client to move.
-- @tparam screen s The screen, default to current + 1.
-- @see screen
-- @see client.move_to_screen
function client.movetoscreen(c, s)
    gdebug.deprecate("Use c:move_to_screen(s) instead of awful.client.movetoscreen", {deprecated_in=4})
    client.object.move_to_screen(c or capi.client.focus, s)
end

--- Move a client to a screen. Default is next screen, cycling.
--
-- @DOC_sequences_client_move_to_screen1_EXAMPLE@
--
-- @method move_to_screen
-- @tparam[opt=c.screen.index+1] screen s The screen, default to current + 1.
-- @see screen
-- @see request::activate
-- @request client activate client.movetoscreen granted When a client could be
--  activated because `c:move_to_screen()` was called.
function client.object.move_to_screen(self, s)
    if self then
        local sc = capi.screen.count()
        if not s then
            s = self.screen.index + 1
        end
        if type(s) == "number" then
            if s > sc then s = 1 elseif s < 1 then s = sc end
        end
        s = get_screen(s)
        if get_screen(self.screen) ~= s then
            local sel_is_focused = self.active
            self.screen = s
            screen.focus(s)

            if sel_is_focused then
                self:emit_signal("request::activate", "client.movetoscreen",
                                {raise=true})
            end
        end
    end
end

--- Find suitable tags for newly created clients.
--
-- In most cases, the functionality you're actually looking for as a user will
-- either be
--
--    c:tags(c.screen.selected_tags)
--
-- or
--
--    local s = awful.screen.focused()
--    c:move_to_screen(s)
--    c:tags(s.selected_tags)
--
-- Despite its naming, this is primarily used to tag newly created clients.
-- As such, this method has no effect when applied to a client that already has
-- tags assigned (except for emitting `property::tag`).
--
-- Additionally, while it is a rare case, if the client's screen has no selected
-- tags at the point of calling this method, it will fall back to the screen's
-- full set of tags.
--
-- @DOC_sequences_client_to_selected_tags1_EXAMPLE@
--
-- @method to_selected_tags
-- @see screen.selected_tags
function client.object.to_selected_tags(self)
    local tags = {}

    -- From the client's current tags, find the ones that
    -- belong to the client's screen
    for _, t in ipairs(self:tags()) do
        if get_screen(t.screen) == get_screen(self.screen) then
            table.insert(tags, t)
        end
    end

    -- If no tags were found,
    -- choose the screen's selected tags, if any, or all of the screens tags
    if self.screen then
        if #tags == 0 then
            tags = self.screen.selected_tags
        end

        if #tags == 0 then
            tags = self.screen.tags
        end
    end

    -- Prevent clients from becoming untagged
    if #tags ~= 0 then
        self:tags(tags)
    end
end

--- If a client is marked or not.
--
-- @property marked
-- @tparam boolean marked
-- @emits marked (for legacy reasons, use `property::marked`)
-- @emits unmarker (for legacy reasons, use `property::marked`)
-- @emits property::marked

function client.object.set_marked(self, value)
    local is_marked = self.marked

    if value == false and is_marked then
        for k, v in pairs(client.data.marked) do
            if self == v then
                table.remove(client.data.marked, k)
            end
        end
        self:emit_signal("unmarked")
    elseif not is_marked and value then
        self:emit_signal("marked")
        table.insert(client.data.marked, self)
    end

    client.property.set(self, "marked", value)
end

function client.object.get_marked(self)
    return client.property.get(self, "marked")
end

--- Mark a client, and then call 'marked' hook.
-- @deprecated awful.client.mark
-- @tparam client c The client to mark, the focused one if not specified.
function client.mark(c)
    gdebug.deprecate("Use c.marked = true instead of awful.client.mark", {deprecated_in=4})
    client.object.set_marked(c or capi.client.focus, true)
end

--- Unmark a client and then call 'unmarked' hook.
-- @deprecated awful.client.unmark
-- @tparam client c The client to unmark, or the focused one if not specified.
function client.unmark(c)
    gdebug.deprecate("Use c.marked = false instead of awful.client.unmark", {deprecated_in=4})
    client.object.set_marked(c or capi.client.focus, false)
end

--- Check if a client is marked.
-- @deprecated awful.client.ismarked
-- @tparam client c The client to check, or the focused one otherwise.
function client.ismarked(c)
    gdebug.deprecate("Use c.marked instead of awful.client.ismarked", {deprecated_in=4})
    return client.object.get_marked(c or capi.client.focus)
end

--- Toggle a client as marked.
-- @deprecated awful.client.togglemarked
-- @tparam client c The client to toggle mark.
function client.togglemarked(c)
    gdebug.deprecate("Use c.marked = not c.marked instead of awful.client.togglemarked", {deprecated_in=4})
    c = c or capi.client.focus
    if c then
        c.marked = not c.marked
    end
end

--- Return the marked clients and empty the marked table.
-- @deprecated awful.client.getmarked
-- @treturn table A table with all marked clients.
function client.getmarked()
    local copy = gtable.clone(client.data.marked, false)

    for _, v in pairs(copy) do
        client.property.set(v, "marked", false)
        v:emit_signal("unmarked")
    end

    client.data.marked = {}

    return copy
end

--- Set a client floating state, overriding auto-detection.
-- Floating client are not handled by tiling layouts.
-- @deprecated awful.client.floating.set
-- @tparam client c A client.
-- @tparam boolean s True or false.
function client.floating.set(c, s)
    gdebug.deprecate("Use c.floating = true instead of awful.client.floating.set", {deprecated_in=4})
    client.object.set_floating(c, s)
end

-- Set a client floating state, overriding auto-detection.
-- Floating client are not handled by tiling layouts.
-- @tparam client c A client.
-- @tparam boolan s True or false.
function client.object.set_floating(c, s)
    c = c or capi.client.focus
    if c and client.property.get(c, "floating") ~= s then
        client.property.set(c, "floating", s)
        local scr = c.screen
        if s == true then
            c:geometry(client.property.get(c, "floating_geometry"))
        end
        c.screen = scr

        if s then
            c:emit_signal("request::border", "floating", {})
        else
            c:emit_signal("request::border", (c.active and "" or "in").."active", {})
        end
    end
end

local function store_floating_geometry(c)
    if client.object.get_floating(c) then
        client.property.set(c, "floating_geometry", c:geometry())
    end
end

-- Store the initial client geometry.
capi.client.connect_signal("new", function(cl)
    local function store_init_geometry(c)
        client.property.set(c, "floating_geometry", c:geometry())
        c:disconnect_signal("property::border_width", store_init_geometry)
    end
    cl:connect_signal("property::border_width", store_init_geometry)
end)

capi.client.connect_signal("property::geometry", store_floating_geometry)

--- Return if a client has a fixed size or not.
-- This function is deprecated, use `c.is_fixed`
-- @tparam client c The client.
-- @deprecated awful.client.isfixed
-- @see is_fixed
-- @see size_hints_honor
function client.isfixed(c)
    gdebug.deprecate("Use c.is_fixed instead of awful.client.isfixed", {deprecated_in=4})
    c = c or capi.client.focus
    return client.object.is_fixed(c)
end

--- Return if a client has a fixed size or not.
--
-- This property is read only.
-- @property is_fixed
-- @tparam[opt=false] boolean is_fixed The fixed size state
-- @propemits false false
-- @see size_hints
-- @see size_hints_honor

function client.object.is_fixed(c)
    if not c then return end
    local h = c.size_hints
    if h.min_width and h.max_width
        and h.max_height and h.min_height
        and h.min_width > 0 and h.max_width > 0
        and h.max_height > 0 and h.min_height > 0
        and h.min_width == h.max_width
        and h.min_height == h.max_height then
        return true
    end
    return false
end

--- Is the client immobilized horizontally?
--
-- Does the client have a fixed horizontal position and width, i.e. is it
-- fullscreen, maximized, or horizontally maximized?
--
-- This property is read only.
-- @property immobilized_horizontal
-- @tparam[opt=false] boolean immobilized_horizontal The immobilized state
-- @see maximized
-- @see maximized_horizontal
-- @see fullscreen

function client.object.is_immobilized_horizontal(c)
    return c.fullscreen or c.maximized or c.maximized_horizontal
end

--- Is the client immobilized vertically?
--
-- Does the client have a fixed vertical position and width, i.e. is it
-- fullscreen, maximized, or vertically maximized?
--
-- This property is read only.
-- @property immobilized_vertical
-- @tparam[opt=false] boolean immobilized_vertical The immobilized state
-- @see maximized
-- @see maximized_vertical
-- @see fullscreen

function client.object.is_immobilized_vertical(c)
    return c.fullscreen or c.maximized or c.maximized_vertical
end

--- Get a client floating state.
-- @tparam client c A client.
-- @see floating
-- @deprecated awful.client.floating.get
-- @treturn boolean True or false. Note that some windows might be floating even if you
-- did not set them manually. For example, windows with a type different than
-- normal.
function client.floating.get(c)
    gdebug.deprecate("Use c.floating instead of awful.client.floating.get", {deprecated_in=4})
    return client.object.get_floating(c)
end

--- The client floating state.
--
-- If the client is part of the tiled layout or free floating.
--
-- Note that some windows might be floating even if you
-- did not set them manually. For example, windows with a type different than
-- normal.
--
-- @DOC_sequences_client_floating1_EXAMPLE@
--
-- @property floating
-- @tparam boolean floating The floating state.
-- @request client border floating granted When a border update is required
--  because the client focus status changed.
-- @request client border active granted When a client becomes active and is not
--  floating.
-- @request client border inactive granted When a client stop being active and
--  is not floating.
-- @propemits false false

function client.object.get_floating(c)
    c = c or capi.client.focus
    if c then
        local value = client.property.get(c, "floating")
        if value ~= nil then
            return value
        end
        return client.property.get(c, "_implicitly_floating") or false
    end
end

-- When a client is not explicitly assigned a floating state, it might
-- implicitly end up being floating. The following makes sure that
-- property::floating is still emitted if this implicit floating state changes.

local function update_implicitly_floating(c)
    local explicit = client.property.get(c, "floating")
    if explicit ~= nil then
        return
    end
    local cur = client.property.get(c, "_implicitly_floating")
    local new = c.type ~= "normal"
            or c.fullscreen
            or c.maximized_vertical
            or c.maximized_horizontal
            or c.maximized
            or client.object.is_fixed(c)
    if cur ~= new then
        client.property.set(c, "_implicitly_floating", new)
        c:emit_signal("property::floating")

        -- Don't send the border signals as they would be sent twice (with this
        -- one having the wrong context). There is some `property::` signal
        -- sent before `request::manage`.
        if client.property.get(c, "_border_init") then
            if cur then
                c:emit_signal("request::border", "floating", {})
            else
                c:emit_signal("request::border", (c.active and "" or "in").."active", {})
            end
        end
    end
end

capi.client.connect_signal("property::type", update_implicitly_floating)
capi.client.connect_signal("property::fullscreen", update_implicitly_floating)
capi.client.connect_signal("property::maximized_vertical", update_implicitly_floating)
capi.client.connect_signal("property::maximized_horizontal", update_implicitly_floating)
capi.client.connect_signal("property::maximized", update_implicitly_floating)
capi.client.connect_signal("property::fullscreen", update_implicitly_floating)
capi.client.connect_signal("property::size_hints", update_implicitly_floating)
capi.client.connect_signal("request::manage", update_implicitly_floating)

--- Toggle the floating state of a client between 'auto' and 'true'.
-- Use `c.floating = not c.floating`
-- @deprecated awful.client.floating.toggle
-- @tparam client c A client.
-- @see floating
function client.floating.toggle(c)
    c = c or capi.client.focus
    -- If it has been set to floating
    client.object.set_floating(c, not client.object.get_floating(c))
end

-- Remove the floating information on a client.
-- @tparam client c The client.
function client.floating.delete(c)
    client.object.set_floating(c, nil)
end

--- The x coordinates.
--
-- `x` (usually) originate from the top left. `x` does *not* include
-- the outer client border, but rather where the content and/or titlebar
-- starts.
--
-- @DOC_sequences_client_x1_EXAMPLE@
--
-- @property x
-- @tparam integer x
-- @emits property::geometry
-- @emitstparam property::geometry table geo The
--  geometry (with `x`, `y`, `width`, `height`).
-- @emits property::x
-- @emits property::position
-- @see geometry
-- @see relative_move

--- The y coordinates.
--
-- `y` (usually) originate from the top left. `y` does *not* include
-- the outer client border, but rather where the content and/or titlebar
-- starts.
--
-- @DOC_sequences_client_y1_EXAMPLE@
--
-- @property y
-- @tparam integer y
-- @emits property::geometry
-- @emitstparam property::geometry table geo The
--  geometry (with `x`, `y`, `width`, `height`).
-- @emits property::y
-- @emits property::position
-- @see geometry
-- @see relative_move

--- The width of the client.
--
-- @DOC_sequences_client_width1_EXAMPLE@
--
-- @property width
-- @tparam integer width
-- @emits property::geometry
-- @emitstparam property::geometry table geo The
--  geometry (with `x`, `y`, `width`, `height`).
-- @emits property::width
-- @emits property::size
-- @see geometry
-- @see relative_move

--- The height of the client.
--
-- @DOC_sequences_client_height1_EXAMPLE@
--
-- @property height
-- @tparam integer height
-- @emits property::geometry
-- @emitstparam property::geometry table geo The
--  geometry (with `x`, `y`, `width`, `height`).
-- @emits property::height
-- @emits property::size
-- @see geometry
-- @see relative_move

-- Add the geometry helpers to match the wibox API
for _, v in ipairs {"x", "y", "width", "height"} do
    client.object["get_"..v] = function(c)
        return c:geometry()[v]
    end

    client.object["set_"..v] = function(c, value)
        return c:geometry({[v] = value})
    end
end


--- Restore (=unminimize) a random client.
--
-- @DOC_sequences_client_restore1_EXAMPLE@
--
-- @staticfct awful.client.restore
-- @tparam screen s The screen to use.
-- @treturn client The restored client if some client was restored, otherwise nil.
function client.restore(s)
    s = s or screen.focused()
    local cls = capi.client.get(s)
    local tags = s.selected_tags
    for _, c in pairs(cls) do
        local ctags = c:tags()
        if c.minimized then
            for _, t in ipairs(tags) do
                if gtable.hasitem(ctags, t) then
                    c.minimized = false
                    return c
                end
            end
        end
    end
    return nil
end

--- Normalize a set of numbers to 1.
-- @tparam table set the set of numbers to normalize.
-- @tparam number num the number of numbers to normalize.
local function normalize(set, num)
    num = num or #set
    local total = 0
    if num then
        for i = 1,num do
            total = total + set[i]
        end
        for i = 1,num do
            set[i] = set[i] / total
        end
    else
        for _,v in ipairs(set) do
            total = total + v
        end

        for i,v in ipairs(set) do
            set[i] = v / total
        end
    end
end

--- Calculate a client's column number, index in that column, and
-- number of visible clients in this column.
--
-- @DOC_screen_wfact4_EXAMPLE@
--
-- @legacylayout awful.client.idx
-- @tparam client c the client
-- @treturn table data A table with "col", "idx" and "num" keys.
-- @treturn integer data.col The column number.
-- @treturn integer data.idx Index of the client in the column.
-- @treturn integer data.num The number of visible clients in the column.
function client.idx(c)
    c = c or capi.client.focus
    if not c then return end

    -- Only check the tiled clients, the others un irrelevant
    local clients = client.tiled(c.screen)
    local idx = nil
    for k, cl in ipairs(clients) do
        if cl == c then
            idx = k
            break
        end
    end

    local t = c.screen.selected_tag
    local nmaster = t.master_count

    -- This will happen for floating or maximized clients
    if not idx then return nil end

    if idx <= nmaster then
        return {idx = idx, col=0, num=nmaster}
    end
    local nother = #clients - nmaster
    idx = idx - nmaster

    -- rather than regenerate the column number we can calculate it
    -- based on the how the tiling algorithm places clients we calculate
    -- the column, we could easily use the for loop in the program but we can
    -- calculate it.
    local ncol = t.column_count
    -- minimum number of clients per column
    local percol = math.floor(nother / ncol)
    -- number of columns with an extra client
    local overcol = math.fmod(nother, ncol)
    -- number of columns filled with [percol] clients
    local regcol = ncol - overcol

    local col = math.floor( (idx - 1) / percol) + 1
    if  col > regcol then
        -- col = math.floor( (idx - (percol*regcol) - 1) / (percol + 1) ) + regcol + 1
        -- simplified
        col = math.floor( (idx + regcol + percol) / (percol+1) )
        -- calculate the index in the column
        idx = idx - percol*regcol - (col - regcol - 1) * (percol+1)
        percol = percol+1
    else
        idx = idx - percol*(col-1)
    end

    return {idx = idx, col=col, num=percol}
end


--- Define how tall a client should be in the tile layout.
--
-- One valid use case for calling this is restoring serialized layouts.
-- This function is rather fragile and the behavior may not remain the
-- same across AwesomeWM versions.
--
-- When setting a value, make sure the sum remains 1. Otherwise, the
-- clients will just go offscreen or get negative size.
--
-- @DOC_screen_wfact3_EXAMPLE@
--
-- @legacylayout awful.client.setwfact
-- @tparam number wfact the window factor value
-- @tparam client c the client
-- @emits property::windowfact Emitted on the c.first_tag object.
-- @see tag.master_width_factor
function client.setwfact(wfact, c)
    -- get the currently selected window
    c = c or capi.client.focus
    if not c or not c:isvisible() then return end

    local w = client.idx(c)

    if not w then return end

    local t = c.screen.selected_tag

    -- n is the number of windows currently visible for which we have to be concerned with the properties
    local data = t.windowfact or {}
    local colfact = data[w.col]

    local need_normalize = colfact ~= nil

    if not need_normalize then
        colfact = {}
    end

    colfact[w.idx] = wfact

    if not need_normalize then
        t:emit_signal("property::windowfact")
        return
    end

    local rest = 1-wfact

    -- calculate the current denominator
    local total = 0
    for i = 1,w.num do
        if i ~= w.idx then
            total = total + colfact[i]
        end
    end

    -- normalize the windows
    for i = 1,w.num do
        if i ~= w.idx then
            colfact[i] = (colfact[i] * rest) / total
        end
    end

    t:emit_signal("property::windowfact")
end

--- Change window factor of a client.
--
-- This will emit `property::windowfact` on the specific tag object
-- `c.screen.selected_tag`.
--
-- @DOC_screen_wfact1_EXAMPLE@
--
-- Changing the gap will make some clients taller:
--
-- @DOC_screen_wfact2_EXAMPLE@
--
-- @legacylayout awful.client.incwfact
-- @tparam number add Amount to increase/decrease the client's window factor by.
--   Should be between `-current_window_factor` and something close to
--   infinite. Normalisation then ensures that the sum of all factors is 1.
-- @tparam client c the client.
-- @emits property::windowfact
function client.incwfact(add, c)
    c = c or capi.client.focus
    if not c then return end

    local t = c.screen.selected_tag
    local w = client.idx(c)
    if not w then return end

    local data = t.windowfact or {}
    local colfact = data[w.col] or {}
    local curr = colfact[w.idx] or 1
    colfact[w.idx] = curr + add

    -- keep our ratios normalized
    normalize(colfact, w.num)

    t:emit_signal("property::windowfact")
end

--- Get a client's dockable state.
--
-- @tparam client c A client.
-- @treturn bool
-- @deprecated awful.client.dockable.get
function client.dockable.get(c)
    gdebug.deprecate("Use c.dockable instead of awful.client.dockable.get", {deprecated_in=4})
    return client.object.get_dockable(c)
end

--- If the client is dockable.
--
-- A dockable client is an application confined to the edge of the screen. The
-- space it occupies is subtracted from the `screen.workarea`.
--
-- Clients with a type of "utility", "toolbar" or "dock" are dockable by
-- default.
--
-- @property dockable
-- @tparam boolean dockable The dockable state
-- @propemits false false
-- @see struts

function client.object.get_dockable(c)
    local value = client.property.get(c, "dockable")

    -- Some sane defaults
    if value == nil then
        if (c.type == "utility" or c.type == "toolbar" or c.type == "dock") then
            value = true
        else
            value = false
        end
    end

    return value
end

--- Set a client's dockable state, overriding auto-detection.
--
-- With this enabled you can dock windows by moving them from the center
-- to the edge of the workarea.
--
-- @tparam client c A client.
-- @tparam boolean value True or false.
-- @deprecated awful.client.dockable.set
function client.dockable.set(c, value)
    gdebug.deprecate("Use c.dockable = value instead of awful.client.dockable.set", {deprecated_in=4})
    client.property.set(c, "dockable", value)
end

--- If the client requests not to be decorated with a titlebar.
--
-- The motif wm hints allow a client to request not to be decorated by the WM in
-- various ways. This property uses the motif `MWM_DECOR_TITLE` hint and
-- interprets it as the client (not) wanting a titlebar.
--
-- @property requests_no_titlebar
-- @tparam boolean requests_no_titlebar Whether the client
--  requests not to get a titlebar.
-- @propemits false false

function client.object.get_requests_no_titlebar(c)
    local hints = c.motif_wm_hints
    if not hints then return false end

    local decor = hints.decorations
    if not decor then return false end

    local result = not decor.title
    if decor.all then
        -- The "all" bit inverts the meaning of the other bits
        result = not result
    end
    return result
end
capi.client.connect_signal("property::motif_wm_hints", function(c)
    -- We cannot be sure that the property actually changes, but whatever
    c:emit_signal("property::requests_no_titlebar")
end)

--- Get a client property.
--
-- This method is deprecated. It is now possible to use `c.value` directly.
--
-- @tparam client c The client.
-- @tparam string prop The property name.
-- @return The property value.
-- @deprecated awful.client.property.get
function client.property.get(c, prop)
    if not c._private._persistent_properties_loaded then
        c._private._persistent_properties_loaded = true
        for p in pairs(client.data.persistent_properties_registered) do
            local value = c:get_xproperty("awful.client.property." .. p)
            if value ~= nil then
                client.property.set(c, p, value)
            end
        end
    end
    if c._private.awful_client_properties then
        return c._private.awful_client_properties[prop]
    end
end

--- Set a client property.
--
-- This method is deprecated. It is now possible to use `c.value = value`
-- directly.
--
-- @tparam client c The client.
-- @tparam string prop The property name.
-- @param value The property value.
-- @deprecated awful.client.property.set
function client.property.set(c, prop, value)
    if not c._private.awful_client_properties then
        c._private.awful_client_properties = {}
    end
    if c._private.awful_client_properties[prop] ~= value then
        if client.data.persistent_properties_registered[prop] then
            c:set_xproperty("awful.client.property." .. prop, value)
        end
        c._private.awful_client_properties[prop] = value
        c:emit_signal("property::" .. prop)
    end
end

--- Set a client property to be persistent across restarts (via X properties).
--
-- @staticfct awful.client.property.persist
-- @tparam string prop The property name.
-- @tparam string kind The type (used for register_xproperty).
--   One of "string", "number" or "boolean".
function client.property.persist(prop, kind)
    local xprop = "awful.client.property." .. prop
    capi.awesome.register_xproperty(xprop, kind)
    client.data.persistent_properties_registered[prop] = true

    -- Make already-set properties persistent
    for _, c in ipairs(capi.client.get()) do
        if c._private.awful_client_properties and c._private.awful_client_properties[prop] ~= nil then
            c:set_xproperty(xprop, c._private.awful_client_properties[prop])
        end
    end
end

--- Returns an iterator to cycle through clients.
--
-- Starting from the client in focus or the given index, all clients that match
-- a given criteria.
--
-- @tparam function filter a function that returns true to indicate a positive match.
-- @tparam integer start  what index to start iterating from.  Defaults to using the
--   index of the currently focused client.
-- @tparam screen s which screen to use.  nil means all screens.
--
-- @staticfct awful.client.iterate
-- @usage -- un-minimize all urxvt instances
-- local urxvt = function (c)
--   return ruled.client.match(c, {class = "URxvt"})
-- end
--
-- for c in awful.client.iterate(urxvt) do
--   c.minimized = false
-- end
function client.iterate(filter, start, s)
    local clients = capi.client.get(s)
    local focused = capi.client.focus
    start         = start or gtable.hasitem(clients, focused)
    return gtable.iterate(clients, filter, start)
end

--- Switch to a client matching the given condition if running, else spawn it.
-- If multiple clients match the given condition then the next one is
-- focused.
--
-- @tparam string cmd the command to execute
-- @tparam function matcher a function that returns true to indicate a matching client
-- @tparam bool|function merge If true then merge tags (select the client's
--   first tag additionally) when the client is not visible.
--   If it is a function, it will be called with the client as argument.
-- @see awful.spawn.once
-- @see awful.spawn.single_instance
-- @see awful.spawn.raise_or_spawn
--
-- @deprecated awful.client.run_or_raise
-- @usage -- run or raise urxvt (perhaps, with tabs) on modkey + semicolon
-- awful.key({ modkey, }, 'semicolon', function ()
--     local matcher = function (c)
--         return ruled.client.match(c, {class = 'URxvt'})
--     end
--     awful.client.run_or_raise('urxvt', matcher)
-- end);
function client.run_or_raise(cmd, matcher, merge)
    gdebug.deprecate("Use awful.spawn.single_instance instead of"..
        "awful.client.run_or_raise", {deprecated_in=5})

    spawn = spawn or require("awful.spawn")
    local clients = capi.client.get()
    local findex  = gtable.hasitem(clients, capi.client.focus) or 1
    local start   = gmath.cycle(#clients, findex + 1)

    local c = client.iterate(matcher, start)()
    if c then
        c:jump_to(merge)
    else
        -- client not found, spawn it
        spawn(cmd)
    end
end

--- Get a matching transient_for client (if any).
-- @deprecated awful.client.get_transient_for_matching
-- @see client.get_transient_for_matching
-- @tparam client c The client.
-- @tparam function matcher A function that should return true, if
--   a matching parent client is found.
-- @treturn client|nil The matching parent client or nil.
function client.get_transient_for_matching(c, matcher)
    gdebug.deprecate("Use c:get_transient_for_matching(matcher) instead of"..
        "awful.client.get_transient_for_matching", {deprecated_in=4})

    return client.object.get_transient_for_matching(c, matcher)
end

--- Get a matching transient_for client (if any).
-- @method get_transient_for_matching
-- @tparam function matcher A function that should return true, if
--   a matching parent client is found.
-- @treturn client|nil The matching parent client or nil.
-- @see transient_for
-- @see modal
-- @see is_transient_for
function client.object.get_transient_for_matching(self, matcher)
    local tc = self.transient_for
    while tc do
        if matcher(tc) then
            return tc
        end
        tc = tc.transient_for
    end
    return nil
end

--- Is a client transient for another one?
--
-- @deprecated awful.client.is_transient_for
-- @see client.is_transient_for
-- @tparam client c The child client (having transient_for).
-- @tparam client c2 The parent client to check.
-- @treturn client|nil The parent client or nil.
function client.is_transient_for(c, c2)
    gdebug.deprecate("Use c:is_transient_for(c2) instead of"..
        "awful.client.is_transient_for", {deprecated_in=4})
    return client.object.is_transient_for(c, c2)
end

--- Is a client transient for another one?
--
-- This will traverse the chain formed by the `transient_for` property of `self`
-- until a client `c` with `c.transient_for == c2` is found. The found client
-- `c` is returned. If no client is found, `nil` is returned.
--
-- While `transient_for` chains are technically possible, they are unlikely, so
-- the most likely return values are `self` and `nil`.
--
-- @method is_transient_for
-- @tparam client c2 The parent client to check.
-- @treturn client|nil The parent client or nil.
-- @see transient_for
-- @see modal
-- @see client.get_transient_for_matching
function client.object.is_transient_for(self, c2)
    local tc = self
    while tc.transient_for do
        if tc.transient_for == c2 then
            return tc
        end
        tc = tc.transient_for
    end
    return nil
end

object.properties._legacy_accessors(client, "buttons", "_buttons", true, function(new_btns)
    return new_btns[1] and (
        type(new_btns[1]) == "button" or new_btns[1]._is_capi_button
    ) or false
end, true, true, "mousebinding")

object.properties._legacy_accessors(client, "keys", "_keys", true, function(new_btns)
    return new_btns[1] and (
        type(new_btns[1]) == "key" or new_btns[1]._is_capi_key
    ) or false
end, true, true, "keybinding")

--- Set the client shape.
--
-- @DOC_awful_client_shape1_EXAMPLE@
--
-- @property shape
-- @tparam gears.shape A gears.shape compatible function.
-- @propemits true false
-- @see gears.shape
function client.object.set_shape(self, shape)
    client.property.set(self, "_shape", shape)
    set_shape(self)
    self:emit_signal("property::shape", shape)
end

-- Proxy those properties to decorate their accessors with an extra flag to
-- define when they are set by the user. This allows to "manage" the value of
-- those properties internally until they are manually overridden.
for _, prop in ipairs { "border_width", "border_color", "opacity" } do
    client.object["get_"..prop] = function(self)
        return self["_"..prop]
    end
    client.object["set_"..prop] = function(self, value)
        if value ~= nil then
            self._private["_user_"..prop] = true
            self["_"..prop] = value
        end
    end
end

--- Activate (focus) a client.
--
-- This method is the correct way to focus a client. While
-- `client.focus = my_client` works and is commonly used in older code, it has
-- some drawbacks. The most obvious one is that it bypasses the activate
-- filters. It also doesn't handle minimized clients well and requires a lot
-- of boilerplate code to make work properly.
--
-- The valid `args.actions` are:
--
-- * **mouse_move**: Move the client when the mouse cursor moves until the
--  mouse buttons are release.
-- * **mouse_resize**: Resize the client when the mouse cursor moves until the
--  mouse buttons are release.
-- * **mouse_center**: Move the mouse cursor to the center of the client if it
--  isn't already within its geometry,
-- * **toggle_minimization**: If the client is already active, minimize it.
--
-- @DOC_sequences_client_activate1_EXAMPLE@
--
-- @method activate
-- @tparam table args
-- @tparam[opt=other] string args.context Why was this activate called?
-- @tparam[opt=true] boolean args.raise Raise the client to the top of its layer
--  and unminimize it (if needed).
-- @tparam[opt=false] boolean args.force Force the activation even for unfocusable
--  clients.
-- @tparam[opt=false] boolean args.switch_to_tags
-- @tparam[opt=false] boolean args.switch_to_tag
-- @tparam[opt=false] boolean args.action Once activated, perform an action.
-- @tparam[opt=false] boolean args.toggle_minimization
-- @see awful.permissions.add_activate_filter
-- @see awful.permissions.activate
-- @see request::activate
-- @see active
function client.object.activate(c, args)
    local new_args = setmetatable({}, {__index = args or {}})

    -- Set the default arguments.
    new_args.raise = new_args.raise == nil and true or args.raise

    if c == capi.client.focus and new_args.action == "toggle_minimization" then
        c.minimized = true
    else
        c:emit_signal(
            "request::activate",
            new_args.context or "other",
            new_args
        )
    end

    if new_args.action and new_args.action == "mouse_move" then
        amousec.move(c)
    elseif new_args.action and new_args.action == "mouse_resize" then
        amousec.resize(c)
    elseif new_args.action and new_args.action == "mouse_center" then
        local coords, geo = capi.mouse.coords(), c:geometry()
        coords.width, coords.height = 1,1

        if not grect.area_intersect_area(geo, coords) then
            -- Do not use `awful.placement` to avoid an useless circular
            -- dependency. Centering is *very* simple.
            capi.mouse.coords {
                x = geo.x + math.ceil(geo.width /2),
                y = geo.y + math.ceil(geo.height/2)
            }
        end
    end
end

--- Grant a permission for a client.
--
-- @method grant
-- @tparam string permission The permission name (just the name, no `request::`).
-- @tparam string context The reason why this permission is requested.
-- @see awful.permissions

--- Deny a permission for a client.
--
-- @method deny
-- @tparam string permission The permission name (just the name, no `request::`).
-- @tparam string context The reason why this permission is requested.
-- @see awful.permissions

pcommon.setup_grant(client.object, "client")

--- Return true if the client is active (has focus).
--
-- This property is **READ ONLY**. Use `c:activate { context = "myreason" }`
-- to change the focus.
--
-- The reason for this is that directly setting the focus
-- (which can also be done using `client.focus = c`) will bypass the focus
-- stealing filters. This is easy at first, but as this gets called from more
-- and more places, it quickly become unmanageable. This coding style is
-- recommended for maintainable code:
--
--    -- Check if a client has focus:
--    if c.active then
--        -- do something
--    end
--
--    -- Check if there is a active (focused) client:
--    if client.focus ~= nil then
--        -- do something
--    end
--
--    -- Get the active (focused) client:
--    local c = client.focus
--
--    -- Set the focus:
--    c:activate {
--        context       = "myreason",
--        switch_to_tag = true,
--    }
--
--    -- Get notified when a client gets or loses the focus:
--    c:connect_signal("property::active", function(c, is_active)
--        -- do something
--    end)
--
--    -- Get notified when any client gets or loses the focus:
--    client.connect_signal("property::active", function(c, is_active)
--        -- do something
--    end)
--
--    -- Get notified when any client gets the focus:
--    client.connect_signal("focus", function(c)
--        -- do something
--    end)
--
--    -- Get notified when any client loses the focus:
--    client.connect_signal("unfocus", function(c)
--        -- do something
--    end)
--
-- @property active
-- @tparam boolean active
-- @request client border active granted When a client becomes active.
-- @request client border inactive granted When a client stop being active.
-- @see activate
-- @see request::activate
-- @see awful.permissions.add_activate_filter

function client.object.get_active(c)
    return capi.client.focus == c
end

function client.object.set_active(c, value)
    if value then
        -- Do it, but print an error popup. Setting the focus without a context
        -- breaks the filters. This seems a good idea at first, then cause
        -- endless pain. QtWidgets also enforces this.
        c:activate()
        assert(false, "You cannot set `active` directly, use `c:activate()`.")
    else
        -- Be permissive given the alternative is a bit convoluted.
        capi.client.focus = nil
        gdebug.print_warning(
            "Consider using `client.focus = nil` instead of `c.active = false"
        )
    end
end

capi.client.connect_signal("property::active", function(c)
    c:emit_signal("request::border", (c.active and "" or "in").."active", {})
end)

--- The last geometry when client was floating.
-- @signal property::floating_geometry

--- Emitted when a client need to get a titlebar.
-- @signal request::titlebars
-- @tparam[opt=nil] string content The context (like "rules")
-- @tparam[opt=nil] table hints Some hints.
-- @classsignal

--- The client marked signal.
-- @deprecatedsignal marked

--- The client unmarked signal.
-- @deprecatedsignal unmarked

--- Emitted when the border client might need to be update.
--
-- The context are:
--
-- * **added**: When a new client is created.
-- * **active**: When client gains the focus (or stop being urgent/floating
--   but is active).
-- * **inactive**: When client loses the focus (or stop being urgent/floating
--   and is not active.
-- * **urgent**: When a client becomes urgent.
-- * **floating**: When the floating or maximization state changes.
--
-- @signal request::border
-- @tparam string context The context.
-- @tparam table hints The hints.
-- @classsignal
-- @see awful.permissions.update_border

--- Jump to the client that received the urgent hint first.
--
-- @DOC_sequences_client_jump_to_urgent1_EXAMPLE@
--
-- @staticfct awful.client.urgent.jumpto
-- @tparam bool|function merge If true then merge tags (select the client's
--   first tag additionally) when the client is not visible.
--   If it is a function, it will be called with the client as argument.

-- Add clients during startup to focus history.
-- This used to happen through permissions.activate, but that only handles visible
-- clients now.
capi.client.connect_signal("request::manage", function (c)
    if capi.awesome.startup
      and not c.size_hints.user_position
      and not c.size_hints.program_position then
        -- Prevent clients from being unreachable after screen count changes.
        require("awful.placement").no_offscreen(c)
    end

    if capi.awesome.startup then
        client.focus.history.add(c)
    end

    client.property.set(c, "_border_init", true)
    c:emit_signal("request::border", "added", {})
end)
capi.client.connect_signal("request::unmanage", client.focus.history.delete)

capi.client.connect_signal("request::unmanage", client.floating.delete)

-- Print a warning when the old `manage` signal is used.
capi.client.connect_signal("manage::connected", function()
    gdebug.deprecate(
        "Use `request::manage` rather than `manage`",
        {deprecated_in=5}
    )
end)
capi.client.connect_signal("unmanage::connected", function()
    gdebug.deprecate(
        "Use `request::unmanage` rather than `unmanage`",
        {deprecated_in=5}
    )
end)

for _, sig in ipairs {"marked", "unmarked"} do
    capi.client.connect_signal(sig.."::connected", function()
        gdebug.deprecate(
            "Use `property::marked` rather than `".. sig .. "`",
            {deprecated_in=4}
        )
    end)
end

-- Connect to "focus" signal, and allow to disable tracking.
do
    local disabled_count = 1

    function client.focus.history.disable_tracking()
        disabled_count = disabled_count + 1
        if disabled_count == 1 then
            capi.client.disconnect_signal("focus", client.focus.history.add)
        end
        return disabled_count
    end

    function client.focus.history.enable_tracking()
        assert(disabled_count > 0)
        disabled_count = disabled_count - 1
        if disabled_count == 0 then
            capi.client.connect_signal("focus", client.focus.history.add)
        end
        return disabled_count == 0
    end

    function client.focus.history.is_enabled()
        return disabled_count == 0, disabled_count
    end
end
client.focus.history.enable_tracking()

-- Register persistent properties
client.property.persist("floating", "boolean")

-- Extend the luaobject
object.properties(capi.client, {
    getter_class    = client.object,
    setter_class    = client.object,
    getter_fallback = client.property.get,
    setter_fallback = client.property.set,
})

--@DOC_object_COMMON@

return client

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
