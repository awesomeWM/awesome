-- luacheck: globals string
function string.wlen(self)
    return #self
end

return function(_, _)

    -- Set the global shims
    -- luacheck: globals awesome root tag screen client mouse drawin button
    -- luacheck: globals mousegrabber keygrabber dbus
    awesome      = require( "awesome"      )
    root         = require( "root"         )
    tag          = require( "tag"          )
    screen       = require( "screen"       )
    client       = require( "client"       )
    mouse        = require( "mouse"        )
    drawin       = require( "drawin"       )
    button       = require( "button"       )
    keygrabber   = require( "keygrabber"   )
    mousegrabber = require( "mousegrabber" )
    dbus         = require( "dbus"         )

    -- Force luacheck to be silent about setting those as unused globals
    assert(awesome and root and tag and screen and client and mouse)

    -- Silence debug warnings
    require("gears.debug").print_warning = function() end
end

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
