local function create_task_boilerplate()
  local date = os.date("%Y-%m-%d %H:%M:%S%z")

  -- convert -0300 → -03:00
  date = date:gsub("([+-]%d%d)(%d%d)$", "%1:%2")

  local boilerplate = {
    "",
    "% Title",
    "",
    ".state todo",
    ".date  " .. date,
    ".tags",
    "",
    "Description",
    "",
  }

  local buf = vim.api.nvim_get_current_buf()
  local last = vim.api.nvim_buf_line_count(buf)

  vim.api.nvim_buf_set_lines(buf, last, last, false, boilerplate)

  -- move cursor to "% Title"
  local title_line = last + 2
  vim.api.nvim_win_set_cursor(0, { title_line, 2 })

  -- start visual selection of "Title"
  vim.cmd("normal! viw")
end

local function set_task_state(state)
  local buf = vim.api.nvim_get_current_buf()
  local cursor = vim.api.nvim_win_get_cursor(0)
  local row = cursor[1] - 1
  local line_count = vim.api.nvim_buf_line_count(buf)

  -- find task start (%)
  local start = nil
  for i = row, 0, -1 do
    local line = vim.api.nvim_buf_get_lines(buf, i, i + 1, false)[1]
    if line:match("^%%") then
      start = i
      break
    end
  end

  if not start then
    return
  end

  -- find task end (next % or EOF)
  local finish = line_count
  for i = start + 1, line_count - 1 do
    local line = vim.api.nvim_buf_get_lines(buf, i, i + 1, false)[1]
    if line:match("^%%") then
      finish = i
      break
    end
  end

  -- search for .state inside block
  for i = start, finish - 1 do
    local line = vim.api.nvim_buf_get_lines(buf, i, i + 1, false)[1]

    if line:match("^%.state") then
      vim.api.nvim_buf_set_lines(buf, i, i + 1, false, { ".state " .. state })

      -- save file
      vim.cmd("write")

      -- return to normal mode
      vim.cmd("stopinsert")

      return
    end
  end
end

local function goto_tags()
  local buf = vim.api.nvim_get_current_buf()
  local cursor = vim.api.nvim_win_get_cursor(0)
  local row = cursor[1] - 1
  local line_count = vim.api.nvim_buf_line_count(buf)

  local start = nil
  for i = row, 0, -1 do
    local line = vim.api.nvim_buf_get_lines(buf, i, i+1, false)[1]
    if line:match("^%%") then
      start = i
      break
    end
  end

  if not start then
    return
  end

  local finish = line_count
  for i = start + 1, line_count - 1 do
    local line = vim.api.nvim_buf_get_lines(buf, i, i+1, false)[1]
    if line:match("^%%") then
      finish = i
      break
    end
  end

  local tags_line = nil
  local date_line = nil

  for i = start, finish - 1 do
    local line = vim.api.nvim_buf_get_lines(buf, i, i+1, false)[1]

    if line:match("^%.tags") then
      tags_line = i
      break
    end

    if line:match("^%.date") then
      date_line = i
    end
  end

  if not tags_line then
    if not date_line then
      return
    end

    tags_line = date_line + 1

    vim.api.nvim_buf_set_lines(buf, tags_line, tags_line, false, {".tags"})
  end

  local line = vim.api.nvim_buf_get_lines(buf, tags_line, tags_line+1, false)[1]

  vim.api.nvim_win_set_cursor(0, {tags_line + 1, #line})
end

local function goto_title()
  local buf = vim.api.nvim_get_current_buf()
  local cursor = vim.api.nvim_win_get_cursor(0)
  local row = cursor[1] - 1

  -- search upward for task start (%)
  for i = row, 0, -1 do
    local line = vim.api.nvim_buf_get_lines(buf, i, i+1, false)[1]

    if line:match("^%%") then
      -- move cursor after "% "
      local col = 1
      if line:match("^%% ") then
        col = 2
      end

      vim.api.nvim_win_set_cursor(0, { i + 1, col })
      return
    end
  end
end

local function goto_description()
  local buf = vim.api.nvim_get_current_buf()
  local cursor = vim.api.nvim_win_get_cursor(0)
  local row = cursor[1] - 1
  local line_count = vim.api.nvim_buf_line_count(buf)

  -- find task start
  local start = nil
  for i = row, 0, -1 do
    local line = vim.api.nvim_buf_get_lines(buf, i, i+1, false)[1]
    if line:match("^%%") then
      start = i
      break
    end
  end

  if not start then
    return
  end

  local i = start + 1

  -- skip blank lines
  while i < line_count do
    local line = vim.api.nvim_buf_get_lines(buf, i, i+1, false)[1]
    if line ~= "" then break end
    i = i + 1
  end

  -- skip property lines
  while i < line_count do
    local line = vim.api.nvim_buf_get_lines(buf, i, i+1, false)[1]
    if not line:match("^%.[a-z]") then break end
    i = i + 1
  end

  -- skip blank lines between properties and description
  while i < line_count do
    local line = vim.api.nvim_buf_get_lines(buf, i, i+1, false)[1]
    if line ~= "" then break end
    i = i + 1
  end

  -- if description does not exist, create it
  if i >= line_count then
    vim.api.nvim_buf_set_lines(buf, line_count, line_count, false, {""})
    i = line_count
  end

  vim.api.nvim_win_set_cursor(0, { i + 1, 0 })
end

local function next_task()
  local buf = vim.api.nvim_get_current_buf()
  local row = vim.api.nvim_win_get_cursor(0)[1]
  local line_count = vim.api.nvim_buf_line_count(buf)

  for i = row, line_count - 1 do
    local line = vim.api.nvim_buf_get_lines(buf, i, i+1, false)[1]

    if line:match("^%%") then
      local col = line:match("^%% ") and 2 or 1
      vim.api.nvim_win_set_cursor(0, { i + 1, col })
      return
    end
  end
end

local function prev_task()
  local buf = vim.api.nvim_get_current_buf()
  local row = vim.api.nvim_win_get_cursor(0)[1] - 2

  for i = row, 0, -1 do
    local line = vim.api.nvim_buf_get_lines(buf, i, i+1, false)[1]

    if line:match("^%%") then
      local col = line:match("^%% ") and 2 or 1
      vim.api.nvim_win_set_cursor(0, { i + 1, col })
      return
    end
  end
end

local function task_picker()
  local pickers = require("telescope.pickers")
  local finders = require("telescope.finders")
  local conf = require("telescope.config").values
  local actions = require("telescope.actions")
  local action_state = require("telescope.actions.state")

  local buf = vim.api.nvim_get_current_buf()
  local lines = vim.api.nvim_buf_get_lines(buf, 0, -1, false)

  local tasks = {}

  for i, line in ipairs(lines) do
    if line:match("^%%") then
      local title = line:gsub("^%% ?", "")
      table.insert(tasks, {
        line = i,
        display = title,
      })
    end
  end

  if #tasks == 0 then
    vim.notify("There is not task to list", vim.log.levels.INFO)
    return
  end

  pickers.new({}, {
    prompt_title = "Tasks",
    finder = finders.new_table({
      results = tasks,
      entry_maker = function(entry)
        return {
          value = entry,
          display = entry.display,
          ordinal = entry.display,
          line = entry.line,
        }
      end,
    }),

    sorter = conf.generic_sorter({}),

    attach_mappings = function(_, map)
      actions.select_default:replace(function(prompt_bufnr)
        local selection = action_state.get_selected_entry()
        actions.close(prompt_bufnr)

        vim.api.nvim_win_set_cursor(0, { selection.line, 0 })
      end)

      return true
    end,
  }):find()
end

local function create_tasks_file()
  vim.ui.input({ prompt = "Tasks file name: " }, function(input)
    if not input or input == "" then
      return
    end

    vim.system({ "wodo", "a", input }, { text = true }, function(result)
      if result.code ~= 0 then
        vim.schedule(function()
          vim.notify("Failed to create tasks file", vim.log.levels.ERROR)
        end)
        return
      end

      local path = vim.trim(result.stdout)

      vim.schedule(function()
        -- open the created file
        vim.cmd("edit " .. path)

        -- run your boilerplate function
        create_task_boilerplate()
      end)
    end)
  end)
end

local function telescope_list_tasks()
  local pickers = require("telescope.pickers")
  local finders = require("telescope.finders")
  local conf = require("telescope.config").values
  local actions = require("telescope.actions")
  local action_state = require("telescope.actions.state")

  vim.system({ "wodo", "l" }, { text = true }, function(result)
    if result.code ~= 0 then
      vim.schedule(function()
        vim.notify("Failed to list task files", vim.log.levels.ERROR)
      end)
      return
    end

    local ok, data = pcall(vim.json.decode, result.stdout)
    if not ok then
      vim.schedule(function()
        vim.notify("Invalid JSON from wodo", vim.log.levels.ERROR)
      end)
      return
    end

    if #data == 0 then
      vim.notify("There is not task to list", vim.log.levels.INFO)
      return
    end

    vim.schedule(function()
      pickers.new({}, {
        prompt_title = "Wodo Task Files",

        finder = finders.new_table({
          results = data,

          entry_maker = function(item)
            return {
              value = item,                 -- store full object
              ordinal = item.name,          -- used for sorting/search
              display = item.name,          -- what Telescope shows
            }
          end,
        }),

        sorter = conf.generic_sorter({}),

        attach_mappings = function(prompt_bufnr)
          actions.select_default:replace(function()
            local entry = action_state.get_selected_entry()
            actions.close(prompt_bufnr)

            local path = entry.value.path
            vim.cmd({ cmd = "edit", args = { path } })
          end)

          return true
        end,
      }):find()
    end)
  end)
end

local function confirm_delete(on_confirm)
  local pickers = require("telescope.pickers")
  local finders = require("telescope.finders")
  local conf = require("telescope.config").values
  local actions = require("telescope.actions")
  local action_state = require("telescope.actions.state")

  pickers.new({}, {
    prompt_title = "Delete file?",

    finder = finders.new_table({
      results = { "no", "yes" },
    }),

    sorter = conf.generic_sorter({}),

    attach_mappings = function(prompt_bufnr)
      actions.select_default:replace(function()
        local choice = action_state.get_selected_entry()[1]
        actions.close(prompt_bufnr)

        if choice == "yes" then
          on_confirm()
        end
      end)

      return true
    end,
  }):find()
end

local function delete_current_tasks_file()
  local path = vim.api.nvim_buf_get_name(0)

  if path == "" then
    vim.notify("Buffer has no file path", vim.log.levels.WARN)
    return
  end

  confirm_delete(function()
    vim.system({ "wodo", "r", path }, { text = true }, function(result)
      if result.code ~= 0 then
        vim.schedule(function()
          vim.notify("Failed to delete file", vim.log.levels.ERROR)
        end)
        return
      end

      vim.schedule(function()
        vim.cmd("bdelete!")
        vim.notify("Tasks file deleted")
      end)
    end)
  end)
end

vim.api.nvim_create_autocmd({ "BufRead", "BufNewFile" }, {
  group = group,
  pattern = { "*.wodo" },
  callback = function()
    vim.opt_local.filetype = 'wodo'
    vim.opt_local.conceallevel = 2
    vim.opt_local.concealcursor = 'nc'

    vim.keymap.set("n", "<leader>wn", create_task_boilerplate)
    vim.keymap.set("n", "<leader>wt", function() set_task_state("todo") end)
    vim.keymap.set("n", "<leader>wi", function() set_task_state("doing") end)
    vim.keymap.set("n", "<leader>wb", function() set_task_state("blocked") end)
    vim.keymap.set("n", "<leader>wd", function() set_task_state("done") end)
    vim.keymap.set("n", "<leader>wx", delete_current_tasks_file)
    vim.keymap.set("n", "gt", goto_title)
    vim.keymap.set("n", "gd", goto_description)
    vim.keymap.set("n", "gT", goto_tags)
    vim.keymap.set("n", "]w", next_task)
    vim.keymap.set("n", "[w", prev_task)
    vim.keymap.set("n", "<leader>wl", task_picker)
  end
})

vim.api.nvim_create_autocmd("BufEnter", {
  group = group,
  pattern = "*",
  callback = function()
    vim.keymap.set("n", "<leader>wa", create_tasks_file)
    vim.keymap.set("n", "<leader>wL", telescope_list_tasks)
  end
})
