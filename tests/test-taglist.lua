-- Test the taglist

local awful = require "awful"
local runner = require "_runner"

local taglist = awful.widget.taglist {
    screen = screen[1],
    filter = awful.widget.taglist.filter.all
}


runner.run_steps {
    function()
        return type(taglist.filter) == "function"
    end,
    function()
        taglist.filter = awful.widget.taglist.filter.selected
        return taglist.filter == awful.widget.taglist.filter.selected
    end,
}
