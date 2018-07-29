local function assert_markup_format(markup)
    if #markup == 0 then
        return
    end
    local tag_count = 0
    for _ in string.gmatch(markup, '<.->') do tag_count = tag_count + 1 end
    assert.are_equal(2, tag_count)
end

local function get_first_part(markup)
    return string.match(markup, '^(.-)<')
end
local function get_highlighted_part(markup)
    return string.match(markup, '>(.-)<')
end
local function get_last_part(markup)
    return string.match(markup, '</span>(.-)$')
end
local function get_prompt_text(markup)
    return get_first_part(markup)..get_highlighted_part(markup)..get_last_part(markup)
end

local function get_tag(markup)
    return string.match(markup, '<.->')
end

describe('helper functions', function()
    local sample_markup = 'First<span property1="foo">Highlighted</span>End'
    it('main', function()
        assert.are_equal('First', get_first_part(sample_markup))
        assert.are_equal('Highlighted', get_highlighted_part(sample_markup))
        assert.are_equal('End', get_last_part(sample_markup))
        assert.are_equal('FirstHighlightedEnd', get_prompt_text(sample_markup))
        assert.are_equal('<span property1="foo">', get_tag(sample_markup))
    end)
end)

local function enter_text(callback, text)
    for char in string.gmatch(text, '.') do
      callback({}, char, 'press')
      callback({}, char, 'release')
  end
end

insulate('main', function ()
    _G.awesome = {
        connect_signal = function() end
    }
    _G.keygrabber = {
        run                       = function() end,
        stop                      = function() end,
    }
    -- luacheck: globals string
    function string.wlen(self)
          return #self
    end
    local keygrabber = require("awful.keygrabber")
    package.loaded['awful.keygrabber'] = mock(keygrabber, true)
    keygrabber = require("awful.keygrabber")
    local prompt_callback
    package.loaded['awful.keygrabber'].run = function(g) prompt_callback = g end
    local gfs = require 'gears.filesystem'
    package.loaded['gears.filesystem'] = mock(gfs, true)
    local prompt = require 'awful.prompt'
    local markup
    local atextbox = {
      set_font = function() end,
      set_markup = function(_, input_markup)
          markup = input_markup
          assert_markup_format(markup)
      end,
    }

    local function assert_prompt_text(first_part, highlighted_part, last_part)
        assert.are_equal(first_part, get_first_part(markup))
        assert.are_equal(highlighted_part, get_highlighted_part(markup))
        assert.are_equal(last_part, get_last_part(markup))
    end

    describe('basic insertion', function()
        it('basic command', function()
          prompt.run{ textbox = atextbox }
          enter_text(prompt_callback, 'command')
          assert.are_equal('command ', get_prompt_text(markup))
        end)
        it('escape xml', function()
          prompt.run{ textbox = atextbox }
          enter_text(prompt_callback, '<')
          assert.are_equal('&lt; ', get_prompt_text(markup))
        end)
    end)

    describe('cancel cases', function()
        local function assert_cancel(mod, key)
            local done_callback = stub.new()
            prompt.run{
                textbox = atextbox,
                done_callback = done_callback
            }
            enter_text(prompt_callback, 'command')
            assert.False(prompt_callback({mod}, key, 'press'))
            assert.stub(done_callback).was_called()
            assert.stub(keygrabber.stop).was_called()
        end

        it('Ctrl+c', function()
            assert_cancel('Control', 'c')
        end)
        it('Ctrl+g', function()
            assert_cancel('Control', 'g')
        end)
        it('Escape', function()
            assert_cancel(nil, 'Escape')
        end)
    end)

    describe('execute cases', function()
        local function assert_execute(mod, key)
            local done_callback = stub.new()
            local exe_callback = stub.new()
            prompt.run{
                textbox = atextbox,
                done_callback = done_callback,
                exe_callback = exe_callback
            }
            enter_text(prompt_callback, 'command')
            prompt_callback({mod}, key, 'press')
            assert.stub(done_callback).was_called()
            assert.spy(exe_callback).was_called_with('command')
            assert.stub(keygrabber.stop).was_called()
        end
        it('Ctrl+j', function()
            assert_execute('Control', 'j')
        end)
        it('Ctrl+m', function()
            assert_execute('Control', 'm')
        end)
        it('Return', function()
            assert_execute(nil, 'Return')
        end)
        it('KP_Enter', function()
            assert_execute(nil, 'KP_Enter')
        end)
    end)

    describe('typing cases', function()
        it('moving cursor', function()
            prompt.run{
                textbox = atextbox,
            }
            enter_text(prompt_callback, 'command')
            prompt_callback({}, 'Home', 'press')
            assert_prompt_text('', 'c', 'ommand ')

            prompt_callback({}, 'Right', 'press')
            assert_prompt_text('c', 'o', 'mmand ')

            prompt_callback({}, 'End', 'press')
            assert_prompt_text('command', ' ', '')

            prompt_callback({}, 'Left', 'press')
            assert_prompt_text('comman', 'd', ' ')
        end)
        it('backspace', function()
            prompt.run{
                textbox = atextbox,
            }
            enter_text(prompt_callback, 'command')
            prompt_callback({}, 'BackSpace', 'press')
            assert_prompt_text('comman', ' ', '')

            prompt_callback({}, 'Home', 'press')
            prompt_callback({}, 'BackSpace', 'press')
            assert_prompt_text('', 'c', 'omman ')

            prompt_callback({}, 'Right', 'press')
            prompt_callback({}, 'BackSpace', 'press')
            assert_prompt_text('', 'o', 'mman ')

            prompt_callback({}, 'Right', 'press')
            prompt_callback({}, 'Right', 'press')
            prompt_callback({}, 'BackSpace', 'press')
            assert_prompt_text('o', 'm', 'an ')
        end)
        it('delete', function()
            prompt.run{
                textbox = atextbox,
            }
            enter_text(prompt_callback, 'command')
            prompt_callback({}, 'Delete', 'press')
            assert_prompt_text('command', ' ', '')

            prompt_callback({}, 'Left', 'press')
            prompt_callback({}, 'Delete', 'press')
            assert_prompt_text('comman', ' ', '')

            prompt_callback({}, 'Home', 'press')
            prompt_callback({}, 'Delete', 'press')
            assert_prompt_text('', 'o', 'mman ')

            prompt_callback({}, 'Right', 'press')
            prompt_callback({}, 'Right', 'press')
            prompt_callback({}, 'Delete', 'press')
            assert_prompt_text('om', 'a', 'n ')
        end)
    end)

    describe('key-pressed callback', function()
        it('callback called with right arguments', function()
            local keypressed_callback = spy.new(function() return true end)
            prompt.run{
                textbox = atextbox,
                keypressed_callback = keypressed_callback,
            }
            prompt_callback({'Control'}, 'x', 'press')
            assert.spy(keypressed_callback).was_called_with({['Control'] = true}, 'x', '')
        end)
        it('callback intercepts input', function()
            local keypressed_callback = spy.new(function() return true end)
            prompt.run{
                textbox = atextbox,
                keypressed_callback = keypressed_callback,
            }
            enter_text(prompt_callback, 'a')
            assert.spy(keypressed_callback).was_called_with({}, 'a', '')
            assert.are_equal(' ', get_prompt_text(markup))
        end)
        it('callback returns new prompt and command', function()
            local keypressed_callback = spy.new(function() return true, 'new_command', 'new_prompt:' end)
            prompt.run{
                textbox = atextbox,
                keypressed_callback = keypressed_callback,
            }
            enter_text(prompt_callback, 'a')
            assert.spy(keypressed_callback).was_called_with({}, 'a', '')
            assert.are_equal('new_prompt:new_command ', get_prompt_text(markup))
        end)
    end)

    describe('key-released callback', function()
        local keyreleased_callback = stub.new()
        it('callback called with mods', function()
            prompt.run{
                textbox = atextbox,
                keyreleased_callback = keyreleased_callback,
            }
            prompt_callback({'Control'}, 'x', 'release')
            assert.stub(keyreleased_callback).was_called_with({['Control'] = true}, 'x', '')
        end)
        it('callbacks called with command', function()
            prompt.run{
                textbox = atextbox,
                keyreleased_callback = keyreleased_callback,
            }
            enter_text(prompt_callback, 'com')
            assert.stub(keyreleased_callback).was_called_with({}, 'c', 'c')
            assert.stub(keyreleased_callback).was_called_with({}, 'o', 'co')
            assert.stub(keyreleased_callback).was_called_with({}, 'm', 'com')
        end)
    end)

    describe('hooks', function()
        it('callback called', function()
            local callback_arg = ''
            local callback = function(arg) callback_arg = arg end
            prompt.run{
                textbox = atextbox,
                hooks = {
                    { {'Shift'}, "Return", callback }
                }
            }
            enter_text(prompt_callback, 'command')
            prompt_callback({'Shift'}, 'Return', 'press')
            assert.are_equal('command', callback_arg)
        end)
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
