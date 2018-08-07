-- Needed for root.fake_input
local keygrabber = {_current_grabber = nil}

local function stop()
    keygrabber._current_grabber = nil
end

local function run(grabber)
    keygrabber._current_grabber = grabber
end

keygrabber = {
    run       = run,
    stop      = stop,
    isrunning = function() return keygrabber._current_grabber ~= nil end,
}

return keygrabber

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
