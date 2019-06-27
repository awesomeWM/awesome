--DOC_GEN_OUTPUT --DOC_NO_USAGE --DOC_HIDE_ALL --DOC_ASTERISK --DOC_RAW_OUTPUT --DOC_GEN_IMAGE
local module = ...

module.generate_nav_table {
    class = "client",
    content = {
        {
            association   = "aggregation",
            class         = "tag",
            to_property   = "c.tags",
            from_property = "t:clients()",
            left = {
                msg   = "Is tagged on",
                card  = "1..N",
            },
            right = {
                msg   = "Attached to",
                card  = "0..N",
            },
        },
        {
            association   = "aggregation",
            class         = "screen",
            to_property   = "c.screen",
            from_property = "s.clients",
            left = {
                msg   = "Is on",
                card  = "1..1",
            },
            right = {
                msg   = "Has",
                card  = "0..N",
            },
        },
        {
            association   = "aggregation",
            class         = "screen",
            from_property = "s.hidden_clients",
            left = {
                msg   = "Is on",
                card  = "1..1",
            },
            right = {
                msg   = "Has",
                card  = "0..N",
            },
        },
        {
            association   = "aggregation",
            class         = "screen",
            from_property = "s.tiled_clients",
            left = {
                msg   = "Is on",
                card  = "1..1",
            },
            right = {
                msg   = "Has",
                card  = "0..N",
            },
        },
        {
            association = "aggregation",
            class       = "awful.key",
            to_property = "c:keys()",
            left = {
                msg   = "Has",
                card  = "0..N",
            },
            right = {
                msg   = "Attached to",
                card  = "0..N",
            },
        },
        {
            association = "aggregation",
            class       = "awful.button",
            to_property = "c:buttons()",
            left = {
                msg   = "Has",
                card  = "0..N",
            },
            right = {
                msg   = "Attached to",
                card  = "0..N",
            },
        },
        {
            association   = "aggregation",
            class         = "mouse",
            from_property = "mouse.current_client",
            left = {
                msg   = "Is below",
                card  = "0..1",
            },
            right = {
                msg   = "Is over",
                card  = "0..1",
            },
        },
    }
}
