"""Evaluate VEX onset, note and bend detection against labelled clips in test_audio/drills/.

Runs every `test_audio/drills/*.json` case through `build/vex-sim` (which shares `analyzer.h` and
`notes.h` with the Teensy sketch and WebAssembly workbench) and reports:
  - missed and extra onsets
  - picked-onset timing error (mean absolute and max error in ms)
  - MIDI note accuracy
  - settled string-bend plateau error in cents

Synthetic clips (`"kind": "synthetic"`) enforce strict accuracy thresholds in `make test`.
Real guitar clips (`"kind": "real"`, recorded with the Workbench "Record clip" button) assert note
count and MIDI sequence while reporting timing and pitch stats.
"""
from pathlib import Path
import argparse, json, subprocess, sys, tempfile

ROOT = Path(__file__).resolve().parents[1]
SIM = ROOT / "build/vex-sim"
DRILLS_DIR = ROOT / "test_audio/drills"

SEMI = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}

def note_to_midi(name: str) -> int:
    s = name.strip()
    semi = SEMI[s[0].upper()]
    idx = 1
    if idx < len(s) and s[idx] == "#":
        semi += 1
        idx += 1
    elif idx < len(s) and s[idx] == "b":
        semi -= 1
        idx += 1
    octv = int(s[idx:])
    return (octv + 1) * 12 + semi

def run_clip(spec: dict, spec_path: Path) -> list[dict]:
    lines = ["reset", "calibrate", f"pitch {spec.get('pitch_mode', 'tuner')}", "events clear"]
    if "wav" in spec:
        wav_path = (spec_path.parent / spec["wav"]).resolve()
        rel = wav_path.relative_to(ROOT)
        cmd = f"wav {rel}"
        if "start_ms" in spec:
            cmd += f" {spec['start_ms']}"
            if "length_ms" in spec:
                cmd += f" {spec['length_ms']}"
        lines.append(cmd)
    for c in spec.get("commands", []):
        lines.append(c)
    lines.append("events")

    with tempfile.TemporaryDirectory() as tmp:
        proc = subprocess.run(
            [str(SIM), f"{tmp}/frame.ppm", f"{tmp}/save.bin"],
            input="\n".join(lines) + "\n",
            text=True,
            capture_output=True,
            cwd=ROOT,
        )
        if proc.returncode != 0:
            raise RuntimeError(f"{spec_path.name}: vex-sim exited {proc.returncode}: {proc.stderr.strip()}")
        last = json.loads(proc.stdout.strip().splitlines()[-1])
        return last.get("events", [])

def evaluate_case(spec: dict, events: list[dict]) -> dict:
    expected = spec.get("expected", [])
    kind = spec.get("kind", "real")
    exp_n, det_n = len(expected), len(events)

    if kind == "synthetic":
        # Greedy monotonic pairing within +-90 ms window (or +-140 ms for legato pitch-step onsets).
        used = set()
        pairs = []
        for ei, exp in enumerate(expected):
            best_di, best_err = None, 1e9
            tol = 140.0 if exp.get("legato") else 80.0
            want_ms = exp.get("onset_ms")
            for di, det in enumerate(events):
                if di in used:
                    continue
                if want_ms is not None:
                    err = abs(det["onset_ms"] - want_ms)
                    if err <= tol and err < best_err:
                        best_di, best_err = di, err
                else:
                    best_di, best_err = di, 0.0
                    break
            if best_di is not None:
                used.add(best_di)
                pairs.append((exp, events[best_di]))
        missed = exp_n - len(pairs)
        extra = det_n - len(pairs)
    else:
        n = min(exp_n, det_n)
        pairs = [(expected[i], events[i]) for i in range(n)]
        missed = max(0, exp_n - det_n)
        extra = max(0, det_n - exp_n)

    timing_errs = []
    note_hits = 0
    note_total = 0
    bend_errs = []
    seq_mismatch = False

    for exp, det in pairs:
        if "onset_ms" in exp and not exp.get("legato"):
            timing_errs.append(abs(det["onset_ms"] - float(exp["onset_ms"])))
        if "note" in exp:
            note_total += 1
            want_midi = note_to_midi(exp["note"])
            if det.get("pitched") and det.get("midi") == want_midi:
                note_hits += 1
            else:
                seq_mismatch = True
        if "bend_cents" in exp:
            bend_errs.append(abs(float(det.get("bend_cents", 0.0)) - float(exp["bend_cents"])))

    return {
        "name": spec.get("name", "unnamed"),
        "kind": kind,
        "expected": exp_n,
        "detected": det_n,
        "missed": missed,
        "extra": extra,
        "timing_errs": timing_errs,
        "note_hits": note_hits,
        "note_total": note_total,
        "bend_errs": bend_errs,
        "seq_mismatch": seq_mismatch,
    }

def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--quiet", action="store_true", help="Print one-line summary unless thresholds fail")
    args = ap.parse_args()

    if not SIM.exists():
        sys.exit("build/vex-sim is missing; run `make sim-build` or `make test` first.")

    paths = sorted(DRILLS_DIR.glob("*.json"))
    if not paths:
        sys.exit(f"No clip specs found in {DRILLS_DIR}")

    results = []
    failures = []

    syn_missed = syn_extra = 0
    syn_timing = []
    syn_note_hits = syn_note_total = 0
    syn_bends = []
    real_cases = 0

    for p in paths:
        spec = json.loads(p.read_text())
        events = run_clip(spec, p)
        res = evaluate_case(spec, events)
        results.append(res)

        if res["kind"] == "synthetic":
            syn_missed += res["missed"]
            syn_extra += res["extra"]
            syn_timing.extend(res["timing_errs"])
            syn_note_hits += res["note_hits"]
            syn_note_total += res["note_total"]
            syn_bends.extend(res["bend_errs"])
            if res["missed"] or res["extra"]:
                failures.append(f"{p.name}: missed={res['missed']} extra={res['extra']}")
            if res["note_hits"] != res["note_total"]:
                failures.append(f"{p.name}: note accuracy {res['note_hits']}/{res['note_total']}")
            if res["timing_errs"] and max(res["timing_errs"]) > 12.0:
                failures.append(f"{p.name}: onset max error {max(res['timing_errs']):.2f} ms > 12.0 ms")
            if res["bend_errs"] and max(res["bend_errs"]) > 5.0:
                failures.append(f"{p.name}: bend max error {max(res['bend_errs']):.2f} c > 5.0 c")
        else:
            real_cases += 1
            if res["missed"] or res["extra"] or res["seq_mismatch"]:
                failures.append(
                    f"{p.name} (real): expected {res['expected']} notes, got {res['detected']} "
                    f"(hits {res['note_hits']}/{res['note_total']})"
                )

    mae_ms = sum(syn_timing) / len(syn_timing) if syn_timing else 0.0
    max_ms = max(syn_timing) if syn_timing else 0.0
    bend_mae = sum(syn_bends) / len(syn_bends) if syn_bends else 0.0
    bend_max = max(syn_bends) if syn_bends else 0.0
    note_pct = 100.0 * syn_note_hits / syn_note_total if syn_note_total else 100.0

    if mae_ms > 5.0:
        failures.append(f"synthetic onset MAE {mae_ms:.2f} ms > 5.0 ms")
    if bend_mae > 3.5:
        failures.append(f"synthetic bend MAE {bend_mae:.2f} c > 3.5 c")

    if not args.quiet:
        for r in results:
            t_str = f"mae={sum(r['timing_errs'])/len(r['timing_errs']):.1f}ms" if r["timing_errs"] else "timing=n/a"
            b_str = f" bend_mae={sum(r['bend_errs'])/len(r['bend_errs']):.1f}c" if r["bend_errs"] else ""
            print(
                f"  [{r['kind']:9s}] {r['name']:28s} notes={r['note_hits']}/{r['note_total']} "
                f"missed={r['missed']} extra={r['extra']} {t_str}{b_str}"
            )

    if failures:
        sys.exit("FAIL audio eval:\n  " + "\n  ".join(failures))

    print(
        f"PASS: audio eval ({len(results)} clips: {len(results)-real_cases} synthetic, {real_cases} real) "
        f"missed={syn_missed} extra={syn_extra} onset_mae={mae_ms:.2f}ms (max {max_ms:.2f}ms) "
        f"notes={syn_note_hits}/{syn_note_total} ({note_pct:.0f}%) bend_mae={bend_mae:.2f}c (max {bend_max:.2f}c)"
    )

if __name__ == "__main__":
    main()
