--DOC_GEN_IMAGE --DOC_HIDE
local parent = ... --DOC_HIDE
local wibox  = require("wibox") --DOC_HIDE
local gears  = {color=require("gears.color"), shape=require("gears.shape")} --DOC_HIDE

--DOC_HIDE for i = 0, 59 do print(math.floor(math.sin(i/30*math.pi)*64 + 0.5)) end
local sine = { --DOC_HIDE
      0,   7,  13,  20,  26,  32,  38,  43,  48,  52,  55,  58,  61,  63,  64, --DOC_HIDE
     64,  64,  63,  61,  58,  55,  52,  48,  43,  38,  32,  26,  20,  13,   7, --DOC_HIDE
      0,  -7, -13, -20, -26, -32, -38, -43, -48, -52, -55, -58, -61, -63, -64, --DOC_HIDE
    -64, -64, -63, -61, -58, -55, -52, -48, -43, -38, -32, -26, -20, -13,  -7, --DOC_HIDE
} --DOC_HIDE

local l = wibox.layout { --DOC_HIDE
    forced_height = 100, --DOC_HIDE
    forced_width  = 100, --DOC_HIDE
    spacing       = 5, --DOC_HIDE
    layout        = wibox.layout.flex.vertical --DOC_HIDE
} --DOC_HIDE

local colors = {
    "#ff0000ff",
    "#00ff00ff",
    "#0000ffff"
}

--DOC_NEWLINE

local w = --DOC_HIDE
wibox.widget {
    group_colors = colors,
    step_width   = 1,
    step_spacing = 3, -- Leave free space for shifted bars
    group_start  = function(self, context, cr, group_idx) --luacheck: no unused
        -- We must set the group color ourselves,
        -- since we have overriden the default behavior.
        cr:set_source(gears.color(self:pick_data_group_color(group_idx)))
        -- Shift each group right for this amount of pixels to prevent overlap
        return group_idx - 1
    end,
    scale        = true, --DOC_HIDE
    border_width = 2, --DOC_HIDE
    margins      = 5, --DOC_HIDE
    widget       = wibox.widget.graph,
}

l:add(w) --DOC_HIDE

for idx = 0, #sine-1 do --DOC_HIDE
    for group = 1, 3 do --DOC_HIDE
        --DOC_HIDE 64*math.sin(group*idx/30*math.pi) * (4-group)
        local v = sine[((idx*group) % #sine)+1] * (4-group) --DOC_HIDE
        w:add_value(v, group) --DOC_HIDE
    end --DOC_HIDE
end --DOC_HIDE

parent:add(l) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
