--DOC_HEADER --DOC_NO_USAGE --DOC_ASTERISK

local awful = { keygrabber = require("awful.keygrabber") } --DOC_HIDE

local works = false --DOC_HIDE

awful.keygrabber{autostart=true, stop_key = "!", --DOC_HIDE
    stop_callback = function(_,_,_, seq) works=seq=="Hello world" end} --DOC_HIDE

  local function send_string_to_client(s, c)
      local old_c = client.focus
      client.focus = c
      for i=1, #s do
          local char = s:sub(i,i)
          root.fake_input("key_press"  , char)
          root.fake_input("key_release", char)
      end
      client.focus = old_c
  end
--DOC_NEWLINE
  send_string_to_client("Hello world!")

assert(works) --DOC_HIDE
