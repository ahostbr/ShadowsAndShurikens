# Expected Outcomes (SPINE_P)

- **HelloWorld**: Function prints a greeting when executed; nodes are linked Entry → PrintString → Result.
- **AdvancedDoor**: Actor graphs create auto-close behavior; verify nodes match spec and compile cleanly.

If results differ:
1. Run `canonicalize_graph_spec` before apply.
2. Ensure `SOTS_ALLOW_APPLY=1` is set.
3. Re-run apply with `dangerous_ok` only when deleting/replacing nodes.
4. Compile the Blueprint and check for warnings.
