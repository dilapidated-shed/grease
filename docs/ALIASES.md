# Grease aliases and readable declarations

The implementation is owned by Oils `grease/main`, selected by Grease's
`source/` gitlink. Conventional `alias` and `unalias` remain available
under `ysh:all`, and alias expansion stays enabled unless explicitly disabled.

Readable declaration spellings can opt in through that same alias mechanism:
`alias procedure=proc` and `alias define_function=func`.
The focused inherited spec exercises both declarations and their invocation.
They use the existing proc/function evaluators, with no different call semantics.

`proc` is a command procedure; `func` is a value-returning function.
Keep those ordinary YSH spellings available. The existing `function` keyword
already selects shell-function grammar, and `do` already participates in shell
loop grammar. This change does not remap either reserved keyword through an
alias or change their meaning in ordinary YSH programs.

The unfinished D backend must implement the same reference compatibility
contract. Its owned ledger is `source/ysh/d/PORTING.md` on the D translation
successor, not a second set of Grease language decisions. This is language
documentation; repeatable operational procedures remain owned by Kitchen,
Flexible Pipes, and Cat Food according to their respective boundaries.
