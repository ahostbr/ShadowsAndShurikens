# Process Launch Rules

- Capture exit codes and stderr.
- Never assume “starts” == “success”.
- For detached GUI launches (pythonw-style), document the behavior and return quickly.
- Always log the exact command line used (for repro).
