# WODO

_Wodo_ is a todo list tracking system vim-based.

You can create `.todo.md` files wherever you want and just call `wodo add /path/to/file` and that's it.

Everytime you want to check all of your todos you can just do `wodo view`, this will list every todo file you have, parse and display it to you.

To use it, you need to add some configurations to vim:

```vim
augroup TodoListFile
  autocmd!
  autocmd BufRead,BufNewFile *.todo* nnoremap <leader>t <esc>$vF-da- Todo<esc>0f[lr 
  autocmd BufRead,BufNewFile *.todo* nnoremap <leader>i <esc>$vF-da- Doing<esc>0f[lr 
  autocmd BufRead,BufNewFile *.todo* nnoremap <leader>c <esc>$vF-da- Done<esc>0f[lrx<esc>j^f[l
  autocmd BufRead,BufNewFile *.todo* nnoremap <leader>a <esc>0Di- [ ] <name> \| --day-- 00, 00:00 - 00:00 - Todo<esc>0fnh
augroup END
```

This configurations will allow you to type:

- `<leader>t` to put a task in `todo` mode
- `<leader>i` to put a task in `progress` mode
- `<leader>c` to put a task in `done` mode
- `<leader>a` to create a new task in a blank line

Then, you can just do `wodo add /path/to/file`.

After do this, everytime you update this file the wodo will be able to see this modifications and when you do `wodo view` you will get all your todo files with the updates states and see where everyone is and the state of which one.

**Check the `examples` folder to a todo file example**

## Info

Maybe I'll update this project to allow you to:

- automatically open a todo file on your vim
- display aligned tasks
- allow modify tasks via cli
    - change state of a task
    - remove a task
    - add a new task (by opening vim tabs)
    - edit a task (by opening vim tabs)
- validate time ranges
- have a calendar view

and some other stuff.

**And of course, I need to refactor this mess of almost 1000 lines main.c file**

