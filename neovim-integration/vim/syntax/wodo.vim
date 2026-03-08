if exists("b:current_syntax")
  finish
endif

" =========================
" Syntax Definitions
" =========================

" ----- Titles (Lines starting with %) -----
syntax match wodoTitle /^%.*/ display

" ----- Headers (Lines starting with #) -----
syntax match wodoHeader /^#.*/ display

" ----- Bold Text (with Conceal) -----
syntax region wodoBold matchgroup=wodoBoldMarker start=/\*\*/ end=/\*\*/ concealends

" ----- Links (Markdown Style: [Text](URL)) -----
" This region starts at [ and ends at )
" 'concealends' hides the [] markers.
" We use a nested match to hide the entire (URL) part.
syntax region wodoLink matchgroup=wodoLinkMarker start=/\[/ end=/\]/ concealends nextgroup=wodoLinkURL
syntax region wodoLinkURL start=/(/ end=/)/ conceal contained

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

" Headers and Bold
highlight wodoHeader guifg=#bd93f9 ctermfg=139 gui=bold cterm=bold
highlight wodoBold gui=bold cterm=bold

" Links
" 'Underlined' is a built-in Vim group, but we'll define it explicitly for safety
highlight wodoLink gui=underline cterm=underline guifg=#8be9fd

" State Colors
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
