----------------------------------------------------
--      tabulous: Fabulous tabs for awesome       --
--                                                --
-- © 2008 Julien Danjou <julien@danjou.info>      --
-- © 2008 Lucas de Vries <lucasdevries@gmail.com> --
----------------------------------------------------

require('awful')

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
local awful = awful

-- Reset env
setfenv(1, P)

local tabbed = {}

awful.hooks.userhook_create('tabbed')
awful.hooks.userhook_create('untabbed')
awful.hooks.userhook_create('tabdisplay')
awful.hooks.userhook_create('tabhide')

---------------
-- Functions --
---------------

-- Find the key in a table
local function findkey(table, cl)
    for i, c in pairs(table) do
        if c == cl then
            return i
        end
    end
    return nil
end

-- Swap which tab is displayed
local function client_display(tabindex, cl)
    local p = tabbed[tabindex][1]
    
    if cl and p ~= cl then
        cl:unhide()
        cl:swap(p)
        p:hide()
        cl:focus_set()

        tabbed[tabindex][1] = cl

        awful.hooks.userhook_call('tabhide', {p})
        awful.hooks.userhook_call('tabdisplay', {cl})
    end
end

-- Check if the client is in the given tabindex
local function client_in_tabindex(tabindex, cl)
    local c = cl or client.focus_get()
    return findkey(tabbed[tabindex], c)
end

-- Find the tab index or return nil if not tabbed
local function client_find_tabindex(cl)
    local c = cl or client.focus_get()

    for tabindex, tabdisplay in pairs(tabbed) do
        -- Loop through all tab displays
        local me = client_in_tabindex(tabindex, c)

        if me ~= nil then
            return tabindex 
        end
    end

    return nil
end

-- Create a new tabbed display with client as the master
local function client_tab_create(cl)
    local c = cl or client.focus_get()

    table.insert(tabbed, {
        c, -- Window currently being displayed
        { c } -- List of windows in tabbed display
    })

    awful.hooks.userhook_call('tabbed', {c})
    return client_find_tabindex(c)
end

-- Add a client to a tabbed display
local function client_tab(tabindex, cl)
    local c = cl or client.focus_get()

    if tabbed[tabindex] ~= nil and client_find_tabindex(c) == nil then
        table.insert(tabbed[tabindex][2], c)
        client_display(tabindex, c)

        awful.hooks.userhook_call('tabbed', {c})
    end
end

-- Get the next client in a tabdisplay
local function client_next(tabindex, cl)
    local c = cl or tabbed[tabindex][1]

    local i = findkey(tabbed[tabindex][2], c)

    if i == nil then
        return nil
    end

    if tabbed[tabindex][2][i+1] == nil then
        return tabbed[tabindex][2][1]
    else
        return tabbed[tabindex][2][i+1]
    end
end

-- Get the previous client in a tabdisplay
local function client_prev(tabindex, cl)
    local c = cl or tabbed[tabindex][1]

    local i = findkey(tabbed[tabindex][2], c)

    if i == nil then
        return nil
    end

    if tabbed[tabindex][2][i-1] == nil then
        return tabbed[tabindex][2][table.maxn(tabbed[tabindex][2])]
    else
        return tabbed[tabindex][2][i-1]
    end
end

-- Remove a client from a tabbed display
local function client_untab(cl)
    local c = cl or client.focus_get()
    local tabindex = client_find_tabindex(c)
    if tabindex == nil then return false end

    local cindex = findkey(tabbed[tabindex][2], c)

    if tabbed[tabindex][1] == c then
        client_display(tabindex, client_next(tabindex, c))
    end

    table.remove(tabbed[tabindex][2], cindex)

    if table.maxn(tabbed[tabindex][2]) == 0 then
        -- Table is empty now, remove the tabbed display
        table.remove(tabbed, tabindex)
    end

    c:unhide()
    awful.hooks.userhook_call('untabbed', {c})
end

-- Untab all clients in a tabbed display
local function client_untab_all(tabindex)
    for i,c in pairs(tabbed[tabindex][2]) do
        awful.hooks.userhook_call('untabbed', {c})
    end

    if tabbed[tabindex] ~= nil then
        table.remove(tabbed, tabindex)
    end
end

-- Get all clients on a tabbed display
local function client_get_clients(tabindex)
    return tabbed[tabindex][2]
end

-- Get the displayed client on a tabbed display
local function client_get_displayed(tabindex)
    return tabbed[tabindex][1]
end

-- Get a client by tab number
local function client_get_position(tabindex, pos)
    if tabbed[tabindex] == nil then return nil end
    return tabbed[tabindex][2][pos]
end

-- Start autotabbing, this automatically
-- tabs new clients when the current client
-- is tabbed
local function autotab_start()
    awful.hooks.manage(function (c)
        local sel = client.focus_get()
        local index = client_find_tabindex(sel)

        if index ~= nil then
            -- Currently focussed client is tabbed, 
            -- add the new window to the tabbed display
            client_tab(index, c)
        end
    end)
end

-- Set up hook so we don't leave lost hidden clients
awful.hooks.unmanage(function (c) client_untab(c) end)

P.findkey = findkey
P.display = client_display
P.tabindex_check = client_in_tabindex
P.tabindex_get = client_find_tabindex

P.tab_create = client_tab_create
P.tab = client_tab
P.untab = client_untab
P.untab_all = client_untab_all

P.next = client_next
P.prev = client_prev

P.clients_get = client_get_clients
P.displayed_get = client_get_displayed
P.position_get = client_get_position

P.autotab_start = autotab_start

P.tabbed = tabbed

return P
