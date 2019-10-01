--DOC_GEN_OUTPUT --DOC_NO_USAGE --DOC_HIDE_ALL --DOC_ASTERISK --DOC_RAW_OUTPUT --DOC_GEN_IMAGE
local module = ...

module.generate_nav_table {
    class = "tag",
    content = {
        {
            association   = "aggregation",
            class         = "client",
            from_property = "c.tags",
            to_property   = "t:clients()",
            right = {
                msg   = "Is tagged on",
                card  = "1..N",
            },
            left  = {
                msg   = "Has",
                card  = "0..N",
            },
        },
        {
            association   = "composition",
            class         = "screen",
            to_property   = "t.screen",
            from_property = "s.tags",
            left = {
                msg   = "Is displayed on",
                card  = "1..1",
            },
            right = {
                msg   = "Has",
                card  = "0..N",
            },
        },
        {
            association   = "composition",
            class         = "screen",
            from_property = "s.selected_tag",
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
            association   = "composition",
            class         = "screen",
            from_property = "s.selected_tags",
            left = {
                msg   = "Is on",
                card  = "1..1",
            },
            right = {
                msg   = "Has",
                card  = "0..N",
            },
        },
    }
}
