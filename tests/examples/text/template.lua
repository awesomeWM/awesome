local file_path, _, luacovpath = ...

-- Set the global shims
-- luacheck: globals awesome root tag screen client mouse drawin
awesome = require( "awesome" )
root    = require( "root"    )
tag     = require( "tag"     )
screen  = require( "screen"  )
client  = require( "client"  )
mouse   = require( "mouse"   )
drawin  = require( "drawin"  )

-- If luacov is available, use it. Else, do nothing.
pcall(function()
    require("luacov.runner")(luacovpath)
end)

-- Silence debug warnings
require("gears.debug").print_warning = function() end

-- Execute the test
loadfile(file_path)()
