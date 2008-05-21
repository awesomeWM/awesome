-----------------------------------------------
-- awful: AWesome Function very UsefuL       --
-- Common useful awesome functions           --
--                                           --
-- © 2008 Julien Danjou <julien@danjou.info> --
-----------------------------------------------

-- We usually are required as 'awful'
-- But that can be changed.
local P = {}   -- package
if _REQUIREDNAME == nil then
    awful = P
else
    _G[_REQUIREDNAME] = P
end

-- Grab environment we need
local ipairs = ipairs
local awesome = awesome
local client = client
local tag = tag
local mouse = mouse
local os = os

-- Reset env
setfenv(1, P)

-- Function to the good value in table, cycling
function array_boundandcycle(t, i)
    if i > #t then
        i = 1
    elseif i < 1 then
        i = #t
    end
    return i
end


-- Function to get a client by its relative index:
-- set i to 1 to get next, -1 to get previous.
function client_next(i)
    -- Get all visible clients
    local cls = client.visible_get(mouse.screen_get(), ".*")
    -- Get currently focused client
    local sel = client.focus_get()
    if not sel then return end
    -- Loop upon each client
    for idx, c in ipairs(cls) do
        if c == sel then
            return cls[array_boundandcycle(cls, idx +i)]
        end
    end
end

-- Focus a client by its relative index.
function client_focus(i)
    local c = client_next(i)
    if c then
        c:focus_set()
    end
end

-- Swap a client by its relative index.
function client_swap(i)
    local c = client_next(i)
    local sel = client.focus_get()
    if c and sel then
        sel:swap(c)
    end
end

function screen_focus(i)
    local sel = client.focus_get()
    local s
    if sel then
        s = sel:screen_get()
    else
        s = mouse.screen_get()
    end
    local count = awesome.screen_count()
    s = s + i
    if s < 1 then
        s = count
    elseif s > count then
        s = 1
    end
    awesome.screen_focus(s)
    -- Move the mouse on the screen
    local screen_coords = awesome.screen_coords_get(s)
    mouse.coords_set(screen_coords['x'], screen_coords['y'])
end

-- Return a table with all visible tags
function getselectedtags()
    local idx = 1
    local screen = mouse.screen_get()
    local tags = tag.get(screen, ".*")
    local vtags = {}
    for i, t in ipairs(tags) do
        if t:isselected() then
            vtags[idx] = t
            idx = idx + 1
        end
    end
    return vtags
end

-- Return only the first element of all visible tags,
-- so that's the first visible tags.
local function getselectedtag()
    return getselectedtags()[1]
end

-- Increase master width factor
function tag_incmwfact(i)
    local t = getselectedtag()
    if t then
        t:mwfact_set(t:mwfact_get() + i)
    end
end

-- Increase number of master windows
function tag_incnmaster(i)
    local t = getselectedtag()
    if t then
        t:nmaster_set(t:nmaster_get() + i)
    end
end

-- Increase number of column windows
function tag_incncol(i)
    local t = getselectedtag()
    if t then
        t:ncol_set(t:ncol_get() + i)
    end
end

-- View no tag
function tag_viewnone()
    local tags = tag.get(mouse.screen_get(), ".*")
    for i, t in ipairs(tags) do
        t:view(false)
    end
end

function tag_viewidx(r)
    local tags = tag.get(mouse.screen_get(), ".*")
    local sel = getselectedtag()
    tag_viewnone()
    for i, t in ipairs(tags) do
        if t == sel then
            tags[array_boundandcycle(tags, i - r)]:view(true)
        end
    end
end

-- View next tag
function tag_viewnext()
    return tag_viewidx(1)
end

-- View previous tag
function tag_viewprev()
    return tag_viewidx(-1)
end

function tag_viewonly(t)
    tag_viewnone()
    t:view(true)
end

function client_movetotag(target, c)
    local sel = c or client.focus_get();
    local tags = tag.get(mouse.screen_get(), ".*")
    for i, t in ipairs(tags) do
        sel:tag(t, false)
    end
    sel:tag(target, true)
end

function client_toggletag(target, c)
    local sel = c or client.focus_get();
    if sel then
        sel:tag(target, not sel:istagged(target))
    end
end

function client_togglefloating(c)
    local sel = c or client.focus_get();
    if sel then
        sel:floating_set(not sel:floating_get())
    end
end

function spawn(cmd)
    return os.execute(cmd .. "&")
end

-- Export tags function
P.tag =
{
    viewnone = tag_viewnone;
    viewprev = tag_viewprev;
    viewnext = tag_viewnext;
    viewonly = tag_viewonly;
    incmwfact = tag_incmwfact;
    incncol = tag_incncol;
    incnmaster = tag_incnmaster;
}
P.client =
{
    next = client_next;
    focus = client_focus;
    swap = client_swap;
    movetotag = client_movetotag;
    toggletag = client_toggletag;
    togglefloating = client_togglefloating;
}
P.screen =
{
    focus = screen_focus;
}
P.spawn = spawn

return P
