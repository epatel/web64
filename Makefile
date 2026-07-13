# web64 — C64 projects. There is no local assembler (the web64 IDE
# builds in the browser); these targets manage the repo artifacts.

.PHONY: all proj check serve manual help

all: proj

## proj: rebuild every projects/*/<name>.web64proj from its sources
##       (deterministic output — no diff when sources are unchanged)
proj:
	python3 tools/build_web64proj.py

## proj-<name>: rebuild one project's .web64proj (e.g. make proj-raytracer-c)
proj-%:
	python3 tools/build_web64proj.py $*

## check: fail if any committed .web64proj is out of sync with its sources
check:
	python3 tools/build_web64proj.py --check

## serve: CORS file server on :8642 for the IDE browser-automation
##        workflow (inject sources / load .web64proj from the page)
serve:
	python3 tools/serve.py

## manual: refresh the local copy of the web64 IDE user manual
##         (source of the reference cards in cards/)
manual:
	curl -fsS -o web64-ide-user-manual.html https://web64.nofs.ai/docs/web64-ide-user-manual.html
	@echo "manual updated — review cards/ against it if it changed"

help:
	@grep -E '^## ' Makefile | sed 's/^## //'
