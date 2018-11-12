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
-- @clientruleproperty placement
-- @see awful.placement

--- When applying the placement, honor the screen padding.
-- @clientruleproperty honor_padding
-- @param[opt=true] boolean
-- @see awful.placement

--- When applying the placement, honor the screen work area.
--
-- The workarea is the part of the screen that excludes the bars and docks.
--
-- @clientruleproperty honor_workarea
-- @param[opt=true] boolean
-- @see awful.placement

--- The client default tag.
-- @clientruleproperty tag
-- @param tag
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
-- @param[opt={tag}] table
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
--    new_tag = {
--        name     = "My new tag!", -- The tag name.
--        layout   = awful.layout.suit.max, -- Set the tag layout.
--        volatile = true, -- Remove the tag when the client is closed.
--    }
--
-- @tparam[opt=false] table|string|boolean new_tag
-- @clientruleproperty new_tag
-- @see tag
-- @see tags
-- @see switch_to_tags

--- Unselect the current tags and select this client tags.
-- Note that this property was called `switchtotag` in previous Awesome versions.
-- @clientruleproperty switch_to_tags
-- @param[opt=false] boolean
-- @see tag.selected

--- Define if the client should grab focus by default.
--
-- The `request::activate` context for this call is `rules`.
--
-- @clientruleproperty focus
-- @param[opt=false] boolean

--- Should this client have a titlebar by default.
-- @clientruleproperty titlebars_enabled
-- @param[opt=false] boolean
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
-- @client c the client to jump to
-- @tparam bool|function merge If true then merge tags (select the client's
--   first tag additionally) when the client is not visible.
--   If it is a function, it will be called with the client and its first
--   tag as arguments.
function client.jumpto(c, merge)
    gdebug.deprecate("Use c:jump_to(merge) instead of awful.client.jumpto", {deprecated_in=4})
    client.object.jump_to(c, merge)
end

--- Jump to the given client.
-- Takes care of focussing the screen, the right tag, etc.
--
-- @function client.jump_to
-- @tparam bool|function merge If true then merge tags (select the client's
--   first tag additionally) when the client is not visible.
--   If it is a function, it will be called with the client and its first
--   tag as arguments.
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
-- @function awful.client.next
-- @tparam int i The index.  Use 1 to get the next, -1 to get the previous.
-- @client[opt] sel The client.
-- @tparam[opt=false] boolean stacked Use stacking order? (top to bottom)
-- @return A client, or nil if no client is available.
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
-- @function awful.client.swap.bydirection
-- @tparam string dir The direction, can be either "up", "down", "left" or "right".
-- @client[opt=focused] c The client.
-- @tparam[opt=false] boolean stacked Use stacking order? (top to bottom)
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

--- Swap a client with another client in the given direction. Swaps across screens.
-- @function awful.client.swap.global_bydirection
-- @param dir The direction, can be either "up", "down", "left" or "right".
-- @client[opt] sel The client.
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
-- @function awful.client.swap.byidx
-- @param i The index.
-- @client[opt] c The client, otherwise focused one is used.
function client.swap.byidx(i, c)
    local sel = c or capi.client.focus
    local target = client.next(i, sel)
    if target then
        target:swap(sel)
    end
end

--- Cycle clients.
--
-- @function awful.client.cycle
-- @param clockwise True to cycle clients clockwise.
-- @param[opt] s The screen where to cycle clients.
-- @tparam[opt=false] boolean stacked Use stacking order? (top to bottom)
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

--- Get the master window.
--
-- @legacylayout awful.client.getmaster
-- @screen_or_idx[opt=awful.screen.focused()] s The screen.
-- @return The master window.
function client.getmaster(s)
    s = s or screen.focused()
    return client.visible(s)[1]
end

--- Set the client as master: put it at the beginning of other windows.
--
-- @legacylayout awful.client.setmaster
-- @client c The window to set as master.
function client.setmaster(c)
    local cls = gtable.reverse(capi.client.get(c.screen))
    for _, v in pairs(cls) do
        c:swap(v)
    end
end

--- Set the client as slave: put it at the end of other windows.
-- @legacylayout awful.client.setslave
-- @client c The window to set as slave.
function client.setslave(c)
    local cls = capi.client.get(c.screen)
    for _, v in pairs(cls) do
        c:swap(v)
    end
end

--- Move/resize a client relative to current coordinates.
-- @deprecated awful.client.moveresize
-- @param x The relative x coordinate.
-- @param y The relative y coordinate.
-- @param w The relative width.
-- @param h The relative height.
-- @client[opt] c The client, otherwise focused one is used.
-- @see client.relative_move
function client.moveresize(x, y, w, h, c)
    gdebug.deprecate("Use c:relative_move(x, y, w, h) instead of awful.client.moveresize", {deprecated_in=4})
    client.object.relative_move(c or capi.client.focus, x, y, w, h)
end

--- Move/resize a client relative to current coordinates.
-- @function client.relative_move
-- @see geometry
-- @tparam[opt=c.x] number x The relative x coordinate.
-- @tparam[opt=c.y] number y The relative y coordinate.
-- @tparam[opt=c.width] number w The relative width.
-- @tparam[opt=c.height] number h The relative height.
function client.object.relative_move(self, x, y, w, h)
    local geometry = self:geometry()
    geometry['x'] = geometry['x'] + x
    geometry['y'] = geometry['y'] + y
    geometry['width'] = geometry['width'] + w
    geometry['height'] = geometry['height'] + h
    self:geometry(geometry)
end

--- Move a client to a tag.
-- @deprecated awful.client.movetotag
-- @param target The tag to move the client to.
-- @client[opt] c The client to move, otherwise the focused one is used.
-- @see client.move_to_tag
function client.movetotag(target, c)
    gdebug.deprecate("Use c:move_to_tag(target) instead of awful.client.movetotag", {deprecated_in=4})
    client.object.move_to_tag(c or capi.client.focus, target)
end

--- Move a client to a tag.
-- @function client.move_to_tag
-- @tparam tag target The tag to move the client to.
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
-- @deprecated awful.client.toggletag
-- @param target The tag to toggle.
-- @client[opt] c The client to toggle, otherwise the focused one is used.
-- @see client.toggle_tag
function client.toggletag(target, c)
    gdebug.deprecate("Use c:toggle_tag(target) instead of awful.client.toggletag", {deprecated_in=4})
    client.object.toggle_tag(c or capi.client.focus, target)
end

--- Toggle a tag on a client.
-- @function client.toggle_tag
-- @tparam tag target The tag to move the client to.
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
-- @client c The client to move.
-- @param s The screen, default to current + 1.
-- @see screen
-- @see client.move_to_screen
function client.movetoscreen(c, s)
    gdebug.deprecate("Use c:move_to_screen(s) instead of awful.client.movetoscreen", {deprecated_in=4})
    client.object.move_to_screen(c or capi.client.focus, s)
end

--- Move a client to a screen. Default is next screen, cycling.
-- @function client.move_to_screen
-- @tparam[opt=c.screen.index+1] screen s The screen, default to current + 1.
-- @see screen
-- @see request::activate
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
            local sel_is_focused = self == capi.client.focus
            self.screen = s
            screen.focus(s)

            if sel_is_focused then
                self:emit_signal("request::activate", "client.movetoscreen",
                                {raise=true})
            end
        end
    end
end

--- Tag a client with the set of current tags.
-- @function client.to_selected_tags
-- @see screen.selected_tags
function client.object.to_selected_tags(self)
    local tags = {}

    for _, t in ipairs(self:tags()) do
        if get_screen(t.screen) == get_screen(self.screen) then
            table.insert(tags, t)
        end
    end

    if self.screen then
        if #tags == 0 then
            tags = self.screen.selected_tags
        end

        if #tags == 0 then
            tags = self.screen.tags
        end
    end

    if #tags ~= 0 then
        self:tags(tags)
    end
end

--- If a client is marked or not.
--
-- **Signal:**
--
-- * *marked* (for legacy reasons, use `property::marked`)
-- * *unmarked* (for legacy reasons, use `property::marked`)
-- * *property::marked*
--
-- @property marked
-- @param boolean

--- The border color when the client is focused.
--
-- @beautiful beautiful.border_marked
-- @param string
--

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
-- @client c The client to mark, the focused one if not specified.
function client.mark(c)
    gdebug.deprecate("Use c.marked = true instead of awful.client.mark", {deprecated_in=4})
    client.object.set_marked(c or capi.client.focus, true)
end

--- Unmark a client and then call 'unmarked' hook.
-- @deprecated awful.client.unmark
-- @client c The client to unmark, or the focused one if not specified.
function client.unmark(c)
    gdebug.deprecate("Use c.marked = false instead of awful.client.unmark", {deprecated_in=4})
    client.object.set_marked(c or capi.client.focus, false)
end

--- Check if a client is marked.
-- @deprecated awful.client.ismarked
-- @client c The client to check, or the focused one otherwise.
function client.ismarked(c)
    gdebug.deprecate("Use c.marked instead of awful.client.ismarked", {deprecated_in=4})
    return client.object.get_marked(c or capi.client.focus)
end

--- Toggle a client as marked.
-- @deprecated awful.client.togglemarked
-- @client c The client to toggle mark.
function client.togglemarked(c)
    gdebug.deprecate("Use c.marked = not c.marked instead of awful.client.togglemarked", {deprecated_in=4})
    c = c or capi.client.focus
    if c then
        c.marked = not c.marked
    end
end

--- Return the marked clients and empty the marked table.
-- @function awful.client.getmarked
-- @return A table with all marked clients.
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
-- @client c A client.
-- @param s True or false.
function client.floating.set(c, s)
    gdebug.deprecate("Use c.floating = true instead of awful.client.floating.set", {deprecated_in=4})
    client.object.set_floating(c, s)
end

-- Set a client floating state, overriding auto-detection.
-- Floating client are not handled by tiling layouts.
-- @client c A client.
-- @param s True or false.
function client.object.set_floating(c, s)
    c = c or capi.client.focus
    if c and client.property.get(c, "floating") ~= s then
        client.property.set(c, "floating", s)
        local scr = c.screen
        if s == true then
            c:geometry(client.property.get(c, "floating_geometry"))
        end
        c.screen = scr
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
-- @client c The client.
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
-- **Signal:**
--
--  * *property::is_fixed*
--
-- This property is read only.
-- @property is_fixed
-- @param boolean The fixed size state
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
-- @property immobilized
-- @param boolean The immobilized state
-- @see maximized
-- @see maximized_horizontal
-- @see maximized_vertical
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
-- @property immobilized
-- @param boolean The immobilized state
-- @see maximized
-- @see maximized_horizontal
-- @see maximized_vertical
-- @see fullscreen

function client.object.is_immobilized_vertical(c)
    return c.fullscreen or c.maximized or c.maximized_vertical
end

--- Get a client floating state.
-- @client c A client.
-- @see floating
-- @deprecated awful.client.floating.get
-- @return True or false. Note that some windows might be floating even if you
-- did not set them manually. For example, windows with a type different than
-- normal.
function client.floating.get(c)
    gdebug.deprecate("Use c.floating instead of awful.client.floating.get", {deprecated_in=4})
    return client.object.get_floating(c)
end

--- The client floating state.
-- If the client is part of the tiled layout or free floating.
--
-- Note that some windows might be floating even if you
-- did not set them manually. For example, windows with a type different than
-- normal.
--
-- **Signal:**
--
--  * *property::floating*
--
-- @property floating
-- @param boolean The floating state

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
    end
end

capi.client.connect_signal("property::type", update_implicitly_floating)
capi.client.connect_signal("property::fullscreen", update_implicitly_floating)
capi.client.connect_signal("property::maximized_vertical", update_implicitly_floating)
capi.client.connect_signal("property::maximized_horizontal", update_implicitly_floating)
capi.client.connect_signal("property::maximized", update_implicitly_floating)
capi.client.connect_signal("property::size_hints", update_implicitly_floating)
capi.client.connect_signal("manage", update_implicitly_floating)

--- Toggle the floating state of a client between 'auto' and 'true'.
-- Use `c.floating = not c.floating`
-- @deprecated awful.client.floating.toggle
-- @client c A client.
-- @see floating
function client.floating.toggle(c)
    c = c or capi.client.focus
    -- If it has been set to floating
    client.object.set_floating(c, not client.object.get_floating(c))
end

-- Remove the floating information on a client.
-- @client c The client.
function client.floating.delete(c)
    client.object.set_floating(c, nil)
end

--- The x coordinates.
--
-- **Signal:**
--
--  * *property::x*
--
-- @property x
-- @param integer

--- The y coordinates.
--
-- **Signal:**
--
--  * *property::y*
--
-- @property y
-- @param integer

--- The width of the client.
--
-- **Signal:**
--
--  * *property::width*
--
-- @property width
-- @param width

--- The height of the client.
--
-- **Signal:**
--
--  * *property::height*
--
-- @property height
-- @param height

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
-- @function awful.client.restore
-- @param s The screen to use.
-- @return The restored client if some client was restored, otherwise nil.
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

--- Normalize a set of numbers to 1
-- @param set the set of numbers to normalize
-- @param num the number of numbers to normalize
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
-- @legacylayout awful.client.idx
-- @client c the client
-- @return col the column number
-- @return idx index of the client in the column
-- @return num the number of visible clients in the column
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


--- Set the window factor of a client
--
-- @legacylayout awful.client.setwfact
-- @param wfact the window factor value
-- @client c the client
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
-- @legacylayout awful.client.incwfact
-- @tparam number add Amount to increase/decrease the client's window factor.
--   Should be between `-current_window_factor` and something close to
--   infinite.  The normalisation then ensures that the sum of all factors is 1.
-- @client c the client
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
-- @client c A client.
-- @treturn bool
-- @deprecated awful.client.dockable.get
function client.dockable.get(c)
    gdebug.deprecate("Use c.dockable instead of awful.client.dockable.get", {deprecated_in=4})
    return client.object.get_dockable(c)
end

--- If the client is dockable.
--
-- A dockable client is an application confined to the edge of the screen. The
-- space it occupies is substracted from the `screen.workarea`.
--
-- Clients with a type of "utility", "toolbar" or "dock" are dockable by
-- default.
--
-- **Signal:**
--
-- * *property::dockable*
--
-- @property dockable
-- @param boolean The dockable state

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
-- @client c A client.
-- @param value True or false.
-- @deprecated awful.client.dockable.set
function client.dockable.set(c, value)
    gdebug.deprecate("Use c.dockable = value instead of awful.client.dockable.set", {deprecated_in=4})
    client.property.set(c, "dockable", value)
end

--- If the client requests not to be decorated with a titlebar.
--
-- The motif wm hints allow a client to request not to be decorated by the WM in
-- various ways. This property uses the motif MWM_DECOR_TITLE hint and
-- interprets it as the client (not) wanting a titlebar.
--
-- **Signal:**
--
-- * *property::requests_no_titlebar*
--
-- @property requests_no_titlebar
-- @param boolean Whether the client requests not to get a titlebar

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
-- @client c The client.
-- @param prop The property name.
-- @return The property.
-- @deprecated awful.client.property.get
function client.property.get(c, prop)
    if not c.data._persistent_properties_loaded then
        c.data._persistent_properties_loaded = true
        for p in pairs(client.data.persistent_properties_registered) do
            local value = c:get_xproperty("awful.client.property." .. p)
            if value ~= nil then
                client.property.set(c, p, value)
            end
        end
    end
    if c.data.awful_client_properties then
        return c.data.awful_client_properties[prop]
    end
end

--- Set a client property.
--
-- This method is deprecated. It is now possible to use `c.value = value`
-- directly.
--
-- @client c The client.
-- @param prop The property name.
-- @param value The value.
-- @deprecated awful.client.property.set
function client.property.set(c, prop, value)
    if not c.data.awful_client_properties then
        c.data.awful_client_properties = {}
    end
    if c.data.awful_client_properties[prop] ~= value then
        if client.data.persistent_properties_registered[prop] then
            c:set_xproperty("awful.client.property." .. prop, value)
        end
        c.data.awful_client_properties[prop] = value
        c:emit_signal("property::" .. prop)
    end
end

--- Set a client property to be persistent across restarts (via X properties).
--
-- @function awful.client.property.persist
-- @param prop The property name.
-- @param kind The type (used for register_xproperty).
--   One of "string", "number" or "boolean".
function client.property.persist(prop, kind)
    local xprop = "awful.client.property." .. prop
    capi.awesome.register_xproperty(xprop, kind)
    client.data.persistent_properties_registered[prop] = true

    -- Make already-set properties persistent
    for c in pairs(capi.client.get()) do
        if c.data.awful_client_properties and c.data.awful_client_properties[prop] ~= nil then
            c:set_xproperty(xprop, c.data.awful_client_properties[prop])
        end
    end
end

---
-- Returns an iterator to cycle through, starting from the client in focus or
-- the given index, all clients that match a given criteria.
--
-- @param filter a function that returns true to indicate a positive match
-- @param start  what index to start iterating from.  Defaults to using the
--   index of the currently focused client.
-- @param s which screen to use.  nil means all screens.
--
-- @function awful.client.iterate
-- @usage -- un-minimize all urxvt instances
-- local urxvt = function (c)
--   return awful.rules.match(c, {class = "URxvt"})
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
-- focussed.
--
-- @param cmd the command to execute
-- @param matcher a function that returns true to indicate a matching client
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
--         return awful.rules.match(c, {class = 'URxvt'})
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
-- @client c The client.
-- @tparam function matcher A function that should return true, if
--   a matching parent client is found.
-- @treturn client.client|nil The matching parent client or nil.
function client.get_transient_for_matching(c, matcher)
    gdebug.deprecate("Use c:get_transient_for_matching(matcher) instead of"..
        "awful.client.get_transient_for_matching", {deprecated_in=4})

    return client.object.get_transient_for_matching(c, matcher)
end

--- Get a matching transient_for client (if any).
-- @function client.get_transient_for_matching
-- @tparam function matcher A function that should return true, if
--   a matching parent client is found.
-- @treturn client.client|nil The matching parent client or nil.
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
-- @deprecated awful.client.is_transient_for
-- @see client.is_transient_for
-- @client c The child client (having transient_for).
-- @client c2 The parent client to check.
-- @treturn client.client|nil The parent client or nil.
function client.is_transient_for(c, c2)
    gdebug.deprecate("Use c:is_transient_for(c2) instead of"..
        "awful.client.is_transient_for", {deprecated_in=4})
    return client.object.is_transient_for(c, c2)
end

--- Is a client transient for another one?
-- @function client.is_transient_for
-- @client c2 The parent client to check.
-- @treturn client.client|nil The parent client or nil.
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

--- Set the client shape.
-- @property shape
-- @tparam gears.shape A gears.shape compatible function.
-- @see gears.shape
function client.object.set_shape(self, shape)
    client.property.set(self, "_shape", shape)
    set_shape(self)
end

-- Register standards signals

--- The last geometry when client was floating.
-- @signal property::floating_geometry

--- Emited when a client need to get a titlebar.
-- @signal request::titlebars
-- @tparam[opt=nil] string content The context (like "rules")
-- @tparam[opt=nil] table hints Some hints.

--- The client marked signal (deprecated).
-- @signal marked

--- The client unmarked signal (deprecated).
-- @signal unmarked

-- Add clients during startup to focus history.
-- This used to happen through ewmh.activate, but that only handles visible
-- clients now.
capi.client.connect_signal("manage", function (c)
    if awesome.startup then
        client.focus.history.add(c)
    end
end)
capi.client.connect_signal("unmanage", client.focus.history.delete)

capi.client.connect_signal("unmanage", client.floating.delete)

-- Connect to "focus" signal, and allow to disable tracking.
do
    local disabled_count = 1
    --- Disable history tracking.
    --
    -- See `awful.client.focus.history.enable_tracking` to enable it again.
    -- @treturn int The internal value of `disabled_count` (calls to this
    --   function without calling `awful.client.focus.history.enable_tracking`).
    -- @function awful.client.focus.history.disable_tracking
    function client.focus.history.disable_tracking()
        disabled_count = disabled_count + 1
        if disabled_count == 1 then
            capi.client.disconnect_signal("focus", client.focus.history.add)
        end
        return disabled_count
    end

    --- Enable history tracking.
    --
    -- This is the default, but can be disabled
    -- through `awful.client.focus.history.disable_tracking`.
    -- @treturn boolean True if history tracking has been enabled.
    -- @function awful.client.focus.history.enable_tracking
    function client.focus.history.enable_tracking()
        assert(disabled_count > 0)
        disabled_count = disabled_count - 1
        if disabled_count == 0 then
            capi.client.connect_signal("focus", client.focus.history.add)
        end
        return disabled_count == 0
    end

    --- Is history tracking enabled?
    -- @treturn bool True if history tracking is enabled.
    -- @treturn int The number of times that tracking has been disabled.
    -- @function awful.client.focus.history.is_enabled
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

return client

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
