--DOC_HIDE --DOC_NO_USAGE
local gears = {matcher = require("gears.matcher")} --DOC_HIDE


    local matcher = gears.matcher()

--DOC_NEWLINE

    matcher:append_rule( "my.source", {
        rule = {
            my_any_rule = true,
        },
        rule_every = {
            every1 = {1, 42},
            every2 = {2, 42},
            every3 = {3, 42},
        },
        except = {
            except1 = 1,
        },
        properties = {
            was_a_match = true,
        },
    })

--DOC_NEWLINE

    local candidate1 = {
        my_any_rule = true,
        every1      = 1,
        every2      = 2,
        every3      = 3,
        was_a_match = false,
    }

--DOC_NEWLINE

    local candidate2 = {
        every2      = 2,
        was_a_match = false,
    }

--DOC_NEWLINE

    local candidate3 = {
        my_any_rule = true,
        was_a_match = false,
        every1      = 1,
        every2      = 2,
        every3      = 3,
        except1     = 1,
    }


--DOC_NEWLINE

    matcher:apply(candidate1)
    matcher:apply(candidate2)
    matcher:apply(candidate3)

--DOC_NEWLINE

    -- Only candidate1 fits all criteria.
    assert(candidate1.was_a_match == true )
    assert(candidate2.was_a_match == false)
    assert(candidate3.was_a_match == false)

--DOC_NEWLINE

    -- It is also possible to match number property by range.
    matcher:append_rule( "my.source", {
        rule_greater = {
            value = 50,
        },
        rule_lesser = {
            value = 100,
        },
        properties = {
            was_a_match = true,
        },
    })

--DOC_NEWLINE

    local candidate4 = { value = 40 , was_a_match = false }
    local candidate5 = { value = 75 , was_a_match = false }
    local candidate6 = { value = 101, was_a_match = false }

--DOC_NEWLINE

    matcher:apply(candidate4)
    matcher:apply(candidate5)
    matcher:apply(candidate6)

--DOC_NEWLINE

    -- Only candidate5 fits all criteria.
    assert(candidate4.was_a_match == false)
    assert(candidate5.was_a_match == true )
    assert(candidate6.was_a_match == false)
