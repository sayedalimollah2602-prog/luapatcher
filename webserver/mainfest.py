#!/usr/bin/env python3
"""
ManifestHub Lua Extractor v1.1 (verbose)
────────────────────────────────────────
Clones the ManifestHub repo, iterates every numeric branch,
and copies <branch>.lua into ./games/

Usage:
    python manifest_grabber.py
    python manifest_grabber.py --appid 2087060
    python manifest_grabber.py --appid 2087060 2087390
    python manifest_grabber.py --all-files
    python manifest_grabber.py --cleanup
"""

import subprocess
import os
import sys
import re
import shutil
import argparse
import time
import threading
from pathlib import Path


REPO_URL   = "https://github.com/SteamAutoCracks/ManifestHub.git"
CLONE_DIR  = "ManifestHub"
OUTPUT_DIR = "games"


# ─────────────────────────────────────────
# Helpers
# ─────────────────────────────────────────

def run(cmd, cwd=None):
    """Run a command silently, return (success, stdout, stderr)."""
    result = subprocess.run(
        cmd,
        cwd=cwd,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    return result.returncode == 0, result.stdout, result.stderr


def run_live(cmd, cwd=None, label=""):
    """
    Run a command with REAL-TIME output streaming.
    Both stdout and stderr are printed line by line.
    Returns (success, combined_output).
    """
    print(f"[>] Running: {' '.join(cmd)}")
    if cwd:
        print(f"[>] CWD: {cwd}")
    print()

    output_lines = []

    process = subprocess.Popen(
        cmd,
        cwd=cwd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,  # merge stderr into stdout
        text=True,
        encoding="utf-8",
        errors="replace",
        bufsize=1,  # line buffered
    )

    # Spinner for when git doesn't produce output for a while
    spinner_chars = ["⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"]
    # Fallback ASCII spinner for terminals that don't support unicode
    try:
        sys.stdout.write("⠋")
        sys.stdout.write("\r")
        sys.stdout.flush()
    except UnicodeEncodeError:
        spinner_chars = ["|", "/", "-", "\\"]

    last_output_time = time.time()
    spinner_idx = 0
    done = False

    def read_output():
        nonlocal done, last_output_time
        for line in process.stdout:
            line = line.rstrip("\n\r")
            if line:
                # Clear spinner line if present
                sys.stdout.write("\r" + " " * 80 + "\r")
                sys.stdout.flush()
                print(f"    {line}")
                output_lines.append(line)
                last_output_time = time.time()
        done = True

    reader = threading.Thread(target=read_output, daemon=True)
    reader.start()

    # Show spinner while waiting for output
    while not done:
        elapsed = time.time() - last_output_time
        if elapsed > 1.0:
            spinner_idx = (spinner_idx + 1) % len(spinner_chars)
            total_elapsed = time.time() - (last_output_time - elapsed + elapsed)
            sys.stdout.write(f"\r    {spinner_chars[spinner_idx]}  Waiting for data... ({time.time() - last_output_time + elapsed:.0f}s elapsed)")
            sys.stdout.flush()
        time.sleep(0.15)

    reader.join(timeout=5)
    process.wait()

    # Clear any spinner remnant
    sys.stdout.write("\r" + " " * 80 + "\r")
    sys.stdout.flush()

    success = process.returncode == 0
    print(f"[{'OK' if success else 'X'}] Exit code: {process.returncode}\n")
    return success, "\n".join(output_lines)


def check_git():
    """Make sure git is installed."""
    ok, out, _ = run(["git", "--version"])
    if not ok:
        print("[X] git is not installed or not in PATH.")
        print("    Install it from https://git-scm.com/downloads")
        sys.exit(1)
    print(f"[OK] {out.strip()}\n")


def format_size(bytes_val):
    """Human-readable file size."""
    for unit in ["B", "KB", "MB", "GB"]:
        if bytes_val < 1024:
            return f"{bytes_val:.1f} {unit}"
        bytes_val /= 1024
    return f"{bytes_val:.1f} TB"


def get_dir_size(path):
    """Get total size of a directory."""
    total = 0
    for dirpath, dirnames, filenames in os.walk(path):
        for f in filenames:
            fp = os.path.join(dirpath, f)
            try:
                total += os.path.getsize(fp)
            except OSError:
                pass
    return total


# ─────────────────────────────────────────
# Clone / Update
# ─────────────────────────────────────────

def clone_or_update(clone_dir):
    """Clone the repo or fetch updates if it already exists."""

    if os.path.isdir(os.path.join(clone_dir, ".git")):
        print(f"[*] Repo already exists at ./{clone_dir}")
        dir_size = get_dir_size(clone_dir)
        print(f"[*] Current size on disk: {format_size(dir_size)}")
        print(f"[*] Fetching latest branches ...\n")

        ok, _ = run_live(
            ["git", "fetch", "--all", "--prune", "--progress"],
            cwd=clone_dir,
            label="Fetch"
        )
        if ok:
            new_size = get_dir_size(clone_dir)
            print(f"[OK] Fetch complete (size: {format_size(new_size)})\n")
        else:
            print(f"[!] Fetch had warnings, continuing anyway\n")
        return True

    print(f"[*] Clone target: ./{clone_dir}")
    print(f"[*] Repository:   {REPO_URL}")
    print(f"[*] Mode:         --no-checkout (fast, no working tree)")
    print(f"[*] Starting clone ...\n")

    start = time.time()

    ok, _ = run_live(
        ["git", "clone", "--filter=blob:none", "--no-checkout", "--progress", REPO_URL, clone_dir],
        label="Clone"
    )

    elapsed = time.time() - start

    if not ok:
        print(f"[X] Clone failed after {elapsed:.1f}s")
        return False

    dir_size = get_dir_size(clone_dir)
    print(f"[OK] Clone complete!")
    print(f"     Time: {elapsed:.1f}s")
    print(f"     Size: {format_size(dir_size)}")
    print()
    return True


# ─────────────────────────────────────────
# Branch listing
# ─────────────────────────────────────────

def get_numeric_branches(clone_dir, verbose=False):
    """Return sorted list of all remote branches that are purely numeric."""
    print("[*] Running: git branch -r")
    ok, stdout, stderr = run(["git", "branch", "-r"], cwd=clone_dir)
    if not ok:
        print(f"[X] Failed to list branches: {stderr[:200]}")
        return []

    all_branches = []
    numeric_branches = []
    skipped = []

    for line in stdout.splitlines():
        name = line.strip()
        if "->" in name:
            continue
        if name.startswith("origin/"):
            name = name[len("origin/"):]

        all_branches.append(name)

        if re.match(r"^\d+$", name):
            numeric_branches.append(name)
        else:
            skipped.append(name)

    numeric_branches = sorted(set(numeric_branches), key=lambda x: int(x))

    print(f"[*] Total remote branches: {len(all_branches)}")
    print(f"[*] Numeric (App ID) branches: {len(numeric_branches)}")
    print(f"[*] Skipped (non-numeric): {len(skipped)}")

    if verbose and skipped:
        print(f"    Skipped branches: {', '.join(skipped[:20])}")
        if len(skipped) > 20:
            print(f"    ... and {len(skipped) - 20} more")

    if verbose and numeric_branches:
        print(f"    ID range: {numeric_branches[0]} - {numeric_branches[-1]}")

    print()
    return numeric_branches


# ─────────────────────────────────────────
# Lua extraction
# ─────────────────────────────────────────

def extract_lua(clone_dir, branch_name, output_dir, verbose=False):
    """
    Use 'git show origin/<branch>:<branch>.lua' to read the lua file
    without switching branches.
    """
    lua_filename = f"{branch_name}.lua"
    ref = f"origin/{branch_name}:{lua_filename}"

    if verbose:
        print(f"      git show {ref}")

    ok, content, err = run(["git", "show", ref], cwd=clone_dir)

    if not ok:
        # Maybe the file has a different name? Try listing files
        if verbose:
            ok2, ls_out, _ = run(
                ["git", "ls-tree", "--name-only", f"origin/{branch_name}"],
                cwd=clone_dir
            )
            if ok2:
                files = [f.strip() for f in ls_out.splitlines() if f.strip()]
                print(f"      Files in branch: {files}")

                # Try to find any .lua file
                lua_files = [f for f in files if f.endswith(".lua")]
                if lua_files and lua_files[0] != lua_filename:
                    alt_ref = f"origin/{branch_name}:{lua_files[0]}"
                    print(f"      Trying alternative: {alt_ref}")
                    ok, content, err = run(["git", "show", alt_ref], cwd=clone_dir)
                    if ok and content.strip():
                        out_path = os.path.join(output_dir, lua_files[0])
                        with open(out_path, "w", encoding="utf-8") as f:
                            f.write(content)
                        return True, out_path
            return False, f"not found ({err.strip()[:80]})"

    if not content.strip():
        return False, "file is empty"

    out_path = os.path.join(output_dir, lua_filename)
    with open(out_path, "w", encoding="utf-8") as f:
        f.write(content)

    size = len(content.encode("utf-8"))
    return True, f"{out_path} ({format_size(size)})"


def extract_all_files(clone_dir, branch_name, output_dir, verbose=False):
    """Extract ALL files from a branch into a subfolder."""
    ref = f"origin/{branch_name}"
    ok, stdout, _ = run(["git", "ls-tree", "--name-only", ref], cwd=clone_dir)
    if not ok:
        return []

    branch_dir = os.path.join(output_dir, branch_name)
    os.makedirs(branch_dir, exist_ok=True)

    saved = []
    for filename in stdout.strip().splitlines():
        filename = filename.strip()
        if not filename:
            continue

        if verbose:
            print(f"      git show {ref}:{filename}")

        ok2, content, _ = run(["git", "show", f"{ref}:{filename}"], cwd=clone_dir)
        if ok2 and content:
            fpath = os.path.join(branch_dir, filename)
            with open(fpath, "w", encoding="utf-8") as f:
                f.write(content)
            size = len(content.encode("utf-8"))
            saved.append((filename, size))

    return saved


# ─────────────────────────────────────────
# Progress bar
# ─────────────────────────────────────────

def progress_bar(current, total, width=30, extra=""):
    """Render an inline progress bar."""
    pct = current / total if total else 0
    filled = int(width * pct)
    bar = "#" * filled + "-" * (width - filled)
    return f"  [{bar}] {current}/{total} ({pct*100:.0f}%) {extra}"


# ─────────────────────────────────────────
# Main
# ─────────────────────────────────────────

def main():
    print()
    print("  =============================================")
    print("    ManifestHub Lua Extractor  v1.1 (verbose)")
    print("  =============================================")
    print()

    parser = argparse.ArgumentParser(
        description="Extract .lua manifest files from ManifestHub branches"
    )
    parser.add_argument(
        "--appid", nargs="*", type=str, default=None,
        help="Specific App ID(s) to extract (default: all)"
    )
    parser.add_argument(
        "--output", "-o", type=str, default=OUTPUT_DIR,
        help=f"Output directory (default: ./{OUTPUT_DIR})"
    )
    parser.add_argument(
        "--clone-dir", type=str, default=CLONE_DIR,
        help=f"Clone directory (default: ./{CLONE_DIR})"
    )
    parser.add_argument(
        "--all-files", action="store_true",
        help="Extract ALL files (lua + json + manifest + vdf) into subfolders"
    )
    parser.add_argument(
        "--cleanup", action="store_true",
        help="Delete cloned repo after extraction"
    )
    parser.add_argument(
        "--list", action="store_true",
        help="List available app IDs only"
    )
    parser.add_argument(
        "-v", "--verbose", action="store_true",
        help="Extra verbose output (show git commands, file contents checks)"
    )

    args = parser.parse_args()
    output_dir = args.output
    clone_dir  = args.clone_dir
    verbose    = args.verbose

    # ── Preflight ──
    print("[STEP 1/4] Checking git installation")
    print("-" * 45)
    check_git()

    # ── Clone / Update ──
    print("[STEP 2/4] Clone / Update repository")
    print("-" * 45)
    if not clone_or_update(clone_dir):
        sys.exit(1)

    # ── Get branches ──
    print("[STEP 3/4] Scanning branches")
    print("-" * 45)
    all_branches = get_numeric_branches(clone_dir, verbose=verbose)

    if not all_branches:
        print("[X] No numeric branches found.")
        sys.exit(1)

    # ── Filter ──
    if args.appid:
        requested = set(args.appid)
        branches = [b for b in all_branches if b in requested]
        missing = requested - set(branches)
        if missing:
            print(f"[!] Not found in repo: {', '.join(sorted(missing))}")
        if not branches:
            print("[X] None of the requested App IDs exist.")
            print(f"    Available: {len(all_branches)} apps")
            print(f"    Use --list to see all IDs")
            sys.exit(1)
        print(f"[*] Matched {len(branches)} of {len(requested)} requested IDs\n")
    else:
        branches = all_branches

    # ── List mode ──
    if args.list:
        print(f"  Available App IDs ({len(all_branches)}):")
        print(f"  {'-'*50}")
        cols = 6
        for i in range(0, len(all_branches), cols):
            row = all_branches[i:i+cols]
            print("  " + "  ".join(f"{b:>10}" for b in row))
        print()
        sys.exit(0)

    # ── Extract ──
    print("[STEP 4/4] Extracting files")
    print("-" * 45)

    os.makedirs(output_dir, exist_ok=True)

    mode = "All files (subfolders)" if args.all_files else "Lua files only"
    print(f"  Mode       : {mode}")
    print(f"  Output     : {os.path.abspath(output_dir)}")
    print(f"  Branches   : {len(branches)}")
    if branches:
        print(f"  First      : {branches[0]}")
        print(f"  Last       : {branches[-1]}")
    print()

    success = 0
    failed  = 0
    failed_ids = []
    total_bytes = 0
    start = time.time()
    width = len(str(len(branches)))

    for idx, branch in enumerate(branches, 1):
        prefix = f"  [{idx:>{width}}/{len(branches)}]"

        if args.all_files:
            if verbose:
                print(f"{prefix} Branch: {branch}")
                print(f"      Listing files in origin/{branch} ...")

            saved = extract_all_files(clone_dir, branch, output_dir, verbose=verbose)

            if saved:
                total_size = sum(s for _, s in saved)
                total_bytes += total_size
                file_list = ", ".join(f"{n} ({format_size(s)})" for n, s in saved)
                print(f"{prefix} {branch:>10} -> {len(saved)} files: {file_list}")
                success += 1
            else:
                print(f"{prefix} {branch:>10} -> FAILED (no files)")
                failed += 1
                failed_ids.append(branch)
        else:
            if verbose:
                print(f"{prefix} Branch: {branch}")

            ok, result = extract_lua(clone_dir, branch, output_dir, verbose=verbose)

            if ok:
                print(f"{prefix} {branch:>10}.lua  [OK]  {result}")
                success += 1
            else:
                print(f"{prefix} {branch:>10}.lua  [FAIL] {result}")
                failed += 1
                failed_ids.append(branch)

        # Show progress bar every 50 items or at the end
        if idx % 50 == 0 or idx == len(branches):
            elapsed = time.time() - start
            rate = idx / elapsed if elapsed > 0 else 0
            eta = (len(branches) - idx) / rate if rate > 0 else 0
            print(progress_bar(
                idx, len(branches),
                extra=f" {rate:.0f}/s  ETA: {eta:.0f}s"
            ))
            print()

    elapsed = time.time() - start

    # ── Summary ──
    print()
    print("  =============================================")
    print(f"    COMPLETE")
    print(f"  =============================================")
    print(f"    Time       : {elapsed:.1f}s ({elapsed/60:.1f} min)")
    print(f"    Speed      : {len(branches)/elapsed:.1f} branches/sec" if elapsed > 0 else "")
    print(f"    Success    : {success}")
    print(f"    Failed     : {failed}")
    if total_bytes:
        print(f"    Total size : {format_size(total_bytes)}")
    print(f"    Output     : {os.path.abspath(output_dir)}")

    if failed_ids and len(failed_ids) <= 20:
        print(f"    Failed IDs : {', '.join(failed_ids)}")
    elif failed_ids:
        print(f"    Failed IDs : {', '.join(failed_ids[:20])} ... +{len(failed_ids)-20} more")

    # Count output files
    out_files = 0
    out_size  = 0
    for dirpath, dirnames, filenames in os.walk(output_dir):
        for f in filenames:
            out_files += 1
            try:
                out_size += os.path.getsize(os.path.join(dirpath, f))
            except OSError:
                pass
    print(f"    Files saved: {out_files} ({format_size(out_size)})")
    print("  =============================================")
    print()

    # ── Cleanup ──
    if args.cleanup and os.path.isdir(clone_dir):
        clone_size = get_dir_size(clone_dir)
        print(f"[*] Removing clone: {clone_dir} ({format_size(clone_size)})")
        shutil.rmtree(clone_dir, ignore_errors=True)
        print("[OK] Cleaned up\n")


if __name__ == "__main__":
    main()