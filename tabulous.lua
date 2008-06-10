-----------------------------------------------
--   tabulous: Fabulous tabs for awesome     --
--                                           --
-- © 2008 Julien Danjou <julien@danjou.info> --
-----------------------------------------------

--------------------
-- Module loading --
--------------------
local P = {}
if _REQUIREDNAME == nil then
    tabulous = P
else
    _G[_REQUIREDNAME] = P
end

-- Grab environment we need
local client = client
local table = table
local pairs = pairs
local rawset = rawset
local setmetatable = setmetatable

-- Reset env
setfenv(1, P)

-- The tab datastructure.
-- It contains keys which are clients. If a c1 is a client,
-- then if tabbed[c1] exists it must contains a client c2.
-- And if so, tabbed[c2] must contain c1.
-- It's cycling i.e. tabbed[c1] = c2, tabbed[c2] = c3, tabbed[c3] = c1
local tabbed = {}

-- Define a special index method.
-- This is useful because we need to compare clients with __eq() to be sure
-- that the key are identical. Otherwise Lua assume that each object is different.
local function tabbed_index(table, key)
    for k, v in pairs(table) do
        if k == key then
            return v
        end
    end
    return nil
end

-- Same as tabbed_index, cheat!
local function tabbed_newindex(table, key, value)
    for k, v in pairs(table) do
        if k == key then
            rawset(table, k, value)
            return
        end
    end
    rawset(table, key, value)
end

-- Set the metatable for tabbed
setmetatable(tabbed, { __index = tabbed_index, __newindex = tabbed_newindex })

---------------
-- Functions --
---------------

-- Get the next tab
local function client_nexttab(s)
    local sel = s or client.focus_get()
    if sel and tabbed[sel] then
        return tabbed[sel]
    end
end

-- Get the previous tab
local function client_prevtab(c, stopc)
    local sel = c or client.focus_get()
    local stop = stopc or sel
    if sel and tabbed[sel] then
        if stop == tabbed[sel] then
            return sel
        else
            return client_prevtab(tabbed[sel], stop)
        end
    end
end

-- Tab a client with another one
local function client_tab(c1, s)
    local sel = s or client.focus_get()
    if c1 and sel and c1 ~= sel then
        if tabbed[c1] then
            tabbed[sel] = tabbed[c1]
            tabbed[c1] = sel
        elseif tabbed[sel] then
            tabbed[c1] = tabbed[sel]
            tabbed[sel] = c1
        else
            tabbed[c1] = sel
            tabbed[sel] = c1
        end
    c1:hide()
    end
end

-- Untab a client
local function client_untab(c)
    local sel = c or client.focus_get()
    if sel and tabbed[sel] then
        local p = client_prevtab(sel)
        tabbed[p] = tabbed[sel]
        tabbed[sel] = nil
        p:unhide()
    end
end

-- Focus the next tab
local function client_focusnexttab(s)
    local sel = s or client.focus_get()
    local n = client_nexttab(sel)
    if n then
        sel:hide()
        n:unhide()
        n:focus_set()
    end
end

-- Focus the previous tab
local function client_focusprevtab(s)
    local sel = s or client.focus_get()
    local p = client_prevtab(sel)
    if p then
        sel:hide()
        p:unhide()
        p:focus_set()
    end
end

P.tab = client_tab
P.untab = client_untab
P.focusnext = client_focusnexttab
P.focusprev = client_focusprevtab
P.next = client_nexttab
P.prev = client_prevtab
return P
