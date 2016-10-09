---------------------------------------------------------------------------
--- Keep track of the focused clients.
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008 Julien Danjou
-- @submodule client
---------------------------------------------------------------------------
local grect = require("gears.geometry").rectangle

local capi =
{
    screen = screen,
    client = client,
}

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

local client
do
    client = setmetatable({}, {
        __index = function(_, k)
            client = require("awful.client")
            return client[k]
        end,
        __newindex = error -- Just to be sure in case anything ever does this
    })
end

local focus = {history = {list = {}}}

local function get_screen(s)
    return s and capi.screen[s]
end

--- Remove a client from the focus history
--
-- @client c The client that must be removed.
-- @function awful.client.focus.history.delete
function focus.history.delete(c)
    for k, v in ipairs(focus.history.list) do
        if v == c then
            table.remove(focus.history.list, k)
            break
        end
    end
end

--- Focus a client by its relative index.
--
-- @function awful.client.focus.byidx
-- @param i The index.
-- @client[opt] c The client.
function focus.byidx(i, c)
    local target = client.next(i, c)
    if target then
        target:emit_signal("request::activate", "client.focus.byidx",
                           {raise=true})
    end
end

--- Filter out window that we do not want handled by focus.
-- This usually means that desktop, dock and splash windows are
-- not registered and cannot get focus.
--
-- @client c A client.
-- @return The same client if it's ok, nil otherwise.
-- @function awful.client.focus.filter
function focus.filter(c)
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
-- @function awful.client.focus.history.add
function focus.history.add(c)
    -- Remove the client if its in stack
    focus.history.delete(c)
    -- Record the client has latest focused
    table.insert(focus.history.list, 1, c)
end

--- Get the latest focused client for a screen in history.
--
-- @tparam int|screen s The screen to look for.
-- @tparam int idx The index: 0 will return first candidate,
--   1 will return second, etc.
-- @tparam function filter An optional filter.  If no client is found in the
--   first iteration, `awful.client.focus.filter` is used by default to get any
--   client.
-- @treturn client.object A client.
-- @function awful.client.focus.history.get
function focus.history.get(s, idx, filter)
    s = get_screen(s)
    -- When this counter is equal to idx, we return the client
    local counter = 0
    local vc = client.visible(s, true)
    for _, c in ipairs(focus.history.list) do
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
    filter = filter or focus.filter
    if counter == 0 then
        for _, v in ipairs(vc) do
            if filter(v) then
                return v
            end
        end
    end
end

--- Focus the previous client in history.
-- @function awful.client.focus.history.previous
function focus.history.previous()
    local sel = capi.client.focus
    local s = sel and sel.screen or screen.focused()
    local c = focus.history.get(s, 1)
    if c then
        c:emit_signal("request::activate", "client.focus.history.previous",
                      {raise=false})
    end
end

--- Focus a client by the given direction.
--
-- @tparam string dir The direction, can be either
--   `"up"`, `"down"`, `"left"` or `"right"`.
-- @client[opt] c The client.
-- @tparam[opt=false] boolean stacked Use stacking order? (top to bottom)
-- @function awful.client.focus.bydirection
function focus.bydirection(dir, c, stacked)
    local sel = c or capi.client.focus
    if sel then
        local cltbl = client.visible(sel.screen, stacked)
        local geomtbl = {}
        for i,cl in ipairs(cltbl) do
            geomtbl[i] = cl:geometry()
        end

        local target = grect.get_in_direction(dir, geomtbl, sel:geometry())

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
-- @function awful.client.focus.global_bydirection
function focus.global_bydirection(dir, c, stacked)
    local sel = c or capi.client.focus
    local scr = get_screen(sel and sel.screen or screen.focused())

    -- change focus inside the screen
    focus.bydirection(dir, sel)

    -- if focus not changed, we must change screen
    if sel == capi.client.focus then
        screen.focus_bydirection(dir, scr)
        if scr ~= get_screen(screen.focused()) then
            local cltbl = client.visible(screen.focused(), stacked)
            local geomtbl = {}
            for i,cl in ipairs(cltbl) do
                geomtbl[i] = cl:geometry()
            end
            local target = grect.get_in_direction(dir, geomtbl, scr.geometry)

            if target then
                cltbl[target]:emit_signal("request::activate",
                                          "client.focus.global_bydirection",
                                          {raise=false})
            end
        end
    end
end

return focus

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
