#!/bin/bash

test -n "$srcdir" || srcdir=`dirname "$0"`
test -n "$srcdir" || srcdir=.

set -x -e

cd "$srcdir"

curbranch=`git symbolic-ref --short HEAD`
currev=`git describe --always --long --abbrev=40`

git checkout gh-pages
set +e
(
	set -e
	cd doc/html
	GIT_WORK_TREE=. git add .
	git commit -m "Update docs from rev $currev." -e
)
git checkout -f "$curbranch"
