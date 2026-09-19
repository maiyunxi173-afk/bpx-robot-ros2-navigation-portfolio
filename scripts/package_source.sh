#!/usr/bin/env bash
set -euo pipefail

repo_root="$(git rev-parse --show-toplevel)"
commit="$(git -C "$repo_root" rev-parse --short=12 HEAD)"
output_dir="${1:-$repo_root/dist}"
archive="$output_dir/bpx-robot-ros2-navigation-portfolio_${commit}.zip"

mkdir -p "$output_dir"
git -C "$repo_root" archive \
  --format=zip \
  --prefix=bpx-robot-ros2-navigation-portfolio/ \
  --output="$archive" \
  HEAD

unzip -t "$archive" >/dev/null
sha256sum "$archive" > "$archive.sha256"

echo "Created: $archive"
echo "Checksum: $archive.sha256"
