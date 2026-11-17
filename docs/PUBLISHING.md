# Publishing without overflowing an agent conversation

Use ordinary Git from a checkout when authenticated network access is available. Git transfers STEP, PDF, image and audio bytes directly; no base64 text belongs in a prompt, tool transcript shown to the model, or handoff.

```sh
git status --short
git add .
git commit -m "Publish the reviewed PedalPartner checkpoint"
git push origin main
```

Inspect the status first and stage only intended files. Preserve any newer remote work; never force-push a recovery checkpoint over it.

## When only the GitHub connector is available

The connector accepts blob contents rather than local file paths. Bridge local reads to blob creation programmatically inside one execution runtime. Do not return those bytes to the language model, copy them into subsequent tool arguments by hand, or print the full API result. Bounded internal reads must be checked for truncation before upload.

- Return only file path, byte count, expected Git blob SHA and completion state.
- Persist each successfully verified blob SHA to a small local journal; checkpoint the manifest too. Reuse completed uploads after interruption.
- Create the tree only after all manifest entries have verified remote blob hashes. Preserve the existing remote tree and commit parent.
- Advance the branch using its expected previous SHA and verify the resulting tree against the local manifest. Stop and inspect if another writer changed the branch.
- Keep logs short. Do not rerun firmware/CAD builds for publication alone. Automatic CI remains the single Linux desktop job; firmware and CAD workflows are manual.

If the runtime cannot transfer a file without routing its contents through the model, stop that transfer method. Use an authenticated local Git push instead of restarting the chunk-encoding conversation loop.
