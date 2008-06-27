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
local string = string
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

P.hooks = {}
P.myhooks = {}
P.prompt = {}
P.completion = {}
P.screen = {}
P.layout = {}
P.client = {}
P.tag = {}

--- Create a new userhook (for external libs).
-- @param name Hook name.
local function userhook_create(name)
    P.myhooks[name] = {}
    P.hooks[name] = function (f)
        table.insert(P.myhooks[name], { callback = f })
    end
end

--- Call a created userhook (for external libs).
-- @param name Hook name.
local function userhook_call(name, args)
    for i, o in pairs(P.myhooks[name]) do
        P.myhooks[name][i]['callback'](unpack(args))
    end
end

--- Make i cycle.
-- @param t A length.
-- @param i An absolute index to fit into #t.
-- @return The object at new index.
local function cycle(t, i)
    while i > t do i = i - t end
    while i < 1 do i = i + t end
    return i
end

--- Get a client by its relative index to the focused window.
-- @usage Set i to 1 to get next, -1 to get previous.
-- @param i The index.
-- @param c Optional client.
-- @return A client, or nil if no client is available.
function P.client.next(i, c)
    -- Get currently focused client
    local sel = c or client.focus_get()
    if sel then
        -- Get all visible clients
        local cls = client.visible_get(sel:screen_get())
        -- Loop upon each client
        for idx, c in ipairs(cls) do
            if c == sel then
                -- Cycle
                return cls[cycle(#cls, idx +i)]
            end
        end
    end
end

--- Focus a client by its relative index.
-- @param i The index.
-- @param c Optional client.
function P.client.focus(i, c)
    local target = P.client.next(i, c)
    if target then
        target:focus_set()
    end
end

--- Swap a client by its relative index.
-- @param i The index.
-- @param c Optional client, otherwise focused one is used.
function P.client.swap(i, c)
    local sel = c or client.focus_get()
    local target = P.client.next(i, sel)
    if target then
        target:swap(sel)
    end
end

--- Get the master window
-- @param screen Optional screen number, otherwise screen mouse is used.
-- @return The master window.
function P.client.master(screen)
    local s = screen or mouse.screen_get()
    return client.visible_get(s)[1]
end

--- Move/resize a client relative to current coordinates.
-- @param x The relative x coordinate.
-- @param y The relative y coordinate.
-- @param w The relative width.
-- @param h The relative height.
-- @param c The optional client, otherwise focused one is used.
function P.client.moveresize(x, y, w, h, c)
    local sel = c or client.focus_get()
    local coords = sel:coords_get()
    coords['x'] = coords['x'] + x
    coords['y'] = coords['y'] + y
    coords['width'] = coords['width'] + w
    coords['height'] = coords['height'] + h
    sel:coords_set(coords)
end

--- Give the focus to a screen, and move pointer.
-- @param Screen number.
function P.screen.focus(i)
    local sel = client.focus_get()
    local s
    if sel then
        s = sel:screen_get()
    else
        s = mouse.screen_get()
    end
    s = cycle(screen.count(), s + i)
    screen.focus(s)
    -- Move the mouse on the screen
    local screen_coords = screen.coords_get(s)
    mouse.coords_set(screen_coords['x'], screen_coords['y'])
end

--- Return a table with all visible tags
-- @param s Screen number.
-- @return A table with all selected tags.
function P.tag.selectedlist(s)
    local screen = s or mouse.screen_get()
    local tags = tag.geti(screen)
    local vtags = {}
    for i, t in pairs(tags) do
        if t:isselected() then
            vtags[#vtags + 1] = t
        end
    end
    return vtags
end

--- Return only the first visible tag.
-- @param s Screen number.
function P.tag.selected(s)
    return P.tag.selectedlist(s)[1]
end

--- Set master width factor.
-- @param mwfact Master width factor.
function P.tag.setmwfact(mwfact)
    local t = P.tag.selected()
    if t then
        t:mwfact_set(mwfact)
    end
end

--- Increase master width factor.
-- @param add Value to add to master width factor.
function P.tag.incmwfact(add)
    local t = P.tag.selected()
    if t then
        t:mwfact_set(t:mwfact_get() + add)
    end
end

--- Set the number of master windows.
-- @param nmaster The number of master windows.
function P.tag.setnmaster(nmaster)
    local t = P.tag.selected()
    if t then
        t:nmaster_set(nmaster)
    end
end

--- Increase the number of master windows.
-- @param add Value to add to number of master windows.
function P.tag.incnmaster(add)
    local t = P.tag.selected()
    if t then
        t:nmaster_set(t:nmaster_get() + add)
    end
end

--- Set number of column windows.
-- @param ncol The number of column.
function P.tag.setncol(ncol)
    local t = P.tag.selected()
    if t then
        t:ncol_set(ncol)
    end
end

--- Increase number of column windows.
-- @param add Value to add to number of column windows.
function P.tag.incncol(add)
    local t = P.tag.selected()
    if t then
        t:ncol_set(t:ncol_get() + add)
    end
end

--- View no tag.
-- @param Optional screen number.
function P.tag.viewnone(screen)
    local tags = tag.get(screen or mouse.screen_get())
    for i, t in pairs(tags) do
        t:view(false)
    end
end

--- View a tag by its index.
-- @param i The relative index to see.
-- @param screen Optional screen number.
function P.tag.viewidx(i, screen)
    local tags = tag.geti(screen or mouse.screen_get())
    local sel = P.tag.selected()
    P.tag.viewnone()
    for k, t in ipairs(tags) do
        if t == sel then
            tags[cycle(#tags, k + i)]:view(true)
        end
    end
end

--- View next tag. This is the same as tag.viewidx(1).
function P.tag.viewnext()
    return P.tag.viewidx(1)
end

--- View previous tag. This is the same a tag.viewidx(-1).
function P.tag.viewprev()
    return P.tag.viewidx(-1)
end

--- View only a tag.
-- @param t The tag object.
function P.tag.viewonly(t)
    P.tag.viewnone()
    t:view(true)
end

--- View only a set of tags.
-- @param tags A table with tags to view only.
-- @param screen Optional screen number of the tags.
function P.tag.viewmore(tags, screen)
    P.tag.viewnone(screen)
    for i, t in pairs(tags) do
        t:view(true)
    end
end

--- Move a client to a tag.
-- @param target The tag to move the client to.
-- @para c Optional client to move, otherwise the focused one is used.
function P.client.movetotag(target, c)
    local sel = c or client.focus_get();
    local tags = tag.get(sel:screen_get())
    for k, t in pairs(tags) do
        sel:tag(t, false)
    end
    sel:tag(target, true)
end

--- Toggle a tag on a client.
-- @param target The tag to toggle.
-- @param c Optional client to toggle, otherwise the focused one is used.
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

--- Toggle the floating status of a client.
-- @param c Optional client, the focused on if not set.
function P.client.togglefloating(c)
    local sel = c or client.focus_get();
    if sel then
        sel:floating_set(not sel:floating_get())
    end
end

--- Move a client to a screen. Default is next screen, cycling.
-- @param c The client to move.
-- @param s The screen number, default to current + 1.
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

--- Get the current layout name.
-- @param screen The screen number.
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

--- Mark a client, and then call 'marked' hook.
-- @param c The client to mark, the focused one if not specified.
-- @return True if the client has been marked. False if the client was already marked.
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

--- Unmark a client and then call 'unmarked' hook.
-- @param c The client to unmark, or the focused one if not specified.
-- @return True if the client has been unmarked. False if the client was not marked.
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

--- Check if a client is marked.
-- @param c The client to check, or the focused one otherwise.
function P.client.ismarked(c)
    local cl = c or client.focus_get()
    if cl then
        for k, v in pairs(awfulmarked) do
            if cl == v then
                return true
            end
        end
    end
    return false
end

--- Toggle a client as marked.
-- @param c The client to toggle mark.
function P.client.togglemarked(c)
    local cl = c or client.focus_get()

    if not P.client.mark(c) then
        P.client.unmark(c)
    end
end

--- Return the marked clients and empty the marked table.
-- @return A table with all marked clients.
function P.client.getmarked()
    for k, v in pairs(awfulmarked) do
        userhook_call('unmarked', {v})
    end

    t = awfulmarked
    awfulmarked = {}
    return t
end

--- Change the layout of the current tag.
-- @param layouts A table of layouts.
-- @param i Relative index.
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

--- Set the layout of the current tag by name.
-- @param layout Layout name.
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

                    for i, o in pairs(P.myhooks[name]) do
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

--- Spawn a program.
-- @param cmd The command.
-- @return The os.execute() return value.
function P.spawn(cmd)
    return os.execute(cmd .. "&")
end

--- Eval Lua code.
-- @return The return value of Lua code.
function P.eval(s)
    return assert(loadstring("return " .. s))()
end

--- Use bash completion system to complete command and filename.
-- @param command The command line.
-- @param cur_pos The cursor position.
-- @paran ncomp The element number to complete.
-- @return The new commande and the new cursor position.
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

--- Draw the prompt text with a cursor.
-- @param text The text.
-- @param text_color The text color.
-- @param cursor_color The cursor color.
-- @param cursor_pos The cursor position.
local function prompt_text_with_cursor(text, text_color, cursor_color, cursor_pos)
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

--- Run a prompt in a box.
-- @param args A table with optional arguments: cursor_fg, cursor_bg, prompt.
-- @param textbox The textbox to use for the prompt.
-- @param exe_callback The callback function to call with command as argument when finished.
-- @param completion_callback The callback function to call to get completion.
function P.prompt(args, textbox, exe_callback, completion_callback)
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
    textbox.text = prompt .. prompt_text_with_cursor(text, inv_col, cur_col, cur_pos)
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
                textbox.text = ""
                return false
            end
        else
            if key == "Return" then
                textbox.text = ""
                exe_callback(command)
                return false
            elseif key == "Escape" then
                textbox.text = ""
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
                if string.len(key) == 1 then
                    command = command:sub(1, cur_pos - 1) .. key .. command:sub(cur_pos)
                    cur_pos = cur_pos + 1
                end
            end
            if cur_pos < 1 then
                cur_pos = 1
            elseif cur_pos > #command + 1 then
                cur_pos = #command + 1
            end
        end

        -- Update textbox
        textbox.text = prompt .. prompt_text_with_cursor(command, inv_col, cur_col, cur_pos)

        return true
    end)
end

--- Escape a string from XML char.
-- Useful to set raw text in textbox.
-- @param text Text to escape.
-- @return Escape text.
function P.escape(text)
    text = text:gsub("&", "&amp;")
    text = text:gsub("<", "&lt;")
    text = text:gsub(">", "&gt;")
    text = text:gsub("'", "&apos;")
    text = text:gsub("\"", "&quot;")
    return text
end

--- Unescape a string from entities.
-- @param text Text to unescape.
-- @return Unescaped text.
function P.unescape(text)
    text = text:gsub("&amp;", "&")
    text = text:gsub("&lt;", "<")
    text = text:gsub("&gt;", ">")
    text = text:gsub("&apos;", "'")
    text = text:gsub("&quot;", "\"")
    return text
end

return P
