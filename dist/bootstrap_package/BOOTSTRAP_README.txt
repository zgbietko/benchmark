Ubuntu benchmark bootstrap package

This package is meant to be unpacked into the root of an existing clone of the benchmark repository.
It does not contain the whole repository.

Recommended flow on a fresh Ubuntu install:
1. Clone the repository.
2. Extract this package into the repo root.
3. Run:
   chmod +x scripts/bootstrap_fresh_ubuntu_benchmark.sh
   ./scripts/bootstrap_fresh_ubuntu_benchmark.sh
4. Add scripts/generated/github_id_ed25519.pub to GitHub SSH keys.
5. Activate environment:
   source scripts/generated/activate_benchmark_env.sh
6. Run exact Filip benchmark:
   python run_workflow.py --workflow filip_original --backend intel --filip-mode exact_reference --filip-case laplace_prism
