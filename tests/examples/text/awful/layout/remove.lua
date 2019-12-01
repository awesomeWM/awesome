--DOC_GEN_OUTPUT --DOC_HIDE
local awful = { layout = require("awful.layout"), --DOC_HIDE
                suit= require("awful.layout.suit")} --DOC_HIDE

awful.layout.append_default_layouts({
    awful.layout.suit.floating,
    awful.layout.suit.tile,
    awful.layout.suit.max,
})

for _, l in ipairs(awful.layout.layouts) do
    print("Before:", l.name)
end

--DOC_NEWLINE

awful.layout.remove_default_layout(awful.layout.suit.tile)

--DOC_NEWLINE

for _, l in ipairs(awful.layout.layouts) do
    print("After:", l.name)
end
