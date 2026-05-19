#!/usr/bin/env python3
from __future__ import annotations

import argparse
from dataclasses import dataclass
from datetime import datetime
import os
from pathlib import Path
import re
import textwrap
from typing import Iterable


ROOT = Path(__file__).resolve().parents[1]
DOCS = ROOT / "docs"

ACCENT = "#0f766e"
ACCENT_DARK = "#134e4a"
ACCENT_LIGHT = "#ccfbf1"
INK = "#0f172a"
MUTED = "#475569"
PAPER = "#fffdf8"
CODE_BG = "#f1f5f9"
TABLE_BG = "#f8fafc"


@dataclass(frozen=True)
class SectionSpec:
    rel: str
    title: str
    subtitle: str = ""


COMPACT_DOCS = [
    SectionSpec("README.md", "Wprowadzenie do v3", "Krótki opis platformy i punkt startowy do pracy."),
    SectionSpec("docs/V3_READY_REFERENCE.md", "Przewodnik startowy", "Najkrótsza ścieżka do uruchamiania eksperymentów i pracy z wynikami."),
    SectionSpec("docs/V3_DOCUMENTATION_INDEX.md", "Indeks dokumentacji", "Mapa wszystkich dokumentów i zalecana kolejność czytania."),
    SectionSpec("docs/THESIS_RESEARCH_PLAN.md", "Plan badawczy rozprawy", "Teza, pytania badawcze, hipotezy i wkład własny."),
    SectionSpec("docs/EXPERIMENTAL_PROTOCOL.md", "Protokół eksperymentalny", "Zasady prowadzenia kampanii i archiwizacji wyników."),
]

THEORY_DOCS = [
    SectionSpec("docs/TEORIA_OD_PODSTAW.md", "Teoria od podstaw", "Wyjaśnienie pojęć i logiki projektu dla osoby bez przygotowania informatycznego."),
]

THEORY_CHAPTER_DOCS = [
    SectionSpec("docs/ROZDZIAL_01_MOTYWACJA_I_KONTEKST.md", "Rozdział 1. Motywacja i kontekst badań", "Dlaczego sama analiza czasu nie wystarcza i jaki problem badawczy rozwiązuje v3."),
    SectionSpec("docs/ROZDZIAL_02_OBLICZENIA_NUMERYCZNE_I_FEM.md", "Rozdział 2. Obliczenia numeryczne i FEM", "Podstawy matematyczne i obliczeniowe potrzebne do zrozumienia realistycznego kernela FEM."),
    SectionSpec("docs/ROZDZIAL_03_ARCHITEKTURA_I_BACKENDY.md", "Rozdział 3. Architektura i backendy", "CPU, GPU, pamięć, backendy i konsekwencje organizacji wykonania dla wydajności."),
    SectionSpec("docs/ROZDZIAL_04_METODOLOGIA_BADAWCZA_I_WALIDACJA.md", "Rozdział 4. Metodologia badawcza i walidacja", "Warstwy mikrobenchmarków, walidacji FEM, exact i replayu poprawności."),
    SectionSpec("docs/ROZDZIAL_05_METRYKI_PROFILING_I_INTERPRETACJA.md", "Rozdział 5. Metryki, profiling i interpretacja", "Jak przejść od liczb do interpretacji mechanizmu działania kernela."),
    SectionSpec("docs/ROZDZIAL_06_SYNTEZA_WKLADU_I_ZAKRES_WNIOSKOW.md", "Rozdział 6. Synteza wkładu i zakres wniosków", "Podsumowanie wkładu własnego, ograniczeń i obszaru uzasadnionych wniosków."),
]

FULL_DOCS = [
    SectionSpec("README.md", "Wprowadzenie do v3", "Krótki opis platformy i punkt startowy do pracy."),
    SectionSpec("docs/TEORIA_OD_PODSTAW.md", "Teoria od podstaw", "Szerokie wyjaśnienie pojęć i warstw projektu dla osoby spoza informatyki."),
    SectionSpec("docs/ROZDZIAL_01_MOTYWACJA_I_KONTEKST.md", "Rozdział 1. Motywacja i kontekst badań", "Dlaczego sama analiza czasu nie wystarcza i jaki problem badawczy rozwiązuje v3."),
    SectionSpec("docs/ROZDZIAL_02_OBLICZENIA_NUMERYCZNE_I_FEM.md", "Rozdział 2. Obliczenia numeryczne i FEM", "Podstawy matematyczne i obliczeniowe potrzebne do zrozumienia realistycznego kernela FEM."),
    SectionSpec("docs/ROZDZIAL_03_ARCHITEKTURA_I_BACKENDY.md", "Rozdział 3. Architektura i backendy", "CPU, GPU, pamięć, backendy i konsekwencje organizacji wykonania dla wydajności."),
    SectionSpec("docs/ROZDZIAL_04_METODOLOGIA_BADAWCZA_I_WALIDACJA.md", "Rozdział 4. Metodologia badawcza i walidacja", "Warstwy mikrobenchmarków, walidacji FEM, exact i replayu poprawności."),
    SectionSpec("docs/ROZDZIAL_05_METRYKI_PROFILING_I_INTERPRETACJA.md", "Rozdział 5. Metryki, profiling i interpretacja", "Jak przejść od liczb do interpretacji mechanizmu działania kernela."),
    SectionSpec("docs/ROZDZIAL_06_SYNTEZA_WKLADU_I_ZAKRES_WNIOSKOW.md", "Rozdział 6. Synteza wkładu i zakres wniosków", "Podsumowanie wkładu własnego, ograniczeń i obszaru uzasadnionych wniosków."),
    SectionSpec("docs/V3_READY_REFERENCE.md", "Przewodnik startowy", "Praktyczny przewodnik do eksperymentów, wykresów i rozprawy."),
    SectionSpec("docs/V3_DOCUMENTATION_INDEX.md", "Indeks dokumentacji", "Mapa dokumentów i zależności między nimi."),
    SectionSpec("docs/PROJECT_MAP.md", "Mapa projektu", "Struktura repozytorium i główne warstwy eksperymentalne."),
    SectionSpec("docs/SYSTEM_REFERENCE.md", "Referencja systemowa", "Opis techniczny zaimplementowanych workflowów i artefaktów."),
    SectionSpec("docs/METHODOLOGY_MICROBENCH_TO_FEM.md", "Metodologia: od mikrobenchmarków do FEM", "Jak warstwa architektoniczna łączy się z realistycznym kernelem FEM."),
    SectionSpec("docs/FILIP_TIMING_PLOTS.md", "Wykresy czasu wykonania i autotuningu", "Jak interpretować wykresy czasu wykonania, ustawień autotuningu i najlepszych konfiguracji."),
    SectionSpec("docs/THESIS_RESEARCH_PLAN.md", "Plan badawczy rozprawy", "Teza, pytania badawcze, hipotezy i plan dowodowy."),
    SectionSpec("docs/THESIS_NAMING_MAP.md", "Mapa nazewnictwa do rozprawy", "Spójne słownictwo dla tekstu, wykresów i prezentacji."),
    SectionSpec("docs/EXPERIMENTAL_PROTOCOL.md", "Protokół eksperymentalny", "Zasady kampanii, powtórzeń, poprawności i archiwizacji."),
    SectionSpec("docs/THREATS_TO_VALIDITY.md", "Zagrożenia dla trafności", "Ograniczenia metodologiczne i ich kontrola."),
    SectionSpec("docs/END_TO_END_TESTING_AND_WRITING.md", "Przepływ pracy od testów do pisania", "Checklisty robocze do testów, raportów i rozprawy."),
    SectionSpec("docs/UBUNTU_FILIP_SETUP.md", "Konfiguracja Linux/OpenCL i exact", "Środowisko referencyjne, replay i eksport danych wejściowych."),
    SectionSpec("docs/CSV_SCHEMA.md", "Schemat danych CSV", "Znaczenie kolumn i spójność artefaktów tabelarycznych."),
    SectionSpec("docs/METRICS.md", "Definicje metryk", "Czas, throughput, GFLOP/s, energia i metryki roofline."),
    SectionSpec("docs/REPRODUCIBILITY.md", "Odtwarzalność eksperymentów", "Provenance, hashe i zamrażanie środowiska eksperymentalnego."),
]


def _try_import_pdfpages():
    mpl_cfg = ROOT / ".cache" / "matplotlib"
    mpl_cfg.mkdir(parents=True, exist_ok=True)
    os.environ.setdefault("MPLCONFIGDIR", str(mpl_cfg))
    import matplotlib

    matplotlib.use("Agg")
    from matplotlib.backends.backend_pdf import PdfPages  # type: ignore
    from matplotlib.figure import Figure  # type: ignore
    from matplotlib.patches import Rectangle  # type: ignore

    return PdfPages, Figure, Rectangle


REPLACEMENTS = [
    ("`Architecture Characterization with Mikrobenchmarki`", "`Charakterystyka architektury z użyciem mikrobenchmarków`"),
    ("Architecture Characterization with Mikrobenchmarki", "Charakterystyka architektury z użyciem mikrobenchmarków"),
    ("# Thesis Research Plan for v3", "# Plan badawczy rozprawy dla v3"),
    ("# System Reference for v3", "# Referencja systemowa v3"),
    ("# Methodology: from microbenchmarks to realistic FEM validation", "# Metodologia: od mikrobenchmarków do realistycznej walidacji FEM"),
    ("# Experimental Protocol for v3", "# Protokół eksperymentalny dla v3"),
    ("# Thesis naming map for v3", "# Mapa nazewnictwa do rozprawy dla v3"),
    ("# apple_microbench v3 - Project Map", "# apple_microbench v3 - mapa projektu"),
    ("# v3 Documentation Index", "# Indeks dokumentacji v3"),
    ("# v3 Ready Reference", "# v3 - przewodnik startowy"),
    ("Ready Reference", "Przewodnik startowy"),
    ("Documentation Bundle", "Pakiet dokumentacji"),
    ("Table of Contents", "Spis treści"),
    ("**microbenchmarks**", "**mikrobenchmarki**"),
    ("Microbenchmarks", "Mikrobenchmarki"),
    ("microbenchmarks", "mikrobenchmarki"),
    ("Reference exact kernels", "Referencyjne kernele exact"),
    ("Correctness replay", "Replay poprawności"),
    ("Native application campaigns", "Natywne kampanie aplikacyjne"),
    ("reference exact", "referencyjny exact"),
    ("correctness replay", "replay poprawności"),
    ("frozen inputs", "zamrożone dane wejściowe"),
    ("expected outputs", "oczekiwane wyniki"),
    ("FEM option validation", "Walidacja opcji FEM"),
    ("Profiler correlation", "Korelacja profilerowa"),
    ("profiler correlation", "korelacja profilerowa"),
    ("workflowy", "przepływy pracy"),
    ("workflowów", "przepływów pracy"),
    ("workflowow", "przepływów pracy"),
    ("workflow", "przepływ pracy"),
    ("strict 1:1", "ścisły 1:1"),
    ("frozen-input replay", "replay na zamrożonych danych wejściowych"),
    ("profiler-assisted correlation layer", "warstwa korelacji wsparta profilerem"),
    ("profiler-assisted correlation", "korelacja wsparta profilerem"),
    ("compact replay bundle", "kompaktowy pakiet replay"),
    ("canonical replay bundles", "kanoniczne pakiety replay"),
    ("replay bundle", "pakiet replay"),
    ("OpenCL exact reference run", "Referencyjny przebieg OpenCL exact"),
    ("Metal correctness replay", "Replay poprawności na Metalu"),
    ("native FEM performance campaign", "natywna kampania wydajnościowa FEM"),
    ("FEM option validation probes", "próby walidacyjne opcji FEM"),
    ("profiler correlation report", "raport korelacji profilerowej"),
    ("Architecture Characterization with Microbenchmarks", "Charakterystyka architektury z użyciem mikrobenchmarków"),
    ("From Microbenchmarks to FEM-Specific Validation Probes", "Od mikrobenchmarków do prób walidacyjnych specyficznych dla FEM"),
    ("Reference Exact Campaign and Frozen-Input Correctness Replay", "Referencyjna kampania exact i replay poprawności na zamrożonych danych wejściowych"),
    ("Profiler-Assisted Correlation Between Architectural Limits and FEM Kernel Behavior", "Korelacja wsparta profilerem między ograniczeniami architektury a zachowaniem kernela FEM"),
    ("Cross-Backend Native FEM Performance Campaigns", "Natywne kampanie wydajnościowe FEM na wielu backendach"),
    ("Threats to Validity and Scope of the Conclusions", "Zagrożenia dla trafności i zakres wniosków"),
    ("Backend roofline from microbenchmark peaks", "Roofline backendu wyznaczony z maksimów mikrobenchmarków"),
    ("FEM option validation deltas by backend", "Delta walidacji opcji FEM w podziale na backendy"),
    ("Exact-reference option landscape", "Krajobraz opcji referencyjnego exact"),
    ("Correctness replay error distribution", "Rozkład błędów replayu poprawności"),
    ("Correlation between probe deltas and best exact configuration", "Korelacja między deltami prób a najlepszą konfiguracją exact"),
    ("FEM option validation probe summary", "Podsumowanie prób walidacyjnych opcji FEM"),
    ("Exact-reference best configurations", "Najlepsze konfiguracje referencyjnego exact"),
    ("Correctness replay error summary", "Podsumowanie błędów replayu poprawności"),
    ("Profiler correlation summary", "Podsumowanie korelacji profilerowej"),
    ("`Architecture Characterization with Microbenchmarks`", "`Charakterystyka architektury z użyciem mikrobenchmarków`"),
    ("`FEM Option Validation Probes as an Interpretation Layer`", "`Próby walidacyjne opcji FEM jako warstwa interpretacyjna`"),
    ("`Reference Exact Campaign and Frozen-Input Correctness Replay`", "`Referencyjna kampania exact i replay poprawności na zamrożonych danych wejściowych`"),
    ("`Profiler-Assisted Correlation of Architectural Limits and FEM Kernel Behavior`", "`Korelacja wsparta profilerem między ograniczeniami architektury a zachowaniem kernela FEM`"),
    ("`Cross-Backend Native FEM Performance Campaigns`", "`Natywne kampanie wydajnościowe FEM na wielu backendach`"),
    ("The core contribution is a methodology that links architecture-level microbenchmarks with a realistic FEM kernel through validation probes, frozen-input correctness replay, and profiler-assisted correlation.", "Głównym wkładem jest metodologia łącząca mikrobenchmarki architektury z realistycznym kernelem FEM przez próby walidacyjne, replay poprawności na zamrożonych danych wejściowych oraz korelację wspartą profilerem."),
    ("`The core contribution is a methodology that links architecture-level microbenchmarks with a realistic FEM kernel through validation probes, frozen-input correctness replay, and profiler-assisted correlation.`", "`Głównym wkładem jest metodologia łącząca mikrobenchmarki architektury z realistycznym kernelem FEM przez próby walidacyjne, replay poprawności na zamrożonych danych wejściowych oraz korelację wspartą profilerem.`"),
    ("The core contribution is a methodology that links architecture-level", "Głównym wkładem jest metodologia łącząca mikrobenchmarki architektury z"),
    ("A methodology that combines architecture-level microbenchmarks, FEM option-validation probes, frozen-input correctness replay, and profiler-assisted correlation can explain and validate backend-dependent performance of a realistic FEM kernel more reliably than application timing alone.", "Metodologia łącząca mikrobenchmarki architektury, próby walidacyjne opcji FEM, replay poprawności na zamrożonych danych wejściowych oraz korelację wspartą profilerem pozwala wiarygodniej wyjaśniać i walidować zależne od backendu zachowanie realistycznego kernela FEM niż sama analiza czasu wykonania."),
    ("`A methodology that combines architecture-level microbenchmarks, FEM option-validation probes, frozen-input correctness replay, and profiler-assisted correlation can explain and validate backend-dependent performance of a realistic FEM kernel more reliably than application timing alone.`", "`Metodologia łącząca mikrobenchmarki architektury, próby walidacyjne opcji FEM, replay poprawności na zamrożonych danych wejściowych oraz korelację wspartą profilerem pozwala wiarygodniej wyjaśniać i walidować zależne od backendu zachowanie realistycznego kernela FEM niż sama analiza czasu wykonania.`"),
    ("and profiler-assisted correlation can explain and validate", "oraz korelacja wsparta profilerem pozwala wyjaśniać i walidować"),
]


def _localize_line(line: str) -> str:
    out = line
    for src, dst in REPLACEMENTS:
        out = out.replace(src, dst)
    return out


def _sanitize_lines(lines: list[str]) -> list[str]:
    out = [_localize_line(line.rstrip("\n")) for line in lines]
    while out and not out[0].strip():
        out.pop(0)
    if out and out[0].startswith("# "):
        out = out[1:]
        while out and not out[0].strip():
            out.pop(0)
    return out


def _load_sections(specs: list[SectionSpec]) -> list[tuple[SectionSpec, list[str]]]:
    sections: list[tuple[SectionSpec, list[str]]] = []
    for spec in specs:
        path = ROOT / spec.rel
        if not path.exists():
            continue
        lines = _sanitize_lines(path.read_text(encoding="utf-8", errors="replace").splitlines())
        sections.append((spec, lines))
    return sections


def _wrap_line(line: str, width: int) -> list[str]:
    if not line.strip():
        return [""]
    if line.lstrip().startswith("|"):
        return [line]
    if line.startswith("```"):
        return [line]
    line = _strip_inline_markdown(line)
    indent = len(line) - len(line.lstrip(" "))
    wrapped = textwrap.wrap(
        line.strip(),
        width=max(20, width - indent),
        break_long_words=False,
        break_on_hyphens=False,
    )
    if not wrapped:
        return [""]
    prefix = " " * indent
    return [prefix + part for part in wrapped]


def _wrap_plain_text(text: str, width: int) -> list[str]:
    text = _strip_inline_markdown(text)
    wrapped = textwrap.wrap(
        text.strip(),
        width=max(20, width),
        break_long_words=False,
        break_on_hyphens=False,
    )
    return wrapped or [text]


def _strip_inline_markdown(line: str) -> str:
    out = line.replace("**", "").replace("__", "").replace("`", "")
    out = re.sub(r"(?<!\S)\*(.+?)\*(?!\S)", r"\1", out)
    return out


def _line_style(line: str) -> tuple[float, float, str]:
    stripped = line.strip()
    if stripped.startswith("# "):
        return 18.0, 0.060, "bold"
    if stripped.startswith("## "):
        return 14.0, 0.048, "bold"
    if stripped.startswith("### "):
        return 12.0, 0.042, "bold"
    if stripped.startswith("```"):
        return 9.5, 0.032, "normal"
    if stripped.startswith("- ") or stripped.startswith("* "):
        return 10.8, 0.036, "normal"
    if stripped and stripped[0].isdigit() and ". " in stripped:
        return 10.8, 0.036, "normal"
    if stripped.startswith("|"):
        return 8.5, 0.030, "normal"
    return 10.8, 0.036, "normal"


def _draw_page_frame(ax, Rectangle, *, page_label: str, section_title: str, footer: str) -> None:
    ax.set_axis_off()
    ax.add_patch(Rectangle((0, 0), 1, 1, facecolor=PAPER, edgecolor="none"))
    ax.add_patch(Rectangle((0, 0.962), 1, 0.038, facecolor=ACCENT_DARK, edgecolor="none"))
    ax.add_patch(Rectangle((0, 0), 1, 0.028, facecolor=ACCENT_DARK, edgecolor="none"))
    ax.text(0.05, 0.978, section_title, fontsize=8.5, color="white", va="top", ha="left")
    ax.text(0.95, 0.978, page_label, fontsize=8.5, color="white", va="top", ha="right")
    ax.text(0.05, 0.012, footer, fontsize=8, color="white", va="bottom", ha="left")


def _render_section_cover(pdf, Figure, Rectangle, spec: SectionSpec, *, document_title: str, generated_at: str) -> None:
    fig = Figure(figsize=(8.27, 11.69))
    ax = fig.add_axes([0, 0, 1, 1])
    ax.set_axis_off()
    ax.add_patch(Rectangle((0, 0), 1, 1, facecolor=PAPER, edgecolor="none"))
    ax.add_patch(Rectangle((0, 0.72), 1, 0.28, facecolor=ACCENT_DARK, edgecolor="none"))
    ax.add_patch(Rectangle((0.06, 0.20), 0.88, 0.44, facecolor="white", edgecolor=ACCENT_LIGHT, linewidth=2.0))
    ax.add_patch(Rectangle((0.06, 0.58), 0.88, 0.06, facecolor=ACCENT_LIGHT, edgecolor="none"))
    ax.text(0.07, 0.95, "apple_microbench v3", fontsize=14, color="white", fontweight="bold", va="top", ha="left")
    ax.text(0.07, 0.90, document_title, fontsize=22, color="white", va="top", ha="left")
    ax.text(0.10, 0.56, spec.title, fontsize=24, color=INK, fontweight="bold", va="top", ha="left")
    if spec.subtitle:
        ax.text(0.10, 0.49, spec.subtitle, fontsize=12, color=MUTED, va="top", ha="left")
    ax.text(0.10, 0.36, f"Plik źródłowy: {spec.rel}", fontsize=10, color=MUTED, va="top", ha="left")
    ax.text(0.10, 0.31, "Sekcja dokumentacji v3 przygotowana do pracy eksperymentalnej i pisania rozprawy.", fontsize=10.5, color=INK, va="top", ha="left")
    ax.text(0.10, 0.12, f"Wygenerowano: {generated_at}", fontsize=9.5, color=MUTED, va="top", ha="left")
    pdf.savefig(fig)


def _render_text_pages(pdf, Figure, Rectangle, spec: SectionSpec, lines: list[str], *, footer: str, page_counter: list[int]) -> None:
    width_chars = 96
    prepared: list[str] = []
    in_code = False
    for line in lines:
        if line.strip().startswith("```"):
            in_code = not in_code
            prepared.append(line)
            continue
        prepared.extend(_wrap_line(line, 96 if in_code else width_chars))

    if not prepared:
        prepared = ["Brak treści do wyświetlenia."]

    idx = 0
    total = len(prepared)
    while idx < total:
        fig = Figure(figsize=(8.27, 11.69))
        ax = fig.add_axes([0, 0, 1, 1])
        _draw_page_frame(
            ax,
            Rectangle,
            page_label=f"strona {page_counter[0]}",
            section_title=spec.title,
            footer=footer,
        )
        ax.add_patch(Rectangle((0.05, 0.05), 0.90, 0.89, facecolor="white", edgecolor="#dbe4ea", linewidth=1.0))
        ax.text(0.08, 0.92, spec.title, fontsize=18, fontweight="bold", color=INK, va="top", ha="left")
        if spec.subtitle:
            ax.text(0.08, 0.885, spec.subtitle, fontsize=10.5, color=MUTED, va="top", ha="left")
        y = 0.845
        in_code_page = False
        while idx < total:
            line = prepared[idx]
            stripped = line.strip()
            if stripped.startswith("```"):
                in_code_page = not in_code_page
            fontsize, step, weight = _line_style(line)
            text = line
            if stripped.startswith("# "):
                text = stripped[2:]
            elif stripped.startswith("## "):
                text = stripped[3:]
            elif stripped.startswith("### "):
                text = stripped[4:]

            if y - step < 0.07:
                break

            family = "DejaVu Sans Mono" if stripped.startswith("```") or stripped.startswith("|") or in_code_page else "DejaVu Sans"
            color = INK
            x = 0.085
            if stripped.startswith("## ") or stripped.startswith("### "):
                color = ACCENT_DARK
            if in_code_page and not stripped.startswith("```"):
                ax.add_patch(Rectangle((0.078, y - step + 0.004), 0.84, step + 0.004, facecolor=CODE_BG, edgecolor="none"))
                x = 0.095
            elif stripped.startswith("|"):
                ax.add_patch(Rectangle((0.078, y - step + 0.004), 0.84, step + 0.004, facecolor=TABLE_BG, edgecolor="none"))
                x = 0.09
            ax.text(x, y, text, fontsize=fontsize, fontweight=weight, va="top", ha="left", family=family, color=color)
            y -= step
            idx += 1

        pdf.savefig(fig)
        page_counter[0] += 1


def build_pdf(out_path: Path, *, title: str, subtitle: str, specs: list[SectionSpec], footer: str) -> Path:
    PdfPages, Figure, Rectangle = _try_import_pdfpages()
    sections = _load_sections(specs)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    generated_at = datetime.now().astimezone().isoformat()
    page_counter = [1]

    with PdfPages(out_path) as pdf:
        fig = Figure(figsize=(8.27, 11.69))
        ax = fig.add_axes([0, 0, 1, 1])
        ax.set_axis_off()
        ax.add_patch(Rectangle((0, 0), 1, 1, facecolor=PAPER, edgecolor="none"))
        ax.add_patch(Rectangle((0, 0.70), 1, 0.30, facecolor=ACCENT_DARK, edgecolor="none"))
        ax.add_patch(Rectangle((0.06, 0.10), 0.88, 0.50, facecolor="white", edgecolor=ACCENT_LIGHT, linewidth=2.0))
        ax.add_patch(Rectangle((0.06, 0.54), 0.88, 0.06, facecolor=ACCENT_LIGHT, edgecolor="none"))
        ax.text(0.08, 0.94, "apple_microbench v3", fontsize=16, color="white", fontweight="bold", va="top", ha="left")
        ax.text(0.08, 0.88, title, fontsize=27, color="white", va="top", ha="left")
        subtitle_lines = _wrap_plain_text(subtitle, 80)
        sub_y = 0.82
        for line in subtitle_lines:
            ax.text(0.08, sub_y, line, fontsize=12, color="#e6fffb", va="top", ha="left")
            sub_y -= 0.03
        ax.text(0.10, 0.51, "Zakres dokumentu", fontsize=15, fontweight="bold", color=INK, va="top", ha="left")
        available_specs = [spec for spec in specs if (ROOT / spec.rel).exists()]
        split = 8 if len(available_specs) > 8 else len(available_specs)
        left_specs = available_specs[:split]
        right_specs = available_specs[split:]
        y_left = 0.47
        for spec in left_specs:
            ax.text(0.11, y_left, f"• {spec.title}", fontsize=11.0, color=INK, va="top", ha="left")
            y_left -= 0.028
        y_right = 0.47
        for spec in right_specs:
            ax.text(0.53, y_right, f"• {spec.title}", fontsize=10.6, color=INK, va="top", ha="left")
            y_right -= 0.028
        ax.text(0.10, 0.16, f"Wygenerowano: {generated_at}", fontsize=10, color=MUTED, va="top", ha="left")
        root_label = f"Katalog roboczy: …/{ROOT.name}"
        ax.text(0.10, 0.12, root_label, fontsize=10, color=MUTED, va="top", ha="left")
        pdf.savefig(fig)

        toc_fig = Figure(figsize=(8.27, 11.69))
        toc_ax = toc_fig.add_axes([0, 0, 1, 1])
        _draw_page_frame(toc_ax, Rectangle, page_label="strona 1", section_title=title, footer=footer)
        toc_ax.add_patch(Rectangle((0.05, 0.05), 0.90, 0.89, facecolor="white", edgecolor="#dbe4ea", linewidth=1.0))
        toc_ax.text(0.08, 0.92, "Spis treści", fontsize=22, fontweight="bold", color=INK, va="top", ha="left")
        toc_ax.text(0.08, 0.88, "Sekcje ułożone w kolejności czytania i pracy eksperymentalnej.", fontsize=10.5, color=MUTED, va="top", ha="left")
        y = 0.82
        for idx, spec in enumerate([spec for spec, _ in sections], 1):
            toc_ax.text(0.09, y, f"{idx}. {spec.title}", fontsize=12, color=INK, va="top", ha="left")
            if spec.subtitle:
                toc_ax.text(0.11, y - 0.028, spec.subtitle, fontsize=9.5, color=MUTED, va="top", ha="left")
                y -= 0.075
            else:
                y -= 0.05
            if y < 0.10:
                break
        pdf.savefig(toc_fig)
        page_counter[0] = 2

        for spec, lines in sections:
            _render_section_cover(pdf, Figure, Rectangle, spec, document_title=title, generated_at=generated_at)
            _render_text_pages(pdf, Figure, Rectangle, spec, lines, footer=footer, page_counter=page_counter)

    return out_path


def main() -> None:
    ap = argparse.ArgumentParser(description="Budowanie estetycznych, polskojęzycznych PDF-ów dokumentacji v3.")
    ap.add_argument("--out", default=str(DOCS / "v3_documentation_bundle.pdf"))
    ap.add_argument("--compact-out", default=str(DOCS / "v3_ready_reference.pdf"))
    ap.add_argument("--theory-out", default=str(DOCS / "v3_teoria_od_podstaw.pdf"))
    ap.add_argument("--theory-chapters-out", default=str(DOCS / "v3_rozdzialy_teoretyczne.pdf"))
    args = ap.parse_args()

    out = Path(args.out).expanduser().resolve()
    compact_out = Path(args.compact_out).expanduser().resolve()
    theory_out = Path(args.theory_out).expanduser().resolve()
    theory_chapters_out = Path(args.theory_chapters_out).expanduser().resolve()

    built_theory = build_pdf(
        theory_out,
        title="Teoria od podstaw v3",
        subtitle="Wprowadzenie do obliczeń numerycznych, FEM, backendów, benchmarków i replayu poprawności dla osoby spoza informatyki",
        specs=THEORY_DOCS,
        footer="apple_microbench v3 · teoria od podstaw",
    )
    built_theory_chapters = build_pdf(
        theory_chapters_out,
        title="Rozdziały teoretyczne v3",
        subtitle="Kompletny zestaw rozdziałów teoretycznych do pracy nad rozprawą: motywacja, FEM, architektura, metodologia, profiling i zakres wniosków",
        specs=THEORY_CHAPTER_DOCS,
        footer="apple_microbench v3 · rozdziały teoretyczne",
    )
    built_compact = build_pdf(
        compact_out,
        title="Przewodnik startowy v3",
        subtitle="Praktyczny dokument do uruchamiania eksperymentów, analizy wykresów i pracy nad rozprawą",
        specs=COMPACT_DOCS,
        footer="apple_microbench v3 · przewodnik startowy",
    )
    built_full = build_pdf(
        out,
        title="Pełna dokumentacja v3",
        subtitle="Mikrobenchmarki → walidacja FEM → referencja exact → replay poprawności → korelacja profilerowa",
        specs=FULL_DOCS,
        footer="apple_microbench v3 · pełna dokumentacja",
    )
    print(f"[OK] teoria od podstaw pdf: {built_theory}")
    print(f"[OK] rozdziały teoretyczne pdf: {built_theory_chapters}")
    print(f"[OK] przewodnik startowy pdf: {built_compact}")
    print(f"[OK] pełna dokumentacja pdf: {built_full}")


if __name__ == "__main__":
    main()
