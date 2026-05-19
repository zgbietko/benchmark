# V6 Authorial FEM-like Workload

`v6` dodaje autorski parametryczny workload `assembly_like`, zaprojektowany jako
most miedzy real kernels a kampania aplikacyjna FEM.

## Co zostalo dodane

- kernel: `assembly_like` w `real_kernels`
- backendy: `cpu`, `cuda`, `metal`, `hip`, `opencl`
- benchmark runner: `real_kernels/benchmarks/run_assembly_like.py`
- integracja z pakietem: `real_kernels/run_all_real_kernels.py`
- problem Firefly: `author_assembly` w `run_firefly_optimization.py`
- workflow launcher: `run_workflow.py --workflow author_assembly_firefly`

## Parametry workloadu

- `n_elements`
- `n_qp`
- `n_dofs`
- `variant`: `qss|sqs|ssq`
- `use_workspace`: `0|1`
- `scatter_accumulate`: `0|1`
- `padding`: `0|1`
- `dtype`

## Uruchomienia

### 1) Bezposredni benchmark workloadu

```bash
cd /Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v6
python3 real_kernels/benchmarks/run_assembly_like.py \
  --backend cpu \
  --runs 3 \
  --warmups 1 \
  --sizes 10000,30000,60000 \
  --n-qp-choices 2,4,6 \
  --n-dofs-choices 4,6,8 \
  --variants qss,sqs,ssq \
  --workspace-choices 0,1 \
  --scatter-choices 0,1 \
  --padding-choices 0,1
```

### 2) Pelny pakiet real kernels (z assembly_like)

```bash
cd /Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v6
python3 real_kernels/run_all_real_kernels.py --backend cpu --benchmark-mode standard
```

### 3) Firefly autotuning dla workloadu autorskiego

```bash
cd /Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v6
python3 run_workflow.py \
  --workflow author_assembly_firefly \
  --backend auto \
  --platform-profile auto \
  --profile quick \
  --population 12 \
  --iterations 20 \
  --repeats 3
```

### 4) Bezposrednio przez run_firefly_optimization.py

```bash
cd /Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v6
python3 run_firefly_optimization.py \
  --problem author_assembly \
  --backend cpu \
  --device-index 0 \
  --population 16 \
  --iterations 25 \
  --repeats 3 \
  --assembly-n-elements-range 10000:250000 \
  --assembly-n-qp-range 1:8 \
  --assembly-n-dofs-choices 4,6,8 \
  --assembly-variants qss,sqs,ssq \
  --assembly-workspace-choices 0,1 \
  --assembly-scatter-choices 0,1 \
  --assembly-padding-choices 0,1
```

## Artefakty

Benchmark CSV:
- `data/runs/<session>/real_kernels/assembly_like__*.csv`

Firefly:
- `data/optimization/<run>__author_assembly__backend-*/summary.json`
- `evaluations.jsonl`
- `iterations.jsonl`
- `pareto_front.jsonl`

Analiza / walidacja modelu:
- `analysis/generate_plots.py`
- `analysis/model_validation.py`
- `analysis/real_kernels_summary.py`
