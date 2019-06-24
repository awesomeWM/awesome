-- This testsuite intends to tests the delayed screen creation mode and it's
-- automatic fallback when no screens are created.

local runner = require("_runner")
local ascreen = require("awful.screen")
local gtable = require("gears.table")
local grect = require("gears.geometry").rectangle

local xft_dpi = 42

local fake_viewports, max_id, allow_zero = screen._viewports(), 1, false
local called = 0

local saved_dpi1, saved_dpi2, saved_dpi3

-- Make sure what is tested matches some criteria.
-- Prevents silly errors when adding more test case.
local function validate_viewports(viewports)
    for _, a in ipairs(viewports) do
        assert(type(a.id) == "number")
        assert(a.id > max_id)
        assert(type(a.outputs ) == "table")
        assert(type(a.geometry) == "table")

        for _, prop in ipairs {"x", "y", "width", "height"} do
            assert(type(a.geometry[prop]) == "number")
            assert(a.geometry[prop] >= 0)
        end

        max_id = a.id
    end
end

local function fake_replace(viewports)
    fake_viewports = viewports
    screen.emit_signal("property::_viewports", fake_viewports)
end

-- Rollback the clock and replay the initialization.
local function fake_replay(viewports)
    if viewports then
        validate_viewports(viewports)
    end

    viewports = viewports or fake_viewports

    ascreen._viewports, fake_viewports = {}, viewports

    while screen.count() > 0 do
        screen[1]:fake_remove()
    end

    assert(screen.count() == 0)

    screen.emit_signal("scanning")
    screen.emit_signal("property::_viewports", fake_viewports)
    screen.emit_signal("scanned")
    assert(screen.count() == #viewports)

    assert(type(ascreen._get_xft_dpi()) ~= "boolean")

    -- Check of the viewports (+new metadata) is created.
    for k, a in ipairs(viewports) do
        local s = screen[k]

        -- Check if the extra metadata are computed.
        assert(s.data.viewport.maximum_dpi)
        assert(s.data.viewport.minimum_dpi)
        assert(s.data.viewport.preferred_dpi)
        assert(s.dpi)
        assert(s.maximum_dpi   == s.data.viewport.maximum_dpi  )
        assert(s.minimum_dpi   == s.data.viewport.minimum_dpi  )
        assert(s.preferred_dpi == s.data.viewport.preferred_dpi)

        -- Make sure it enters either the main `if` or the fallback one.
        assert(s.data.viewport.minimum_dpi   ~= math.huge)
        assert(s.data.viewport.preferred_dpi ~= math.huge)

        -- Check the range.
        assert(s.data.viewport.maximum_dpi   >= s.data.viewport.minimum_dpi)
        assert(s.data.viewport.preferred_dpi >= s.data.viewport.minimum_dpi)
        assert(s.data.viewport.preferred_dpi <= s.data.viewport.maximum_dpi)

        -- Check if the screen was created using the right viewport
        -- (order *is* relevant).
        assert(#s.data.viewport.outputs == #a.outputs)
        assert(s.data.viewport and s.data.viewport.id == a.id)
    end

    -- The original shall not be modified, the CAPI for this is read-only.
    for _, a in ipairs(fake_viewports) do
        assert(not a.minimum_dpi  )
        assert(not a.maximum_dpi  )
        assert(not a.preferred_dpi)
        assert(not a.dpi          )
    end
end

local function remove_output(viewport, name)
    for k, output in ipairs(viewport.outputs) do
        if output.name == name then
            table.remove(viewport.outputs, k)
            screen.emit_signal("property::_viewports", fake_viewports)
            return
        end
    end
    assert(false)
end

local function add_output(viewport, output)
    table.insert(viewport.outputs, output)
    screen.emit_signal("property::_viewports", fake_viewports)
end

local function remove_viewport(id)
    for k, a in ipairs(fake_viewports) do
        if a.id == id then
            table.remove(fake_viewports, k)
            screen.emit_signal("property::_viewports", fake_viewports)
            return
        end
    end
    assert(false)
end

local function add_viewport(viewport)
    assert(viewport.id)
    assert(viewport.geometry)
    assert(viewport.outputs)
    table.insert(fake_viewports, viewport)
    screen.emit_signal("property::_viewports", fake_viewports)
end

local steps = {

-- Monkeypatch for the tests.
function()
    -- This should be true for all tests for.
    assert(not screen.automatic_factory)

    -- Avoids touching the real XRDB, the tests don't control it.
    function ascreen._get_xft_dpi()
        return xft_dpi
    end

    -- This testsuite needs to manipulate this.
    function ascreen._get_viewports()
        local clone = gtable.clone(fake_viewports, true)
        assert(#clone > 0 or allow_zero)
        return clone
    end

    return true
end,

-- Remove all the screens, which is **not** supported, but necessary to play
-- with the initial screen creation code.
function()
    -- Simple scenario, a single screen.
    fake_replay {
        {
            -- 1 has already been consumed, this makes the test more future proof.
            id       = 2,
            geometry = { x = 0, y = 0, width = 1337, height = 42 },
            outputs  = {{ name = "leet", mm_width = 353, mm_height = 11 }},
        },
    }

    -- It should honor the XFT DPI when set.
    assert(xft_dpi == screen[1].dpi)

    -- Now that we know it works, unset it for the other steps.
    xft_dpi = nil

    -- Check if the "normal" fallback works.
    fake_replay()
    assert(screen[1].dpi ~= 42)

    return true
end,

-- Test with multiple outputs.
function()
    -- Simple scenario, a single screen.
    fake_replay {
        {
            -- 1 has already been consumed, this makes the test more future proof.
            id       = 3,
            geometry = { x = 0, y = 0, width = 1337, height = 42 },
            outputs  = {
                { name = "leet", mm_width = 353  , mm_height = 11   },
                { name = "leet", mm_width = 353/2, mm_height = 11/2 },
                { name = "leet", mm_width = 353/3, mm_height = 11/3 },
            },
        },
    }

    assert(screen[1].maximum_dpi   == screen[1].minimum_dpi*3)
    assert(screen[1].preferred_dpi == screen[1].minimum_dpi  )

    return true
end,

-- Test with zero outputs.
function()
    -- Simple scenario, a single screen.
    fake_replay {
        {
            -- 1 has already been consumed, this makes the test more future proof.
            id       = 4,
            geometry = { x = 0, y = 0, width = 1337, height = 42 },
            outputs  = {},
        },
    }

    assert(screen[1].maximum_dpi   == screen[1].minimum_dpi)
    assert(screen[1].preferred_dpi == screen[1].minimum_dpi)

    return true
end,

-- Test with 2 screens.
function()
    fake_replay {
        {
            -- 1 has already been consumed, this makes the test more future proof.
            id       = 5,
            geometry = { x = 0, y = 0, width = 1337, height = 42 },
            outputs  = {{ name = "0x1ee7", mm_width = 353, mm_height = 11 }},
        },
        {
            -- 1 has already been consumed, this makes the test more future proof.
            id       = 6,
            geometry = { x = 1337, y = 0, width = 1337, height = 42 },
            outputs  = {{ name = "3leet", mm_width = 353, mm_height = 11 }},
        },
    }

    saved_dpi1 = screen[1].preferred_dpi

    return true
end,

-- Test with 2 screens with twice the density.
function()
    -- Simple scenario, a single screen.
    fake_replay {
        {
            -- 1 has already been consumed, this makes the test more future proof.
            id       = 7,
            geometry = { x = 0, y = 0, width = 1337, height = 42 },
            outputs  = {{ name = "0x1ee7", mm_width = 353/2, mm_height = 11/2 }},
        },
        {
            -- 1 has already been consumed, this makes the test more future proof.
            id       = 8,
            geometry = { x = 1337, y = 0, width = 1337, height = 42 },
            outputs  = {{ name = "3leet", mm_width = 353/3, mm_height = 11/3 }},
        },
    }

    -- Test if scaling works as expected.
    assert(saved_dpi1*2 == screen[1].preferred_dpi)
    assert(saved_dpi1*3 == screen[2].preferred_dpi)

    -- Revert to the previous settings if the DPI happens to accidentally match.
    -- For the following tests, it has to be different from the fallback.
    if screen[1].preferred_dpi == screen[1].dpi then
        fake_replay {
            {
                -- 1 has already been consumed, this makes the test more future proof.
                id       = 9,
                geometry = { x = 0, y = 0, width = 1337, height = 42 },
                outputs  = {{ name = "0x1ee7", mm_width = 353, mm_height = 11 }},
            }
        }
    end

    saved_dpi1 = screen[1].preferred_dpi

    -- The next test requires a different value.
    assert(saved_dpi1 ~= screen[1].dpi)

    -- Enable autodpi.
    ascreen.set_auto_dpi_enabled(true)

    return true
end,

-- Test if autodpi does something.
function()
    -- Replay with the same settings.
    fake_replay()

    -- If this isn't true, then auto-dpi didn't do its job.
    assert(screen[1].dpi == saved_dpi1)

    -- Revert to a single viewport for the next tests.
    fake_replay {
        {
            -- 1 has already been consumed, this makes the test more future proof.
            id       = 10,
            geometry = { x = 0, y = 0, width = 1337, height = 42 },
            outputs  = {{ name = "leet", mm_width = 353, mm_height = 11 }},
        },
    }

    saved_dpi1 = screen[1].dpi
    saved_dpi2 = screen[1].maximum_dpi
    saved_dpi3 = screen[1].minimum_dpi

    return true
end,

-- Test adding an output.
function()
    local viewport = screen[1].data.viewport

    add_output(fake_viewports[1], {
        name      = "foobar",
        mm_width  = 353/2,
        mm_height = 11/2
    })
    add_output(fake_viewports[1], {
        name      = "foobar2",
        mm_width  = 353*2,
        mm_height = 11*2
    })

    -- It should have been kept.
    assert(viewport == screen[1].data.viewport)

    -- If this isn't true, then auto-dpi didn't do its job.
    assert(screen[1].dpi ~= saved_dpi1)

    -- Now that there is multiple DPIs for the same viewport, the number
    -- should double.
    assert(#screen[1].data.viewport.outputs == 3)
    assert(screen[1].maximum_dpi == saved_dpi2*2)
    assert(screen[1].minimum_dpi == saved_dpi3/2)
    assert(screen[1].dpi         == saved_dpi1/2)

    remove_output(fake_viewports[1], "leet")
    assert(#screen[1].data.viewport.outputs == 2)
    remove_output(fake_viewports[1], "foobar")

    -- At this point, only 1 DPI is left.
    assert(#screen[1].data.viewport.outputs == 1)
    assert(screen[1].maximum_dpi == saved_dpi1/2)
    assert(screen[1].minimum_dpi == saved_dpi1/2)
    assert(screen[1].dpi         == saved_dpi1/2)

    return true
end,

-- Test adding/removing viewports.
function()
    assert(screen.count() == 1)

    add_viewport {
        id = 11,
        geometry = { x = 1337, y = 0, width = 1337, height = 42 },
        outputs = {{
            name      = "foobar",
            mm_width  = 353/2,
            mm_height = 11/2
        }}
    }

    assert(screen.count() == 2)
    assert(screen[1].data.viewport.id == 10)
    assert(screen[2].data.viewport.id == 11)
    assert(grect.are_equal(screen[1].data.viewport.geometry, screen[1].geometry))
    assert(grect.are_equal(screen[2].data.viewport.geometry, screen[2].geometry))
    assert(#ascreen._viewports == 2)

    remove_viewport(10)

    assert(#ascreen._viewports == 1)
    assert(screen.count() == 1)
    assert(screen[1].data.viewport.id == 11)
    assert(grect.are_equal(screen[1].data.viewport.geometry, screen[1].geometry))

    return true
end,

-- Test resizing.
function()
    local s, sa = screen[1], screen[1].data.viewport
    assert(screen.count() == 1)
    assert(#ascreen._viewports == 1)

    -- First, to the same size (replace internal screen with external screen).
    -- It may or may not create a different viewport in practice, but it's possible
    -- if the first screen is removed then the new one added in 2 steps.
    fake_replace {
        {
            id = 12,
            geometry = { x = 1337, y = 0, width = 1337, height = 42 },
            outputs  = {{ name = "foobar3", mm_width  = 353/2, mm_height = 11/2}}
        }
    }

    assert(screen.count() == 1)
    assert(s == screen[1])
    assert(s.data.viewport ~= sa)
    assert(s.data.viewport.id == 12)

    -- Now 2 smaller (resolution) screens side by side to make sure it doesn't
    -- go haywire with overlapping
    fake_replace {
        {
            id = 13,
            geometry = { x = 1337, y = 0, width = 680, height = 42 },
            outputs  = {{ name = "foobar4", mm_width  = 353/2, mm_height = 11/2}}
        },
        {
            id = 14,
            geometry = { x = 1337+680, y = 0, width = 1337, height = 42 },
            outputs  = {{ name = "foobar5", mm_width  = 353/2, mm_height = 11/2}}
        }
    }

    assert(screen.count() == 2)
    assert(s == screen[1])
    assert(s.data.viewport.id == 13)
    assert(s.geometry.x == 1337)
    assert(s.geometry.width == 680)

    -- Disconnect the default handler to test the fallback.
    screen.disconnect_signal("request::create", ascreen.create_screen_handler)
    screen.disconnect_signal("request::remove", ascreen.remove_screen_handler)
    screen.disconnect_signal("request::resize", ascreen.resize_screen_handler)

    return true
end,

-- Test the fallback screen creation.
function()
    fake_replay {
        {
            -- 1 has already been consumed, this makes the test more future proof.
            id       = 15,
            geometry = { x = 0, y = 0, width = 1337, height = 42 },
            outputs  = {{ name = "0x1ee7", mm_width = 353, mm_height = 11 }},
        },
        {
            -- 1 has already been consumed, this makes the test more future proof.
            id       = 16,
            geometry = { x = 1337, y = 0, width = 1337, height = 42 },
            outputs  = {{ name = "3leet", mm_width = 353, mm_height = 11 }},
        },
    }

    assert(screen.count() == 2)
    assert(screen[1].data.viewport.id == 15)
    assert(screen[2].data.viewport.id == 16)

    -- Connect custom handler and see if the internals accidently depend on
    -- implementation details.
    screen.connect_signal("request::create", function(viewport)
        assert(viewport.minimum_dpi)
        assert(viewport.maximum_dpi)
        assert(viewport.preferred_dpi)
        local geo = viewport.geometry
        local s = screen.fake_add(geo.x, geo.y, geo.width, geo.height)
        s.outputs = viewport.outputs
        called = called + 1
    end)
    screen.connect_signal("request::remove", function(viewport)
        local geo = viewport.geometry
        for s in screen do
            if grect.are_equal(geo, s.geometry) then
                s:fake_remove()
                called = called + 1
                return
            end
        end
    end)

    return true
end,

-- Test if alternate implementations don't explode the internals.
function()
    ascreen._viewports = {}

    while screen.count() > 0 do
        screen[1]:fake_remove()
    end

    assert(screen.count() == 0)

--     screen.emit_signal("scanning")
    screen.emit_signal("property::_viewports", fake_viewports)
    screen.emit_signal("scanned")
    assert(screen.count() == 2)

    assert(grect.are_equal(
        screen[1].geometry, {x = 0, y = 0, width = 1337, height = 42}
    ))
    assert(grect.are_equal(
        screen[2].geometry, {x = 1337, y = 0, width = 1337, height = 42}
    ))

    -- Call the DPI properties just to check it doesn't cause an error)
    assert(screen[1].dpi           or true)
    assert(screen[1].maximum_dpi   or true)
    assert(screen[1].minimum_dpi   or true)
    assert(screen[1].preferred_dpi or true)
    assert(screen[2].dpi           or true)
    assert(screen[2].maximum_dpi   or true)
    assert(screen[2].minimum_dpi   or true)
    assert(screen[2].preferred_dpi or true)
    assert(called == 2)

    -- Try adding an viewport.
    table.insert(fake_viewports, {
        -- 1 has already been consumed, this makes the test more future proof.
        id       = 17,
        geometry = { x = 1337, y = 42, width = 1337, height = 42 },
        outputs  = {{ name = "last", mm_width = 353, mm_height = 11 }},
    })
    screen.emit_signal("property::_viewports", fake_viewports)
    assert(screen.count() == 3)
    assert(called == 3)
    assert(grect.are_equal(
        screen[3].geometry, {x = 1337, y = 42, width = 1337, height = 42}
    ))

    -- Remove the middle one.
    table.remove(fake_viewports, 2)
    screen.emit_signal("property::_viewports", fake_viewports)

    assert(screen.count() == 2)
    assert(called == 4)
    assert(grect.are_equal(
        screen[1].geometry, {x = 0, y = 0, width = 1337, height = 42}
    ))
    assert(grect.are_equal(
        screen[2].geometry, {x = 1337, y = 42, width = 1337, height = 42}
    ))

    return true
end
}

runner.run_steps(steps)
