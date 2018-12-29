--DOC_HEADER --DOC_NO_USAGE

local gears = {table = require("gears.table")} --DOC_HIDE

local awful = { keygrabber = require("awful.keygrabber"), --DOC_HIDE
    key = require("awful.key") } --DOC_HIDE

    local map, actions = {
        verbs = {
            m = "move" , f = "focus" , d = "delete" , a = "append",
            w = "swap" , p = "print" , n = "new"    ,
        },
        adjectives = { h = "left"  , j = "down" , k = "up"    , l = "right" , },
        nouns      = { c = "client", t = "tag"  , s = "screen", y = "layout", },
    }, {}
--DOC_NEWLINE
    function actions.client(action, adj) print("IN CLIENT!") end --luacheck: no unused args
    function actions.tag   (action, adj) print("IN TAG!"   ) end --luacheck: no unused args
    function actions.screen(action, adj) print("IN SCREEN!") end --luacheck: no unused args
    function actions.layout(action, adj) print("IN LAYOUT!") end --luacheck: no unused args
--DOC_NEWLINE
    local function parse(_, stop_key, _, sequence)
        local parsed, count = { verbs = "", adjectives = "", nouns = "", }, ""
        sequence = sequence..stop_key
--DOC_NEWLINE
        for i=1, #sequence do
            local char = sequence:sub(i,i)
            if char >= "0" and char <= "9" then
                count = count .. char
            else
                for kind in pairs(parsed) do
                    parsed[kind] = map[kind][char] or parsed[kind]
                end
            end
        end
--DOC_NEWLINE
        if parsed.nouns == "" then return end

        for _=1, count == "" and 1 or tonumber(count) do
            actions[parsed.nouns](parsed.verbs, parsed.adjectives)
        end
    end
--DOC_NEWLINE
    awful.keygrabber {
        stop_callback = parse,
        stop_key   = gears.table.keys(map.verbs),
        root_keybindings = {
            awful.key({"Mod4"}, "v")
        },
    }
