#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${script_dir}"

output_dir="${1:-${script_dir}/build/generated}"
mkdir -p "${output_dir}"

# SWIG 4.3 reports 312 for the anonymous unions handled by the extension
# properties in pack_message.i and 401 for intentionally opaque templates.
SWIG_FLAGS=(-python -c++ -w312,401 -I../../Utils -I.. -I.)

swig "${SWIG_FLAGS[@]}" \
    -outdir "${output_dir}" \
    -o "${output_dir}/pack_message_wrap.cxx" pack_message.i
swig "${SWIG_FLAGS[@]}" \
    -outdir "${output_dir}" \
    -o "${output_dir}/shm_server_wrap.cxx" shm_server.i
swig "${SWIG_FLAGS[@]}" \
    -outdir "${output_dir}" \
    -o "${output_dir}/shm_connection_wrap.cxx" shm_connection.i
swig "${SWIG_FLAGS[@]}" \
    -outdir "${output_dir}" \
    -o "${output_dir}/spscqueue_wrap.cxx" spscqueue.i
