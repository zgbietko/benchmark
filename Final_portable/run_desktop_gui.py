#!/usr/bin/env python3
from __future__ import annotations

import json
import sys
import threading
import time
from pathlib import Path
import tkinter as tk
from tkinter import filedialog, messagebox, ttk
from tkinter.scrolledtext import ScrolledText

ROOT = Path(__file__).resolve().parent
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from web import pipeline_server as ui_backend

try:
    import matplotlib
    matplotlib.use("TkAgg")
    import matplotlib.image as mpimg
    from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2Tk
    from matplotlib.figure import Figure

    HAS_MPL = True
    MPL_ERROR = ""
except Exception as exc:
    HAS_MPL = False
    MPL_ERROR = f"{type(exc).__name__}: {exc}"
    mpimg = None  # type: ignore
    FigureCanvasTkAgg = None  # type: ignore
    NavigationToolbar2Tk = None  # type: ignore
    Figure = None  # type: ignore


class PlotBrowser(ttk.Frame):
    def __init__(self, master: tk.Misc, title: str) -> None:
        super().__init__(master, padding=8)
        self.title = title
        self.entries: list[dict[str, str]] = []
        self.selected_path = ""
        self._image_cache = None
        self.zoom_window: tk.Toplevel | None = None

        self.columnconfigure(1, weight=1)
        self.rowconfigure(1, weight=1, minsize=300)

        header = ttk.Label(self, text=title, font=("TkDefaultFont", 11, "bold"))
        header.grid(row=0, column=0, columnspan=2, sticky="w", pady=(0, 6))

        left = ttk.Frame(self)
        left.grid(row=1, column=0, sticky="nsw", padx=(0, 8))
        left.columnconfigure(0, weight=1)
        left.rowconfigure(0, weight=1, minsize=220)

        self.listbox = tk.Listbox(left, exportselection=False, width=38, height=20)
        self.listbox.grid(row=0, column=0, sticky="nsew")
        self.listbox.bind("<<ListboxSelect>>", self._on_select)
        self.listbox.bind("<MouseWheel>", self._on_listbox_wheel)
        self.listbox.bind("<Button-4>", self._on_listbox_wheel)
        self.listbox.bind("<Button-5>", self._on_listbox_wheel)
        scroll = ttk.Scrollbar(left, orient="vertical", command=self.listbox.yview)
        scroll.grid(row=0, column=1, sticky="ns")
        self.listbox.configure(yscrollcommand=scroll.set)

        btn_row = ttk.Frame(left)
        btn_row.grid(row=1, column=0, columnspan=2, sticky="ew", pady=(8, 0))
        btn_row.columnconfigure(0, weight=1)
        btn_row.columnconfigure(1, weight=1)
        ttk.Button(btn_row, text="Otworz plik", command=self.open_selected).grid(row=0, column=0, sticky="ew", padx=(0, 4))
        ttk.Button(btn_row, text="Powieksz", command=self.open_zoom).grid(row=0, column=1, sticky="ew", padx=(4, 0))

        right = ttk.Frame(self)
        right.grid(row=1, column=1, sticky="nsew")
        right.columnconfigure(0, weight=1)
        right.rowconfigure(0, weight=1, minsize=220)
        right.rowconfigure(2, weight=0)

        if HAS_MPL:
            self.figure = Figure(figsize=(7.5, 4.8), dpi=110)
            self.ax = self.figure.add_subplot(111)
            self.ax.axis("off")
            self.canvas = FigureCanvasTkAgg(self.figure, master=right)
            self.canvas.get_tk_widget().grid(row=0, column=0, sticky="nsew")
            self.canvas.get_tk_widget().bind("<Double-Button-1>", lambda _e: self.open_zoom())
            self.toolbar = NavigationToolbar2Tk(self.canvas, right, pack_toolbar=False)
            self.toolbar.update()
            self.toolbar.grid(row=1, column=0, sticky="ew", pady=(6, 0))
        else:
            self.figure = None
            self.ax = None
            self.canvas = None
            self.toolbar = None
            ttk.Label(
                right,
                text=f"Podglad wykresow niedostepny: {MPL_ERROR}",
                foreground="#a63c3c",
                wraplength=520,
                justify="left",
            ).grid(row=0, column=0, sticky="nw")

        self.info_var = tk.StringVar(value="Brak wykresow do podgladu.")
        ttk.Label(right, textvariable=self.info_var, wraplength=680, justify="left").grid(
            row=2, column=0, sticky="ew", pady=(8, 0)
        )

    def set_entries(self, entries: list[dict[str, str]]) -> None:
        previous = self.selected_path
        incoming = list(entries)
        old_sig = [(item.get("name", ""), item.get("path", "")) for item in self.entries]
        new_sig = [(item.get("name", ""), item.get("path", "")) for item in incoming]
        if old_sig == new_sig and self.entries:
            if previous and any(item.get("path") == previous for item in self.entries):
                return
        self.entries = incoming
        self.listbox.delete(0, tk.END)
        for item in self.entries:
            self.listbox.insert(tk.END, item.get("name", Path(item.get("path", "")).name))
        if not self.entries:
            self.selected_path = ""
            self._clear_preview("Brak wykresow w tej sekcji.")
            return
        selected_index = 0
        for idx, item in enumerate(self.entries):
            if item.get("path") == previous:
                selected_index = idx
                break
        self.listbox.selection_clear(0, tk.END)
        self.listbox.selection_set(selected_index)
        self.listbox.activate(selected_index)
        self._show_entry(self.entries[selected_index])

    def _clear_preview(self, message: str) -> None:
        self.info_var.set(message)
        self.selected_path = ""
        if self.ax is not None and self.canvas is not None:
            self.ax.clear()
            self.ax.axis("off")
            self.canvas.draw_idle()

    def _on_select(self, _event=None) -> None:
        selection = self.listbox.curselection()
        if not selection:
            return
        idx = int(selection[0])
        if 0 <= idx < len(self.entries):
            self._show_entry(self.entries[idx])

    def _on_listbox_wheel(self, event) -> str:
        try:
            if hasattr(event, "delta") and int(event.delta) != 0:
                step = -1 if int(event.delta) > 0 else 1
            else:
                step = -1 if int(getattr(event, "num", 0)) == 4 else 1
            self.listbox.yview_scroll(step, "units")
        except Exception:
            pass
        return "break"

    def _show_entry(self, entry: dict[str, str]) -> None:
        path = str(entry.get("path", ""))
        if not path:
            self._clear_preview("Brak sciezki wykresu.")
            return
        self.selected_path = path
        label = entry.get("name", Path(path).name)
        if self.ax is None or self.canvas is None or not HAS_MPL:
            self.info_var.set(f"{label}\n{path}")
            return
        try:
            image = mpimg.imread(path)
            self.ax.clear()
            self.ax.imshow(image)
            self.ax.axis("off")
            self.figure.tight_layout(pad=0.2)
            self.canvas.draw_idle()
            self.info_var.set(f"{label}\n{path}")
        except Exception as exc:
            self._clear_preview(f"Nie udalo sie zaladowac wykresu: {type(exc).__name__}: {exc}")

    def open_selected(self) -> None:
        if not self.selected_path:
            return
        ok, msg = ui_backend._open_path(Path(self.selected_path))
        if not ok:
            messagebox.showerror("Nie mozna otworzyc pliku", msg)

    def open_zoom(self) -> None:
        if not self.selected_path:
            return
        if not HAS_MPL:
            self.open_selected()
            return
        if self.zoom_window is not None and self.zoom_window.winfo_exists():
            self.zoom_window.lift()
            self.zoom_window.focus_force()
            return
        path = self.selected_path
        label = Path(path).name
        win = tk.Toplevel(self)
        win.title(f"Powiekszony wykres: {label}")
        win.geometry("1500x980")
        win.minsize(980, 620)
        win.rowconfigure(0, weight=1)
        win.columnconfigure(0, weight=1)
        host = ttk.Frame(win, padding=8)
        host.grid(row=0, column=0, sticky="nsew")
        host.rowconfigure(1, weight=1)
        host.columnconfigure(0, weight=1)
        fig = Figure(figsize=(11.0, 7.0), dpi=120)
        ax = fig.add_subplot(111)
        ax.axis("off")
        try:
            image = mpimg.imread(path)
            ax.imshow(image)
        except Exception as exc:
            ax.text(
                0.5,
                0.5,
                f"Nie udalo sie zaladowac obrazu:\n{type(exc).__name__}: {exc}",
                ha="center",
                va="center",
                transform=ax.transAxes,
            )
        canvas = FigureCanvasTkAgg(fig, master=host)
        toolbar = NavigationToolbar2Tk(canvas, host, pack_toolbar=False)
        toolbar.update()
        toolbar.grid(row=0, column=0, sticky="ew", pady=(0, 6))
        canvas.get_tk_widget().grid(row=1, column=0, sticky="nsew")
        canvas.draw_idle()
        ttk.Label(host, text=path, justify="left").grid(row=2, column=0, sticky="ew", pady=(6, 0))

        def _close_zoom() -> None:
            try:
                canvas.get_tk_widget().destroy()
            except Exception:
                pass
            try:
                win.destroy()
            except Exception:
                pass
            self.zoom_window = None

        win.protocol("WM_DELETE_WINDOW", _close_zoom)
        self.zoom_window = win


class PipelineDesktopApp(tk.Tk):
    POLL_MS = 1800

    def __init__(self) -> None:
        super().__init__()
        self.title("Final Desktop GUI")
        self.geometry("1820x1140")
        self.minsize(1420, 900)
        self.configure(background="#edf2f7")

        self.style = ttk.Style(self)
        try:
            self.style.theme_use("clam")
        except Exception:
            pass
        self._apply_modern_styles()

        self.selected_campaign_dir = ""
        self.current_view: dict[str, object] = {}
        self.current_job: dict[str, object] = {}
        self.selected_step_id = ""

        self.backend_var = tk.StringVar(value="auto")
        self.mode_var = tk.StringVar(value="standard")
        self.platform_var = tk.StringVar(value="auto")
        self.device_choice_var = tk.StringVar(value="auto")
        self.device_index_var = tk.StringVar(value="0")
        self.filip_case_var = tk.StringVar(value="prism_pair")
        self.replay_root_var = tk.StringVar(value="")
        self.repeats_var = tk.StringVar(value="5")
        self.real_runs_var = tk.StringVar(value="5")
        self.trials_var = tk.StringVar(value="256")
        self.population_var = tk.StringVar(value="24")
        self.iterations_var = tk.StringVar(value="40")
        self.validation_elements_var = tk.StringVar(value="16384")
        self.validation_qp_var = tk.StringVar(value="6")
        self.validation_wg_var = tk.StringVar(value="64")
        self.validation_operators_var = tk.StringVar(value="laplace,test")
        self.validation_variants_var = tk.StringVar(value="qss,sqs,ssq")
        self.profiler_reports_var = tk.StringVar(value="")
        self.bench_threads_var = tk.IntVar(value=1)
        self.real_threads_var = tk.IntVar(value=1)
        self.filip_threads_var = tk.IntVar(value=1)
        self.advanced_visible = tk.BooleanVar(value=False)
        self.cpu_thread_limit_max = 1
        self.thread_defaults_initialized = False

        self.action_widgets: dict[str, dict[str, object]] = {}
        self.stage_widgets: dict[str, dict[str, object]] = {}
        self.plot_browsers: dict[str, PlotBrowser] = {}
        self.campaign_paths: dict[str, str] = {}
        self._closing = False

        self._build_ui()
        self.protocol("WM_DELETE_WINDOW", self._on_close)
        self._refresh_state(force=True)
        self.after(self.POLL_MS, self._poll)

    def _apply_modern_styles(self) -> None:
        base_bg = "#eef2f7"
        panel_bg = "#ffffff"
        text_col = "#0f172a"
        muted = "#475569"
        accent = "#0f766e"
        border = "#d8e0ea"
        self.style.configure(".", background=base_bg, foreground=text_col)
        self.style.configure("TFrame", background=base_bg)
        self.style.configure("TLabel", background=base_bg, foreground=text_col)
        self.style.configure("Muted.TLabel", background=base_bg, foreground=muted)
        self.style.configure("Header.TLabel", background=base_bg, foreground=text_col, font=("TkDefaultFont", 20, "bold"))
        self.style.configure("TLabelframe", background=panel_bg, bordercolor=border, relief="solid", borderwidth=1)
        self.style.configure("TLabelframe.Label", background=base_bg, foreground=text_col, font=("TkDefaultFont", 10, "bold"))
        self.style.configure("Card.TLabelframe", background=panel_bg, bordercolor=border, relief="solid", borderwidth=1)
        self.style.configure("Card.TLabelframe.Label", background=panel_bg, foreground=text_col, font=("TkDefaultFont", 11, "bold"))
        self.style.configure("TNotebook", background=base_bg, borderwidth=0)
        self.style.configure("TNotebook.Tab", padding=(16, 10), font=("TkDefaultFont", 10, "bold"))
        self.style.map("TNotebook.Tab", background=[("selected", panel_bg)], foreground=[("selected", accent)])
        self.style.configure("TButton", padding=(10, 8))
        self.style.configure("Primary.TButton", padding=(12, 9))
        self.style.configure("Treeview", rowheight=26, fieldbackground=panel_bg, background=panel_bg)
        self.style.configure("Treeview.Heading", font=("TkDefaultFont", 10, "bold"))
        self.style.configure("TProgressbar", troughcolor="#dde4ee", background="#0f766e", bordercolor="#c9d4e3", lightcolor="#0f766e", darkcolor="#0f766e")

    def _build_ui(self) -> None:
        self.columnconfigure(0, weight=1)
        self.rowconfigure(1, weight=1)

        header = ttk.Frame(self, padding=(20, 14, 20, 8))
        header.grid(row=0, column=0, sticky="ew")
        header.columnconfigure(0, weight=1)
        ttk.Label(header, text="apple_microbench_variant2_streamfix / Final", style="Muted.TLabel", foreground="#0f766e").grid(row=0, column=0, sticky="w")
        ttk.Label(header, text="Panel kampanii doktorskiej", style="Header.TLabel").grid(row=1, column=0, sticky="w", pady=(2, 4))
        ttk.Label(
            header,
            text="Nowy uklad kart: Dashboard, Kampania i Wykresy. Mniej chaosu, pelna kontrola pipeline.",
            wraplength=1100,
            justify="left",
            style="Muted.TLabel",
        ).grid(row=2, column=0, sticky="w")
        actions = ttk.Frame(header)
        actions.grid(row=0, column=1, rowspan=3, sticky="e")
        ttk.Button(actions, text="Odswiez", command=lambda: self._refresh_state(force=True)).grid(row=0, column=0, padx=4)
        ttk.Button(actions, text="STOP", command=self._stop_current_job).grid(row=0, column=1, padx=4)
        ttk.Button(actions, text="Pelna kampania", style="Primary.TButton", command=self._run_full_campaign).grid(row=0, column=2, padx=4)
        ttk.Button(actions, text="Otworz kampanie", command=self._open_campaign_dir).grid(row=0, column=3, padx=4)

        self.main_tabs = ttk.Notebook(self)
        self.main_tabs.grid(row=1, column=0, sticky="nsew", padx=14, pady=(4, 12))

        dashboard_tab = ttk.Frame(self.main_tabs, padding=10)
        campaign_tab = ttk.Frame(self.main_tabs, padding=10)
        plots_tab = ttk.Frame(self.main_tabs, padding=10)
        self.main_tabs.add(dashboard_tab, text="Dashboard")
        self.main_tabs.add(campaign_tab, text="Kampania")
        self.main_tabs.add(plots_tab, text="Wykresy")

        dashboard_tab.columnconfigure(0, weight=3)
        dashboard_tab.columnconfigure(1, weight=4)
        dashboard_tab.rowconfigure(0, weight=1)

        launchpad = ttk.LabelFrame(dashboard_tab, text="Glowne pakiety", style="Card.TLabelframe", padding=12)
        launchpad.grid(row=0, column=0, sticky="nsew", padx=(0, 8))
        for col in range(3):
            launchpad.columnconfigure(col, weight=1)
        self._build_action_card(
            launchpad,
            0,
            "benchmarks",
            "Benchmarki platformy",
            "CPU i GPU microbenchmarki, roofline, przepustowosc i opoznienia pamieci.",
            self._run_group_benchmarks,
            row=0,
        )
        self._build_action_card(
            launchpad,
            1,
            "real_kernels",
            "Real kernels",
            "Warstwa posrednia: CPU/GPU real kernels oraz sciezki akceleracji AI.",
            self._run_group_real_kernels,
            row=0,
        )
        self._build_action_card(
            launchpad,
            2,
            "filip_test",
            "Test Filipa",
            "FEM validation, portable sweep, autotuning i Firefly.",
            self._run_group_filip,
            row=0,
        )

        controls = ttk.LabelFrame(dashboard_tab, text="Ustawienia uruchomienia", style="Card.TLabelframe", padding=12)
        controls.grid(row=0, column=1, sticky="nsew")
        for col in range(2):
            controls.columnconfigure(col, weight=1)

        self.backend_combo = self._labeled_combo(controls, 0, 0, "Backend", self.backend_var)
        self.mode_combo = self._labeled_combo(controls, 0, 1, "Tryb benchmarkow", self.mode_var)
        self.platform_combo = self._labeled_combo(controls, 1, 0, "Platform profile", self.platform_var)
        self.device_combo = self._labeled_combo(controls, 1, 1, "Dostepne urzadzenie", self.device_choice_var)
        self.device_combo.bind("<<ComboboxSelected>>", lambda _e: self._sync_device_choice())
        self.filip_combo = self._labeled_combo(controls, 2, 0, "Filip case", self.filip_case_var)
        self.backend_hint_var = tk.StringVar(value="")
        ttk.Label(controls, textvariable=self.backend_hint_var, foreground="#1b7f5f", wraplength=620, justify="left").grid(row=3, column=0, columnspan=2, sticky="w", pady=(6, 8))

        self._build_thread_slider(controls, 4, "Benchmarki: rdzenie CPU", self.bench_threads_var)
        self._build_thread_slider(controls, 5, "Real kernels: rdzenie CPU", self.real_threads_var)
        self._build_thread_slider(controls, 6, "Test Filipa: rdzenie CPU", self.filip_threads_var)

        advanced_toggle_row = ttk.Frame(controls)
        advanced_toggle_row.grid(row=7, column=0, columnspan=2, sticky="ew", pady=(8, 2))
        advanced_toggle_row.columnconfigure(0, weight=1)
        self.advanced_toggle_btn = ttk.Button(
            advanced_toggle_row,
            text="Pokaz parametry rozszerzone",
            command=self._toggle_advanced_controls,
        )
        self.advanced_toggle_btn.grid(row=0, column=0, sticky="w")

        self.advanced_frame = ttk.LabelFrame(controls, text="Parametry rozszerzone", padding=10)
        self.advanced_frame.grid(row=8, column=0, columnspan=2, sticky="ew", pady=(6, 0))
        for col in range(2):
            self.advanced_frame.columnconfigure(col, weight=1)
        self._labeled_entry(self.advanced_frame, 0, 0, "Replay dump root", self.replay_root_var, columnspan=2)
        self._labeled_entry(self.advanced_frame, 1, 0, "Powtorzenia", self.repeats_var)
        self._labeled_entry(self.advanced_frame, 1, 1, "Real runs", self.real_runs_var)
        self._labeled_entry(self.advanced_frame, 2, 0, "Trials", self.trials_var)
        self._labeled_entry(self.advanced_frame, 2, 1, "Population", self.population_var)
        self._labeled_entry(self.advanced_frame, 3, 0, "Iterations", self.iterations_var)
        self._labeled_entry(self.advanced_frame, 3, 1, "Validation elements", self.validation_elements_var)
        self._labeled_entry(self.advanced_frame, 4, 0, "Validation n_qp", self.validation_qp_var)
        self._labeled_entry(self.advanced_frame, 4, 1, "Validation WG", self.validation_wg_var)
        self._labeled_entry(self.advanced_frame, 5, 0, "Validation operators", self.validation_operators_var)
        self._labeled_entry(self.advanced_frame, 5, 1, "Validation variants", self.validation_variants_var)
        self._labeled_entry(self.advanced_frame, 6, 0, "Raporty profilera (lista)", self.profiler_reports_var, columnspan=2)
        self._update_advanced_controls_visibility()

        campaign_tab.columnconfigure(0, weight=1)
        campaign_tab.rowconfigure(1, weight=1)

        campaign_box = ttk.LabelFrame(campaign_tab, text="Kampanie i narzedzia", style="Card.TLabelframe", padding=10)
        campaign_box.grid(row=0, column=0, sticky="ew", pady=(0, 10))
        campaign_box.columnconfigure(0, weight=1)
        ttk.Label(campaign_box, text="Zaladowana kampania").grid(row=0, column=0, sticky="w")
        self.campaign_combo = ttk.Combobox(campaign_box, state="readonly")
        self.campaign_combo.grid(row=1, column=0, sticky="ew", pady=(4, 8))
        btn_row = ttk.Frame(campaign_box)
        btn_row.grid(row=2, column=0, sticky="ew")
        for i in range(4):
            btn_row.columnconfigure(i, weight=1)
        ttk.Button(btn_row, text="Wczytaj kampanie", command=self._load_campaign).grid(row=0, column=0, sticky="ew", padx=(0, 4))
        ttk.Button(btn_row, text="Wykresy zbiorcze", command=lambda: self._run_refresh_plots("session")).grid(row=0, column=1, sticky="ew", padx=4)
        ttk.Button(btn_row, text="Wykresy Filipa", command=lambda: self._run_refresh_plots("filip")).grid(row=0, column=2, sticky="ew", padx=4)
        ttk.Button(btn_row, text="ZIP wykresow", command=self._build_zip).grid(row=0, column=3, sticky="ew", padx=(4, 0))

        campaign_split = ttk.PanedWindow(campaign_tab, orient=tk.HORIZONTAL)
        campaign_split.grid(row=1, column=0, sticky="nsew")

        left_campaign = ttk.LabelFrame(campaign_split, text="Postep etapow badawczych", style="Card.TLabelframe", padding=10)
        right_campaign = ttk.LabelFrame(campaign_split, text="Kroki i log", style="Card.TLabelframe", padding=10)
        campaign_split.add(left_campaign, weight=2)
        campaign_split.add(right_campaign, weight=3)
        try:
            campaign_split.paneconfigure(left_campaign, minsize=500)
            campaign_split.paneconfigure(right_campaign, minsize=680)
        except Exception:
            pass

        left_campaign.columnconfigure(0, weight=1)
        for idx, stage in enumerate(ui_backend.PIPELINE_STAGES):
            row = ttk.Frame(left_campaign)
            row.grid(row=idx, column=0, sticky="ew", pady=4)
            row.columnconfigure(0, weight=1)
            label_var = tk.StringVar(value=stage["label"])
            meta_var = tk.StringVar(value="0% | oczekuje")
            ttk.Label(row, textvariable=label_var, font=("TkDefaultFont", 10, "bold")).grid(row=0, column=0, sticky="w")
            ttk.Label(row, textvariable=meta_var, foreground="#51627a").grid(row=0, column=1, sticky="e")
            progress = ttk.Progressbar(row, maximum=100, value=0, mode="determinate")
            progress.grid(row=1, column=0, columnspan=2, sticky="ew", pady=(4, 0))
            self.stage_widgets[stage["id"]] = {"label": label_var, "meta": meta_var, "progress": progress, "animating": False}

        right_campaign.columnconfigure(0, weight=1)
        right_campaign.rowconfigure(2, weight=2)
        right_campaign.rowconfigure(4, weight=3)
        self.job_status_var = tk.StringVar(value="Brak aktywnego zadania.")
        ttk.Label(right_campaign, textvariable=self.job_status_var, wraplength=900, justify="left").grid(row=0, column=0, sticky="ew")
        self.step_tree = ttk.Treeview(right_campaign, columns=("status", "elapsed"), show="tree headings", height=10)
        self.step_tree.heading("#0", text="Krok")
        self.step_tree.heading("status", text="Status")
        self.step_tree.heading("elapsed", text="Czas [s]")
        self.step_tree.column("#0", width=520)
        self.step_tree.column("status", width=140, anchor="center")
        self.step_tree.column("elapsed", width=110, anchor="e")
        self.step_tree.grid(row=2, column=0, sticky="nsew", pady=(8, 8))
        self.step_tree.bind("<<TreeviewSelect>>", self._on_step_selected)
        open_row = ttk.Frame(right_campaign)
        open_row.grid(row=3, column=0, sticky="ew")
        ttk.Button(open_row, text="Otworz wynik kroku", command=self._open_selected_step_result).pack(side="left", padx=(0, 6))
        ttk.Button(open_row, text="Otworz log kroku", command=self._open_selected_step_log).pack(side="left")
        self.log_text = ScrolledText(right_campaign, height=12, wrap="word")
        self.log_text.grid(row=4, column=0, sticky="nsew", pady=(10, 0))
        self.log_text.configure(state="disabled")

        plots_tab.columnconfigure(0, weight=1)
        plots_tab.rowconfigure(0, weight=1)
        plots = ttk.Notebook(plots_tab)
        plots.grid(row=0, column=0, sticky="nsew")
        for key, title in [
            ("benchmark", "Benchmarki platformy"),
            ("real", "Real kernels i AI"),
            ("filip_variants", "Filip: warianty"),
            ("filip_tuning", "Filip: tuning"),
            ("exact", "Exact / replay"),
        ]:
            browser = PlotBrowser(plots, title)
            plots.add(browser, text=title)
            self.plot_browsers[key] = browser

    def _labeled_combo(self, master: tk.Misc, row: int, column: int, label: str, variable: tk.StringVar) -> ttk.Combobox:
        frame = ttk.Frame(master)
        frame.grid(row=row, column=column, sticky="ew", padx=4, pady=4)
        frame.columnconfigure(0, weight=1)
        ttk.Label(frame, text=label).grid(row=0, column=0, sticky="w")
        combo = ttk.Combobox(frame, textvariable=variable, state="readonly")
        combo.grid(row=1, column=0, sticky="ew")
        return combo

    def _labeled_entry(self, master: tk.Misc, row: int, column: int, label: str, variable: tk.StringVar, columnspan: int = 1) -> ttk.Entry:
        frame = ttk.Frame(master)
        frame.grid(row=row, column=column, columnspan=columnspan, sticky="ew", padx=4, pady=4)
        frame.columnconfigure(0, weight=1)
        ttk.Label(frame, text=label).grid(row=0, column=0, sticky="w")
        entry = ttk.Entry(frame, textvariable=variable)
        entry.grid(row=1, column=0, sticky="ew")
        return entry

    def _build_thread_slider(self, master: tk.Misc, row: int, label: str, variable: tk.IntVar) -> None:
        frame = ttk.Frame(master)
        frame.grid(row=row, column=0, columnspan=2, sticky="ew", pady=2)
        frame.columnconfigure(1, weight=1)
        ttk.Label(frame, text=label).grid(row=0, column=0, sticky="w")
        value_var = tk.StringVar(value="1 / 1")
        scale_var = tk.IntVar(value=max(1, int(variable.get() or 1)))
        scale = tk.Scale(
            frame,
            from_=1,
            to=1,
            orient="horizontal",
            variable=scale_var,
            showvalue=False,
            resolution=1,
            highlightthickness=0,
            command=lambda raw, var=variable, text=value_var, sc=None, svar=scale_var: self._on_thread_scale(raw, var, text, svar),
        )
        scale.grid(row=0, column=1, sticky="ew", padx=8)
        ttk.Label(frame, textvariable=value_var, width=12).grid(row=0, column=2, sticky="e")
        frame.scale = scale  # type: ignore[attr-defined]
        frame.scale_var = scale_var  # type: ignore[attr-defined]
        frame.value_var = value_var  # type: ignore[attr-defined]
        variable.trace_add(
            "write",
            lambda *_args, sc=scale, vv=value_var, var=variable, svar=scale_var: self._sync_scale(var, vv, sc, svar),
        )
        if not hasattr(self, "thread_slider_frames"):
            self.thread_slider_frames = []
        self.thread_slider_frames.append((frame, variable))
        self._sync_scale(variable, value_var, scale, scale_var)

    def _build_action_card(
        self,
        master: tk.Misc,
        column: int,
        group_id: str,
        title: str,
        description: str,
        command,
        row: int = 0,
    ) -> None:
        card = ttk.LabelFrame(master, text=title, padding=10)
        card.grid(row=row, column=column, sticky="nsew", padx=6, pady=4)
        card.columnconfigure(0, weight=1)
        ttk.Label(card, text=description, wraplength=280, justify="left").grid(row=0, column=0, sticky="w")
        status_var = tk.StringVar(value="Brak danych")
        ttk.Label(card, textvariable=status_var, foreground="#51627a").grid(row=1, column=0, sticky="w", pady=(6, 0))
        progress = ttk.Progressbar(card, maximum=100, value=0, mode="determinate")
        progress.grid(row=2, column=0, sticky="ew", pady=(6, 4))
        meta_var = tk.StringVar(value="0% | oczekuje")
        ttk.Label(card, textvariable=meta_var, foreground="#51627a").grid(row=3, column=0, sticky="w")
        ttk.Button(card, text=f"Uruchom: {title}", style="Primary.TButton", command=command).grid(row=4, column=0, sticky="ew", pady=(8, 0))
        self.action_widgets[group_id] = {"status": status_var, "progress": progress, "meta": meta_var, "animating": False}

    def _set_progress_state(self, progress: ttk.Progressbar, *, running: bool, value: int, bag: dict[str, object] | None = None) -> None:
        value = max(0, min(100, int(value)))
        animating = bool((bag or {}).get("animating", False))
        if running:
            try:
                progress.configure(mode="indeterminate")
                if not animating:
                    progress.start(12)
                    if bag is not None:
                        bag["animating"] = True
            except Exception:
                progress.configure(mode="determinate", value=value)
            return
        try:
            if animating:
                progress.stop()
                if bag is not None:
                    bag["animating"] = False
            progress.configure(mode="determinate", value=value)
        except Exception:
            pass

    def _update_advanced_controls_visibility(self) -> None:
        is_visible = bool(self.advanced_visible.get())
        if is_visible:
            self.advanced_frame.grid()
            self.advanced_toggle_btn.configure(text="Ukryj parametry rozszerzone")
        else:
            self.advanced_frame.grid_remove()
            self.advanced_toggle_btn.configure(text="Pokaz parametry rozszerzone")

    def _toggle_advanced_controls(self) -> None:
        self.advanced_visible.set(not bool(self.advanced_visible.get()))
        self._update_advanced_controls_visibility()

    def _on_thread_scale(
        self,
        raw_value: str,
        variable: tk.IntVar,
        label_var: tk.StringVar,
        scale_var: tk.IntVar | tk.DoubleVar,
    ) -> None:
        try:
            value = int(round(float(raw_value)))
        except Exception:
            value = int(round(float(scale_var.get() or variable.get() or 1)))
        value = max(1, min(self.cpu_thread_limit_max, value))
        current = int(variable.get() or 1)
        if current != value:
            variable.set(value)
        current_scale = int(round(float(scale_var.get() or 1)))
        if current_scale != value:
            scale_var.set(float(value))
        label_var.set(f"{value} / {self.cpu_thread_limit_max}")

    def _sync_scale(
        self,
        variable: tk.IntVar,
        label_var: tk.StringVar,
        scale: tk.Scale | ttk.Scale | None = None,
        scale_var: tk.IntVar | tk.DoubleVar | None = None,
    ) -> None:
        value = int(round(float(variable.get() or 1)))
        value = max(1, min(self.cpu_thread_limit_max, value))
        if variable.get() != value:
            variable.set(value)
            return
        if scale_var is not None:
            current_scale = int(round(float(scale_var.get() or 1)))
            if current_scale != value:
                scale_var.set(float(value))
        elif scale is not None:
            scale.set(value)
        if scale is not None:
            widget_value = int(round(float(scale.get() or 1)))
            if widget_value != value:
                scale.set(value)
        label_var.set(f"{value} / {self.cpu_thread_limit_max}")

    def _config_payload(self) -> dict[str, object]:
        return {
            "backend": self.backend_var.get().strip() or "auto",
            "benchmark_mode": self.mode_var.get().strip() or "standard",
            "platform_profile": self.platform_var.get().strip() or "auto",
            "benchmarks_max_cpu_threads": int(self.bench_threads_var.get() or 1),
            "real_kernels_max_cpu_threads": int(self.real_threads_var.get() or 1),
            "filip_max_cpu_threads": int(self.filip_threads_var.get() or 1),
            "device_index": int(self.device_index_var.get() or 0),
            "filip_case": self.filip_case_var.get().strip() or "prism_pair",
            "replay_dump_root": self.replay_root_var.get().strip(),
            "repeats": int(self.repeats_var.get() or 5),
            "real_runs": int(self.real_runs_var.get() or 5),
            "trials": int(self.trials_var.get() or 256),
            "population": int(self.population_var.get() or 24),
            "iterations": int(self.iterations_var.get() or 40),
            "validation_operators": self.validation_operators_var.get().strip() or "laplace,test",
            "validation_variants": self.validation_variants_var.get().strip() or "qss,sqs,ssq",
            "validation_n_elements": int(self.validation_elements_var.get() or 16384),
            "validation_n_qp": int(self.validation_qp_var.get() or 6),
            "validation_workgroup_size": int(self.validation_wg_var.get() or 64),
            "correlation_profiler_reports": self.profiler_reports_var.get().strip(),
        }

    def _apply_available(self, payload: dict[str, object]) -> None:
        def fill_combo(combo: ttk.Combobox, values: list[str], current: str) -> None:
            combo["values"] = values
            if current in values:
                combo.set(current)
            elif values:
                combo.set(values[0])

        backend_choices = [item["value"] for item in payload.get("backend_choices", [])]
        mode_choices = [item["value"] for item in payload.get("benchmark_mode_choices", [])]
        profile_choices = [item["value"] for item in payload.get("platform_profile_choices", [])]
        case_choices = [item["value"] for item in payload.get("filip_case_choices", [])]
        device_choices = [item["value"] for item in payload.get("device_choices", [])]
        fill_combo(self.backend_combo, backend_choices, self.backend_var.get())
        fill_combo(self.mode_combo, mode_choices, self.mode_var.get())
        fill_combo(self.platform_combo, profile_choices, self.platform_var.get())
        fill_combo(self.filip_combo, case_choices, self.filip_case_var.get())
        fill_combo(self.device_combo, device_choices, self.device_choice_var.get())
        self.backend_hint_var.set(str(payload.get("backend_hint", "")))
        self.cpu_thread_limit_max = max(1, int(payload.get("cpu_thread_limit_max", 1) or 1))
        if not self.thread_defaults_initialized:
            self.bench_threads_var.set(self.cpu_thread_limit_max)
            self.real_threads_var.set(self.cpu_thread_limit_max)
            self.filip_threads_var.set(self.cpu_thread_limit_max)
            self.thread_defaults_initialized = True
        for frame, variable in getattr(self, "thread_slider_frames", []):
            scale = frame.scale  # type: ignore[attr-defined]
            scale_var = frame.scale_var  # type: ignore[attr-defined]
            value_var = frame.value_var  # type: ignore[attr-defined]
            scale.configure(from_=1, to=self.cpu_thread_limit_max)
            value = int(variable.get() or self.cpu_thread_limit_max)
            value = max(1, min(self.cpu_thread_limit_max, value))
            variable.set(value)
            scale_var.set(float(value))
            scale.set(value)
            value_var.set(f"{value} / {self.cpu_thread_limit_max}")
        self._sync_device_choice()

    def _sync_device_choice(self) -> None:
        raw = self.device_choice_var.get().strip()
        if raw and raw != "auto" and ":" in raw:
            backend, device_index = raw.split(":", 1)
            self.backend_var.set(backend)
            self.device_index_var.set(device_index)
        elif raw == "auto":
            self.device_index_var.set("0")

    def _current_campaign_path(self) -> Path | None:
        if self.selected_campaign_dir:
            return Path(self.selected_campaign_dir)
        latest = ui_backend._latest_campaign_dir()
        return latest

    def _refresh_state(self, force: bool = False) -> None:
        available = ui_backend._ui_capabilities(force=force)
        self._apply_available(available)

        job = ui_backend.JOB.snapshot()
        campaign_path = self._current_campaign_path()
        live_campaign_dir = str(job.get("selected_campaign_dir", "") or "").strip()
        if bool(job.get("running")) and live_campaign_dir:
            live_path = Path(live_campaign_dir)
            if live_path.exists():
                campaign_path = live_path
        elif (not bool(job.get("running"))) and live_campaign_dir:
            preferred_path = Path(live_campaign_dir)
            if preferred_path.exists():
                campaign_path = preferred_path
        if campaign_path is not None and not campaign_path.exists():
            campaign_path = ui_backend._latest_campaign_dir()
        summary = ui_backend._load_campaign_summary(campaign_path)
        view = ui_backend._campaign_view(summary)
        job_mode = str(job.get("mode", "") or "")
        is_pipeline_job = job_mode in {"group", "full", "step"}
        if (not bool(job.get("running"))) or (not is_pipeline_job):
            for node in view.get("nodes", []) or []:
                if str(node.get("status", "")) == "running":
                    node["status"] = "pending"
            summary_view = view.get("summary")
            if isinstance(summary_view, dict):
                summary_view["running"] = False
        self.current_view = view
        self.current_job = job
        if campaign_path is not None:
            self.selected_campaign_dir = str(campaign_path.resolve())
        self._update_campaign_combo(view.get("campaigns", []))
        self._update_action_cards(view, job)
        self._update_stage_progress(view, job)
        self._update_step_tree(view)
        self._update_job_panel(view, job)
        self._update_plot_browsers(view)

    def _update_campaign_combo(self, campaigns: list[dict[str, object]]) -> None:
        names = []
        self.campaign_paths = {}
        selected_name = ""
        for item in campaigns:
            name = str(item.get("name", ""))
            path = str(item.get("path", ""))
            names.append(name)
            self.campaign_paths[name] = path
            if path == self.selected_campaign_dir:
                selected_name = name
        self.campaign_combo["values"] = names
        if selected_name:
            self.campaign_combo.set(selected_name)
        elif names:
            self.campaign_combo.set(names[0])

    def _group_progress(self, group_id: str, view: dict[str, object], job: dict[str, object]) -> tuple[int, str, str, bool]:
        nodes = {node["id"]: node for node in view.get("nodes", [])}
        group = ui_backend.RUN_GROUP_INDEX[group_id]
        step_ids = list(group.get("step_ids", []))
        total = max(1, len(step_ids))
        closed = 0
        failed = 0
        running = False
        running_label = ""
        for step_id in step_ids:
            node = nodes.get(step_id) or {}
            status = str(node.get("status", "pending"))
            if status in {"ok", "failed", "skipped"}:
                closed += 1
            if status == "failed":
                failed += 1
            if str(job.get("current_step", "")) == step_id or status == "running":
                running = True
                running_label = str(node.get("label", "") or step_id)
        closed_for_pct = closed
        if running and total > 1 and closed_for_pct >= total:
            closed_for_pct = total - 1
        partial = 0.5 if running else 0.0
        pct = int(round(((closed_for_pct + partial) / total) * 100))
        pct = max(0, min(100, pct))
        queued = max(total - closed_for_pct - (1 if running else 0), 0)
        if failed:
            return pct, "Blad", f"{pct}% | zakonczone: {closed}/{len(step_ids)}, bledy: {failed}", False
        if running:
            active_txt = f" | aktywny: {running_label}" if running_label else ""
            return pct, "Trwa", f"{pct}% | w toku, po biezacym zostanie {queued} krokow{active_txt}", True
        if len(step_ids) > 0 and all(str((nodes.get(sid) or {}).get("status", "pending")) in {"ok", "skipped"} for sid in step_ids):
            return 100, "OK", f"100% | zakonczono ({closed}/{len(step_ids)})", False
        return pct, "Oczekuje", f"{pct}% | do wykonania: {max(len(step_ids) - closed, 0)} krokow", False

    def _update_action_cards(self, view: dict[str, object], job: dict[str, object]) -> None:
        for group_id, widgets in self.action_widgets.items():
            pct, status, meta, running = self._group_progress(group_id, view, job)
            widgets["status"].set(status)  # type: ignore[index]
            widgets["meta"].set(meta)  # type: ignore[index]
            self._set_progress_state(widgets["progress"], running=running, value=pct, bag=widgets)  # type: ignore[index,arg-type]

    def _update_stage_progress(self, view: dict[str, object], job: dict[str, object] | None = None) -> None:
        nodes = view.get("nodes", [])
        active_step = str((job or {}).get("current_step", ""))
        job_running = bool((job or {}).get("running"))

        def _is_excluded(node: dict[str, object]) -> bool:
            status = str(node.get("status", ""))
            reason = str(node.get("reason", "")).lower()
            if status == "skipped" and "nie nalezy do wybranego pakietu" in reason:
                return True
            if status == "skipped" and "not part of selected run group" in reason:
                return True
            return False

        for stage in ui_backend.PIPELINE_STAGES:
            stage_nodes = [node for node in nodes if node.get("stage_id") == stage["id"]]
            included_nodes = [node for node in stage_nodes if not _is_excluded(node)]
            total = len(included_nodes)
            widgets = self.stage_widgets[stage["id"]]
            if total == 0:
                widgets["meta"].set("Pominiety w tej kampanii")
                self._set_progress_state(widgets["progress"], running=False, value=0, bag=widgets)  # type: ignore[index,arg-type]
                continue

            done = sum(1 for node in included_nodes if str(node.get("status", "")) in {"ok", "failed", "skipped"})
            failed = sum(1 for node in included_nodes if str(node.get("status", "")) == "failed")
            step_ids = {str(node.get("id", "")) for node in included_nodes}
            running = any(str(node.get("status", "")) == "running" for node in stage_nodes)
            if job_running and active_step and active_step in step_ids:
                running = True
            done_for_pct = done
            if running and total > 1 and done_for_pct >= total:
                done_for_pct = total - 1
            partial = 0.5 if running else 0.0
            pct = int(round(((done_for_pct + partial) / total) * 100))
            pct = max(0, min(100, pct))
            queued = max(total - done_for_pct - (1 if running else 0), 0)
            if failed:
                meta = f"{pct}% | zakonczone: {done}/{len(stage_nodes)}, bledy: {failed}"
            elif running:
                meta = f"{pct}% | w toku, po biezacym zostanie {queued} krokow"
            elif len(stage_nodes) > 0 and all(str(node.get("status", "")) in {"ok", "skipped"} for node in stage_nodes):
                meta = f"100% | zakonczono ({done}/{len(stage_nodes)})"
            else:
                meta = f"{pct}% | do wykonania: {max(len(stage_nodes) - done, 0)} krokow"
            widgets["meta"].set(meta)  # type: ignore[index]
            self._set_progress_state(widgets["progress"], running=running, value=pct, bag=widgets)  # type: ignore[index,arg-type]

    def _update_step_tree(self, view: dict[str, object]) -> None:
        selected = self.selected_step_id
        self.step_tree.delete(*self.step_tree.get_children())
        for node in view.get("nodes", []):
            status = str(node.get("status", "pending"))
            elapsed = node.get("elapsed_s")
            elapsed_txt = "" if elapsed in (None, "") else f"{float(elapsed):.1f}"
            iid = str(node.get("id"))
            self.step_tree.insert("", tk.END, iid=iid, text=str(node.get("label", iid)), values=(status, elapsed_txt))
        if selected and self.step_tree.exists(selected):
            self.step_tree.selection_set(selected)
        elif view.get("nodes"):
            first = str(view["nodes"][0]["id"])
            self.step_tree.selection_set(first)
            self.selected_step_id = first

    def _step_map(self) -> dict[str, dict[str, object]]:
        return {str(node.get("id")): node for node in self.current_view.get("nodes", [])}

    def _update_job_panel(self, view: dict[str, object], job: dict[str, object]) -> None:
        label = str(job.get("label", ""))
        status = str(job.get("status", "idle"))
        current_step = str(job.get("current_step", ""))
        campaign_label = self.selected_campaign_dir or str(ui_backend._latest_campaign_dir() or "")
        if job.get("running"):
            text = f"Aktywne zadanie: {label or 'kampania'} | status={status} | krok={current_step or 'n/a'}\nKampania: {campaign_label}"
        else:
            text = f"Brak aktywnego zadania. Zaladowana kampania: {campaign_label or 'brak'}"
        self.job_status_var.set(text)

        node = self._step_map().get(self.selected_step_id) if self.selected_step_id else None
        if job.get("running") and str(job.get("log_path", "")).strip():
            path = Path(str(job.get("log_path")))
            log_text = ui_backend._tail_file(path) if path.exists() else "Log jeszcze nie istnieje."
        elif node is not None:
            log_text = json.dumps(ui_backend._step_summary(node), indent=2, ensure_ascii=False)
        else:
            summary = view.get("summary", {}) or {}
            log_text = json.dumps(summary, indent=2, ensure_ascii=False) if summary else "Brak summary kampanii."
        self.log_text.configure(state="normal")
        self.log_text.delete("1.0", tk.END)
        self.log_text.insert("1.0", log_text)
        self.log_text.configure(state="disabled")

    def _update_plot_browsers(self, view: dict[str, object]) -> None:
        plot_sections = view.get("plot_sections", {}) or {}
        for key, browser in self.plot_browsers.items():
            browser.set_entries(list(plot_sections.get(key, [])))

    def _on_step_selected(self, _event=None) -> None:
        selection = self.step_tree.selection()
        if selection:
            self.selected_step_id = str(selection[0])
            self._update_job_panel(self.current_view, self.current_job)

    def _open_selected_step_result(self) -> None:
        node = self._step_map().get(self.selected_step_id)
        if not node:
            return
        result_dir = str(node.get("result_dir", "")).strip()
        if not result_dir:
            return
        ok, msg = ui_backend._open_path(Path(result_dir))
        if not ok:
            messagebox.showerror("Nie mozna otworzyc katalogu", msg)

    def _open_selected_step_log(self) -> None:
        node = self._step_map().get(self.selected_step_id)
        if not node:
            return
        log_path = str(node.get("log_path", "")).strip()
        if not log_path:
            return
        ok, msg = ui_backend._open_path(Path(log_path))
        if not ok:
            messagebox.showerror("Nie mozna otworzyc logu", msg)

    def _load_campaign(self) -> None:
        name = self.campaign_combo.get().strip()
        if not name:
            return
        path = self.campaign_paths.get(name, "")
        if not path:
            return
        self.selected_campaign_dir = path
        self._refresh_state(force=True)

    def _open_campaign_dir(self) -> None:
        path = self._current_campaign_path()
        if path is None:
            return
        ok, msg = ui_backend._open_path(path)
        if not ok:
            messagebox.showerror("Nie mozna otworzyc katalogu kampanii", msg)

    def _run_group_benchmarks(self) -> None:
        self._run_group("benchmarks")

    def _run_group_real_kernels(self) -> None:
        self._run_group("real_kernels")

    def _run_group_filip(self) -> None:
        self._run_group("filip_test")

    def _run_group(self, group_id: str) -> None:
        ok, msg = ui_backend._start_job(ui_backend._run_group_pipeline_job, group_id, self._config_payload(), self.selected_campaign_dir)
        if not ok:
            messagebox.showwarning("Nie mozna uruchomic pakietu", msg)
        else:
            self._refresh_state(force=True)

    def _run_full_campaign(self) -> None:
        ok, msg = ui_backend._start_job(ui_backend._run_full_pipeline_job, self._config_payload())
        if not ok:
            messagebox.showwarning("Nie mozna uruchomic kampanii", msg)
        else:
            self._refresh_state(force=True)

    def _stop_current_job(self) -> None:
        snapshot = ui_backend.JOB.snapshot()
        if not snapshot.get("running"):
            messagebox.showinfo("Brak aktywnej kampanii", "Nie ma aktywnego procesu do zatrzymania.")
            return
        ok, msg = ui_backend._stop_active_job(reason="manual_stop_from_desktop", wait_s=2.5)
        if not ok:
            messagebox.showwarning("Nie mozna zatrzymac", msg)
        else:
            self.job_status_var.set("Zatrzymywanie kampanii...")
            self._refresh_state(force=True)

    def _start_utility_job(self, label: str, step_name: str, cmd: list[str]) -> None:
        def runner() -> None:
            ui_log_dir = ui_backend._make_ui_log_dir()
            ts = time.strftime("%Y%m%d_%H%M%S")
            log_path = ui_log_dir / f"{ts}__{step_name}.log"
            with ui_backend.JOB.lock:
                ui_backend.JOB.running = True
                ui_backend.JOB.job_id = ts
                ui_backend.JOB.mode = "utility"
                ui_backend.JOB.label = label
                ui_backend.JOB.started_at = time.time()
                ui_backend.JOB.finished_at = 0.0
                ui_backend.JOB.exit_code = None
                ui_backend.JOB.status = "running"
                ui_backend.JOB.log_path = str(log_path)
                ui_backend.JOB.current_step = step_name
                ui_backend.JOB.command = cmd
                ui_backend.JOB.latest_payload = {}
                ui_backend.JOB.selected_campaign_dir = self.selected_campaign_dir
                ui_backend.JOB.stop_requested = False
                ui_backend.JOB.active_pid = None
                ui_backend.JOB.active_process = None
            rc, payload = ui_backend._run_process(cmd, log_path=log_path, label=label)
            with ui_backend.JOB.lock:
                ui_backend.JOB.running = False
                ui_backend.JOB.finished_at = time.time()
                ui_backend.JOB.exit_code = rc
                if ui_backend.JOB.stop_requested:
                    ui_backend.JOB.status = "failed"
                    payload = {"error": "Przerwano przez uzytkownika.", **(payload or {})}
                else:
                    ui_backend.JOB.status = "ok" if rc == 0 else "failed"
                ui_backend.JOB.current_step = ""
                ui_backend.JOB.latest_payload = payload
                ui_backend.JOB.stop_requested = False
                ui_backend.JOB.active_pid = None
                ui_backend.JOB.active_process = None

        ok, msg = ui_backend._start_job(runner)
        if not ok:
            messagebox.showwarning("Nie mozna uruchomic narzedzia", msg)
        else:
            self._refresh_state(force=True)

    def _run_refresh_plots(self, mode: str) -> None:
        if mode == "session":
            cmd = [ui_backend.PYTHON, str(ROOT / "analysis" / "generate_plots.py")]
            self._start_utility_job("Pakiet wykresow zbiorczych", "session_plots", cmd)
            return
        path = self._current_campaign_path()
        summary = ui_backend._load_campaign_summary(path)
        if not summary:
            messagebox.showwarning("Brak kampanii", "Nie znaleziono summary.json do odswiezenia wykresow Filipa.")
            return
        portable = ui_backend._find_step(summary, "filip_original_portable") or {}
        optimization_dir = str(portable.get("result_dir") or "").strip()
        if not optimization_dir:
            messagebox.showwarning("Brak wynikow Filipa", "Wybrana kampania nie ma katalogu filip_original_portable.")
            return
        cmd = [ui_backend.PYTHON, str(ROOT / "analysis" / "filip_article_plots.py"), "--optimization-dir", optimization_dir]
        self._start_utility_job("Odswiezenie wykresow Filipa", "filip_plots", cmd)

    def _build_zip(self) -> None:
        path = self._current_campaign_path()
        if path is None:
            messagebox.showwarning("Brak kampanii", "Najpierw wybierz kampanie do zbudowania ZIP-a.")
            return
        default_name = f"plots_bundle__{path.name}.zip"
        chosen = filedialog.asksaveasfilename(
            title="Zapisz ZIP z wykresami i CSV",
            defaultextension=".zip",
            filetypes=[("ZIP archive", "*.zip"), ("All files", "*.*")],
            initialfile=default_name,
            initialdir=str(path),
        )
        if not chosen:
            return
        cmd = [
            ui_backend.PYTHON,
            str(ROOT / "analysis" / "build_plot_zip.py"),
            "--campaign-dir",
            str(path),
            "--out",
            str(Path(chosen).expanduser().resolve()),
        ]
        self._start_utility_job("Budowa ZIP wykresow", "plot_zip", cmd)

    def _poll(self) -> None:
        if self._closing:
            return
        try:
            self._refresh_state(force=False)
        finally:
            if not self._closing:
                self.after(self.POLL_MS, self._poll)

    def _on_close(self) -> None:
        self._closing = True
        if ui_backend.JOB.snapshot().get("running"):
            ui_backend._stop_active_job(reason="desktop_window_closed", wait_s=2.5)
        self.destroy()


def main() -> None:
    app = PipelineDesktopApp()
    app.mainloop()


if __name__ == "__main__":
    main()
