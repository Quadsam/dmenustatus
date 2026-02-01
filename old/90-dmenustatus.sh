#!/usr/bin/env sh

if command -v dmenustatus >/dev/null 2>&1; then
	exec dmenustatus -q &
fi
