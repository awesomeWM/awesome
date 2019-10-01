--DOC_GEN_OUTPUT --DOC_NO_USAGE --DOC_HIDE_ALL --DOC_ASTERISK --DOC_RAW_OUTPUT --DOC_GEN_IMAGE
local module = ...

module.generate_nav_table {
    class = "screen",
    content = {
        {
            association   = "aggregation",
            class         = "tag",
            to_property   = "s.tags",
            from_property = "t.screen",
            left = {
                msg   = "Has",
                card  = "0..N",
            },
            right = {
                msg   = "Attached to",
                card  = "1..1",
            },
        },
        {
            association   = "composition",
            class         = "tag",
            to_property   = "s.selected_tag",
            left = {
                msg   = "Has",
                card  = "0..1",
            },
            right = {
                msg   = "Is on",
                card  = "1..1",
            },
        },
        {
            association   = "composition",
            class         = "tag",
            to_property   = "s.selected_tags",
            left = {
                msg   = "Has",
                card  = "0..N",
            },
            right = {
                msg   = "Is on",
                card  = "1..1",
            },
        },
        {
            association   = "aggregation",
            class         = "client",
            from_property = "c.screen",
            to_property   = "s.clients",
            left = {
                msg   = "Has",
                card  = "0..N",
            },
            right = {
                msg   = "Is on",
                card  = "1..1",
            },
        },
        {
            association   = "aggregation",
            class         = "client",
            to_property   = "s.hidden_clients",
            left = {
                msg   = "Has",
                card  = "1..1",
            },
            right = {
                msg   = "Is on",
                card  = "1..1",
            },
        },
        {
            association   = "aggregation",
            class         = "client",
            to_property   = "s.tiled_clients",
            left = {
                msg   = "Has",
                card  = "0..N",
            },
            right = {
                msg   = "Has",
                card  = "1..1",
            },
        },
        {
            association   = "aggregation",
            class         = "naughty.notification",
            from_property = "n.screen",
            left = {
                msg   = "Displays",
                card  = "0..N",
            },
            right = {
                msg   = "Is displayed on",
                card  = "1..N",
            },
        },
    }
}
