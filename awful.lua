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
local pairs = pairs
local awesome = awesome
local screen = screen
local client = client
local workspace = workspace
local mouse = mouse
local os = os
local table = table
local hooks = hooks

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
    local cls = client.visible_get(mouse.screen_get())
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

-- Move/resize a client relativ to current coordinates.
function client_moveresize(x, y, w, h)
    local sel = client.focus_get()
    local coords = sel:coords_get()
    coords['x'] = coords['x'] + x
    coords['y'] = coords['y'] + y
    coords['width'] = coords['width'] + w
    coords['height'] = coords['height'] + h
    sel:coords_set(coords)
end

function screen_focus(i)
    local s = mouse.screen_get()
    local count = screen.count()
    s = s + i
    if s < 1 then
        s = count
    elseif s > count then
        s = 1
    end
    local ws = screen.workspace_get(s)
    if ws then
        ws:focus_set()
    end
    -- Move the mouse on the screen
    local screen_coords = screen.coords_get(s)
    mouse.coords_set(screen_coords['x'], screen_coords['y'])
end

-- Set master width factor
function workspace_setmwfact(i)
    local t = workspace_selected()
    if t then
        t:mwfact_set(i)
    end
end

-- Increase master width factor
function workspace_incmwfact(i)
    local t = workspace_selected()
    if t then
        t:mwfact_set(t:mwfact_get() + i)
    end
end

-- Set number of master windows
function workspace_setnmaster(i)
    local t = workspace_selected()
    if t then
        t:nmaster_set(i)
    end
end

-- Increase number of master windows
function workspace_incnmaster(i)
    local t = workspace_selected()
    if t then
        t:nmaster_set(t:nmaster_get() + i)
    end
end

-- Set number of column windows
function workspace_setncol(i)
    local t = workspace_selected()
    if t then
        t:ncol_set(i)
    end
end

-- Increase number of column windows
function workspace_incncol(i)
    local t = workspace_selected()
    if t then
        t:ncol_set(t:ncol_get() + i)
    end
end

function workspace_viewidx(r)
    local workspaces = workspace.get()
    local sel = workspace.visible_get(mouse.screen_get())
    for i, t in ipairs(workspaces) do
        if t == sel then
            workspaces[array_boundandcycle(workspaces, i + r)]:screen_set(mouse.screen_get())
        end
    end
end

-- View next workspace
function workspace_viewnext()
    return workspace_viewidx(1)
end

-- View previous workspace
function workspace_viewprev()
    return workspace_viewidx(-1)
end

function client_togglefloating(c)
    local sel = c or client.focus_get();
    if sel then
        sel:floating_set(not sel:floating_get())
    end
end

-- Move a client to a screen. Default is next screen, cycling.
function client_movetoscreen(c, s)
    local sel = c or client.focus_get();
    if sel then
        local sc = screen.count()
        if not s then
            s = sel:screen_get() + 1
        end
        if s > sc then s = 1 elseif s < 1 then s = sc end
        sel:screen_set(s)
    end
end

-- Function to change the layout of the current workspace.
-- layouts = table of layouts (define in .awesomerc.lua)
-- i = relative index
function layout_inc(layouts, i)
    local t = workspace.visible_get(mouse.screen_get())
    local number_of_layouts = 0
    local rev_layouts = {}
    for i, v in ipairs(layouts) do
	rev_layouts[v] = i
	number_of_layouts = number_of_layouts + 1
    end
    if t then
	local cur_layout = t:layout_get()
	local new_layout_index = (rev_layouts[cur_layout] + i) % number_of_layouts
	if new_layout_index == 0 then
	    new_layout_index = number_of_layouts
	end
	t:layout_set(layouts[new_layout_index])
    end
end

-- function to set the layout of the current workspace by name.
function layout_set(layout)
    local t = workspace.visible_get(mouse.screen_get())
    if t then
	t:layout_set(layout)
    end
end

-- Hook functions, wrappers around awesome's hooks. functions so we
-- can easily add multiple functions per hook.
P.hooks = {}
P.myhooks = {}

for name, hook in pairs(hooks) do
    if name ~= 'timer' then
        P.hooks[name] = function (f)

            if P.myhooks[name] == nil then
                P.myhooks[name] = {}
                hooks[name](function (...)

                    for i,o in pairs(P.myhooks[name]) do
                       P.myhooks[name][i]['callback'](...)
                    end

                end)
            end

            table.insert(P.myhooks[name], {callback = f})
        end
    else
        P.hooks[name] = function (time, f, runnow)

            if P.myhooks[name] == nil then
                P.myhooks[name] = {}
                hooks[name](1, function (...)

                    for i,o in pairs(P.myhooks[name]) do
                        if P.myhooks[name][i]['counter'] >= P.myhooks[name][i]['timer'] then
                            P.myhooks[name][i]['counter'] = 1
                            P.myhooks[name][i]['callback'](...)
                        else
                            P.myhooks[name][i]['counter'] = P.myhooks[name][i]['counter']+1
                        end
                    end

                end)
            end

            if runnow then
                table.insert(P.myhooks[name], {callback = f, timer = time, counter = time})
            else
                table.insert(P.myhooks[name], {callback = f, timer = time, counter = 0})
            end
        end
    end
end

function spawn(cmd)
    return os.execute(cmd .. "&")
end

-- Export workspaces function
P.workspace =
{
    viewprev = workspace_viewprev;
    viewnext = workspace_viewnext;
    setmwfact = workspace_setmwfact;
    incmwfact = workspace_incmwfact;
    setncol = workspace_setncol;
    incncol = workspace_incncol;
    setnmaster = workspace_setnmaster;
    incnmaster = workspace_incnmaster;
}
P.client =
{
    next = client_next;
    focus = client_focus;
    swap = client_swap;
    movetoworkspace = client_movetoworkspace;
    togglefloating = client_togglefloating;
    moveresize = client_moveresize;
    movetoscreen = client_movetoscreen;
}
P.screen =
{
    focus = screen_focus;
}
P.layout =
{
    set = layout_set;
    inc = layout_inc;
}
P.spawn = spawn

return P
