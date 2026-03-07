local function add_todo_boilerplate()
  local date = os.date("%A %d-%m-%y %H:%M - %H:%M")
  local text = '"" ' .. date .. ' Todo'

  local row = vim.api.nvim_win_get_cursor(0)[1] - 1
  local line = vim.api.nvim_get_current_line()

  if line == "" then
    vim.api.nvim_set_current_line(text)
  else
    vim.api.nvim_buf_set_lines(0, row + 1, row + 1, false, { text })
    vim.api.nvim_win_set_cursor(0, { row + 2, 0 })
  end
end

local function set_state(state)
  local line = vim.api.nvim_get_current_line()

  if string.match(line, "Todo$") then
    line = line:gsub(" Todo$", " " .. state)
  elseif string.match(line, "Doing$") then
    line = line:gsub(" Doing$", " " .. state)
  elseif string.match(line, "Done$") then
    line = line:gsub(" Done$", " " .. state)
  end

  vim.api.nvim_set_current_line(line)
end

local group = vim.api.nvim_create_augroup("WodoFile", { clear = true })

vim.api.nvim_create_autocmd({ "BufRead", "BufNewFile" }, {
  group = group,
  pattern = { "*.todo.md" },
  callback = function()
    vim.keymap.set("n", "<leader>t", function() set_state("Todo") end)
    vim.keymap.set("n", "<leader>i", function() set_state("Doing") end)
    vim.keymap.set("n", "<leader>c", function() set_state("Done") end)
    vim.keymap.set("n", "<leader>a", add_todo_boilerplate)
  end,
})

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
    local line = vim.api.nvim_buf_get_lines(buf, i, i+1, false)[1]
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
    local line = vim.api.nvim_buf_get_lines(buf, i, i+1, false)[1]
    if line:match("^%%") then
      finish = i
      break
    end
  end

  -- search for .state inside block
  for i = start, finish - 1 do
    local line = vim.api.nvim_buf_get_lines(buf, i, i+1, false)[1]

    if line:match("^%.state") then
      vim.api.nvim_buf_set_lines(buf, i, i+1, false, {".state " .. state})
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

vim.api.nvim_create_autocmd({ "BufRead", "BufNewFile" }, {
  group = group,
  pattern = { "*.wodo" },
  callback = function()
    vim.keymap.set("n", "<leader>wt", create_task_boilerplate)
    vim.keymap.set("n", "<leader>wst", function() set_task_state("todo") end)
    vim.keymap.set("n", "<leader>wsi", function() set_task_state("doing") end)
    vim.keymap.set("n", "<leader>wsb", function() set_task_state("blocked") end)
    vim.keymap.set("n", "<leader>wsd", function() set_task_state("done") end)
    vim.keymap.set("n", "<leader>wgt", goto_tags)
    vim.keymap.set("n", "<leader>wgT", goto_title)
    vim.keymap.set("n", "<leader>wgd", goto_description)
  end
})
