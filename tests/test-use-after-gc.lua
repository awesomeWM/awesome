-- Test behaviour of C objects after they were finalized

local runner = require("_runner")

local done
do
    local obj
    local func = function()
        assert(obj.valid == false)
        assert(not pcall(function()
            print(obj.visible)
        end))
        assert(not pcall(function()
            obj.visible = true
        end))
        done = true
    end
    if _VERSION >= "Lua 5.2" then
        setmetatable({}, { __gc = func })
    else
        local newproxy = newproxy -- luacheck: globals newproxy
        getmetatable(newproxy(true)).__gc = func
    end
    obj = drawin({})
end
collectgarbage("collect")
assert(done)

runner.run_steps({ function() return true end })

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
