local screen = require("screen")

local coords = {x=100,y=100}

local mouse = {
    old_histories = {},
    history       = {},
}

function mouse.coords(args)
    if args then
        coords.x, coords.y = args.x, args.y
        table.insert(mouse.history, {x=coords.x, y=coords.y})
    end

    return coords
end

function mouse.set_newindex_miss_handler(h)
    rawset(mouse, "_ni_handler", h)
end

function mouse.set_index_miss_handler(h)
    rawset(mouse, "_i_handler", h)
end

function mouse.push_history()
    table.insert(mouse.old_histories, mouse.history)
    mouse.history = {}
end

return setmetatable(mouse, {
    __index = function(self, key)
        if key == "screen" then
            return screen[1]
        end
        local h = rawget(mouse,"_i_handler")
        if h then
            return h(self, key)
        end
    end,
    __newindex = function(...)
        local h = rawget(mouse,"_ni_handler")
        if h then
            h(...)
        end
    end,
})

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
