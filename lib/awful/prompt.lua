---------------------------------------------------------------------------
--- Prompt module for awful.
--
-- **Keyboard navigation**:
--
-- The following readline keyboard shortcuts are implemented as expected:
-- <table class='widget_list' border=1>
--   <tr><th>Name</th><th>Usage</th></tr>
--   <tr><td><kbd>CTRL+A</kbd></td><td>beginning-of-line</td></tr>
--   <tr><td><kbd>CTRL+B</kbd></td><td>backward-char</td></tr>
--   <tr><td><kbd>CTRL+C</kbd></td><td>cancel</td></tr>
--   <tr><td><kbd>CTRL+D</kbd></td><td>delete-char</td></tr>
--   <tr><td><kbd>CTRL+E</kbd></td><td>end-of-line</td></tr>
--   <tr><td><kbd>CTRL+J</kbd></td><td>accept-line</td></tr>
--   <tr><td><kbd>CTRL+M</kbd></td><td>accept-line</td></tr>
--   <tr><td><kbd>CTRL+F</kbd></td><td>move-cursor-right</td></tr>
--   <tr><td><kbd>CTRL+H</kbd></td><td>backward-delete-char</td></tr>
--   <tr><td><kbd>CTRL+K</kbd></td><td>kill-line</td></tr>
--   <tr><td><kbd>CTRL+U</kbd></td><td>unix-line-discard</td></tr>
--   <tr><td><kbd>CTRL+W</kbd></td><td>unix-word-rubout</td></tr>
--   <tr><td><kbd>CTRL+BACKSPACE</kbd></td><td>unix-word-rubout</td></tr>
--   <tr><td><kbd>SHIFT+INSERT</kbd></td><td>paste</td></tr>
--   <tr><td><kbd>HOME</kbd></td><td>beginning-of-line</td></tr>
--   <tr><td><kbd>END</kbd></td><td>end-of-line</td></tr>
-- </table>
--
-- The following shortcuts implement additional history manipulation commands
-- where the search term is defined as the substring of the command from first
-- character to cursor position.
--
-- * <kbd>CTRL+R</kbd>: reverse history search, matches any history entry
-- containing search term.
-- * <kbd>CTRL+S</kbd>: forward history search, matches any history entry
-- containing search term.
-- * <kbd>CTRL+UP</kbd>: ZSH up line or search, matches any history entry
-- starting with search term.
-- * <kbd>CTRL+DOWN</kbd>: ZSH down line or search, matches any history
-- entry starting with search term.
-- * <kbd>CTRL+DELETE</kbd>: delete the currently visible history entry from
-- history file. This does not delete new commands or history entries under
-- user editing.
--
-- **Basic usage**:
--
-- By default, `rc.lua` will create one `awful.widget.prompt` per screen called
-- `mypromptbox`. It is used for both the command execution (`mod4+r`) and
-- Lua prompt (`mod4+x`). It can be re-used for random inputs using:
--
-- @DOC_wibox_awidget_prompt_simple_EXAMPLE@
--
--     -- Then **IN THE globalkeys TABLE** add a new shortcut
--     awful.key({ modkey }, "e", echo_test,
--         {description = "Echo a string", group = "custom"}),
--
-- Note that this assumes an `rc.lua` file based on the default one. The way
-- to access the screen prompt may vary.
--
-- **Extra key hooks**:
--
-- The Awesome prompt also supports adding custom extensions to specific
-- keyboard keybindings. Those keybindings have precedence over the built-in
-- ones. Therefor, they can be used to override the default ones.
--
-- *[Example one] Adding pre-configured `awful.spawn` commands:*
--
-- @DOC_wibox_awidget_prompt_hooks_EXAMPLE@
--
-- *[Example two] Modifying the command (+ vi like input)*:
--
-- The hook system also allows to modify the command before interpreting it in
-- the `exe_callback`.
--
-- @DOC_wibox_awidget_prompt_vilike_EXAMPLE@
--
-- *[Example three] Key listener*:
--
-- The 2 previous examples were focused on changing the prompt behavior. This
-- one explains how to "spy" on the prompt events. This can be used for
--
-- * Implementing more complex mutator
-- * Synchronising other widgets
-- * Showing extra tips to the user
--
-- @DOC_wibox_awidget_prompt_keypress_EXAMPLE@
--
-- **highlighting**:
--
-- The prompt also support custom highlighters:
--
-- @DOC_wibox_awidget_prompt_highlight_EXAMPLE@
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008 Julien Danjou
-- @module awful.prompt
---------------------------------------------------------------------------

--- The prompt cursor foreground color.
-- @beautiful beautiful.prompt_fg_cursor
-- @param color
-- @see gears.color

--- The prompt cursor background color.
-- @beautiful beautiful.prompt_bg_cursor
-- @param color
-- @see gears.color

--- The prompt text font.
-- @beautiful beautiful.prompt_font
-- @param string
-- @see string

-- Grab environment we need
local assert = assert
local io = io
local table = table
local math = math
local ipairs = ipairs
local pcall = pcall
local capi =
{
    selection = selection
}
local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)
local keygrabber = require("awful.keygrabber")
local beautiful = require("beautiful")
local akey = require("awful.key")
local gdebug = require('gears.debug')
local gtable = require("gears.table")
local gcolor = require("gears.color")
local gstring = require("gears.string")
local gfs = require("gears.filesystem")

local prompt = {}

--- Private data
local data = {}
data.history = {}

local search_term = nil
local function itera (inc,a, i)
    i = i + inc
    local v = a[i]
    if v then return i,v end
end

--- Load history file in history table
-- @param id The data.history identifier which is the path to the filename.
-- @param[opt] max The maximum number of entries in file.
local function history_check_load(id, max)
    if id and id ~= "" and not data.history[id] then
        data.history[id] = { max = 50, table = {} }

        if max then
            data.history[id].max = max
        end

        local f = io.open(id, "r")
        if not f then return end

        -- Read history file
        for line in f:lines() do
            if gtable.hasitem(data.history[id].table, line) == nil then
                table.insert(data.history[id].table, line)
                if #data.history[id].table >= data.history[id].max then
                    break
                end
            end
        end
        f:close()
    end
end

local function is_word_char(c)
    if string.find("[{[(,.:;_-+=@/ ]", c) then
        return false
    else
        return true
    end
end

local function cword_start(s, pos)
    local i = pos
    if i > 1 then
        i = i - 1
    end
    while i >= 1 and not is_word_char(s:sub(i, i)) do
        i = i - 1
    end
    while i >= 1 and is_word_char(s:sub(i, i)) do
        i = i - 1
    end
    if i <= #s then
        i = i + 1
    end
    return i
end

local function cword_end(s, pos)
    local i = pos
    while i <= #s and not is_word_char(s:sub(i, i)) do
        i = i + 1
    end
    while i <= #s and  is_word_char(s:sub(i, i)) do
        i = i + 1
    end
    return i
end

--- Save history table in history file
-- @param id The data.history identifier
local function history_save(id)
    if data.history[id] then
        local f = io.open(id, "w")
        if not f then
            local i = 0
            for d in id:gmatch(".-/") do
                i = i + #d
            end
            gfs.mkdir(id:sub(1, i - 1))
            f = assert(io.open(id, "w"))
        end
        for i = 1, math.min(#data.history[id].table, data.history[id].max) do
            f:write(data.history[id].table[i] .. "\n")
        end
       f:close()
    end
end

--- Return the number of items in history table regarding the id
-- @param id The data.history identifier
-- @return the number of items in history table, -1 if history is disabled
local function history_items(id)
    if data.history[id] then
        return #data.history[id].table
    else
        return -1
    end
end

--- Add an entry to the history file
-- @param id The data.history identifier
-- @param command The command to add
local function history_add(id, command)
    if data.history[id] and command ~= "" then
        local index = gtable.hasitem(data.history[id].table, command)
        if index == nil then
            table.insert(data.history[id].table, command)

            -- Do not exceed our max_cmd
            if #data.history[id].table > data.history[id].max then
                table.remove(data.history[id].table, 1)
            end

            history_save(id)
        else
            -- Bump this command to the end of history
            table.remove(data.history[id].table, index)
            table.insert(data.history[id].table, command)
            history_save(id)
        end
    end
end


--- Draw the prompt text with a cursor.
-- @tparam table args The table of arguments.
-- @field text The text.
-- @field font The font.
-- @field prompt The text prefix.
-- @field text_color The text color.
-- @field cursor_color The cursor color.
-- @field cursor_pos The cursor position.
-- @field cursor_ul The cursor underline style.
-- @field selectall If true cursor is rendered on the entire text.
local function prompt_text_with_cursor(args)
    local char, spacer, text_start, text_end, ret
    local text = args.text or ""
    local _prompt = args.prompt or ""
    local underline = args.cursor_ul or "none"

    if args.selectall then
        if #text == 0 then char = " " else char = gstring.xml_escape(text) end
        spacer = " "
        text_start = ""
        text_end = ""
    elseif #text < args.cursor_pos then
        char = " "
        spacer = ""
        text_start = gstring.xml_escape(text)
        text_end = ""
    else
        char = gstring.xml_escape(text:sub(args.cursor_pos, args.cursor_pos))
        spacer = " "
        text_start = gstring.xml_escape(text:sub(1, args.cursor_pos - 1))
        text_end = gstring.xml_escape(text:sub(args.cursor_pos + 1))
    end

    local cursor_color = gcolor.ensure_pango_color(args.cursor_color)
    local text_color = gcolor.ensure_pango_color(args.text_color)

    if args.highlighter then
        text_start, text_end = args.highlighter(text_start, text_end)
    end

    ret = _prompt .. text_start .. "<span background=\"" .. cursor_color ..
        "\" foreground=\"" .. text_color .. "\" underline=\"" .. underline ..
        "\">" .. char .. "</span>" .. text_end .. spacer
    return ret
end

--- The callback function to call with command as argument when finished.
-- @usage local function my_exe_cb(command)
--    -- do something
-- end
-- @callback exe_callback
-- @tparam string command The command (as entered).

--- The callback function to call to get completion.
-- @usage local function my_completion_cb(command_before_comp, cur_pos_before_comp, ncomp)
--    return command_before_comp.."foo", cur_pos_before_comp+3, 1
-- end
--
-- @callback completion_callback
-- @tparam string command_before_comp The current command.
-- @tparam number cur_pos_before_comp The current cursor position.
-- @tparam number ncomp The number of completion elements.
-- @treturn string command
-- @treturn number cur_pos
-- @treturn number matches

--- The callback function to always call without arguments, regardless of
-- whether the prompt was cancelled.
-- @usage local function my_done_cb()
--    -- do something
-- end
-- @callback done_callback

---  The callback function to call with command as argument when a command was
-- changed.
-- @usage local function my_changed_cb(command)
--    -- do something
-- end
-- @callback changed_callback
-- @tparam string command The current command.

--- The callback function to call with mod table, key and command as arguments
-- when a key was pressed.
-- @usage local function my_keypressed_cb(mod, key, command)
--    -- do something
-- end
-- @callback keypressed_callback
-- @tparam table mod The current modifiers (like "Control" or "Shift").
-- @tparam string key The key name.
-- @tparam string command The current command.

--- The callback function to call with mod table, key and command as arguments
-- when a key was released.
-- @usage local function my_keyreleased_cb(mod, key, command)
--    -- do something
-- end
-- @callback keyreleased_callback
-- @tparam table mod The current modifiers (like "Control" or "Shift").
-- @tparam string key The key name.
-- @tparam string command The current command.

--- A function to add syntax highlighting to the command.
-- @usage local function my_highlighter(before_cursor, after_cursor)
--    -- do something
--    return before_cursor, after_cursor
-- end
-- @callback highlighter
-- @tparam string before_cursor
-- @tparam string after_cursor

--- A callback when a key combination is triggered.
-- This callback can return many things:
--
-- * a modified command
-- * `true` If the command is successful (then it won't exit)
-- * nothing or `nil` to execute the `exe_callback` and `done_callback` and exit
--
-- An optional second return value controls if the prompt should exit or simply
-- update the command (from the first return value) and keep going. The default
-- is to execute the `exe_callback` and `done_callback` before exiting.
--
-- @usage local function my_hook(command)
--    return command.."foo", false
-- end
--
-- @callback hook
-- @tparam string command The current command.

--- Run a prompt in a box.
--
-- @tparam[opt={}] table args A table with optional arguments
-- @tparam[opt] gears.color args.fg_cursor
-- @tparam[opt] gears.color args.bg_cursor
-- @tparam[opt] gears.color args.ul_cursor
-- @tparam[opt] widget args.prompt
-- @tparam[opt] string args.text
-- @tparam[opt] boolean args.selectall
-- @tparam[opt] string args.font
-- @tparam[opt] boolean args.autoexec
-- @tparam widget args.textbox The textbox to use for the prompt.
-- @tparam[opt] function args.highlighter A function to add syntax highlighting to
--  the command.
-- @tparam function args.exe_callback The callback function to call with command as argument
-- when finished.
-- @tparam function args.completion_callback The callback function to call to get completion.
-- @tparam[opt] string args.history_path File path where the history should be
-- saved, set nil to disable history
-- @tparam[opt] function args.history_max Set the maximum entries in history
-- file, 50 by default
-- @tparam[opt] function args.done_callback The callback function to always call
-- without arguments, regardless of whether the prompt was cancelled.
-- @tparam[opt] function args.changed_callback The callback function to call
-- with command as argument when a command was changed.
-- @tparam[opt] function args.keypressed_callback The callback function to call
--   with mod table, key and command as arguments when a key was pressed.
-- @tparam[opt] function args.keyreleased_callback The callback function to call
--   with mod table, key and command as arguments when a key was pressed.
-- @tparam[opt] table args.hooks The "hooks" argument uses a syntax similar to
--   `awful.key`.  It will call a function for the matching modifiers + key.
--   It receives the command (widget text/input) as an argument.
--   If the callback returns a command, this will be passed to the
--   `exe_callback`, otherwise nothing gets executed by default, and the hook
--   needs to handle it.
--     hooks = {
--       -- Apply startup notification properties with Shift-Return.
--       {{"Shift"  }, "Return", function(command)
--         awful.screen.focused().mypromptbox:spawn_and_handle_error(
--           command, {floating=true})
--       end},
--       -- Override default behavior of "Return": launch commands prefixed
--       -- with ":" in a terminal.
--       {{}, "Return", function(command)
--         if command:sub(1,1) == ":" then
--           return terminal .. ' -e ' .. command:sub(2)
--         end
--         return command
--       end}
--     }
-- @param textbox The textbox to use for the prompt. [**DEPRECATED**]
-- @param exe_callback The callback function to call with command as argument
-- when finished. [**DEPRECATED**]
-- @param completion_callback The callback function to call to get completion.
--   [**DEPRECATED**]
-- @param[opt] history_path File path where the history should be
-- saved, set nil to disable history [**DEPRECATED**]
-- @param[opt] history_max Set the maximum entries in history
-- file, 50 by default [**DEPRECATED**]
-- @param[opt] done_callback The callback function to always call
-- without arguments, regardless of whether the prompt was cancelled.
--  [**DEPRECATED**]
-- @param[opt] changed_callback The callback function to call
-- with command as argument when a command was changed. [**DEPRECATED**]
-- @param[opt] keypressed_callback The callback function to call
--   with mod table, key and command as arguments when a key was pressed.
--   [**DEPRECATED**]
-- @see gears.color
function prompt.run(args, textbox, exe_callback, completion_callback,
                    history_path, history_max, done_callback,
                    changed_callback, keypressed_callback)
    local grabber
    local theme = beautiful.get()
    if not args then args = {} end
    local command = args.text or ""
    local command_before_comp
    local cur_pos_before_comp
    local prettyprompt = args.prompt or ""
    local inv_col = args.fg_cursor or theme.prompt_fg_cursor or theme.fg_focus or "black"
    local cur_col = args.bg_cursor or theme.prompt_bg_cursor or theme.bg_focus or "white"
    local cur_ul = args.ul_cursor
    local text = args.text or ""
    local font = args.font or theme.prompt_font or theme.font
    local selectall = args.selectall
    local highlighter = args.highlighter
    local hooks = {}

    local deprecated = function(name)
        gdebug.deprecate(string.format(
            'awful.prompt.run: the argument %s is deprecated, please use args.%s instead',
            name, name), {raw=true, deprecated_in=4})
    end
    if textbox             then deprecated('textbox') end
    if exe_callback        then deprecated('exe_callback') end
    if completion_callback then deprecated('completion_callback') end
    if history_path        then deprecated('history_path') end
    if history_max         then deprecated('history_max') end
    if done_callback       then deprecated('done_callback') end
    if changed_callback    then deprecated('changed_callback') end
    if keypressed_callback then deprecated('keypressed_callback') end

    -- This function has already an absurd number of parameters, allow them
    -- to be set using the args to avoid a "nil, nil, nil, nil, foo" scenario
    keypressed_callback = keypressed_callback or args.keypressed_callback
    changed_callback    = changed_callback    or args.changed_callback
    done_callback       = done_callback       or args.done_callback
    history_max         = history_max         or args.history_max
    history_path        = history_path        or args.history_path
    completion_callback = completion_callback or args.completion_callback
    exe_callback        = exe_callback        or args.exe_callback
    textbox             = textbox             or args.textbox

    search_term=nil

    history_check_load(history_path, history_max)
    local history_index = history_items(history_path) + 1
    -- The cursor position
    local cur_pos = (selectall and 1) or text:wlen() + 1
    -- The completion element to use on completion request.
    local ncomp = 1
    if not textbox then
        return
    end

    -- Build the hook map
    for _,v in ipairs(args.hooks or {}) do
        if #v == 3 then
            local _,key,callback = unpack(v)
            if type(callback) == "function" then
                hooks[key] = hooks[key] or {}
                hooks[key][#hooks[key]+1] = v
            else
                gdebug.print_warning("The hook's 3rd parameter has to be a function.")
            end
        else
            gdebug.print_warning("The hook has to have 3 parameters.")
        end
    end

    textbox:set_font(font)
    textbox:set_markup(prompt_text_with_cursor{
        text = text, text_color = inv_col, cursor_color = cur_col,
        cursor_pos = cur_pos, cursor_ul = cur_ul, selectall = selectall,
        prompt = prettyprompt, highlighter =  highlighter})

    local function exec(cb, command_to_history)
        textbox:set_markup("")
        history_add(history_path, command_to_history)
        keygrabber.stop(grabber)
        if cb then cb(command) end
        if done_callback then done_callback() end
    end

    -- Update textbox
    local function update()
        textbox:set_font(font)
        textbox:set_markup(prompt_text_with_cursor{
                               text = command, text_color = inv_col, cursor_color = cur_col,
                               cursor_pos = cur_pos, cursor_ul = cur_ul, selectall = selectall,
                               prompt = prettyprompt, highlighter =  highlighter })
    end

    grabber = keygrabber.run(
    function (modifiers, key, event)
        -- Convert index array to hash table
        local mod = {}
        for _, v in ipairs(modifiers) do mod[v] = true end

        if event ~= "press" then
            if args.keyreleased_callback then
                args.keyreleased_callback(mod, key, command)
            end

            return
        end

        -- Call the user specified callback. If it returns true as
        -- the first result then return from the function. Treat the
        -- second and third results as a new command and new prompt
        -- to be set (if provided)
        if keypressed_callback then
            local user_catched, new_command, new_prompt =
                keypressed_callback(mod, key, command)
            if new_command or new_prompt then
                if new_command then
                    command = new_command
                end
                if new_prompt then
                    prettyprompt = new_prompt
                end
                update()
            end
            if user_catched then
                if changed_callback then
                    changed_callback(command)
                end
                return
            end
        end

        local filtered_modifiers = {}

        -- User defined cases
        if hooks[key] then
            -- Remove caps and num lock
            for _, m in ipairs(modifiers) do
                if not gtable.hasitem(akey.ignore_modifiers, m) then
                    table.insert(filtered_modifiers, m)
                end
            end

            for _,v in ipairs(hooks[key]) do
                if #filtered_modifiers == #v[1] then
                    local match = true
                    for _,v2 in ipairs(v[1]) do
                        match = match and mod[v2]
                    end
                    if match then
                        local cb
                        local ret, quit = v[3](command)
                        local original_command = command

                        -- Support both a "simple" and a "complex" way to
                        -- control if the prompt should quit.
                        quit = quit == nil and (ret ~= true) or (quit~=false)

                        -- Allow the callback to change the command
                        command = (ret ~= true) and ret or command

                        -- Quit by default, but allow it to be disabled
                        if ret and type(ret) ~= "boolean" then
                            cb = exe_callback
                            if not quit then
                                cur_pos = ret:wlen() + 1
                                update()
                            end
                        elseif quit then
                            -- No callback.
                            cb = function() end
                        end

                        -- Execute the callback
                        if cb then
                            exec(cb, original_command)
                        end

                        return
                    end
                end
            end
        end

        -- Get out cases
        if (mod.Control and (key == "c" or key == "g"))
            or (not mod.Control and key == "Escape") then
            keygrabber.stop(grabber)
            textbox:set_markup("")
            history_save(history_path)
            if done_callback then done_callback() end
            return false
        elseif (mod.Control and (key == "j" or key == "m"))
            or (not mod.Control and key == "Return")
            or (not mod.Control and key == "KP_Enter") then
            exec(exe_callback, command)
            -- We already unregistered ourselves so we don't want to return
            -- true, otherwise we may unregister someone else.
            return
        end

        -- Control cases
        if mod.Control then
            selectall = nil
            if key == "a" then
                cur_pos = 1
            elseif key == "b" then
                if cur_pos > 1 then
                    cur_pos = cur_pos - 1
                end
            elseif key == "d" then
                if cur_pos <= #command then
                    command = command:sub(1, cur_pos - 1) .. command:sub(cur_pos + 1)
                end
            elseif key == "p" then
                if history_index > 1 then
                    history_index = history_index - 1

                    command = data.history[history_path].table[history_index]
                    cur_pos = #command + 2
                end
            elseif key == "n" then
                if history_index < history_items(history_path) then
                    history_index = history_index + 1

                    command = data.history[history_path].table[history_index]
                    cur_pos = #command + 2
                elseif history_index == history_items(history_path) then
                    history_index = history_index + 1

                    command = ""
                    cur_pos = 1
                end
            elseif key == "e" then
                cur_pos = #command + 1
            elseif key == "r" then
                search_term = search_term or command:sub(1, cur_pos - 1)
                for i,v in (function(a,i) return itera(-1,a,i) end), data.history[history_path].table, history_index do
                    if v:find(search_term,1,true) ~= nil then
                        command=v
                        history_index=i
                        cur_pos=#command+1
                        break
                    end
                end
            elseif key == "s" then
                search_term = search_term or command:sub(1, cur_pos - 1)
                for i,v in (function(a,i) return itera(1,a,i) end), data.history[history_path].table, history_index do
                    if v:find(search_term,1,true) ~= nil then
                        command=v
                        history_index=i
                        cur_pos=#command+1
                        break
                    end
                end
            elseif key == "f" then
                if cur_pos <= #command then
                    cur_pos = cur_pos + 1
                end
            elseif key == "h" then
                if cur_pos > 1 then
                    command = command:sub(1, cur_pos - 2) .. command:sub(cur_pos)
                    cur_pos = cur_pos - 1
                end
            elseif key == "k" then
                command = command:sub(1, cur_pos - 1)
            elseif key == "u" then
                command = command:sub(cur_pos, #command)
                cur_pos = 1
            elseif key == "Up" then
                search_term = command:sub(1, cur_pos - 1) or ""
                for i,v in (function(a,i) return itera(-1,a,i) end), data.history[history_path].table, history_index do
                    if v:find(search_term,1,true) == 1 then
                        command=v
                        history_index=i
                        break
                    end
                end
            elseif key == "Down" then
                search_term = command:sub(1, cur_pos - 1) or ""
                for i,v in (function(a,i) return itera(1,a,i) end), data.history[history_path].table, history_index do
                    if v:find(search_term,1,true) == 1 then
                        command=v
                        history_index=i
                        break
                    end
                end
            elseif key == "w" or key == "BackSpace" then
                local wstart = 1
                local wend = 1
                local cword_start_pos = 1
                local cword_end_pos = 1
                while wend < cur_pos do
                    wend = command:find("[{[(,.:;_-+=@/ ]", wstart)
                    if not wend then wend = #command + 1 end
                    if cur_pos >= wstart and cur_pos <= wend + 1 then
                        cword_start_pos = wstart
                        cword_end_pos = cur_pos - 1
                        break
                    end
                    wstart = wend + 1
                end
                command = command:sub(1, cword_start_pos - 1) .. command:sub(cword_end_pos + 1)
                cur_pos = cword_start_pos
            elseif key == "Delete" then
                -- delete from history only if:
                --  we are not dealing with a new command
                --  the user has not edited an existing entry
                if command == data.history[history_path].table[history_index] then
                    table.remove(data.history[history_path].table, history_index)
                    if history_index <= history_items(history_path) then
                        command = data.history[history_path].table[history_index]
                        cur_pos = #command + 2
                    elseif history_index > 1 then
                        history_index = history_index - 1

                        command = data.history[history_path].table[history_index]
                        cur_pos = #command + 2
                    else
                        command = ""
                        cur_pos = 1
                    end
                end
            end
        elseif mod.Mod1 or mod.Mod3 then
            if key == "b" then
                cur_pos = cword_start(command, cur_pos)
            elseif key == "f" then
                cur_pos = cword_end(command, cur_pos)
            elseif key == "d" then
                command = command:sub(1, cur_pos - 1) .. command:sub(cword_end(command, cur_pos))
            elseif key == "BackSpace" then
                local wstart = cword_start(command, cur_pos)
                command = command:sub(1, wstart - 1) .. command:sub(cur_pos)
                cur_pos = wstart
            end
        else
            if completion_callback then
                if key == "Tab" or key == "ISO_Left_Tab" then
                    if key == "ISO_Left_Tab" then
                        if ncomp == 1 then return end
                        if ncomp == 2 then
                            command = command_before_comp
                            textbox:set_font(font)
                            textbox:set_markup(prompt_text_with_cursor{
                                text = command_before_comp, text_color = inv_col, cursor_color = cur_col,
                                cursor_pos = cur_pos, cursor_ul = cur_ul, selectall = selectall,
                                prompt = prettyprompt })
                            return
                        end

                        ncomp = ncomp - 2
                    elseif ncomp == 1 then
                        command_before_comp = command
                        cur_pos_before_comp = cur_pos
                    end
                    local matches
                    command, cur_pos, matches = completion_callback(command_before_comp, cur_pos_before_comp, ncomp)
                    ncomp = ncomp + 1
                    key = ""
                    -- execute if only one match found and autoexec flag set
                    if matches and #matches == 1 and args.autoexec then
                        exec(exe_callback)
                        return
                    end
                else
                    ncomp = 1
                end
            end

            -- Typin cases
            if mod.Shift and key == "Insert" then
                local selection = capi.selection()
                if selection then
                    -- Remove \n
                    local n = selection:find("\n")
                    if n then
                        selection = selection:sub(1, n - 1)
                    end
                    command = command:sub(1, cur_pos - 1) .. selection .. command:sub(cur_pos)
                    cur_pos = cur_pos + #selection
                end
            elseif key == "Home" then
                cur_pos = 1
            elseif key == "End" then
                cur_pos = #command + 1
            elseif key == "BackSpace" then
                if cur_pos > 1 then
                    command = command:sub(1, cur_pos - 2) .. command:sub(cur_pos)
                    cur_pos = cur_pos - 1
                end
            elseif key == "Delete" then
                command = command:sub(1, cur_pos - 1) .. command:sub(cur_pos + 1)
            elseif key == "Left" then
                cur_pos = cur_pos - 1
            elseif key == "Right" then
                cur_pos = cur_pos + 1
            elseif key == "Up" then
                if history_index > 1 then
                    history_index = history_index - 1

                    command = data.history[history_path].table[history_index]
                    cur_pos = #command + 2
                end
            elseif key == "Down" then
               if history_index < history_items(history_path) then
                    history_index = history_index + 1

                    command = data.history[history_path].table[history_index]
                    cur_pos = #command + 2
                elseif history_index == history_items(history_path) then
                    history_index = history_index + 1

                    command = ""
                    cur_pos = 1
                end
            else
                -- wlen() is UTF-8 aware but #key is not,
                -- so check that we have one UTF-8 char but advance the cursor of # position
                if key:wlen() == 1 then
                    if selectall then command = "" end
                    command = command:sub(1, cur_pos - 1) .. key .. command:sub(cur_pos)
                    cur_pos = cur_pos + #key
                end
            end
            if cur_pos < 1 then
                cur_pos = 1
            elseif cur_pos > #command + 1 then
                cur_pos = #command + 1
            end
            selectall = nil
        end

        local success = pcall(update)
        while not success do
            -- TODO UGLY HACK TODO
            -- Setting the text failed. Most likely reason is that the user
            -- entered a multibyte character and pressed backspace which only
            -- removed the last byte. Let's remove another byte.
            if cur_pos <= 1 then
                -- No text left?!
                break
            end

            command = command:sub(1, cur_pos - 2) .. command:sub(cur_pos)
            cur_pos = cur_pos - 1
            success = pcall(update)
        end

        if changed_callback then
            changed_callback(command)
        end
    end)
end

return prompt

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
