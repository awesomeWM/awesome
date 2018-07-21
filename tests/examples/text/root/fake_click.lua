--DOC_NO_USAGE --DOC_ASTERISK

  local function click(button_id, x, y)
      mouse.coords {x = x, y = y}
      root.fake_input("button_press" , button_id)
      root.fake_input("button_release", button_id)
  end
--DOC_NEWLINE
  click(1, 42, 42)
