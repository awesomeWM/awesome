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
local io = io

-- Reset env
setfenv(1, P)

-- Hook functions, wrappers around awesome's hooks. functions so we
-- can easily add multiple functions per hook.
P.hooks = {}
P.myhooks = {}
P.menu = {}
P.screen = {}
P.layout = {}
P.completion = {}
P.client = {}
P.tag = {}

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
function P.client.next(i)
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
function P.client.focus(i)
    local c = P.client.next(i)
    if c then
        c:focus_set()
    end
end

-- Swap a client by its relative index.
function P.client.swap(i)
    local c = P.client.next(i)
    local sel = client.focus_get()
    if c and sel then
        sel:swap(c)
    end
end

function P.client.master()
    return client.visible_get(mouse.screen_get())[1]
end

-- Move/resize a client relativ to current coordinates.
function P.client.moveresize(x, y, w, h)
    local sel = client.focus_get()
    local coords = sel:coords_get()
    coords['x'] = coords['x'] + x
    coords['y'] = coords['y'] + y
    coords['width'] = coords['width'] + w
    coords['height'] = coords['height'] + h
    sel:coords_set(coords)
end

function P.screen.focus(i)
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
function P.tag.selectedlist(s)
    local idx = 1
    local screen = s or mouse.screen_get()
    local tags = tag.geti(screen)
    local vtags = {}
    for i, t in pairs(tags) do
        if t:isselected() then
            vtags[idx] = t
            idx = idx + 1
        end
    end
    return vtags
end

-- Return only the first element of all visible tags,
-- so that's the first visible tags.
function P.tag.selected(s)
    return P.tag.selectedlist(s)[1]
end

-- Set master width factor
function P.tag.setmwfact(i)
    local t = P.tag.selected()
    if t then
        t:mwfact_set(i)
    end
end

-- Increase master width factor
function P.tag.incmwfact(i)
    local t = P.tag.selected()
    if t then
        t:mwfact_set(t:mwfact_get() + i)
    end
end

-- Set number of master windows
function P.tag.setnmaster(i)
    local t = P.tag.selected()
    if t then
        t:nmaster_set(i)
    end
end

-- Increase number of master windows
function P.tag.incnmaster(i)
    local t = P.tag.selected()
    if t then
        t:nmaster_set(t:nmaster_get() + i)
    end
end

-- Set number of column windows
function P.tag.setncol(i)
    local t = P.tag.selected()
    if t then
        t:ncol_set(i)
    end
end

-- Increase number of column windows
function P.tag.incncol(i)
    local t = P.tag.selected()
    if t then
        t:ncol_set(t:ncol_get() + i)
    end
end

-- View no tag
function P.tag.viewnone()
    local tags = tag.get(mouse.screen_get())
    for i, t in pairs(tags) do
        t:view(false)
    end
end

function P.tag.viewidx(r)
    local tags = tag.geti(mouse.screen_get())
    local sel = P.tag.selected()
    P.tag.viewnone()
    for i, t in ipairs(tags) do
        if t == sel then
            tags[array_boundandcycle(tags, i + r)]:view(true)
        end
    end
end

-- View next tag
function P.tag.viewnext()
    return P.tag.viewidx(1)
end

-- View previous tag
function P.tag.viewprev()
    return P.tag.viewidx(-1)
end

function P.tag.viewonly(t)
    P.tag.viewnone()
    t:view(true)
end

function P.tag.viewmore(tags)
    P.tag.viewnone()
    for i, t in pairs(tags) do
        t:view(true)
    end
end

function P.client.movetotag(target, c)
    local sel = c or client.focus_get();
    local tags = tag.get(mouse.screen_get())
    for k, t in pairs(tags) do
        sel:tag(t, false)
    end
    sel:tag(target, true)
end

function P.client.toggletag(target, c)
    local sel = c or client.focus_get();
    local toggle = false
    if sel then
        -- Count how many tags has the client
        -- an only toggle tag if the client has at least one tag other than target
        for k, v in pairs(tag.get(sel:screen_get())) do
            if target ~= v and sel:istagged(v) then
                toggle = true
                break
            end
        end
        if toggle then
            sel:tag(target, not sel:istagged(target))
        end
    end
end

function P.client.togglefloating(c)
    local sel = c or client.focus_get();
    if sel then
        sel:floating_set(not sel:floating_get())
    end
end

-- Move a client to a screen. Default is next screen, cycling.
function P.client.movetoscreen(c, s)
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

function P.layout.get(screen)
    local t = P.tag.selected(screen)
    if t then
        return t:layout_get()
    end
end

-- Just set an awful mark to a client to move it later.
local awfulmarked = {}
userhook_create('marked')
userhook_create('unmarked')

-- Mark a client
function P.client.mark (c)
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
function P.client.unmark(c)
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
function P.client.ismarked(c)
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
function P.client.togglemarked(c)
    local cl = c or client.focus_get()

    if not P.client.mark(c) then
        P.client.unmark(c)
    end
end

-- Return the marked clients and empty the table
function P.client.getmarked()
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
function P.layout.inc(layouts, i)
    local t = P.tag.selected()
    local number_of_layouts = 0
    local rev_layouts = {}
    for i, v in ipairs(layouts) do
	rev_layouts[v] = i
	number_of_layouts = number_of_layouts + 1
    end
    if t then
	local cur_layout = layout.get()
	local new_layout_index = (rev_layouts[cur_layout] + i) % number_of_layouts
	if new_layout_index == 0 then
	    new_layout_index = number_of_layouts
	end
	t:layout_set(layouts[new_layout_index])
    end
end

-- function to set the layout of the current tag by name.
function P.layout.set(layout)
    local t = P.tag.selected()
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

function P.spawn(cmd)
    return os.execute(cmd .. "&")
end

function P.eval(s)
    return assert(loadstring("return " .. s))()
end

function P.completion.bash(command, cur_pos, ncomp)
    local wstart = 1
    local wend = 1
    local words = {}
    local cword_index = 0
    local cword_start = 0
    local cword_end = 0
    local i = 1
    local comptype = "file"

    -- do nothing if we are on a letter, i.e. not at len + 1 or on a space
    if cur_pos ~= #command + 1 and command:sub(cur_pos, cur_pos) ~= " " then
        return command, cur_pos
    elseif #command == 0 then
        return command, cur_pos
    end

    while wend <= #command do
        wend = command:find(" ", wstart)
        if not wend then wend = #command + 1 end
        table.insert(words, command:sub(wstart, wend - 1))
        if cur_pos >= wstart and cur_pos <= wend + 1 then
            cword_start = wstart
            cword_end = wend
            cword_index = i
        end
        wstart = wend + 1
        i = i + 1
    end

    if cword_index == 1 then
        comptype = "command"
    end

    local c = io.popen("/usr/bin/env bash -c 'compgen -A " .. comptype .. " " .. words[cword_index] .. "'")
    local output = {}
    i = 0
    while true do
        local line = c:read("*line")
        if not line then break end
        table.insert(output, line)
    end

    c:close()

    -- no completion, return
    if #output == 0 then
        return command, cur_pos
    end

    -- cycle
    while ncomp > #output do
        ncomp = ncomp - #output
    end

    local str = command:sub(1, cword_start - 1) .. output[ncomp] .. command:sub(cword_end)
    cur_pos = cword_end + #output[ncomp] + 1

    return str, cur_pos
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

function P.menu(args, textbox, exe_callback, completion_callback)
    if not args then return end
    local command = ""
    local command_before_comp
    local cur_pos_before_comp
    local prompt = args.prompt or ""
    local inv_col = args.cursor_fg or "black"
    local cur_col = args.cursor_bg or "white"
    -- The cursor position
    local cur_pos = 1
    -- The completion element to use on completion request.
    local ncomp = 1
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
        if has_ctrl then
            if key == "g" then
                textbox:set("text", "")
                return false
            end
        else
            if key == "Return" then
                textbox:set("text", "")
                exe_callback(command)
                return false
            elseif key == "Escape" then
                textbox:set("text", "")
                return false
            end
        end

        -- Control cases
        if has_ctrl then
            if key == "a" then
                cur_pos = 1
            elseif key == "e" then
                cur_pos = #command + 1
            elseif key == "w" then
                local wstart = 1
                local wend = 1
                local cword_start = 1
                local cword_end = 1
                while wend < cur_pos do
                    wend = command:find(" ", wstart)
                    if not wend then wend = #command + 1 end
                    if cur_pos >= wstart and cur_pos <= wend + 1 then
                        cword_start = wstart
                        cword_end = cur_pos - 1
                        break
                    end
                    wstart = wend + 1
                end
                command = command:sub(1, cword_start - 1) .. command:sub(cword_end + 1)
                cur_pos = cword_start
            end
        else
            if completion_callback then
                -- That's tab
                if key:byte() == 9 then
                    if ncomp == 1 then
                        command_before_comp = command
                        cur_pos_before_comp = cur_pos
                    end
                    command, cur_pos = completion_callback(command_before_comp, cur_pos_before_comp, ncomp)
                    ncomp = ncomp + 1
                    key = ""
                else
                    ncomp = 1
                end
            end

            -- Typin cases
            if key == "Home" then
                cur_pos = 1
            elseif key == "End" then
                cur_pos = #command + 1
            elseif key == "BackSpace" then
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

function P.escape(text)
    text = text:gsub("&", "&amp;")
    text = text:gsub("<", "&lt;")
    text = text:gsub(">", "&gt;")
    text = text:gsub("'", "&apos;")
    text = text:gsub("\"", "&quot;")
    return text
end

function P.unescape(text)
    text = text:gsub("&amp;", "&")
    text = text:gsub("&lt;", "<")
    text = text:gsub("&gt;", ">")
    text = text:gsub("&apos;", "'")
    text = text:gsub("&quot;", "\"")
    return text
end

return P
