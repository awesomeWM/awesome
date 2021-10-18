--DOC_GEN_IMAGE --DOC_NO_USAGE --DOC_HIDE_START --DOC_GEN_OUTPUT

screen[1]._resize {x = 0, width = 640, height = 480}


local awful = {
    client = require("awful.client"),
    wibar = require("awful.wibar"),
    tag = require("awful.tag"),
    tag_layout = require("awful.layout.suit.tile")
}

function awful.spawn(_, args)
    local c = client.gen_fake{}
    c:tags({args.tag})
    assert(#c:tags() == 1)
    assert(c:tags()[1] == args.tag)
end

screen[1].padding = {
    left   = 40,
    right  = 40,
    top    = 20,
    bottom = 20,
}

local wibar = awful.wibar {
    position = "top",
    height   = 24,
}

awful.tag.add("1", {
    screen              = screen[1],
    selected            = true,
    layout              = awful.tag_layout.right,
    gap                 = 5,
    master_count        = 2,
    master_width_factor = 0.66
})

local clients = {
    ['master #1 \nCol:1, Sum: 2\nRatio: 1/2'] = client.gen_fake{},
    ['master #2 \nCol:1, Sum: 2\nRatio: 1/2'] = client.gen_fake{},
    ['slave  #1 \nCol:2, Sum: 3\nRatio: 1/3'] = client.gen_fake{},
    ['slave  #2 \nCol:2, Sum: 3\nRatio: 1/3'] = client.gen_fake{},
    ['slave  #3 \nCol:2, Sum: 3\nRatio: 1/3'] = client.gen_fake{}
}

for _,c in ipairs(clients) do
    c:tags{"1"}
end

local tag = screen[1].selected_tag

local param = {
    tag              = tag,
    screen           = 1,
    clients          = tag:clients(),
    focus            = nil,
    geometries       = setmetatable({}, {__mode = "k"}),
    workarea         = tag.screen.workarea,
    useless_gap      = tag.gaps or 4,
    apply_size_hints = false,
}

-- wfact only works after the first arrange call...
tag.layout.arrange(param)

--DOC_HIDE_END

   for i, c in ipairs(client.get()) do
       local data = awful.client.idx(c)
       print("Client #"..i..":", data.col, data.idx, data.num)
   end

--DOC_HIDE_START

return {
    factor                = 2    ,
    show_boxes            = true,
    draw_wibars           = {wibar},
    draw_clients          = clients,
    display_screen_info   = false,
    draw_mwfact           = true,
    draw_wfact            = true,
    draw_areas            = false,
}
