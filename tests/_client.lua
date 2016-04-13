local spawn = require("awful.spawn")

-- This file provide a simple, yet flexible, test client.
-- It is used to test the `awful.rules`

return function(class, title)
    title = title or 'Awesome test client'

    local cmd = {"lua" , "-e", table.concat {
        "local lgi = require 'lgi';",
        "local Gtk = lgi.require('Gtk');",
        "Gtk.init();",
        "local class = '",
        class or 'test_app',"';",
        "local window = Gtk.Window {",
        "    default_width  = 100,",
        "    default_height = 100,",
        "    title          = '",title,
        "'};",
        "window:set_wmclass(class, class);",
        "local app = Gtk.Application {",
        "    application_id = 'org.awesomewm.tests.",class,
        "'};",
        "function app:on_activate()",
        "    window.application = self;",
        "    window:show_all();",
        "end;",
        "app:run {''}"
    }}

    spawn(cmd)
end
