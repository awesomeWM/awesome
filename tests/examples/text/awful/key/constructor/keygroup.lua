--DOC_NO_USAGE

local awful = require("awful") --DOC_HIDE

-- luacheck: ignore unused variable show_tag_by_numrow_index --DOC_HIDE
    local show_tag_by_numrow_index = awful.key {
        modifiers = { "Mod4" },
        keygroup = awful.key.keygroup.NUMROW,
        description = "only view tag",
        group = "tag",
        on_press = function (index)
            local screen = awful.screen.focused()
            local tag = screen.tags[index]

            if tag then
                tag:view_only()
            end
        end
    }

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
