if exists("b:current_syntax") | finish | endif
syn keyword noqeriKeyword let const function if else while return
syn keyword noqeriBoolean true false null
syn keyword noqeriType void bool int int64 float float64 string
syn match noqeriComment "//.*$"
syn region noqeriString start=+"+ skip=+\\"+ end=+"+
highlight default link noqeriKeyword Keyword
highlight default link noqeriBoolean Boolean
highlight default link noqeriType Type
highlight default link noqeriComment Comment
highlight default link noqeriString String
let b:current_syntax = "noqeri"
