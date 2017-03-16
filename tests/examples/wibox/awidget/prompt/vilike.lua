local parent    = ... --DOC_NO_USAGE --DOC_HIDE
local wibox     = require( "wibox"     ) --DOC_HIDE
local awful     = { prompt = require("awful.prompt"),--DOC_HIDE
                    util   = require("awful.util")}--DOC_HIDE
local beautiful = require( "beautiful" ) --DOC_HIDE
local gfs       = require("gears.filesystem") --DOC_HIDE
local terminal  = "xterm"  --DOC_HIDE

    local atextbox = wibox.widget.textbox()

    -- Store a list of verbs characters in a hash
    local verbs = {
        -- Spawn in a terminal
        t = function(adjs, count, cmd) return {terminal, "-e", cmd} end,  --luacheck: no unused args
        -- Spawn with a shell
        s = function(adjs, count, cmd) return {awful.util.shell, '-c', cmd} end, --luacheck: no unused args
    }

    local function vi_parse(action, command)
        local req, ret = {count={}, adjectives={}}
        -- Quite dumb, don't do something like <num>+<adj>+<num>+<verb>
        for char in action:gmatch('(.)') do
            if     tonumber(char)  then table.insert(req.count, char)
            elseif verbs[char]     then req.verb = char
            else   table.insert(ret.adjectives, char) end
            if req.verb then
                req.count = tonumber(table.concat(req.count)) or 1
                ret = ret or verbs[req.verb](req.adjectives, req.count, command)
                req = {count={}, adjectives={}}
            end
        end
        return ret
    end

    awful.prompt.run {
        prompt       = '<b>Run: </b>',
        text         = ':t htop', --DOC_HIDE
        hooks        = {
            {{},'Return', function(cmd)
                if (not cmd) or cmd:sub(1,1) ~= ':' then return cmd end
                local act, cmd2 = cmd:gmatch(':([a-zA-Z1-9]+)[ ]+(.*)')()
                if not act then return cmd end
                return vi_parse(act, cmd2)
            end},
        },
        textbox      = atextbox,
        history_path = gfs.get_dir('cache') .. '/history',
        exe_callback = function(cmd) awful.spawn(cmd) end
    }


parent:add( wibox.widget {    --DOC_HIDE
    atextbox,  --DOC_HIDE
    bg = beautiful.bg_normal,  --DOC_HIDE
    widget = wibox.container.background  --DOC_HIDE
}) --DOC_HIDE
