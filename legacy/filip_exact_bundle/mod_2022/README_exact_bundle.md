# Filip exact bundle

This directory is a Git-safe minimal snapshot of `mod_2022` for exact OpenCL
reference runs used by:

- `run_filip_reference_exact.py`
- `run_workflow.py --workflow filip_original --filip-mode exact_reference`

It intentionally contains only:

- `src/`
- `work/diff_in_box/`
- `work/test_scalar/`

That is enough for:

- `laplace_prism`
- `test_prism`
- `prism_pair`

It does not include historical results, unrelated workspaces, build outputs,
or the original embedded `.git` metadata.
