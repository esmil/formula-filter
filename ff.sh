#!/bin/sh
cd `dirname $0`
set -e

echo -n "$1" | cat ff-header.tex - ff-footer.tex > /tmp/formula-filter.tex
latex --interact=nonstopmode --output-directory /tmp /tmp/formula-filter.tex > /dev/null
dvipng -bg transparent -Q 10 -T tight --follow -o "$2" /tmp/formula-filter.dvi > /dev/null
