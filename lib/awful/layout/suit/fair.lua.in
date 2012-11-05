---------------------------------------------------------------------------
-- @author Josh Komoroske
-- @copyright 2012 Josh Komoroske
-- @release @AWESOME_VERSION@
---------------------------------------------------------------------------

-- Grab environment we need
local ipairs = ipairs
local math = math

--- Fair layouts module for awful
-- awful.layout.suit.fair
local fair = {}

local function do_fair(p, orientation)
    local wa = p.workarea
    local cls = p.clients

    -- Swap workarea dimensions, if our orientation is "east"
    if orientation == 'east' then
        wa.width, wa.height = wa.height, wa.width
        wa.x, wa.y = wa.y, wa.x
    end

    if #cls > 0 then
        local rows, cols = 0, 0
        if #cls == 2 then
            rows, cols = 1, 2
        else
            rows = math.ceil(math.sqrt(#cls))
            cols = math.ceil(#cls / rows)
        end

        for k, c in ipairs(cls) do
            k = k - 1
            local g = {}

            local row, col = 0, 0
            row = k % rows
            col = math.floor(k / rows)

            local lrows, lcols = 0, 0
            if k >= rows * cols - rows then
                lrows = #cls - (rows * cols - rows)
                lcols = cols
            else
                lrows = rows
                lcols = cols
            end

            if row == lrows - 1 then
                g.height = wa.height - math.ceil(wa.height / lrows) * row
                g.y = wa.height - g.height
            else
                g.height = math.ceil(wa.height / lrows)
                g.y = g.height * row
            end

            if col == lcols - 1 then
                g.width = wa.width - math.ceil(wa.width / lcols) * col
                g.x = wa.width - g.width
            else
                g.width = math.ceil(wa.width / lcols)
                g.x = g.width * col
            end

            g.height = g.height - c.border_width * 2
            g.width = g.width - c.border_width * 2
            g.y = g.y + wa.y
            g.x = g.x + wa.x

            -- Swap window dimensions, if our orientation is "east"
            if orientation == 'east' then
                g.width, g.height = g.height, g.width
                g.x, g.y = g.y, g.x
            end

            c:geometry(g)
        end
    end
end

--- Horizontal fair layout.
-- @param screen The screen to arrange.
fair.horizontal = {}
fair.horizontal.name = "fairh"
function fair.horizontal.arrange(p)
    return do_fair(p, "east")
end

-- Vertical fair layout.
-- @param screen The screen to arrange.
fair.name = "fairv"
function fair.arrange(p)
    return do_fair(p, "south")
end

return fair
