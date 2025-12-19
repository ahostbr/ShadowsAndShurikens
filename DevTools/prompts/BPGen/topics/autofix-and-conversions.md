# AutoFix and Conversions

When to enable auto-fix
- Pin name drift (aliases like Execute/Then, InString/String).
- Schema rejects due to type mismatch (e.g., int -> float).
- You want an explainable, bounded best-effort pass before manual fixes.

GraphSpec flags
- `auto_fix`: enable heuristics (default false).
- `auto_fix_max_steps`: max fixes per apply (default 5).
- `auto_fix_insert_conversions`: allow conversion node insertion.

What you get back
- `auto_fix_steps`: ordered list of fix steps with `code`, `description`, and `before/after`.
- Warnings include lines like `Heuristic pin match applied: old -> new`.

Recommended workflow
1) Apply without auto-fix first to see raw errors.
2) Re-apply with `auto_fix=true` for alias/swap fixes.
3) Add `auto_fix_insert_conversions=true` only when schema rejects with type mismatch.
4) Verify with `bpgen_list`/`bpgen_describe`, then compile/save.

Disable when needed
- For strict deterministic output, keep `auto_fix=false` and handle fixes manually in the spec.
