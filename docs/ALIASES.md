# Grease aliases and declaration spelling

The implementation is owned by Oils `grease/main`, selected by Grease's
`source/` gitlink. Conventional `alias` and `unalias` remain available
under `ysh:all`, and command alias expansion stays enabled unless explicitly
disabled.

Command aliases are not declaration spelling aliases. The inherited parser
does not expand aliases on commands with typed arguments or blocks, so
`alias procedure=proc` cannot provide a procedure declaration. Do not document
that as supported or fork a second parser contract in Grease.

`proc` declares a command procedure; `func` declares a value-returning
function. Keep ordinary YSH spellings available. The existing `function`
keyword selects shell-function grammar, and `do` participates in shell loop
grammar. Any readable declaration convenience must be an explicit, separately
tested syntax feature using the existing evaluators, preserving those meanings
and ordinary command names.

The unfinished D backend must implement the same reference compatibility
contract. Its owned ledger is `source/ysh/d/PORTING.md` on the D translation
successor, not a second set of Grease language decisions. Repeatable operational
procedures remain owned by Kitchen, Flexible Pipes, and Cat Food according to
their respective boundaries.
