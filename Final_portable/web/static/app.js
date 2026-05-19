const state = {
  data: null,
  selectedStepId: null,
  selectedCampaignDir: "",
  defaultsApplied: false,
  pollInFlight: false,
};

const SECTION_PREFS_KEY = 'final.pipeline.sectionPrefs';

const els = {
  jobBanner: document.getElementById('jobBanner'),
  summaryStrip: document.getElementById('summaryStrip'),
  stageOverview: document.getElementById('stageOverview'),
  detailsContent: document.getElementById('detailsContent'),
  benchmarkPlots: document.getElementById('benchmarkPlots'),
  realPlots: document.getElementById('realPlots'),
  filipVariantPlots: document.getElementById('filipVariantPlots'),
  filipTuningPlots: document.getElementById('filipTuningPlots'),
  exactPlots: document.getElementById('exactPlots'),
  campaignSelect: document.getElementById('campaignSelect'),
  stepRail: document.getElementById('stepRail'),
  runFullBtn: document.getElementById('runFullBtn'),
  refreshStateBtn: document.getElementById('refreshStateBtn'),
  stopJobBtn: document.getElementById('stopJobBtn'),
  expandSectionsBtn: document.getElementById('expandSectionsBtn'),
  collapseSectionsBtn: document.getElementById('collapseSectionsBtn'),
  loadCampaignBtn: document.getElementById('loadCampaignBtn'),
  openCampaignBtn: document.getElementById('openCampaignBtn'),
  refreshSessionPlotsBtn: document.getElementById('refreshSessionPlotsBtn'),
  refreshFilipPlotsBtn: document.getElementById('refreshFilipPlotsBtn'),
  buildPlotsZipBtn: document.getElementById('buildPlotsZipBtn'),
  runBenchmarksBtn: document.getElementById('runBenchmarksBtn'),
  runRealKernelsBtn: document.getElementById('runRealKernelsBtn'),
  runFilipTestBtn: document.getElementById('runFilipTestBtn'),
  imageModal: document.getElementById('imageModal'),
  imageModalBackdrop: document.getElementById('imageModalBackdrop'),
  imageModalClose: document.getElementById('imageModalClose'),
  imageModalOpenPath: document.getElementById('imageModalOpenPath'),
  imageModalImg: document.getElementById('imageModalImg'),
  imageModalTitle: document.getElementById('imageModalTitle'),
  imageModalPath: document.getElementById('imageModalPath'),
  backendAvailabilityHint: document.getElementById('backendAvailabilityHint'),
  deviceSelectionHint: document.getElementById('deviceSelectionHint'),
  cpuTopologyHint: document.getElementById('cpuTopologyHint'),
};

let currentModalPath = '';

function $(id) { return document.getElementById(id); }

function availableOptions() {
  return state.data?.available || {};
}

function readConfig() {
  return {
    backend: $('backendInput').value.trim() || 'auto',
    benchmark_mode: $('benchmarkModeInput').value.trim() || 'standard',
    platform_profile: $('platformProfileInput').value.trim() || 'auto',
    benchmarks_max_cpu_threads: Number($('benchmarksCpuThreadsInput').value || 1),
    real_kernels_max_cpu_threads: Number($('realKernelsCpuThreadsInput').value || 1),
    filip_max_cpu_threads: Number($('filipCpuThreadsInput').value || 1),
    device_index: Number($('deviceIndexInput').value || 0),
    filip_case: $('filipCaseInput').value.trim() || 'prism_pair',
    replay_dump_root: $('replayRootInput').value.trim(),
    repeats: Number($('repeatsInput').value || 5),
    real_runs: Number($('realRunsInput').value || 5),
    trials: Number($('trialsInput').value || 256),
    population: Number($('populationInput').value || 24),
    iterations: Number($('iterationsInput').value || 40),
    validation_operators: $('validationOperatorsInput').value.trim() || 'laplace,test',
    validation_variants: $('validationVariantsInput').value.trim() || 'qss,sqs,ssq',
    validation_n_elements: Number($('validationElementsInput').value || 16384),
    validation_n_qp: Number($('validationQpInput').value || 6),
    validation_workgroup_size: Number($('validationWgInput').value || 64),
    correlation_profiler_reports: $('profilerReportsInput').value,
  };
}

function fillSelect(selectEl, items, preferredValue, fallbackValue = '') {
  if (!selectEl) return;
  const safeItems = Array.isArray(items) ? items : [];
  const previous = String(preferredValue ?? '');
  const fallback = String(fallbackValue ?? '');
  selectEl.innerHTML = safeItems.map((item) => {
    const value = String(item.value ?? '');
    const label = String(item.label ?? value);
    return `<option value="${escapeHtml(value)}">${escapeHtml(label)}</option>`;
  }).join('');
  const allowed = new Set(safeItems.map((item) => String(item.value ?? '')));
  if (allowed.has(previous)) {
    selectEl.value = previous;
  } else if (allowed.has(fallback)) {
    selectEl.value = fallback;
  } else if (safeItems.length) {
    selectEl.value = String(safeItems[0].value ?? '');
  }
}

function deviceChoices() {
  return availableOptions().device_choices || [];
}

function cpuThreadLimitMax() {
  const raw = Number(availableOptions().cpu_thread_limit_max || 1);
  return Number.isFinite(raw) && raw > 0 ? Math.max(1, Math.round(raw)) : 1;
}

function updateCpuThreadSlider(sliderId, labelId) {
  const slider = $(sliderId);
  const label = $(labelId);
  if (!slider || !label) return;
  const max = cpuThreadLimitMax();
  slider.min = '1';
  slider.max = String(max);
  let value = Number(slider.value || max);
  if (!Number.isFinite(value) || value < 1) value = max;
  value = Math.min(max, Math.max(1, Math.round(value)));
  slider.value = String(value);
  label.textContent = `${value} / ${max}`;
}

function refreshCpuThreadControls() {
  updateCpuThreadSlider('benchmarksCpuThreadsInput', 'benchmarksCpuThreadsLabel');
  updateCpuThreadSlider('realKernelsCpuThreadsInput', 'realKernelsCpuThreadsLabel');
  updateCpuThreadSlider('filipCpuThreadsInput', 'filipCpuThreadsLabel');
  const bench = $('benchmarksCpuThreadsInput')?.value || '1';
  const real = $('realKernelsCpuThreadsInput')?.value || '1';
  const filip = $('filipCpuThreadsInput')?.value || '1';
  if ($('groupCpuLimit-benchmarks')) $('groupCpuLimit-benchmarks').textContent = `${bench} / ${cpuThreadLimitMax()}`;
  if ($('groupCpuLimit-real_kernels')) $('groupCpuLimit-real_kernels').textContent = `${real} / ${cpuThreadLimitMax()}`;
  if ($('groupCpuLimit-filip_test')) $('groupCpuLimit-filip_test').textContent = `${filip} / ${cpuThreadLimitMax()}`;
}

function updateCpuTopologyHint() {
  if (!els.cpuTopologyHint) return;
  const topology = availableOptions().cpu_topology || {};
  const logical = Number(availableOptions().cpu_thread_limit_max || topology.logical_cpus || 1);
  const perf = Number(topology.perf_logical_cpus || 0);
  const eff = Number(topology.eff_logical_cpus || 0);
  const source = String(topology.source || 'auto');
  if (perf > 0 || eff > 0) {
    els.cpuTopologyHint.textContent = `Topologia CPU: logical=${logical}, P=${perf || '-'}, E=${eff || '-'} (source: ${source}). Suwaki ustawiają górny limit rdzeni dla każdego pakietu.`;
    return;
  }
  els.cpuTopologyHint.textContent = `Topologia CPU: logical=${logical} (source: ${source}). Suwaki ustawiają górny limit rdzeni dla każdego pakietu.`;
}

function updateDeviceHint() {
  const selected = deviceChoices().find((item) => String(item.value) === String($('deviceChoiceInput').value));
  if (!selected || selected.value === 'auto') {
    els.deviceSelectionHint.textContent = 'Automatyczny dobór urządzenia. Backend i device-index pozostają sterowane ustawieniami podstawowymi.';
    return;
  }
  els.deviceSelectionHint.textContent = `${selected.backend} | dev${selected.device_index} | ${selected.device_name || 'brak nazwy urządzenia'}`;
}

function syncDeviceChoiceFromFields() {
  const backend = $('backendInput').value.trim() || 'auto';
  const index = Number($('deviceIndexInput').value || 0);
  const match = deviceChoices().find((item) => item.backend === backend && Number(item.device_index) === index);
  $('deviceChoiceInput').value = match ? String(match.value) : 'auto';
  updateDeviceHint();
}

function applyAvailableOptions(options) {
  const currentBackend = $('backendInput')?.value || 'auto';
  const currentBenchmarkMode = $('benchmarkModeInput')?.value || 'standard';
  const currentProfile = $('platformProfileInput')?.value || 'auto';
  const currentCase = $('filipCaseInput')?.value || 'prism_pair';
  const currentDeviceChoice = $('deviceChoiceInput')?.value || 'auto';
  fillSelect($('backendInput'), options.backend_choices || [], currentBackend, 'auto');
  fillSelect($('benchmarkModeInput'), options.benchmark_mode_choices || [], currentBenchmarkMode, 'standard');
  fillSelect($('platformProfileInput'), options.platform_profile_choices || [], currentProfile, 'auto');
  fillSelect($('filipCaseInput'), options.filip_case_choices || [], currentCase, 'prism_pair');
  fillSelect($('deviceChoiceInput'), options.device_choices || [], currentDeviceChoice, 'auto');
  els.backendAvailabilityHint.textContent = options.backend_hint || '';
  refreshCpuThreadControls();
  updateCpuTopologyHint();
}

function applyDefaults(defaults) {
  if (!defaults) return;
  $('backendInput').value = defaults.backend ?? 'auto';
  $('benchmarkModeInput').value = defaults.benchmark_mode ?? 'standard';
  $('platformProfileInput').value = defaults.platform_profile ?? 'auto';
  $('benchmarksCpuThreadsInput').value = defaults.benchmarks_max_cpu_threads ?? cpuThreadLimitMax();
  $('realKernelsCpuThreadsInput').value = defaults.real_kernels_max_cpu_threads ?? cpuThreadLimitMax();
  $('filipCpuThreadsInput').value = defaults.filip_max_cpu_threads ?? cpuThreadLimitMax();
  $('deviceIndexInput').value = defaults.device_index ?? 0;
  $('filipCaseInput').value = defaults.filip_case ?? 'prism_pair';
  $('replayRootInput').value = defaults.replay_dump_root ?? '';
  $('repeatsInput').value = defaults.repeats ?? 5;
  $('realRunsInput').value = defaults.real_runs ?? 5;
  $('trialsInput').value = defaults.trials ?? 256;
  $('populationInput').value = defaults.population ?? 24;
  $('iterationsInput').value = defaults.iterations ?? 40;
  $('validationOperatorsInput').value = defaults.validation_operators ?? 'laplace,test';
  $('validationVariantsInput').value = defaults.validation_variants ?? 'qss,sqs,ssq';
  $('validationElementsInput').value = defaults.validation_n_elements ?? 16384;
  $('validationQpInput').value = defaults.validation_n_qp ?? 6;
  $('validationWgInput').value = defaults.validation_workgroup_size ?? 64;
  $('profilerReportsInput').value = defaults.correlation_profiler_reports ?? '';
  refreshCpuThreadControls();
  syncDeviceChoiceFromFields();
}

async function apiGet(url) {
  const res = await fetch(url);
  const data = await res.json();
  if (!res.ok) throw new Error(data.error || `HTTP ${res.status}`);
  return data;
}

async function apiPost(url, payload) {
  const res = await fetch(url, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(payload || {}),
  });
  const data = await res.json();
  if (!res.ok) throw new Error(data.error || `HTTP ${res.status}`);
  return data;
}

function formatDuration(value) {
  if (value === null || value === undefined || value === '') return 'brak';
  const seconds = Number(value);
  if (!Number.isFinite(seconds)) return String(value);
  if (seconds < 60) return `${seconds.toFixed(1)} s`;
  const mins = Math.floor(seconds / 60);
  const rem = seconds - mins * 60;
  return `${mins} min ${rem.toFixed(0)} s`;
}

function statusLabel(status) {
  if (status === 'ok') return 'OK';
  if (status === 'failed') return 'Błąd';
  if (status === 'skipped') return 'Pominięty';
  if (status === 'stopping') return 'Zatrzymywanie';
  return 'Brak danych';
}

function escapeHtml(text) {
  return String(text || '')
    .replaceAll('&', '&amp;')
    .replaceAll('<', '&lt;')
    .replaceAll('>', '&gt;');
}

function selectedNode() {
  return state.data?.campaign?.nodes?.find((node) => node.id === state.selectedStepId) || null;
}

function stages() {
  return state.data?.campaign?.stages || [];
}

function stageMap() {
  return new Map(stages().map((stage) => [stage.id, stage]));
}

function groups() {
  return state.data?.campaign?.groups || [];
}

function nodesById() {
  return new Map((state.data?.campaign?.nodes || []).map((node) => [node.id, node]));
}

function plotSections() {
  return state.data?.campaign?.plot_sections || {};
}

function campaignSummary() {
  return state.data?.campaign?.summary || {};
}

function findNode(nodeId) {
  return (state.data?.campaign?.nodes || []).find((node) => node.id === nodeId) || null;
}

function basename(path) {
  if (!path) return 'brak';
  const parts = String(path).split('/');
  return parts[parts.length - 1] || path;
}

function summaryStatusClass(status) {
  return `status-${status || 'pending'}`;
}

function deviceLabelFor(backend, index) {
  const match = deviceChoices().find((item) => item.backend === backend && Number(item.device_index) === Number(index));
  if (match?.label) return match.label;
  if (!backend || backend === 'auto') return 'dobór automatyczny';
  return `${backend} | dev${index}`;
}

function humanizeExactEquivalence(value) {
  const raw = String(value || '').trim();
  if (!raw) return 'brak danych walidacyjnych';
  if (raw === 'numerically_equivalent') return 'zgodność numeryczna potwierdzona';
  if (raw === 'not_numerically_equivalent_to_opencl_exact') return 'brak potwierdzonej zgodności 1:1 z referencją OpenCL exact';
  return raw.replaceAll('_', ' ');
}

function describeExactState(node) {
  const archivedExactCount = (plotSections().exact || []).length;
  if (!node) {
    return {
      status: 'pending',
      headline: archivedExactCount ? 'Ostatnie figury exact' : 'Nie uruchomiono',
      meta: archivedExactCount
        ? 'Brak aktywnej kampanii z podsumowaniem exact. Panel pokazuje ostatnie znalezione figury walidacyjne.'
        : 'Krok walidacji exact / replay nie ma jeszcze danych w bieżącej kampanii.',
      chips: archivedExactCount ? [`figur exact: ${archivedExactCount}`] : [],
    };
  }
  const payload = node.payload || {};
  const replaySource = String(payload.replay_dump_root_source || '').trim();
  const comparisonNote = String(payload.comparison_note || '').trim();
  const numericalEquivalence = String(payload.numerical_equivalence || '').trim();
  if ((node.status === 'pending' || !node.result_dir) && archivedExactCount && !replaySource && !comparisonNote && !numericalEquivalence) {
    return {
      status: 'pending',
      headline: 'Ostatnie figury exact',
      meta: 'Bieżąca kampania nie ma jeszcze zapisanego kroku exact, ale panel odnalazł ostatnie figury walidacyjne.',
      chips: [`figur exact: ${archivedExactCount}`],
    };
  }
  let headline = 'Nie uruchomiono';
  if (replaySource) {
    headline = 'Replay 1:1';
  } else if (node.status === 'ok' || node.status === 'failed' || node.status === 'running' || comparisonNote || numericalEquivalence) {
    headline = 'Fallback natywny';
  }
  const metaParts = [
    `status kroku: ${statusLabel(node.status)}`,
    `zgodność: ${humanizeExactEquivalence(numericalEquivalence)}`,
  ];
  if (replaySource) metaParts.push(`źródło replay bundle: ${replaySource}`);
  if (!replaySource && comparisonNote) metaParts.push('uruchomiono bez replay bundle; to nie jest pełne odtworzenie 1:1');
  const chips = [];
  if (node.result_dir) chips.push(`wynik: ${basename(node.result_dir)}`);
  if (replaySource) chips.push(`replay: ${replaySource}`);
  else if (comparisonNote) chips.push('fallback natywny');
  return {
    status: node.status || 'pending',
    headline,
    meta: metaParts.join(' • '),
    chips,
  };
}

function nodeResultLabel(node) {
  if (node.result_dir) return basename(node.result_dir);
  if (node.reason) return 'wymaga uwagi';
  return 'brak katalogu';
}

function progressForNode(node) {
  const status = node.status || 'pending';
  if (status === 'ok' || status === 'skipped' || status === 'failed') {
    return { pct: 100, text: status === 'failed' ? 'zakonczono z bledem' : 'zakonczono', indeterminate: false };
  }
  if (status === 'running') {
    return { pct: 55, text: 'w trakcie', indeterminate: true };
  }
  return { pct: 0, text: 'oczekuje', indeterminate: false };
}

function groupState(group) {
  const nodeMap = nodesById();
  const groupNodes = (group.step_ids || []).map((id) => nodeMap.get(id)).filter(Boolean);
  const total = groupNodes.length || 1;
  const done = groupNodes.filter((node) => ['ok', 'failed', 'skipped'].includes(node.status)).length;
  const runningNodes = groupNodes.filter((node) => node.status === 'running');
  const hasRunning = runningNodes.length > 0;
  const hasFailed = groupNodes.some((node) => node.status === 'failed');
  const allOkOrSkipped = groupNodes.length > 0 && groupNodes.every((node) => ['ok', 'skipped'].includes(node.status));
  let status = 'pending';
  if (hasRunning) status = 'running';
  else if (hasFailed) status = 'failed';
  else if (allOkOrSkipped) status = 'ok';
  const partial = runningNodes.length ? 0.5 : 0.0;
  const pct = Math.round(((done + partial) / total) * 100);
  const remaining = Math.max(groupNodes.length - done, 0);
  const queued = Math.max(groupNodes.length - done - runningNodes.length, 0);
  let text = 'oczekuje';
  if (hasRunning) text = 'w trakcie';
  else if (hasFailed) text = 'zakonczono z bledem';
  else if (allOkOrSkipped) text = 'zakonczono';
  return {
    total,
    done,
    remaining,
    queued,
    runningNodes,
    status,
    pct,
    text,
    currentLabel: runningNodes[0]?.label || '',
  };
}

function statusLabelLong(status) {
  if (status === 'ok') return 'Gotowe';
  if (status === 'failed') return 'Błąd';
  if (status === 'skipped') return 'Pominięty';
  if (status === 'stopping') return 'Zatrzymywanie';
  if (status === 'running') return 'W trakcie';
  return 'Brak danych';
}

function stageState(stage) {
  const stageNodes = (state.data?.campaign?.nodes || []).filter((node) => node.stage_id === stage.id);
  const total = stageNodes.length || 1;
  const done = stageNodes.filter((node) => ['ok', 'failed', 'skipped'].includes(node.status)).length;
  const runningNodes = stageNodes.filter((node) => node.status === 'running');
  const hasRunning = runningNodes.length > 0;
  const hasFailed = stageNodes.some((node) => node.status === 'failed');
  const allOkOrSkipped = stageNodes.length > 0 && stageNodes.every((node) => ['ok', 'skipped'].includes(node.status));
  let status = 'pending';
  if (hasRunning) status = 'running';
  else if (hasFailed) status = 'failed';
  else if (allOkOrSkipped) status = 'ok';
  const partial = runningNodes.length ? 0.5 : 0.0;
  const pct = Math.round(((done + partial) / total) * 100);
  const remaining = Math.max(stageNodes.length - done, 0);
  const queued = Math.max(stageNodes.length - done - runningNodes.length, 0);
  return {
    nodes: stageNodes,
    total,
    done,
    remaining,
    queued,
    runningNodes,
    status,
    pct,
    currentLabel: runningNodes[0]?.label || '',
  };
}

function progressMetaText(info) {
  if (info.status === 'ok') {
    return `Zakończono: ${info.done}/${info.total} kroków`;
  }
  if (info.status === 'failed') {
    return `Wymaga uwagi: ${info.done}/${info.total} kroków zamkniętych`;
  }
  if (info.status === 'running') {
    const current = info.currentLabel ? ` • aktywny: ${info.currentLabel}` : '';
    return `W toku • po bieżącym zostanie ${info.queued} kroków${current}`;
  }
  return `Do wykonania: ${info.total - info.done} kroków`;
}

function renderSummaryStrip() {
  const summary = campaignSummary();
  const nodes = state.data?.campaign?.nodes || [];
  const ok = nodes.filter((n) => n.status === 'ok').length;
  const failed = nodes.filter((n) => n.status === 'failed').length;
  const skipped = nodes.filter((n) => n.status === 'skipped').length;
  const pending = nodes.filter((n) => !['ok','failed','skipped'].includes(n.status)).length;
  const exactInfo = describeExactState(findNode('filip_exact_reference'));
  const sections = plotSections();
  const benchmarkCount = (sections.benchmark || []).length;
  const realCount = (sections.real || []).length;
  const filipCount = (sections.filip_variants || []).length + (sections.filip_tuning || []).length;
  const exactCount = (sections.exact || []).length;
  const totalPlots = benchmarkCount + realCount + filipCount + exactCount;
  const expectedBenchmark = 6;
  const expectedReal = 5;
  const expectedCore = expectedBenchmark + expectedReal;
  const missingBenchmark = Math.max(expectedBenchmark - benchmarkCount, 0);
  const missingReal = Math.max(expectedReal - realCount, 0);
  const missingCore = Math.max(expectedCore - (benchmarkCount + realCount), 0);
  const figuresStatus = missingCore === 0 ? 'ok' : (totalPlots > 0 ? 'running' : 'pending');
  const figuresStatusText = missingCore === 0 ? 'komplet' : `braki: ${missingCore}`;
  const campaignName = summary.campaign_dir ? basename(summary.campaign_dir) : (state.selectedCampaignDir ? basename(state.selectedCampaignDir) : 'brak kampanii');
  const deviceLabel = deviceLabelFor(summary.requested_backend || readConfig().backend, summary.device_index ?? readConfig().device_index);
  const overallStatus = failed > 0 ? 'failed' : (pending > 0 ? 'running' : (ok > 0 || skipped > 0 ? 'ok' : 'pending'));
  const cards = [
    `
      <article class="summary-card">
        <div class="summary-card-label">Kampania</div>
        <div class="summary-card-main">
          <div class="summary-card-value">${escapeHtml(campaignName)}</div>
          <div class="action-status ${summaryStatusClass(overallStatus)}">${escapeHtml(statusLabelLong(overallStatus))}</div>
        </div>
        <div class="summary-card-meta">
          workflow: ${escapeHtml(summary.workflow || 'brak')} • profil: ${escapeHtml(summary.profile || 'brak')} • tryb: ${escapeHtml(summary.benchmark_mode || readConfig().benchmark_mode || 'standard')}
        </div>
        <div class="summary-card-list">
          <span class="summary-chip">OK: ${ok}</span>
          <span class="summary-chip">błędy: ${failed}</span>
          <span class="summary-chip">pominięte: ${skipped}</span>
          <span class="summary-chip">w toku / brak danych: ${pending}</span>
        </div>
      </article>
    `,
    `
      <article class="summary-card">
        <div class="summary-card-label">Platforma</div>
        <div class="summary-card-main">
          <div class="summary-card-value">${escapeHtml((summary.requested_backend || readConfig().backend || 'auto').toUpperCase())}</div>
          <div class="action-status status-ok">${escapeHtml(summary.platform_profile || readConfig().platform_profile || 'auto')}</div>
        </div>
        <div class="summary-card-meta">
          urządzenie: ${escapeHtml(deviceLabel)} • benchmark mode: ${escapeHtml(summary.benchmark_mode || readConfig().benchmark_mode || 'standard')}
        </div>
        <div class="summary-card-list">
          <span class="summary-chip">CPU B/R/F: ${readConfig().benchmarks_max_cpu_threads}/${readConfig().real_kernels_max_cpu_threads}/${readConfig().filip_max_cpu_threads}</span>
          <span class="summary-chip">backendy: ${(availableOptions().available_backends || []).map((item) => escapeHtml(item)).join(', ') || 'brak'}</span>
        </div>
      </article>
    `,
    `
      <article class="summary-card">
        <div class="summary-card-label">Exact / replay</div>
        <div class="summary-card-main">
          <div class="summary-card-value">${escapeHtml(exactInfo.headline)}</div>
          <div class="action-status ${summaryStatusClass(exactInfo.status)}">${escapeHtml(statusLabelLong(exactInfo.status))}</div>
        </div>
        <div class="summary-card-meta">${escapeHtml(exactInfo.meta)}</div>
        <div class="summary-card-list">
          ${exactInfo.chips.map((chip) => `<span class="summary-chip">${escapeHtml(chip)}</span>`).join('') || '<span class="summary-chip">brak dodatkowych artefaktów</span>'}
        </div>
      </article>
    `,
    `
      <article class="summary-card">
        <div class="summary-card-label">Finalne figury</div>
        <div class="summary-card-main">
          <div class="summary-card-value">${escapeHtml(String(totalPlots))}</div>
          <div class="action-status ${summaryStatusClass(figuresStatus)}">${escapeHtml(figuresStatusText)}</div>
        </div>
        <div class="summary-card-meta">
          thesis_core: oczekiwane ${expectedCore} (benchmarki: ${expectedBenchmark}, real+AI: ${expectedReal}).
        </div>
        <div class="summary-card-list">
          <span class="summary-chip">platforma: ${benchmarkCount}${missingBenchmark ? ` (brakuje ${missingBenchmark})` : ''}</span>
          <span class="summary-chip">real kernels: ${realCount}${missingReal ? ` (brakuje ${missingReal})` : ''}</span>
          <span class="summary-chip">Filip: ${filipCount}</span>
          <span class="summary-chip">exact: ${exactCount}</span>
        </div>
      </article>
    `,
  ];
  els.summaryStrip.innerHTML = cards.join('');
}

function renderPrimaryActions() {
  for (const group of groups()) {
    const info = groupState(group);
    const statusEl = document.getElementById(`groupStatus-${group.id}`);
    const textEl = document.getElementById(`groupText-${group.id}`);
    const pctEl = document.getElementById(`groupPct-${group.id}`);
    const fillEl = document.getElementById(`groupFill-${group.id}`);
    const cardEl = document.querySelector(`.action-card[data-group-id="${group.id}"]`);
    if (!statusEl || !textEl || !pctEl || !fillEl || !cardEl) continue;
    statusEl.textContent = statusLabelLong(info.status);
    statusEl.className = `action-status ${summaryStatusClass(info.status)}`;
    textEl.textContent = progressMetaText(info);
    pctEl.textContent = `${info.pct}%`;
    fillEl.style.width = `${info.pct}%`;
    cardEl.classList.toggle('active', info.status === 'running');
    cardEl.classList.toggle('failed', info.status === 'failed');
    cardEl.classList.toggle('completed', info.status === 'ok');
  }
}

function renderStageOverview() {
  const stageList = stages();
  if (!stageList.length) {
    els.stageOverview.innerHTML = '<div class="empty-state">Brak zdefiniowanych etapów badawczych.</div>';
    return;
  }
  els.stageOverview.innerHTML = stageList.map((stage) => {
    const info = stageState(stage);
    const label = info.nodes.map((node) => node.label).join(' • ');
    const chips = info.nodes.map((node) => `<span class="step-chip status-${escapeHtml(node.status || 'pending')}">${escapeHtml(node.label)}</span>`).join('');
    return `
      <article class="stage-card">
        <div class="stage-card-top">
          <div>
            <div class="stage-card-order">Etap ${escapeHtml(stage.order)}</div>
            <h3>${escapeHtml(stage.label)}</h3>
            <div class="stage-card-subtitle">${escapeHtml(stage.subtitle || '')}</div>
          </div>
          <div class="action-status ${summaryStatusClass(info.status)}">${escapeHtml(statusLabelLong(info.status))}</div>
        </div>
        <div class="action-progress">
          <div class="action-progress-label">
            <span>${escapeHtml(progressMetaText(info))}</span>
            <span>${escapeHtml(`${info.pct}%`)}</span>
          </div>
          <div class="node-progress-bar"><div class="node-progress-fill" style="width:${info.pct}%"></div></div>
        </div>
        <p>${escapeHtml(stage.description || '')}</p>
        <div class="stage-card-steps">${chips || `<span class="note">${escapeHtml(label || 'brak kroków')}</span>`}</div>
      </article>
    `;
  }).join('');
}

function renderCampaignSelect() {
  const campaigns = state.data?.campaign?.campaigns || [];
  const current = state.selectedCampaignDir || state.data?.latest_campaign_dir || '';
  if (!campaigns.length) {
    els.campaignSelect.innerHTML = '<option value="">Brak kampanii</option>';
    return;
  }
  els.campaignSelect.innerHTML = campaigns.map((entry) => {
    const selected = entry.path === current ? 'selected' : '';
    const flag = entry.critical_success ? 'OK' : 'uwaga';
    return `<option value="${escapeHtml(entry.path)}" ${selected}>${escapeHtml(entry.name)} (${flag})</option>`;
  }).join('');
}

function renderStepRail() {
  const nodes = state.data?.campaign?.nodes || [];
  const stageMarkup = stages().map((stage) => {
    const stageNodes = nodes.filter((node) => node.stage_id === stage.id);
    return `
      <section class="step-stage-group">
        <div class="step-stage-title">${escapeHtml(stage.label)}</div>
        <div class="step-stage-subtitle">${escapeHtml(stage.subtitle || '')}</div>
        ${stageNodes.map((node) => `
          <button class="step-rail-btn ${node.id === state.selectedStepId ? 'active' : ''}" data-step-id="${escapeHtml(node.id)}">
            ${escapeHtml(node.label)}
            <small>${escapeHtml(statusLabel(node.status))} • ${escapeHtml(formatDuration(node.elapsed_s))}</small>
          </button>
        `).join('')}
      </section>
    `;
  }).join('');
  els.stepRail.innerHTML = stageMarkup || nodes.map((node) => `
    <button class="step-rail-btn ${node.id === state.selectedStepId ? 'active' : ''}" data-step-id="${escapeHtml(node.id)}">
      ${escapeHtml(node.label)}
      <small>${escapeHtml(statusLabel(node.status))} • ${escapeHtml(formatDuration(node.elapsed_s))}</small>
    </button>
  `).join('');
  els.stepRail.querySelectorAll('.step-rail-btn').forEach((btn) => {
    btn.addEventListener('click', () => {
      state.selectedStepId = btn.dataset.stepId;
      renderStepRail();
      renderDetails();
    });
  });
}

function renderJobBanner() {
  const job = state.data?.job;
  if (!job) {
    els.jobBanner.classList.add('hidden');
    els.jobBanner.innerHTML = '';
    return;
  }
  const status = job.running ? 'Trwa uruchomienie' : (job.status === 'failed' ? 'Ostatnie uruchomienie zakończyło się błędem' : (job.status === 'ok' ? 'Ostatnie uruchomienie zakończone' : 'Gotowe'));
  const logPath = job.log_path ? `<div><strong>Log:</strong> <code>${escapeHtml(job.log_path)}</code></div>` : '';
  const cmd = Array.isArray(job.command) && job.command.length ? `<div><strong>Komenda:</strong> <code>${escapeHtml(job.command.join(' '))}</code></div>` : '';
  els.jobBanner.classList.remove('hidden');
  els.jobBanner.innerHTML = `
    <strong>${escapeHtml(status)}</strong>
    <div>${escapeHtml(job.label || '')}</div>
    <div>${job.running ? 'Krok aktywny: ' + escapeHtml(job.current_step || 'pełna kampania') : 'Kod wyjścia: ' + escapeHtml(job.exit_code)}</div>
    ${logPath}
    ${cmd}
  `;
}

function captureStepLogScrollSnapshot() {
  const stepLog = document.getElementById('stepLogBox');
  if (!stepLog) return null;
  const distanceToBottom = Math.max(0, stepLog.scrollHeight - (stepLog.scrollTop + stepLog.clientHeight));
  return {
    scrollTop: stepLog.scrollTop,
    stickToBottom: distanceToBottom <= 24,
  };
}

function restoreStepLogScrollSnapshot(snapshot) {
  if (!snapshot) return;
  const stepLog = document.getElementById('stepLogBox');
  if (!stepLog) return;
  if (snapshot.stickToBottom) {
    stepLog.scrollTop = stepLog.scrollHeight;
    return;
  }
  const maxTop = Math.max(0, stepLog.scrollHeight - stepLog.clientHeight);
  stepLog.scrollTop = Math.min(Math.max(0, snapshot.scrollTop || 0), maxTop);
}

async function renderDetails() {
  const node = selectedNode();
  if (!node) {
    els.detailsContent.innerHTML = '<div class="empty-state">Wybierz krok, aby zobaczyć jego status, wynik, log i wykresy. Szczegóły techniczne są schowane niżej, żeby nie robić szumu.</div>';
    els.detailsContent.dataset.nodeId = '';
    return;
  }

  const previousNodeId = els.detailsContent.dataset.nodeId || '';
  const logScrollSnapshot = previousNodeId === node.id ? captureStepLogScrollSnapshot() : null;

  const payload = node.payload || {};
  const images = node.images || [];
  const stage = stageMap().get(node.stage_id);
  const reason = node.reason ? `<p class="note">Uwaga: ${escapeHtml(node.reason)}</p>` : '';
  const deps = (node.depends_on || []).map((id) => `<li>${escapeHtml(id)}</li>`).join('');
  const isExactNode = node.id === 'filip_exact_reference';
  const exactInfo = isExactNode ? describeExactState(node) : null;
  const validationSummary = payload.validation_summary && typeof payload.validation_summary === 'object'
    ? payload.validation_summary
    : null;
  const validationEntries = validationSummary
    ? Object.entries(validationSummary)
        .filter(([, value]) => value !== null && value !== undefined && value !== '')
        .map(([key, value]) => `<li><strong>${escapeHtml(key.replaceAll('_', ' '))}:</strong> ${escapeHtml(typeof value === 'object' ? JSON.stringify(value) : String(value))}</li>`)
        .join('')
    : '';

  let logText = 'Brak logu.';
  if (node.log_path) {
    try {
      const log = await apiGet(`/api/log?path=${encodeURIComponent(node.log_path)}`);
      logText = log.text || 'Brak treści logu.';
    } catch (err) {
      logText = `Nie udało się wczytać logu: ${err.message}`;
    }
  }

  els.detailsContent.innerHTML = `
    <div class="eyebrow">Szczegóły kroku</div>
    <h3>${escapeHtml(node.label)}</h3>
    <div class="status-badge status-${escapeHtml(node.status || 'pending')}">${escapeHtml(statusLabel(node.status))}</div>
    <p>${escapeHtml(node.description || '')}</p>
    ${reason}
    <div class="kv-grid">
      <div class="kv-card"><span class="kv-label">Etap badawczy</span><div class="kv-value">${escapeHtml(stage?.label || 'brak')}</div></div>
      <div class="kv-card"><span class="kv-label">Workflow</span><div class="kv-value">${escapeHtml(node.workflow || '')}</div></div>
      <div class="kv-card"><span class="kv-label">Czas</span><div class="kv-value">${escapeHtml(formatDuration(node.elapsed_s))}</div></div>
      <div class="kv-card"><span class="kv-label">Katalog wyniku</span><div class="kv-value">${escapeHtml(node.result_dir || 'brak')}</div></div>
      <div class="kv-card"><span class="kv-label">Log</span><div class="kv-value">${escapeHtml(node.log_path || 'brak')}</div></div>
    </div>
    ${isExactNode ? `
      <div class="kv-card">
        <span class="kv-label">Stan walidacji exact / replay</span>
        <div class="kv-value"><strong>${escapeHtml(exactInfo?.headline || 'brak')}</strong></div>
        <div class="note">${escapeHtml(exactInfo?.meta || 'brak dodatkowych informacji')}</div>
        ${(payload.comparison_note || payload.numerical_equivalence || payload.replay_dump_root_source || validationEntries) ? `
          <ul class="info-list">
            ${payload.numerical_equivalence ? `<li><strong>Zgodność numeryczna:</strong> ${escapeHtml(humanizeExactEquivalence(payload.numerical_equivalence))}</li>` : ''}
            ${payload.replay_dump_root_source ? `<li><strong>Źródło replay bundle:</strong> ${escapeHtml(payload.replay_dump_root_source)}</li>` : ''}
            ${payload.comparison_note ? `<li><strong>Uwagi:</strong> ${escapeHtml(payload.comparison_note)}</li>` : ''}
            ${validationEntries}
          </ul>
        ` : ''}
      </div>
    ` : ''}
    <div class="details-actions">
      <button class="btn btn-primary" id="runSelectedStepBtn">Uruchom ten krok</button>
      <button class="btn btn-secondary" id="openResultBtn" ${node.result_dir ? '' : 'disabled'}>Otwórz wynik</button>
      <button class="btn btn-secondary" id="openLogBtn" ${node.log_path ? '' : 'disabled'}>Otwórz log</button>
    </div>
    <div class="kv-card">
      <span class="kv-label">Log (ostatnie linie)</span>
      <pre class="log-box" id="stepLogBox">${escapeHtml(logText)}</pre>
    </div>
    <div class="kv-card">
      <span class="kv-label">Wykresy i obrazy powiązane z krokiem</span>
      <div class="image-grid">
        ${images.length ? images.map((path) => `
          <figure class="image-card" data-image-path="${escapeHtml(path)}">
            <img src="/api/image?path=${encodeURIComponent(path)}" alt="${escapeHtml(path)}">
            <figcaption>${escapeHtml(path.split('/').slice(-1)[0])}<br><span class="note">Kliknij, aby powiększyć</span></figcaption>
          </figure>
        `).join('') : '<div class="empty-state">Dla tego kroku nie znaleziono zapisanych obrazów.</div>'}
      </div>
    </div>
    <details class="tech-details">
      <summary>Techniczne szczegóły kroku</summary>
      <div class="kv-card">
        <span class="kv-label">Zależności</span>
        <ul class="info-list">${deps || '<li>brak</li>'}</ul>
      </div>
      <div class="kv-card">
        <span class="kv-label">Payload</span>
        <pre class="log-box">${escapeHtml(JSON.stringify(payload, null, 2))}</pre>
      </div>
    </details>
  `;

  $('runSelectedStepBtn').addEventListener('click', async () => {
    await runSelectedStep(node.id);
  });
  $('openResultBtn')?.addEventListener('click', () => node.result_dir && openPath(node.result_dir));
  $('openLogBtn')?.addEventListener('click', () => node.log_path && openPath(node.log_path));
  els.detailsContent.querySelectorAll('.image-card[data-image-path]').forEach((card) => {
    card.addEventListener('click', () => openImageModal(card.dataset.imagePath || ''));
  });
  els.detailsContent.dataset.nodeId = node.id;
  restoreStepLogScrollSnapshot(logScrollSnapshot);
}

function renderPlotGallery(target, plots, emptyText) {
  if (!target) return;
  if (!plots.length) {
    target.innerHTML = `<div class="empty-state">${escapeHtml(emptyText)}</div>`;
    return;
  }
  target.innerHTML = plots.map((plot) => `
    <figure class="image-card" data-image-path="${escapeHtml(plot.path)}">
      <img src="/api/image?path=${encodeURIComponent(plot.path)}" alt="${escapeHtml(plot.name)}">
      <figcaption>${escapeHtml(plot.name)}<br><span class="note">Kliknij, aby powiększyć</span></figcaption>
    </figure>
  `).join('');
  target.querySelectorAll('.image-card[data-image-path]').forEach((card) => {
    card.addEventListener('click', () => openImageModal(card.dataset.imagePath || ''));
  });
}

function renderPlotSections() {
  const sections = plotSections();
  renderPlotGallery(
    els.benchmarkPlots,
    sections.benchmark || [],
    'Brak wykresów platformy. Uruchom benchmarki albo zbuduj wykresy zbiorcze.',
  );
  renderPlotGallery(
    els.realPlots,
    sections.real || [],
    'Brak wykresów real kernels / roofline / AI acceleration. Uruchom pakiet real kernels albo zbuduj wykresy zbiorcze.',
  );
  renderPlotGallery(
    els.filipVariantPlots,
    sections.filip_variants || [],
    'Brak wariantowych wykresów Filipa. Uruchom test Filipa albo odśwież wykresy Filipa.',
  );
  renderPlotGallery(
    els.filipTuningPlots,
    sections.filip_tuning || [],
    'Brak wykresów strojenia Filipa. Uruchom test Filipa albo odśwież wykresy Filipa.',
  );
  renderPlotGallery(
    els.exactPlots,
    sections.exact || [],
    'Brak figur exact / replay. Uruchom krok walidacji albo pełną kampanię z warstwą exact.',
  );
}

function openImageModal(path) {
  if (!path) return;
  currentModalPath = path;
  els.imageModalTitle.textContent = basename(path);
  els.imageModalPath.textContent = path;
  els.imageModalImg.src = `/api/image?path=${encodeURIComponent(path)}`;
  els.imageModalImg.alt = basename(path);
  els.imageModal.classList.remove('hidden');
  els.imageModal.setAttribute('aria-hidden', 'false');
}

function readSectionPrefs() {
  try {
    return JSON.parse(localStorage.getItem(SECTION_PREFS_KEY) || '{}');
  } catch {
    return {};
  }
}

function writeSectionPrefs(prefs) {
  try {
    localStorage.setItem(SECTION_PREFS_KEY, JSON.stringify(prefs));
  } catch {
    // ignore localStorage failures
  }
}

function bindSectionToggles() {
  const prefs = readSectionPrefs();
  document.querySelectorAll('details[data-section-key]').forEach((section) => {
    const key = section.dataset.sectionKey;
    if (!key) return;
    if (!section.dataset.bound) {
      section.dataset.bound = 'true';
      if (Object.prototype.hasOwnProperty.call(prefs, key)) {
        section.open = !!prefs[key];
      }
      section.addEventListener('toggle', () => {
        const nextPrefs = readSectionPrefs();
        nextPrefs[key] = section.open;
        writeSectionPrefs(nextPrefs);
      });
    }
  });
}

function setAllSections(open) {
  const nextPrefs = readSectionPrefs();
  document.querySelectorAll('details[data-section-key]').forEach((section) => {
    const key = section.dataset.sectionKey;
    if (!key) return;
    section.open = open;
    nextPrefs[key] = open;
  });
  writeSectionPrefs(nextPrefs);
}

function closeImageModal() {
  els.imageModal.classList.add('hidden');
  els.imageModal.setAttribute('aria-hidden', 'true');
  els.imageModalImg.removeAttribute('src');
  currentModalPath = '';
}

async function loadState(campaignDir = '') {
  const suffix = campaignDir ? `?campaign_dir=${encodeURIComponent(campaignDir)}` : '';
  state.data = await apiGet(`/api/state${suffix}`);
  const liveCampaignDir = (state.data.job?.selected_campaign_dir && state.data.job.running)
    ? state.data.job.selected_campaign_dir
    : '';
  if (liveCampaignDir && liveCampaignDir !== campaignDir) {
    state.selectedCampaignDir = liveCampaignDir;
    state.selectedStepId = null;
    return loadState(liveCampaignDir);
  }
  const preferredCampaignDir = (state.data.job?.selected_campaign_dir && !state.data.job.running)
    ? state.data.job.selected_campaign_dir
    : '';
  if (preferredCampaignDir && preferredCampaignDir !== campaignDir) {
    state.selectedCampaignDir = preferredCampaignDir;
    return loadState(preferredCampaignDir);
  }
  if (preferredCampaignDir) {
    state.selectedCampaignDir = preferredCampaignDir;
  } else if (liveCampaignDir) {
    state.selectedCampaignDir = liveCampaignDir;
  } else if (!state.selectedCampaignDir) {
    state.selectedCampaignDir = state.data.latest_campaign_dir || '';
  }
  applyAvailableOptions(state.data.available || {});
  if (!state.defaultsApplied) {
    applyDefaults(state.data.defaults);
    state.defaultsApplied = true;
  } else {
    syncDeviceChoiceFromFields();
  }
  renderCampaignSelect();
  renderJobBanner();
  renderSummaryStrip();
  renderPrimaryActions();
  renderStageOverview();
  if (!state.selectedStepId) {
    const firstInteresting = state.data.campaign.nodes.find((node) => node.status === 'running')
      || state.data.campaign.nodes.find((node) => node.status === 'failed')
      || state.data.campaign.nodes.find((node) => node.status === 'ok')
      || state.data.campaign.nodes[0];
    state.selectedStepId = firstInteresting?.id || null;
  }
  renderStepRail();
  await renderDetails();
  renderPlotSections();
  bindSectionToggles();
}

async function loadCampaign() {
  const campaignDir = els.campaignSelect.value;
  state.selectedCampaignDir = campaignDir;
  state.selectedStepId = null;
  await loadState(campaignDir);
}

async function openPath(path) {
  try {
    await apiPost('/api/open', { path });
  } catch (err) {
    alert(err.message);
  }
}

async function runFullPipeline() {
  try {
    await apiPost('/api/run/full', { config: readConfig() });
    await loadState(state.selectedCampaignDir);
  } catch (err) {
    alert(err.message);
  }
}

async function runGroup(groupId) {
  try {
    await apiPost('/api/run/group', {
      group_id: groupId,
      config: readConfig(),
      campaign_dir: state.selectedCampaignDir,
    });
    await loadState(state.selectedCampaignDir);
  } catch (err) {
    alert(err.message);
  }
}

async function runSelectedStep(stepId) {
  try {
    await apiPost('/api/run/step', {
      step_id: stepId,
      config: readConfig(),
      campaign_dir: state.selectedCampaignDir,
    });
    await loadState(state.selectedCampaignDir);
  } catch (err) {
    alert(err.message);
  }
}

async function stopJob(reason = 'web_stop_button') {
  try {
    await apiPost('/api/stop', { reason });
    await loadState(state.selectedCampaignDir);
  } catch (err) {
    alert(err.message);
  }
}

async function refreshPlots(mode) {
  try {
    await apiPost('/api/refresh-plots', {
      mode,
      config: readConfig(),
      campaign_dir: state.selectedCampaignDir,
    });
    await loadState(state.selectedCampaignDir);
  } catch (err) {
    alert(err.message);
  }
}

async function buildPlotsZip() {
  try {
    const suggestedPath = state.selectedCampaignDir
      ? `${state.selectedCampaignDir}/artifacts/plots_bundle_notebooklm.zip`
      : '';
    const requestedOut = window.prompt(
      'Podaj sciezke zapisu ZIP (opcjonalnie). Pozostaw puste, aby zapisac domyslnie.',
      suggestedPath,
    );
    if (requestedOut === null) {
      return;
    }
    const response = await apiPost('/api/build-plots-zip', {
      campaign_dir: state.selectedCampaignDir,
      config: readConfig(),
      out_zip: (requestedOut || '').trim(),
    });
    if (response?.zip_path) {
      alert(
        `ZIP gotowy:\n${response.zip_path}\n\n` +
        `Wykresy globalne: ${response.analysis_plot_count || 0}\n` +
        `Wykresy Filipa: ${response.filip_plot_count || 0}\n` +
        `CSV: ${response.result_csv_count || 0}`,
      );
      await openPath(response.zip_path);
      return;
    }
    alert('Zbudowano ZIP z wykresami.');
  } catch (err) {
    alert(err.message);
  }
}

function bindEvents() {
  els.runFullBtn.addEventListener('click', runFullPipeline);
  els.runBenchmarksBtn.addEventListener('click', () => runGroup('benchmarks'));
  els.runRealKernelsBtn.addEventListener('click', () => runGroup('real_kernels'));
  els.runFilipTestBtn.addEventListener('click', () => runGroup('filip_test'));
  els.stopJobBtn.addEventListener('click', () => stopJob('web_stop_button'));
  els.refreshStateBtn.addEventListener('click', () => loadState(state.selectedCampaignDir));
  els.expandSectionsBtn.addEventListener('click', () => setAllSections(true));
  els.collapseSectionsBtn.addEventListener('click', () => setAllSections(false));
  els.loadCampaignBtn.addEventListener('click', loadCampaign);
  els.openCampaignBtn.addEventListener('click', () => {
    const path = els.campaignSelect.value;
    if (path) openPath(path);
  });
  els.refreshSessionPlotsBtn.addEventListener('click', () => refreshPlots('session'));
  els.refreshFilipPlotsBtn.addEventListener('click', () => refreshPlots('filip'));
  els.buildPlotsZipBtn.addEventListener('click', buildPlotsZip);
  els.imageModalBackdrop.addEventListener('click', closeImageModal);
  els.imageModalClose.addEventListener('click', closeImageModal);
  els.imageModalOpenPath.addEventListener('click', () => currentModalPath && openPath(currentModalPath));
  $('deviceChoiceInput').addEventListener('change', () => {
    const selected = deviceChoices().find((item) => String(item.value) === String($('deviceChoiceInput').value));
    if (selected && selected.value !== 'auto') {
      $('backendInput').value = selected.backend;
      $('deviceIndexInput').value = selected.device_index;
    }
    if (!selected || selected.value === 'auto') {
      $('deviceIndexInput').value = 0;
    }
    updateDeviceHint();
  });
  $('backendInput').addEventListener('change', syncDeviceChoiceFromFields);
  ['benchmarksCpuThreadsInput', 'realKernelsCpuThreadsInput', 'filipCpuThreadsInput'].forEach((id) => {
    $(id).addEventListener('input', refreshCpuThreadControls);
    $(id).addEventListener('change', refreshCpuThreadControls);
  });
  window.addEventListener('keydown', (event) => {
    if (event.key === 'Escape' && !els.imageModal.classList.contains('hidden')) {
      closeImageModal();
    }
  });
}

async function poll() {
  if (state.pollInFlight) return;
  state.pollInFlight = true;
  try {
    await loadState(state.selectedCampaignDir);
  } catch (err) {
    console.error(err);
  } finally {
    state.pollInFlight = false;
  }
}

bindEvents();
loadState().catch((err) => {
  els.detailsContent.innerHTML = `<div class="empty-state">Nie udało się załadować stanu panelu: ${escapeHtml(err.message)}</div>`;
});
setInterval(poll, 5000);
