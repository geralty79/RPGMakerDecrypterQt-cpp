# RPG Maker Decrypter (C++/Qt 6) — Roadmap & Plans

Status: feature parity with the C# CLI reached, all core self-tests pass.

---

## 1. Plan: parallelize decryption (multi-threaded I/O)

Currently all work runs on a single worker `QThread`. The UI stays responsive,
but files are processed strictly one at a time. Since the crypto is a trivial
per-byte XOR, the workload is I/O-bound; parallelizing file writes helps on SSDs
and for deployments with thousands of small assets.

### 1.1 MV/MZ directory decryption (`DirectoryFilesDecrypter::decrypt`)

Highest value: deployments contain many independent files.

1. Collect the encrypted file list (this pass stays single-threaded — cheap and
   makes `total` available upfront for the progress bar).
2. Split the list into chunks and process them concurrently. Options:
   - `QtConcurrent::map` with a bounded `QThreadPool` (simplest), or
   - hand-rolled worker pool if we want tighter control over queueing.
   Suggested bound: `QThread::idealThreadCount()`, capped at 8.
3. Progress: replace the loop index with an `std::atomic<int>` counter of
   finished files; emit a throttled progress signal (e.g. every N files or
   every 100 ms, whichever comes first — coalescing avoids flooding the GUI
   with queued signal events).
4. Cancellation: a `QAtomicInteger`/`std::atomic<bool>` stop flag; each worker
   checks it before starting the next file, a cancelled reduction sets an
   "aborted" result flag that the caller turns into `OperationCancelledException`.
5. Error aggregation: collect per-file failures (path + reason) into a
   thread-safe container; report them as warnings in the GUI log instead of
   aborting — matching today's skip-empty behavior. Hard errors that one file
   shouldn't cancel the whole batch.

### 1.2 RGSSAD archive extraction (`RGSSADArchive::extractAllFiles`)

Each `ArchiveEntry` already carries its own per-file `key`, so entries are
independent — embarrassingly parallel once the directory is parsed.

1. Decouple entry parsing (fast, sequential, stays on the worker thread) from
   extraction.
2. Extraction of entry N = seek at `offset`, read `size` bytes, XOR-decrypt,
   write out. Steps that must stay serialized:
   - creating nested output directories (or use `mkpath()`, which is
     race-safe enough via atomic mkdir, but double-check per platform),
   - the skip-if-existing / overwrite decision per output path (currently
     implicit in `extractFile`). Do the decisions on a single thread, then
     hand the file list to the pool.
   - Name collisions: two entries mapping to the same output path must have a
     deterministic winner (first-in-archive wins today); enforce that before dispatch.
3. Reading from one shared `QFile` from multiple threads is not safe. Options:
   - give each worker its own open `QFile` handle (simplest, minor FD cost), or
   - `pread`-style positioned reads via `QFile::read` with per-worker handles.
4. Progress/cancel/progress-coalescing as in 1.1.

### 1.3 Project reconstruction (`copyDirectory`)

Lower priority (bounded copy volume). Simplest safe approach: build the flat
list of (source, destination) file pairs first (recursive walk on one thread),
then copy in parallel with the MV/MZ machinery. Directory creation happens
before dispatch.

### 1.4 GUI/threading contract (unchanged)

- All external-facing API remains: worker emits `message`/`progress`/`finished`;
  `Qt::QueuedConnection` to the GUI thread (already in place).
- `DecryptWorker::cancel()` stays the single entry point; it must work for all pools.
- Add a settings spin box "Threads" (1 = old sequential behavior) so users on
  spinning disks can opt out. Default: `idealThreadCount()`.

### 1.5 Testing additions

- Concurrency unit tests: 10k tiny-file archive with `1` vs `N` threads yields
  identical output trees (byte-compare).
- Race test: entries that resolve to the same output file name.
- Cancellation test: cancel after the first observed progress event.

---

## 2. Improvement backlog (suggested, unordered)

### UX
- [ ] Drag & drop: drop an archive onto the window to fill the input field;
      drop a folder for MV/MZ mode.
- [ ] Application icon: convert `../Resources/icon_256x256.xcf` to PNG set,
      add via `qt_add_resources`; set per-platform app icons.
- [ ] Recent-inputs list (QSettings) remembering last archives/projects.
- [ ] Show estimated remaining time + throughput (MB/s) once progress has history.
- [ ] Per-file error list panel: failures collected during parallel runs can be
      reviewed and retried individually.
- [ ] Indeterminate-vs-determinate progress: MV/MZ currently shows "unknown
      total" until the scan finishes; show a spinner phase then switch.
- [ ] Dark-mode friendly stylesheet (or `QPalette` review) and HiDPI-check.

### Robustness
- [ ] Log file parity feature: mirror GUI log to a timestamped file next to the
      executable (like the C# `ExceptionLogger`) to ease bug reports.
- [ ] Streaming extraction: huge archive entries are currently read fully into
      RAM; stream decrypt+write in ~1 MB chunks instead (decryptFileData loops
      make chunking easy since only the current key state matters).
- [ ] Sanity-limit entry metadata (name length/size) to avoid pathological
      allocations from corrupted archives (already partially done in readers).
- [ ] Verify behavior for MZ `.ogg_` duplicates and spaces-in-name edge cases.

### Packaging & CI
- [ ] CMake install rules + CPack: Windows installer/zip, macOS dmg
      (`MACOSX_BUNDLE` is already set), Linux AppImage.
- [ ] GitHub Actions workflow: ubuntu/windows/macos matrix that runs ctest.
- [ ] `qt_add_translations` + lupdate pass; translations already use `tr()`,
      so adding .ts files is cheap.

### Code cleanup
- [ ] Remove the now-unneeded `DecryptWorker::cancellationRequested()` helper
      if nothing uses it after the parallelization rework.
- [ ] Split `DirectoryFilesDecrypter::decrypt` into (scan, process-one-file) so
      the parallel variant reuses single-file logic (also aids unit testing).
- [ ] Consider `QtConcurrent` only where the extra dependency pull is already
      satisfied (`find_package(Qt6 COMPONENTS Concurrent)`).

### Optional features
- [ ] Re-add a thin C++ CLI binary that reuses the core library (parity remnant
      for scripting/batch usage).
- [ ] Alternative decryption key entry: allow pasting the 32-hex key manually
      when `System.json` was renamed/missing.
- [ ] Folder mode: pick a folder containing many RGSSAD archives and decrypt
      them all in one run.

---

## 3. Directory hygiene

Done: the original C# projects were moved under `legacy/` for reference and
still build with the .NET 8 SDK. The fixtures in
`legacy/RPGMakerDecrypter.Tests/EncryptedArchives` (real archives produced by
the RPG Maker editors) are valuable for validating the C++ core against real
encrypted data — see the backlog section.

Safe to remove now:

- `build/` (repo root) — generated CMake output, fully regenerable.
  Prefer out-of-tree builds (`cmake -S RPGMakerDecrypter.Cpp -B build-cpp`).
- Not in the repo but regenerable: `/tmp/opencode/rpgmaker-build` scratch build.

Can be removed once the C++ core has been validated against the real C# test
fixtures:

- `legacy/` — the complete C# reference implementation.

Keep:

- `RPGMakerDecrypter.Cpp/` — the GUI application
- `Resources/` — icon sources (drag-and-drop/icon work will need it)
- `README.MD`, `CHANGELOG.MD`, `LICENSE.txt`, `.gitignore`, `.gitattributes`

Do NOT touch:

- `New Folder/` — the user's own decrypted MV project output (6.1 GB).
