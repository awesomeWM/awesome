local screen = require("screen")

local coords = {x=100,y=100}

local mouse = {
    old_histories = {},
    history       = {},
}

-- Avoid gears when possible
local function contains(rect, point)
    return point.x >= rect.x and point.x < rect.x+rect.width and
        point.y >= rect.y and point.y < rect.y+rect.height
end

function mouse.coords(args)
    if args then
        local old = {x = coords.x, y = coords.y}
        coords.x, coords.y = args.x, args.y
        table.insert(mouse.history, {x=coords.x, y=coords.y})

        for _, d in ipairs(drawin.get()) do
            local geo = d.geometry()
            local was_in, is_in = contains(geo, old), contains(geo, coords)
            local sig = {(is_in and not was_in) and "mouse::enter" or
                (was_in and not is_in) and "mouse::leave" or "mouse::move"}

            -- Enter is also a move, otherwise drawable.handle_motion isn't called
            if sig[1] == "mouse::enter" then
                table.insert(sig, "mouse::move")
            end

            for _, s in ipairs(sig) do
                d:emit_signal(s, coords.x - geo.x, coords.y - geo.y)
                d.drawable:emit_signal(s, coords.x - geo.x, coords.y - geo.y)
            end
        end
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

local forced_screen = nil

return setmetatable(mouse, {
    __index = function(self, key)
        if key == "screen" then
            if forced_screen and screen._deleted[forced_screen] then
                forced_screen = nil
            end

            -- Using capi.mouse.screen is *not* supported when there is zero
            -- screen. Nearly all the code uses `mouse.screen` as its ultimate
            -- fallback. Having no screens is tolerated during early
            -- initialization and that's it.
            return screen.count() > 0 and screen[1] or assert(
                false, "Calling `mouse.screen` without screens isn't supported"
            )
        end
        local h = rawget(mouse,"_i_handler")
        if h then
            return h(self, key)
        end
    end,
    __newindex = function(_, k, v)
        local h = rawget(mouse,"_ni_handler")
        if k == "screen" then
            -- This will assert if the screen is invalid
            forced_screen = v and screen[v] or nil
        elseif h then
            h(_, k, v)
        end
    end,
})

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
