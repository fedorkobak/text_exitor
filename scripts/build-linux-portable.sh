#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
output_dir="${project_dir}/dist/linux-portable"
mkdir -p "${output_dir}"

docker build \
  --progress plain \
  --file "${project_dir}/Dockerfile.linux-appimage" \
  --target artifact \
  --output "type=local,dest=${output_dir}" \
  "${project_dir}"

chmod +x "${output_dir}/TextEditor-x86_64.AppImage"
printf 'Portable Linux executable: %s\n' \
  "${output_dir}/linux-x86_64.AppImage"
