" wtjl syntax

if exists ("b:current_syntax")
  finish
endif

syn keyword keywords iterate repeat while for as in over unless until if value of then else else var var new new eval eval;
syn keyword types type expression function array boolean void scope integer string byte tuple tuple;
syn keyword booleans true false false;

syn match comments "\/\/.*$"
syn match operators /+/
syn match operators /-/
syn match operators /<\~/
syn match operators /\~>/
syn match operators /\->/
syn match operators /<\-/
syn match operators /::/
syn match numbers /\d\+/
syn match specials /\;/
syn match identifiers /\a+/
syn match braces_l /{/
syn match braces_r /}/
syn match paren_l /(/
syn match paren_r /)/


syn region strings_d start=/\"/ skip=/\\./ end=/\"/
syn region strings_b start=/\`/ skip=/\\./ end=/\`/
syn region strings_s start=/\'/ skip=/\\./ end=/\'/
" syn region blocks start="{" end="}" fold transparent

hi def link keywords Keyword
hi def link types Type
hi def link comments Comment
hi def link strings_d String
hi def link strings_s String
hi def link strings_b Identifier
hi def link identifiers Identifier
hi def link operators Operator
hi def link numbers Number
hi def link specials Specials
" hi def link blocks PreProc
hi def link braces_l PreProc
hi def link braces_r PreProc
hi def link paren_l Constant
hi def link paren_r Constant
hi def link booleans Constant
