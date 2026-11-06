#!/bin/bash
set -e

if [ $# -ne 1 ]; then
    echo "Usage: $0 <input.v>"
    exit 1
fi

make plugin
yosys -m ./myverilog_frontend.so -p "read_myverilog $1; opt_muxtree; opt_expr; opt; stat"
