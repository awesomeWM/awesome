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
local assert = assert
local loadstring = loadstring
local ipairs = ipairs
local pairs = pairs
local unpack = unpack
local awesome = awesome
local screen = screen
local client = client
local tag = tag
local mouse = mouse
local os = os
local table = table
local hooks = hooks
local keygrabber = keygrabber

-- Reset env
setfenv(1, P)

-- Hook functions, wrappers around awesome's hooks. functions so we
-- can easily add multiple functions per hook.
P.hooks = {}
P.myhooks = {}

-- Create a new userhook (for external libs)
local function userhook_create(name)
    P.myhooks[name] = {}
    P.hooks[name] = function (f)
        table.insert(P.myhooks[name], {callback = f})
    end
end

-- Call a created userhook (for external libs
local function userhook_call(name, args)
    for i,o in pairs(P.myhooks[name]) do
       P.myhooks[name][i]['callback'](unpack(args))
    end
end

-- Function to the good value in table, cycling
local function array_boundandcycle(t, i)
    if i > #t then
        i = 1
    elseif i < 1 then
        i = #t
    end
    return i
end

-- Function to get a client by its relative index:
-- set i to 1 to get next, -1 to get previous.
local function client_next(i)
    -- Get all visible clients
    local cls = client.visible_get(mouse.screen_get())
    -- Get currently focused client
    local sel = client.focus_get()
    if not sel then return end
    -- Loop upon each client
    for idx, c in ipairs(cls) do
        if c == sel then
            return cls[array_boundandcycle(cls, idx + i)]
        end
    end
end

-- Focus a client by its relative index.
local function client_focus(i)
    local c = client_next(i)
    if c then
        c:focus_set()
    end
end

-- Swap a client by its relative index.
local function client_swap(i)
    local c = client_next(i)
    local sel = client.focus_get()
    if c and sel then
        sel:swap(c)
    end
end

-- Move/resize a client relativ to current coordinates.
local function client_moveresize(x, y, w, h)
    local sel = client.focus_get()
    local coords = sel:coords_get()
    coords['x'] = coords['x'] + x
    coords['y'] = coords['y'] + y
    coords['width'] = coords['width'] + w
    coords['height'] = coords['height'] + h
    sel:coords_set(coords)
end

local function screen_focus(i)
    local sel = client.focus_get()
    local s
    if sel then
        s = sel:screen_get()
    else
        s = mouse.screen_get()
    end
    local count = screen.count()
    s = s + i
    if s < 1 then
        s = count
    elseif s > count then
        s = 1
    end
    screen.focus(s)
    -- Move the mouse on the screen
    local screen_coords = screen.coords_get(s)
    mouse.coords_set(screen_coords['x'], screen_coords['y'])
end

-- Return a table with all visible tags
local function tag_selectedlist(s)
    local idx = 1
    local screen = s or mouse.screen_get()
    local tags = tag.get(screen)
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
local function tag_selected(s)
    return tag_selectedlist(s)[1]
end

-- Set master width factor
local function tag_setmwfact(i)
    local t = tag_selected()
    if t then
        t:mwfact_set(i)
    end
end

-- Increase master width factor
local function tag_incmwfact(i)
    local t = tag_selected()
    if t then
        t:mwfact_set(t:mwfact_get() + i)
    end
end

-- Set number of master windows
local function tag_setnmaster(i)
    local t = tag_selected()
    if t then
        t:nmaster_set(i)
    end
end

-- Increase number of master windows
local function tag_incnmaster(i)
    local t = tag_selected()
    if t then
        t:nmaster_set(t:nmaster_get() + i)
    end
end

-- Set number of column windows
local function tag_setncol(i)
    local t = tag_selected()
    if t then
        t:ncol_set(i)
    end
end

-- Increase number of column windows
local function tag_incncol(i)
    local t = tag_selected()
    if t then
        t:ncol_set(t:ncol_get() + i)
    end
end

-- View no tag
local function tag_viewnone()
    local tags = tag.get(mouse.screen_get())
    for i, t in ipairs(tags) do
        t:view(false)
    end
end

local function tag_viewidx(r)
    local tags = tag.get(mouse.screen_get())
    local sel = tag_selected()
    tag_viewnone()
    for i, t in ipairs(tags) do
        if t == sel then
            tags[array_boundandcycle(tags, i + r)]:view(true)
        end
    end
end

-- View next tag
local function tag_viewnext()
    return tag_viewidx(1)
end

-- View previous tag
local function tag_viewprev()
    return tag_viewidx(-1)
end

local function tag_viewonly(t)
    tag_viewnone()
    t:view(true)
end

local function tag_viewmore(tags)
    tag_viewnone()
    for i, t in ipairs(tags) do
        t:view(true)
    end
end

local function client_movetotag(target, c)
    local sel = c or client.focus_get();
    local tags = tag.get(mouse.screen_get())
    for i, t in ipairs(tags) do
        sel:tag(t, false)
    end
    sel:tag(target, true)
end

local function client_toggletag(target, c)
    local sel = c or client.focus_get();
    local toggle = false
    -- Count how many tags has the client
    -- an only toggle tag if the client has at least one tag other than target
    for k, v in ipairs(tag.get(sel:screen_get())) do
        if target ~= v and sel:istagged(v) then
            toggle = true
            break
        end
    end
    if toggle and sel then
        sel:tag(target, not sel:istagged(target))
    end
end

local function client_togglefloating(c)
    local sel = c or client.focus_get();
    if sel then
        sel:floating_set(not sel:floating_get())
    end
end

-- Move a client to a screen. Default is next screen, cycling.
local function client_movetoscreen(c, s)
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

local function layout_get(screen)
    local t = tag_selected(screen)
    if t then
        return t:layout_get()
    end
end

-- Just set an awful mark to a client to move it later.
local awfulmarked = {}
userhook_create('marked')
userhook_create('unmarked')

-- Mark a client
local function client_mark (c)
    local cl = c or client.focus_get()
    if cl then
        for k, v in pairs(awfulmarked) do
            if cl == v then
                return false
            end
        end

        table.insert(awfulmarked, cl)

        -- Call callback
        userhook_call('marked', {cl})
        return true
    end
end

-- Unmark a client
local function client_unmark(c)
    local cl = c or client.focus_get()

    for k, v in pairs(awfulmarked) do
        if cl == v then
            table.remove(awfulmarked, k)
            userhook_call('unmarked', {cl})
            return true
        end
    end

    return false
end

-- Check if marked
local function client_ismarked(c)
    local cl = c or client.focus_get()
    if cl then
        for k, v in pairs(awfulmarked) do
            if cl == v then
                return true
            end
        end

        return false
    end
end

-- Toggle marked
local function client_togglemarked(c)
    local cl = c or client.focus_get()

    if not client_mark(c) then
        client_unmark(c)
    end
end

-- Return the marked clients and empty the table
local function client_getmarked ()
    for k, v in pairs(awfulmarked) do
        userhook_call('unmarked', {v})
    end

    t = awfulmarked
    awfulmarked = {}
    return t
end

-- Function to change the layout of the current tag.
-- layouts = table of layouts (define in .awesomerc.lua)
-- i = relative index
local function layout_inc(layouts, i)
    local t = tag_selected()
    local number_of_layouts = 0
    local rev_layouts = {}
    for i, v in ipairs(layouts) do
	rev_layouts[v] = i
	number_of_layouts = number_of_layouts + 1
    end
    if t then
	local cur_layout = layout_get()
	local new_layout_index = (rev_layouts[cur_layout] + i) % number_of_layouts
	if new_layout_index == 0 then
	    new_layout_index = number_of_layouts
	end
	t:layout_set(layouts[new_layout_index])
    end
end

-- function to set the layout of the current tag by name.
local function layout_set(layout)
    local t = tag_selected()
    if t then
	t:layout_set(layout)
    end
end

P.hooks['userhook_create'] = userhook_create
P.hooks['userhook_call'] = userhook_call

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

local function spawn(cmd)
    return os.execute(cmd .. "&")
end

local function eval(cmd)
    assert(loadstring(cmd))()
end

-- Menu functions
local function menu_text_with_cursor(text, text_color, cursor_color, cursor_pos)
    local char
    if not text then text = "" end
    if #text < cursor_pos then
        char = " "
    else
        char = escape(text:sub(cursor_pos, cursor_pos))
    end
    local text_start = escape(text:sub(1, cursor_pos - 1))
    local text_end = escape(text:sub(cursor_pos + 1))
    return text_start .. "<span background=\"" .. cursor_color .. "\" foreground=\"" .. text_color .. "\">" .. char .. "</span>" .. text_end
end

local function menu(args, textbox, exe_callback)
    if not args then return end
    local command = ""
    local prompt = args.prompt or ""
    local inv_col = args.cursor_fg or "black"
    local cur_col = args.cursor_bg or "white"
    local cur_pos = 1
    if not textbox or not exe_callback then
        return
    end
    textbox:set("text", prompt .. menu_text_with_cursor(text, inv_col, cur_col, cur_pos))
    keygrabber.run(
    function (mod, key)
        local has_ctrl = false

        -- Compute modifiers keys
        for k, v in ipairs(mod) do
            if v == "Control" then has_ctrl = true end
        end

        -- Get out cases
        if key == "Return" then
            textbox:set("text", "")
            exe_callback(command)
            return false
        elseif key == "Escape" then
            textbox:set("text", "")
            return false
        end

        -- Control cases
        if has_ctrl then
            if key == "a" then
                cur_pos = 1
            elseif key == "e" then
                cur_pos = #command + 1
            end
        else
            -- Typin cases
            if key == "BackSpace" then
                if cur_pos > 1 then
                    command = command:sub(1, cur_pos - 2) .. command:sub(cur_pos)
                    cur_pos = cur_pos - 1
                end
            -- That's DEL
            elseif key:byte() == 127 then
                command = command:sub(1, cur_pos - 1) .. command:sub(cur_pos + 1)
            elseif key == "Left" then
                cur_pos = cur_pos - 1
            elseif key == "Right" then
                cur_pos = cur_pos + 1
            else
                command = command:sub(1, cur_pos - 1) .. key .. command:sub(cur_pos)
                cur_pos = cur_pos + 1
            end
            if cur_pos < 1 then
                cur_pos = 1
            elseif cur_pos > #command + 1 then
                cur_pos = #command + 1
            end
        end

        -- Update textbox
        textbox:set("text", prompt .. menu_text_with_cursor(command, inv_col, cur_col, cur_pos))

        return true
    end)
end

local function escape(text)
    text = text:gsub("&", "&amp;")
    text = text:gsub("<", "&lt;")
    text = text:gsub(">", "&gt;")
    text = text:gsub("'", "&apos;")
    text = text:gsub("\"", "&quot;")
    return text
end

local function unescape(text)
    text = text:gsub("&amp;", "&")
    text = text:gsub("&lt;", "<")
    text = text:gsub("&gt;", ">")
    text = text:gsub("&apos;", "'")
    text = text:gsub("&quot;", "\"")
    return text
end

-- Export tags function
P.tag =
{
    viewnone = tag_viewnone;
    viewprev = tag_viewprev;
    viewnext = tag_viewnext;
    viewonly = tag_viewonly;
    viewmore = tag_viewmore;
    setmwfact = tag_setmwfact;
    incmwfact = tag_incmwfact;
    setncol = tag_setncol;
    incncol = tag_incncol;
    setnmaster = tag_setnmaster;
    incnmaster = tag_incnmaster;
    selected = tag_selected;
    selectedlist = tag_selectedlist;
}
P.client =
{
    next = client_next;
    focus = client_focus;
    swap = client_swap;
    movetotag = client_movetotag;
    toggletag = client_toggletag;
    togglefloating = client_togglefloating;
    moveresize = client_moveresize;
    movetoscreen = client_movetoscreen;
    mark = client_mark;
    unmark = client_unmark;
    ismarked = client_ismarked;
    togglemarked = client_togglemarked;
    getmarked = client_getmarked;
}
P.screen =
{
    focus = screen_focus;
}
P.layout =
{
    get = layout_get;
    set = layout_set;
    inc = layout_inc;
}
P.spawn = spawn
P.menu = menu
P.escape = escape
P.eval = eval

return P
