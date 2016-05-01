local spawn = require("awful.spawn")

-- This file provide a simple, yet flexible, test client.
-- It is used to test the `awful.rules`

return function(class, title, use_sn)
    class = class or 'test_app'
    title = title or 'Awesome test client'

    local cmd = {"lua" , "-e", table.concat {
        "local lgi = require 'lgi';",
        "local Gtk = lgi.require('Gtk');",
        "Gtk.init();",
        "local class = '",class,"';",
        "local window = Gtk.Window {",
        "    default_width  = 100,",
        "    default_height = 100,",
        "    on_destroy     = Gtk.main_quit,",
        "    title          = '",title,
        "'};",
        "window:set_wmclass(class, class);",
        "window:show_all();",
        "Gtk:main{...}"
    }}

    return spawn(cmd, use_sn)
end

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
