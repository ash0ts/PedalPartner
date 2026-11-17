# Publish a focused PR without exhausting the conversation

Use ordinary authenticated Git whenever available. Start from current `main`,
preserve other changes, work on a review branch, stage only intended files and
open a PR. Do not push directly to `main` or force-push a recovery snapshot.

```sh
git switch -c docs/cad-hero-and-handoff
git status --short
# Stage the exact reviewed paths, commit, then:
git push -u origin HEAD
```

## Keep publication independent of builds

- Run only checks needed by the change, once. Reuse evidence for unchanged CAD
  and firmware. Automatic CI remains one Linux `make test` job; firmware/CAD
  workflows stay manual. Packaging or publishing does not require target builds.
- Keep logs under ignored `build/` and inspect short error excerpts. A target
  download failure, a compiler failure and a conversation-length limit are
  different outcomes; record the observed one precisely.
- Publish the changed scripts/docs and small finished images. Reuse unchanged
  STEP/PDF blobs from the existing Git tree; never re-upload the whole repository
  to update a README or render. Do not commit ZIPs, meshes or Blender intermediates.

## If only a connector is available

Use one bounded, complete-file transfer inside the execution runtime. The model
must see only path, size, expected blob hash and status. For this workflow, cap
each file at **256 KiB**; normal Git handles larger files. This is an operational
limit for safe transfers, not a GitHub file-size limit.

1. Read the current branch commit/tree. Compare local Git blob hashes and form a
   changed-file manifest. Do not discard newer remote work.
2. Read each small file completely inside one runtime call. Check completion,
   expected byte/base64 length and absence of truncation before passing the
   content directly to blob creation. Never print the payload or full result.
3. Verify the returned Git blob SHA against the local SHA and persist the receipt
   after each successful upload. Reuse a matching receipt if interrupted.
4. Build a tree on the captured base tree using only changed entries; create a
   commit with the captured base commit as parent, then a new review branch.
   If updating an existing branch, use its expected previous SHA as a lease.
5. Verify the remote tree against the intended file hashes, open the PR and check
   its diff. Report actual CI status; do not start optional workflows to finish.

**Stop that method immediately** if complete-file reads are unavailable or
truncated. Do not replace it with base64 chunks, shared-memory segments, bridge
reads, repeated range extraction or manually copied payloads. The earlier failed
conversations ended at the chat-length limit during such transfers. A retry loop
does not fix this. Preserve the finished patch/archive for local Git upload.

The lean mentor ZIP is produced by `make pack-mentor`; see [MENTOR_PACK.md](MENTOR_PACK.md).
It is a shareable review packet, not a repository replacement or publication channel.
