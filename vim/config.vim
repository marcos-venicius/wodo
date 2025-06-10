function! AddTodoBoilerplate()
    let l:date = strftime("%A %d-%m-%y %H:%M - %H:%M")

    if getline('.') ==# ''
        execute "normal! 0i\"\" " . l:date . " Todo\<Esc>0l"
    else
        execute "normal! o\"\" " . l:date . " Todo\<Esc>0l"
    endif
endfunction

augroup TodoListFile
  autocmd!
  autocmd BufRead,BufNewFile *.todo* nnoremap <leader>t <esc>$BDaTodo<esc>0l
  autocmd BufRead,BufNewFile *.todo* nnoremap <leader>i <esc>$BDaDoing<esc>0l
  autocmd BufRead,BufNewFile *.todo* nnoremap <leader>c <esc>$BDaDone<esc>0l
  autocmd BufRead,BufNewFile *.todo* nnoremap <leader>a :call AddTodoBoilerplate()<cr>
augroup END
