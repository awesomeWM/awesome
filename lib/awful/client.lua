---------------------------------------------------------------------------
--- Useful client manipulation functions.
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008 Julien Danjou
-- @release @AWESOME_VERSION@
-- @module awful.client
---------------------------------------------------------------------------

-- Grab environment we need
local util = require("awful.util")
local spawn = require("awful.spawn")
local tag = require("awful.tag")
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
local client = {}

-- Private data
client.data = {}
client.data.focus = {}
client.data.urgent = {}
client.data.marked = {}
client.data.properties = setmetatable({}, { __mode = 'k' })
client.data.persistent_properties_registered = {} -- keys are names of persistent properties, value always true
client.data.persistent_properties_loaded = setmetatable({}, { __mode = 'k' }) -- keys are clients, value always true

-- Functions
client.urgent = {}
client.focus = {}
client.focus.history = {}
client.swap = {}
client.floating = {}
client.dockable = {}
client.property = {}
client.shape = require("awful.client.shape")

--- Jump to the given client.
-- Takes care of focussing the screen, the right tag, etc.
--
-- @client c the client to jump to
-- @tparam bool merge If true then merge tags when clients are not visible.
function client.jumpto(c, merge)
    local s = get_screen(screen.focused())
    -- focus the screen
    if s ~= get_screen(c.screen) then
        screen.focus(c.screen)
    end

    -- Try to make client visible, this also covers e.g. sticky
    local t = c.first_tag
    if t and not c:isvisible() then
        if merge then
            t.selected = true
        else
            tag.viewonly(t)
        end
    end

    c:emit_signal("request::activate", "client.jumpto", {raise=true})
end

--- Get the first client that got the urgent hint.
--
-- @treturn client.object The first urgent client.
function client.urgent.get()
    if #client.data.urgent > 0 then
        return client.data.urgent[1]
    else
        -- fallback behaviour: iterate through clients and get the first urgent
        local clients = capi.client.get()
        for _, cl in pairs(clients) do
            if cl.urgent then
                return cl
            end
        end
    end
end

--- Jump to the client that received the urgent hint first.
--
-- @tparam bool merge If true then merge tags when clients are not visible.
function client.urgent.jumpto(merge)
    local c = client.urgent.get()
    if c then
        client.jumpto(c, merge)
    end
end

--- Adds client to urgent stack.
--
-- @client c The client object.
-- @param prop The property which is updated.
function client.urgent.add(c, prop)
    if type(c) == "client" and prop == "urgent" and c.urgent then
        table.insert(client.data.urgent, c)
    end
end

--- Remove client from urgent stack.
--
-- @client c The client object.
function client.urgent.delete(c)
    for k, cl in ipairs(client.data.urgent) do
        if c == cl then
            table.remove(client.data.urgent, k)
            break
        end
    end
end

--- Remove a client from the focus history
--
-- @client c The client that must be removed.
function client.focus.history.delete(c)
    for k, v in ipairs(client.data.focus) do
        if v == c then
            table.remove(client.data.focus, k)
            break
        end
    end
end

--- Filter out window that we do not want handled by focus.
-- This usually means that desktop, dock and splash windows are
-- not registered and cannot get focus.
--
-- @client c A client.
-- @return The same client if it's ok, nil otherwise.
function client.focus.filter(c)
    if c.type == "desktop"
        or c.type == "dock"
        or c.type == "splash"
        or not c.focusable then
        return nil
    end
    return c
end

--- Update client focus history.
--
-- @client c The client that has been focused.
function client.focus.history.add(c)
    -- Remove the client if its in stack
    client.focus.history.delete(c)
    -- Record the client has latest focused
    table.insert(client.data.focus, 1, c)
end

--- Get the latest focused client for a screen in history.
--
-- @tparam int|screen s The screen to look for.
-- @tparam int idx The index: 0 will return first candidate,
--   1 will return second, etc.
-- @tparam function filter An optional filter.  If no client is found in the
--   first iteration, client.focus.filter is used by default to get any
--   client.
-- @treturn client.object A client.
function client.focus.history.get(s, idx, filter)
    s = get_screen(s)
    -- When this counter is equal to idx, we return the client
    local counter = 0
    local vc = client.visible(s, true)
    for _, c in ipairs(client.data.focus) do
        if get_screen(c.screen) == s then
            if not filter or filter(c) then
                for _, vcc in ipairs(vc) do
                    if vcc == c then
                        if counter == idx then
                            return c
                        end
                        -- We found one, increment the counter only.
                        counter = counter + 1
                        break
                    end
                end
            end
        end
    end
    -- Argh nobody found in history, give the first one visible if there is one
    -- that passes the filter.
    filter = filter or client.focus.filter
    if counter == 0 then
        for _, v in ipairs(vc) do
            if filter(v) then
                return v
            end
        end
    end
end

--- Focus the previous client in history.
function client.focus.history.previous()
    local sel = capi.client.focus
    local s = sel and sel.screen or screen.focused()
    local c = client.focus.history.get(s, 1)
    if c then
        c:emit_signal("request::activate", "client.focus.history.previous",
                      {raise=false})
    end
end

--- Get visible clients from a screen.
--
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
-- @tparam integer|screen s The screen, or nil for all screens.
-- @tparam[opt=false] boolean stacked Use stacking order? (top to bottom)
-- @treturn table A table with all visible and tiled clients.
function client.tiled(s, stacked)
    local clients = client.visible(s, stacked)
    local tclients = {}
    -- Remove floating clients
    for _, c in pairs(clients) do
        if not client.floating.get(c)
            and not c.fullscreen
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
                return cls[util.cycle(#cls, idx + i)]
            end
        end
    end
end

--- Focus a client by the given direction.
--
-- @tparam string dir The direction, can be either
--   `"up"`, `"down"`, `"left"` or `"right"`.
-- @client[opt] c The client.
-- @tparam[opt=false] boolean stacked Use stacking order? (top to bottom)
function client.focus.bydirection(dir, c, stacked)
    local sel = c or capi.client.focus
    if sel then
        local cltbl = client.visible(sel.screen, stacked)
        local geomtbl = {}
        for i,cl in ipairs(cltbl) do
            geomtbl[i] = cl:geometry()
        end

        local target = util.get_rectangle_in_direction(dir, geomtbl, sel:geometry())

        -- If we found a client to focus, then do it.
        if target then
            cltbl[target]:emit_signal("request::activate",
                                      "client.focus.bydirection", {raise=false})
        end
    end
end

--- Focus a client by the given direction. Moves across screens.
--
-- @param dir The direction, can be either "up", "down", "left" or "right".
-- @client[opt] c The client.
-- @tparam[opt=false] boolean stacked Use stacking order? (top to bottom)
function client.focus.global_bydirection(dir, c, stacked)
    local sel = c or capi.client.focus
    local scr = get_screen(sel and sel.screen or screen.focused())

    -- change focus inside the screen
    client.focus.bydirection(dir, sel)

    -- if focus not changed, we must change screen
    if sel == capi.client.focus then
        screen.focus_bydirection(dir, scr)
        if scr ~= get_screen(screen.focused()) then
            local cltbl = client.visible(screen.focused(), stacked)
            local geomtbl = {}
            for i,cl in ipairs(cltbl) do
                geomtbl[i] = cl:geometry()
            end
            local target = util.get_rectangle_in_direction(dir, geomtbl, scr.geometry)

            if target then
                cltbl[target]:emit_signal("request::activate",
                                          "client.focus.global_bydirection",
                                          {raise=false})
            end
        end
    end
end

--- Focus a client by its relative index.
--
-- @param i The index.
-- @client[opt] c The client.
function client.focus.byidx(i, c)
    local target = client.next(i, c)
    if target then
        target:emit_signal("request::activate", "client.focus.byidx",
                           {raise=true})
    end
end

--- Swap a client with another client in the given direction.
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
        local target = util.get_rectangle_in_direction(dir, geomtbl, sel:geometry())

        -- If we found a client to swap with, then go for it
        if target then
            cltbl[target]:swap(sel)
        end
    end
end

--- Swap a client with another client in the given direction. Swaps across screens.
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
        elseif get_screen(sel.screen) ~= get_screen(c.screen) and sel == c then
            client.movetoscreen(sel, screen.focused())

        -- swapping to a nonempty screen
        elseif get_screen(sel.screen) ~= get_screen(c.screen) and sel ~= c then
            client.movetoscreen(sel, c.screen)
            client.movetoscreen(c, scr)
        end

        screen.focus(sel.screen)
        sel:emit_signal("request::activate", "client.swap.global_bydirection",
                        {raise=false})
    end
end

--- Swap a client by its relative index.
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
-- @param[opt] s The screen number, defaults to focused screen.
-- @return The master window.
function client.getmaster(s)
    s = s or screen.focused()
    return client.visible(s)[1]
end

--- Set the client as master: put it at the beginning of other windows.
--
-- @client c The window to set as master.
function client.setmaster(c)
    local cls = util.table.reverse(capi.client.get(c.screen))
    for _, v in pairs(cls) do
        c:swap(v)
    end
end

--- Set the client as slave: put it at the end of other windows.
-- @client c The window to set as slave.
function client.setslave(c)
    local cls = capi.client.get(c.screen)
    for _, v in pairs(cls) do
        c:swap(v)
    end
end

--- Move/resize a client relative to current coordinates.
-- @param x The relative x coordinate.
-- @param y The relative y coordinate.
-- @param w The relative width.
-- @param h The relative height.
-- @client[opt] c The client, otherwise focused one is used.
function client.moveresize(x, y, w, h, c)
    local sel = c or capi.client.focus
    local geometry = sel:geometry()
    geometry['x'] = geometry['x'] + x
    geometry['y'] = geometry['y'] + y
    geometry['width'] = geometry['width'] + w
    geometry['height'] = geometry['height'] + h
    sel:geometry(geometry)
end

--- Move a client to a tag.
-- @param target The tag to move the client to.
-- @client[opt] c The client to move, otherwise the focused one is used.
function client.movetotag(target, c)
    local sel = c or capi.client.focus
    local s = tag.getscreen(target)
    if sel and s then
        if sel == capi.client.focus then
            sel:emit_signal("request::activate", "client.movetotag", {raise=true})
        end
        -- Set client on the same screen as the tag.
        sel.screen = s
        sel:tags({ target })
    end
end

--- Toggle a tag on a client.
-- @param target The tag to toggle.
-- @client[opt] c The client to toggle, otherwise the focused one is used.
function client.toggletag(target, c)
    local sel = c or capi.client.focus
    -- Check that tag and client screen are identical
    if sel and get_screen(sel.screen) == get_screen(tag.getscreen(target)) then
        local tags = sel:tags()
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
        sel:tags(tags)
    end
end

--- Move a client to a screen. Default is next screen, cycling.
-- @client c The client to move.
-- @param s The screen, default to current + 1.
function client.movetoscreen(c, s)
    local sel = c or capi.client.focus
    if sel then
        local sc = capi.screen.count()
        if not s then
            s = sel.screen + 1
        end
        if type(s) == "number" and s > sc then s = 1 elseif s < 1 then s = sc end
        s = get_screen(s)
        if get_screen(sel.screen) ~= s then
            local sel_is_focused = sel == capi.client.focus
            sel.screen = s
            screen.focus(s)

            if sel_is_focused then
                sel:emit_signal("request::activate", "client.movetoscreen",
                                {raise=true})
            end
        end
    end
end

--- Mark a client, and then call 'marked' hook.
-- @client c The client to mark, the focused one if not specified.
-- @return True if the client has been marked. False if the client was already marked.
function client.mark(c)
    local cl = c or capi.client.focus
    if cl then
        for _, v in pairs(client.data.marked) do
            if cl == v then
                return false
            end
        end

        table.insert(client.data.marked, cl)

        -- Call callback
        cl:emit_signal("marked")
        return true
    end
end

--- Unmark a client and then call 'unmarked' hook.
-- @client c The client to unmark, or the focused one if not specified.
-- @return True if the client has been unmarked. False if the client was not marked.
function client.unmark(c)
    local cl = c or capi.client.focus

    for k, v in pairs(client.data.marked) do
        if cl == v then
            table.remove(client.data.marked, k)
            cl:emit_signal("unmarked")
            return true
        end
    end

    return false
end

--- Check if a client is marked.
-- @client c The client to check, or the focused one otherwise.
function client.ismarked(c)
    local cl = c or capi.client.focus
    if cl then
        for _, v in pairs(client.data.marked) do
            if cl == v then
                return true
            end
        end
    end
    return false
end

--- Toggle a client as marked.
-- @client c The client to toggle mark.
function client.togglemarked(c)
    c = c or capi.client.focus
    if not client.mark(c) then
        client.unmark(c)
    end
end

--- Return the marked clients and empty the marked table.
-- @return A table with all marked clients.
function client.getmarked()
    for _, v in pairs(client.data.marked) do
        v:emit_signal("unmarked")
    end

    local t = client.data.marked
    client.data.marked = {}
    return t
end

--- Set a client floating state, overriding auto-detection.
-- Floating client are not handled by tiling layouts.
-- @client c A client.
-- @param s True or false.
function client.floating.set(c, s)
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
    if client.floating.get(c) then
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

--- Return if a client has a fixe size or not.
-- @client c The client.
function client.isfixed(c)
    c = c or capi.client.focus
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

--- Get a client floating state.
-- @client c A client.
-- @return True or false. Note that some windows might be floating even if you
-- did not set them manually. For example, windows with a type different than
-- normal.
function client.floating.get(c)
    c = c or capi.client.focus
    if c then
        local value = client.property.get(c, "floating")
        if value ~= nil then
            return value
        end
        if c.type ~= "normal"
            or c.fullscreen
            or c.maximized_vertical
            or c.maximized_horizontal
            or client.isfixed(c) then
            return true
        end
        return false
    end
end

--- Toggle the floating state of a client between 'auto' and 'true'.
-- @client c A client.
function client.floating.toggle(c)
    c = c or capi.client.focus
    -- If it has been set to floating
    if client.floating.get(c) then
        client.floating.set(c, false)
    else
        client.floating.set(c, true)
    end
end

--- Remove the floating information on a client.
-- @client c The client.
function client.floating.delete(c)
    client.floating.set(c, nil)
end

--- Restore (=unminimize) a random client.
-- @param s The screen to use.
-- @return The restored client if some client was restored, otherwise nil.
function client.restore(s)
    s = s or screen.focused()
    local cls = capi.client.get(s)
    local tags = tag.selectedlist(s)
    for _, c in pairs(cls) do
        local ctags = c:tags()
        if c.minimized then
            for _, t in ipairs(tags) do
                if util.table.hasitem(ctags, t) then
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

    local t = tag.selected(c.screen)
    local nmaster = tag.getnmaster(t)

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
    local ncol = tag.getncol(t)
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
-- @param wfact the window factor value
-- @client c the client
function client.setwfact(wfact, c)
    -- get the currently selected window
    c = c or capi.client.focus
    if not c or not c:isvisible() then return end

    local w = client.idx(c)

    if not w then return end

    local t = tag.selected(c.screen)

    -- n is the number of windows currently visible for which we have to be concerned with the properties
    local data = tag.getproperty(t, "windowfact") or {}
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

--- Increment a client's window factor
--
-- @param add amount to increase the client's window
-- @client c the client
function client.incwfact(add, c)
    c = c or capi.client.focus
    if not c then return end

    local t = tag.selected(c.screen)

    local w = client.idx(c)

    local data = tag.getproperty(t, "windowfact") or {}
    local colfact = data[w.col] or {}
    local curr = colfact[w.idx] or 1
    colfact[w.idx] = curr + add

    -- keep our ratios normalized
    normalize(colfact, w.num)

    t:emit_signal("property::windowfact")
end

--- Get a client dockable state.
--
-- @client c A client.
-- @return True or false. Note that some windows might be dockable even if you
--   did not set them manually. For example, windows with a type "utility",
--   "toolbar" or "dock"
function client.dockable.get(c)
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

--- Set a client dockable state, overriding auto-detection.
-- With this enabled you can dock windows by moving them from the center
-- to the edge of the workarea.
--
-- @client c A client.
-- @param value True or false.
function client.dockable.set(c, value)
    client.property.set(c, "dockable", value)
end

--- Get a client property.
--
-- @client c The client.
-- @param prop The property name.
-- @return The property.
function client.property.get(c, prop)
    if not client.data.persistent_properties_loaded[c] then
        client.data.persistent_properties_loaded[c] = true
        for p in pairs(client.data.persistent_properties_registered) do
            local value = c:get_xproperty("awful.client.property." .. p)
            if value ~= nil then
                client.property.set(c, p, value)
            end
        end
    end
    if client.data.properties[c] then
        return client.data.properties[c][prop]
    end
end

--- Set a client property.
-- These properties are internal to awful. Some are used to move clients, etc.
--
-- @client c The client.
-- @param prop The property name.
-- @param value The value.
function client.property.set(c, prop, value)
    if not client.data.properties[c] then
        client.data.properties[c] = {}
    end
    if client.data.properties[c][prop] ~= value then
        if client.data.persistent_properties_registered[prop] then
            c:set_xproperty("awful.client.property." .. prop, value)
        end
        client.data.properties[c][prop] = value
        c:emit_signal("property::" .. prop)
    end
end

--- Set a client property to be persistent across restarts (via X properties).
--
-- @param prop The property name.
-- @param kind The type (used for register_xproperty).
--   One of "string", "number" or "boolean".
function client.property.persist(prop, kind)
    local xprop = "awful.client.property." .. prop
    capi.awesome.register_xproperty(xprop, kind)
    client.data.persistent_properties_registered[prop] = true

    -- Make already-set properties persistent
    for c in pairs(client.data.properties) do
        if client.data.properties[c] and client.data.properties[c][prop] ~= nil then
            c:set_xproperty(xprop, client.data.properties[c][prop])
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
    start         = start or util.table.hasitem(clients, focused)
    return util.table.iterate(clients, filter, start)
end

--- Switch to a client matching the given condition if running, else spawn it.
-- If multiple clients match the given condition then the next one is
-- focussed.
--
-- @param cmd the command to execute
-- @param matcher a function that returns true to indicate a matching client
-- @param merge if true then merge tags when clients are not visible
--
-- @usage -- run or raise urxvt (perhaps, with tabs) on modkey + semicolon
-- awful.key({ modkey, }, 'semicolon', function ()
--     local matcher = function (c)
--         return awful.rules.match(c, {class = 'URxvt'})
--     end
--     awful.client.run_or_raise('urxvt', matcher)
-- end);
function client.run_or_raise(cmd, matcher, merge)
    local clients = capi.client.get()
    local findex  = util.table.hasitem(clients, capi.client.focus) or 1
    local start   = util.cycle(#clients, findex + 1)

    local c = client.iterate(matcher, start)()
    if c then
        client.jumpto(c, merge)
    else
        -- client not found, spawn it
        spawn(cmd)
    end
end

--- Get a matching transient_for client (if any).
-- @client c The client.
-- @tparam function matcher A function that should return true, if
--   a matching parent client is found.
-- @treturn client.client|nil The matching parent client or nil.
function client.get_transient_for_matching(c, matcher)
    local tc = c.transient_for
    while tc do
        if matcher(tc) then
            return tc
        end
        tc = tc.transient_for
    end
    return nil
end

--- Is a client transient for another one?
-- @client c The child client (having transient_for).
-- @client c2 The parent client to check.
-- @treturn client.client|nil The parent client or nil.
function client.is_transient_for(c, c2)
    local tc = c
    while tc.transient_for do
        if tc.transient_for == c2 then
            return tc
        end
        tc = tc.transient_for
    end
    return nil
end

-- Register standards signals
capi.client.add_signal("property::floating_geometry")
capi.client.add_signal("property::floating")
capi.client.add_signal("property::dockable")
capi.client.add_signal("marked")
capi.client.add_signal("unmarked")

capi.client.connect_signal("focus", client.focus.history.add)
-- Add clients during startup to focus history.
-- This used to happen through ewmh.activate, but that only handles visible
-- clients now.
capi.client.connect_signal("manage", function (c)
    if awesome.startup then
        client.focus.history.add(c)
    end
end)
capi.client.connect_signal("unmanage", client.focus.history.delete)

capi.client.connect_signal("property::urgent", client.urgent.add)
capi.client.connect_signal("focus", client.urgent.delete)
capi.client.connect_signal("unmanage", client.urgent.delete)

capi.client.connect_signal("unmanage", client.floating.delete)

-- Register persistent properties
client.property.persist("floating", "boolean")

return client

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
