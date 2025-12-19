# BPGen FAQ (SPINE_P)

**Q: Why do apply actions fail with "APPLY blocked"?**  
A: Set `SOTS_ALLOW_APPLY=1` in the editor environment; the gate prevents accidental mutations.

**Q: How do I run deletes safely?**  
A: Keep bridge `safe_mode=true` (default) and pass `dangerous_ok=true` only when you intend to delete/replace. Prefer dry runs first.

**Q: Where are NodeIds stored?**  
A: BPGen keeps NodeId in `NodeComment`. Debug annotate uses `[BPGEN]` bubbles but preserves the comment content.

**Q: Does bridge expose beyond loopback?**  
A: Default bind is 127.0.0.1. Use config to opt into non-loopback; not recommended for packaged builds.

**Q: Are templates versioned?**  
A: Yes—spec_version = 1. Run canonicalize before apply if template/spec drift is suspected.

**Q: Is any non-redistributable content included?**  
A: No. All examples/templates are JSON/text. Users create their own assets following the guides.
