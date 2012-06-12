---------------------------------------------------------------------------
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @author Sébastien Gross &lt;seb-awesome@chezwam.org&gt;
-- @copyright 2008 Julien Danjou, Sébastien Gross
-- @release @AWESOME_VERSION@
---------------------------------------------------------------------------

-- Grab environment we need
local io = io
local os = os
local table = table
local math = math
local print = print
local pairs = pairs
local string = string
local util = require("awful.util")

--- Completion module.
-- This module store a set of function using shell to complete commands name.
-- awful.completion
local completion = {}

-- mapping of command/completion function
local bashcomp_funcs = {}
local bashcomp_src = "@SYSCONFDIR@/bash_completion"

--- Enable programmable bash completion in awful.completion.bash at the price of
-- a slight overhead.
-- @param src The bash completion source file, /etc/bash_completion by default.
function completion.bashcomp_load(src)
    if src then bashcomp_src = src end
    local c, err = io.popen("/usr/bin/env bash -c 'source " .. bashcomp_src .. "; complete -p'")
    if c then
        while true do
            local line = c:read("*line")
            if not line then break end
            -- if a bash function is used for completion, register it
            if line:match(".* -F .*") then
                bashcomp_funcs[line:gsub(".* (%S+)$","%1")] = line:gsub(".*-F +(%S+) .*$", "%1")
            end
        end
        c:close()
    else
        print(err)
    end
end

local function bash_escape(str)
    str = str:gsub(" ", "\\ ")
    str = str:gsub("%[", "\\[")
    str = str:gsub("%]", "\\]")
    str = str:gsub("%(", "\\(")
    str = str:gsub("%)", "\\)")
    return str
end

--- Use shell completion system to complete command and filename.
-- @param command The command line.
-- @param cur_pos The cursor position.
-- @param ncomp The element number to complete.
-- @param shell The shell to use for completion (bash (default) or zsh).
-- @return The new command, the new cursor position, the table of all matches.
function completion.shell(command, cur_pos, ncomp, shell)
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

    if cword_index == 1 and not string.find(words[cword_index], "/") then
        comptype = "command"
    end

    local shell_cmd
    if shell == "zsh" or (not shell and os.getenv("SHELL"):match("zsh$")) then
        if comptype == "file" then
            shell_cmd = "/usr/bin/env zsh -c 'local -a res; res=( " .. words[cword_index] .. "* ); print -ln -- ${res[@]}'"
        else
            -- check commands, aliases, builtins, functions and reswords
            shell_cmd = "/usr/bin/env zsh -c 'local -a res; "..
            "res=( "..
            "\"${(k)commands[@]}\" \"${(k)aliases[@]}\" \"${(k)builtins[@]}\" \"${(k)functions[@]}\" \"${(k)reswords[@]}\" "..
            "${PWD}/*(:t)"..
            "); "..
            "print -ln -- ${(M)res[@]:#"..words[cword_index].."*}'"
        end
    else
        if bashcomp_funcs[words[1]] then
            -- fairly complex command with inline bash script to get the possible completions
            shell_cmd = "/usr/bin/env bash -c 'source " .. bashcomp_src .. "; " ..
            "__print_completions() { for ((i=0;i<${#COMPREPLY[*]};i++)); do echo ${COMPREPLY[i]}; done }; " ..
            "COMP_WORDS=(" ..  command .."); COMP_LINE=\"" .. command .. "\"; " ..
            "COMP_COUNT=" .. cur_pos ..  "; COMP_CWORD=" .. cword_index-1 .. "; " ..
            bashcomp_funcs[words[1]] .. "; __print_completions'"
        else
            shell_cmd = "/usr/bin/env bash -c 'compgen -A " .. comptype .. " " .. words[cword_index] .. "'"
        end
    end
    local c, err = io.popen(shell_cmd .. " | sort -u")
    local output = {}
    i = 0
    if c then
        while true do
            local line = c:read("*line")
            if not line then break end
            if os.execute("test -d " .. line) == 0 then
                line = line .. "/"
            end
            table.insert(output, bash_escape(line))
        end

        c:close()
    else
        print(err)
    end

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

    return str, cur_pos, output
end

--- Run a generic completion.
-- For this function to run properly the awful.completion.keyword table should
-- be fed up with all keywords. The completion is run against these keywords.
-- @param text The current text the user had typed yet.
-- @param cur_pos The current cursor position.
-- @param ncomp The number of yet requested completion using current text.
-- @param keywords The keywords table uised for completion.
-- @return The new match, the new cursor position, the table of all matches.
function completion.generic(text, cur_pos, ncomp, keywords)
    -- The keywords table may be empty
    if #keywords == 0 then
        return text, #text + 1
    end

    -- if no text had been typed yet, then we could start cycling around all
    -- keywords with out filtering and move the cursor at the end of keyword
    if text == nil or #text == 0 then
        ncomp = math.fmod(ncomp - 1, #keywords) + 1
        return keywords[ncomp], #keywords[ncomp] + 2
    end

    -- Filter out only keywords starting with text
    local matches = {}
    for _, x in pairs(keywords) do
        if x:sub(1, #text) == text then
            table.insert(matches, x)
        end
    end

    -- if there are no matches just leave out with the current text and position
    if #matches == 0 then
        return text, #text + 1, matches
    end

    --  cycle around all matches
    ncomp = math.fmod(ncomp - 1, #matches) + 1
    return matches[ncomp], #matches[ncomp] + 1, matches
end

return completion

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
