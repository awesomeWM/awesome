local awful = require("awful")

local foo = {}

local function do_useless_work()
    print("doing useless work", os.date())
    require("menubar.utils").parse_dir('/usr/share/applications/', do_useless_work)
end
do_useless_work()

for i=1,10 do
    table.insert(foo, function()
        collectgarbage("collect")
        require("menubar").show()
        return true
    end)
    for i=1,4 do
        table.insert(foo, function(count)
            collectgarbage("collect")
            if count == 5 then
                return true
            end
        end)
    end
    table.insert(foo, function()
        collectgarbage("collect")
        require("menubar").hide()
        return true
    end)
    table.insert(foo, function(count)
        collectgarbage("collect")
        if count == 5 then
            return true
        end
    end)
end

require("_runner").run_steps(foo)
