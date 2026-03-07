if exists("b:current_syntax")
  finish
endif

" ----- Titles -----
syntax match wodoTitle /^%.*/

" ----- State -----
syntax match wodoStateKey /^\.state\ze\(\s\|$\)/ nextgroup=wodoStateTodo,wodoStateDoing,wodoStateDone,wodoStateBlocked skipwhite

syntax keyword wodoStateTodo todo contained
syntax keyword wodoStateDoing doing contained
syntax keyword wodoStateDone done contained
syntax keyword wodoStateBlocked blocked contained

" ----- Tags -----
syntax match wodoTagsKey /^\.tags\ze\(\s\|$\)/ nextgroup=wodoTag skipwhite
syntax match wodoTag /\<[a-zA-Z0-9_]\+\>/ contained nextgroup=wodoTag skipwhite

" ----- Date -----
syntax match wodoDateKey /^\.date\ze\(\s\|$\)/ nextgroup=wodoDate skipwhite
syntax match wodoDate /\d\{4}-\d\d-\d\d \d\d:\d\d:\d\d\(Z\|[+-]\d\d:\d\d\)/ contained

" =========================
" Highlights
" =========================

highlight wodoTitle guifg=#50fa7b

highlight wodoStateKey guifg=#f1fa8c
highlight wodoTagsKey guifg=#f1fa8c
highlight wodoDateKey guifg=#f1fa8c

highlight wodoStateTodo guifg=#ffb86c
highlight wodoStateDoing guifg=#8be9fd
highlight wodoStateDone guifg=#50fa7b
highlight wodoStateBlocked guifg=#ff5555

highlight wodoTag guifg=#61afef
highlight wodoDate guifg=#ff6b6b

let b:current_syntax = "wodo"

