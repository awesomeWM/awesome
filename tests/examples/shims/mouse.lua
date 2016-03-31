local screen = require("screen")

local coords = {x=100,y=100}

local mouse = {
    screen  = screen[1],
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

function mouse.push_history()
    table.insert(mouse.old_histories, mouse.history)
    mouse.history = {}
end

return mouse

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
