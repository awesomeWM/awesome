-- Test if client's c:swap() corrupts the Lua stack

local runner = require("_runner")
local test_client = require("_client")

runner.run_steps({
                 -- Spawn two clients
                 function(count)
                     if count == 1 then
                         test_client()
                         test_client()
                     end
                     if #client.get() >= 2 then
                         return true
                     end
                 end,

                 -- Swap them
                 function()
                     assert(#client.get() == 2, #client.get())
                     local c1 = client.get()[1]
                     local c2 = client.get()[2]

                     c2:swap(c1)
                     c1:swap(c2)
                     c1:swap(c2)
                     c1:swap(c2)
                     c2:swap(c1)
                     return true
                 end,
             })

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
