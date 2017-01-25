local awful = require("awful")

require("_runner").run_steps({
    function()
        require("menubar").show()
        return true
    end,
    function(count)
        if count == 5 then
            return true
        end
    end,
    function(count)
        if count == 5 then
            return true
        end
    end,
    function(count)
        if count == 5 then
            return true
        end
    end,
    function(count)
        if count == 5 then
            return true
        end
    end,
    function(count)
        if count == 5 then
            return true
        end
    end,
    function(count)
        if count == 5 then
            return true
        end
    end,
    function(count)
        if count == 5 then
            return true
        end
    end,
    function(count)
        if count == 5 then
            return true
        end
    end,
    function(count)
        if count == 5 then
            return true
        end
    end,
    function(count)
        if count == 5 then
            return true
        end
    end,
    function(count)
        if count == 5 then
            require("menubar").hide()
            return true
        end
    end,
    function(count)
        if count == 5 then
            return true
        end
    end,
})
