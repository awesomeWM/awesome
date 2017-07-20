---------------------------------------------------------------------------
-- Corner layout.
-- Display master client in a corner of the screen, and slaves in one
-- column and one row around the master.
-- See Pull Request for example : https://github.com/awesomeWM/awesome/pull/251
-- @module awful.layout
-- @author Alexis Brenon &lt;brenon.alexis+awesomewm@gmail.com&gt;
-- @copyright 2015 Alexis Brenon

-- Grab environment we need
local ipairs = ipairs
local math = math
local capi = {screen = screen}

--- The cornernw layout layoutbox icon.
-- @beautiful beautiful.layout_cornernw
-- @param surface
-- @see gears.surface

--- The cornerne layout layoutbox icon.
-- @beautiful beautiful.layout_cornerne
-- @param surface
-- @see gears.surface

--- The cornersw layout layoutbox icon.
-- @beautiful beautiful.layout_cornersw
-- @param surface
-- @see gears.surface

--- The cornerse layout layoutbox icon.
-- @beautiful beautiful.layout_cornerse
-- @param surface
-- @see gears.surface

-- Actually arrange clients of p.clients for corner layout
-- @param p Mandatory table containing required informations for layouts
-- (clients to arrange, workarea geometry, etc.)
-- @param orientation String indicating in which corner is the master window.
-- Available values are : NE, NW, SW, SE
local function do_corner(p, orientation)
    local t = p.tag or capi.screen[p.screen].selected_tag
    local wa = p.workarea
    local cls = p.clients

    if #cls == 0 then return end

    local master = {}
    local column = {}
    local row = {}
    -- Use the nmaster field of the tag in a cheaty way
    local row_privileged = ((cls[1].screen.selected_tag.master_count % 2) == 0)

    local master_factor = cls[1].screen.selected_tag.master_width_factor
    master.width = master_factor * wa.width
    master.height = master_factor * wa.height

    local number_privileged_win = math.ceil((#cls - 1)/2)
    local number_unprivileged_win = (#cls - 1) - number_privileged_win

    -- Define some obvious parameters
    column.width = wa.width - master.width
    column.x_increment = 0
    row.height = wa.height - master.height
    row.y_increment = 0

    -- Place master at the right place and move row and column accordingly
    column.y = wa.y
    row.x = wa.x
    if orientation:match('N.') then
        master.y = wa.y
        row.y = master.y + master.height
    elseif orientation:match('S.') then
        master.y = wa.y + wa.height - master.height
        row.y = wa.y
    end
    if orientation:match('.W') then
        master.x = wa.x
        column.x = master.x + master.width
    elseif orientation:match('.E') then
        master.x = wa.x + wa.width - master.width
        column.x = wa.x
    end
    -- At this point, master is in a corner
    -- but row and column are overlayed in the opposite corner...

    -- Reduce the unprivileged slaves to remove overlay
    -- and define actual width and height
    if row_privileged then
        row.width = wa.width
        row.number_win = number_privileged_win
        column.y = master.y
        column.height = master.height
        column.number_win = number_unprivileged_win
    else
        column.height = wa.height
        column.number_win = number_privileged_win
        row.x = master.x
        row.width = master.width
        row.number_win = number_unprivileged_win
    end

    column.win_height = column.height/column.number_win
    column.win_width = column.width
    column.y_increment = column.win_height
    column.win_idx = 0

    row.win_width = row.width/row.number_win
    row.win_height = row.height
    row.x_increment = row.win_width
    row.win_idx = 0

    -- Extend master if there is only a few windows and "expand" policy is set
    if #cls < 3 then
        if row_privileged then
            master.x = wa.x
            master.width = wa.width
        else
            master.y = wa.y
            master.height = wa.height
        end
        if #cls < 2  then
            if t.master_fill_policy == "expand" then
                master = wa
            else
                master.x = master.x + (wa.width - master.width)/2
                master.y = master.y + (wa.height - master.height)/2
            end
        end
    end

    for i, c in ipairs(cls) do
        local g
        -- Handle master window
        if i == 1 then
            g = {
                x = master.x,
                y = master.y,
                width = master.width,
                height = master.height
            }
        -- handle column windows
        elseif i % 2 == 0 then
            g = {
                x = column.x + column.win_idx * column.x_increment,
                y = column.y + column.win_idx * column.y_increment,
                width = column.win_width,
                height = column.win_height
            }
            column.win_idx = column.win_idx + 1
        else
            g = {
                x = row.x + row.win_idx * row.x_increment,
                y = row.y + row.win_idx * row.y_increment,
                width = row.win_width,
                height = row.win_height
            }
            row.win_idx = row.win_idx + 1
        end
        p.geometries[c] = g
    end
end

local corner = {}
corner.row_privileged = false

function corner.skip_gap(nclients, t)
    return nclients == 1 and t.master_fill_policy == "expand"
end

--- Corner layout.
-- Display master client in a corner of the screen, and slaves in one
-- column and one row around the master.
-- @clientlayout awful.layout.suit.corner.nw
corner.nw = {
        name = "cornernw",
        arrange = function (p) return do_corner(p, "NW") end,
        skip_gap = corner.skip_gap
    }

--- Corner layout.
-- Display master client in a corner of the screen, and slaves in one
-- column and one row around the master.
-- @clientlayout awful.layout.suit.corner.ne
corner.ne = {
        name = "cornerne",
        arrange = function (p) return do_corner(p, "NE") end,
        skip_gap = corner.skip_gap
    }

--- Corner layout.
-- Display master client in a corner of the screen, and slaves in one
-- column and one row around the master.
-- @clientlayout awful.layout.suit.corner.sw
corner.sw = {
        name = "cornersw",
        arrange = function (p) return do_corner(p, "SW") end,
        skip_gap = corner.skip_gap
    }

--- Corner layout.
-- Display master client in a corner of the screen, and slaves in one
-- column and one row around the master.
-- @clientlayout awful.layout.suit.corner.se
corner.se = {
        name = "cornerse",
        arrange = function (p) return do_corner(p, "SE") end,
        skip_gap = corner.skip_gap
    }

return corner

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
