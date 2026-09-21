#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
output_dir="${project_dir}/dist/windows"
mkdir -p "${output_dir}"

docker build \
  --progress plain \
  --file "${project_dir}/Dockerfile.windows" \
  --target artifact \
  --output "type=local,dest=${output_dir}" \
  "${project_dir}"

printf 'Windows package: %s\n' "${output_dir}"
