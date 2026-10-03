#!/usr/bin/env bash

set -e

cd "$(dirname "$0")"
npx --yes @vscode/vsce package --out trench-highlighting.vsix
