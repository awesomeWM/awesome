--DOC_GEN_IMAGE --DOC_HIDE
local awful = { placement = require("awful.placement") }--DOC_HIDE

require("awful.tag").add("1", {screen=screen[1], selected=true}) --DOC_HIDE
screen[1]._resize {x= 0, width = 640, height=200} --DOC_HIDE

local parent_client = client.gen_fake {x = 0, y = 0, width=350, height=70} --DOC_HIDE
parent_client:_hide_all() --DOC_HIDE
awful.placement.centered(client.focus) --DOC_HIDE
parent_client:set_label("Parent client") --DOC_HIDE

for _, pos in ipairs{"left", "right", "top", "bottom"} do
    for _, anchor in ipairs{"front", "middle", "back"} do
        local c1 = client.gen_fake {x = 0, y = 0, width=80, height=20} --DOC_HIDE
        c1:_hide_all() --DOC_HIDE
        local _,p,a = --DOC_HIDE
        awful.placement.next_to(
            client.focus,
            {
                preferred_positions = pos,
                preferred_anchors   = anchor,
                geometry            = parent_client,
            }
        )
        c1:set_label(pos.."+"..anchor) --DOC_HIDE
        assert(pos    == p) --DOC_HIDE
        assert(anchor == a) --DOC_HIDE

        --DOC_HIDE Make sure the border are correctly applied
        if pos == "left" then  --DOC_HIDE
            assert(c1.x + c1.width + 2*c1.border_width == parent_client.x) --DOC_HIDE
        end --DOC_HIDE
        if pos == "right" then --DOC_HIDE
            assert(c1.x == parent_client.x+parent_client.width+2*parent_client.border_width) --DOC_HIDE
        end --DOC_HIDE
        if pos == "top" then --DOC_HIDE
            assert(c1.y + c1.height + 2*c1.border_width == parent_client.y) --DOC_HIDE
        end --DOC_HIDE
        if pos == "bottom" then --DOC_HIDE
            assert(c1.y == parent_client.y+parent_client.height+2*parent_client.border_width)--DOC_HIDE
        end --DOC_HIDE

        --DOC_HIDE Make sure the "children" don't overshoot the parent geometry
        if pos ~= "right" then --DOC_HIDE
            assert(c1.x+c1.width+2*c1.border_width--DOC_HIDE
                <= parent_client.x+parent_client.width+2*parent_client.border_width) --DOC_HIDE
        end --DOC_HIDE
        if pos ~= "bottom" then --DOC_HIDE
            assert(c1.y+c1.height+2*c1.border_width --DOC_HIDE
                <= parent_client.y+parent_client.height+2*parent_client.border_width) --DOC_HIDE
        end --DOC_HIDE
        if pos ~= "left" then --DOC_HIDE
            assert(c1.x >= parent_client.x) --DOC_HIDE
        end --DOC_HIDE
        if pos ~= "top"  then --DOC_HIDE
            assert(c1.y >= parent_client.y) --DOC_HIDE
        end --DOC_HIDE
    end
end

return {hide_lines=true} --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
