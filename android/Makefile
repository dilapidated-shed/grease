# Bootstrap interface only. Grease owns the subsequent build orchestration.
SOURCE ?= $(CURDIR)/source-worktree
HOST_OUTPUT ?= $(CURDIR)/build/host-runtime

.PHONY: source-pin dependencies bootstrap
source-pin:
	@printf 'revision=%s\n' "$$(git rev-parse HEAD:source)"

dependencies:
	sudo apt-get update
	sudo apt-get install -y ripgrep

bootstrap:
	test "$$(git rev-parse HEAD:source)" = "$$(git -C "$(SOURCE)" rev-parse HEAD)"
	test "$$(git rev-parse HEAD:source)" = "$$(cat "$(SOURCE)/_tmp/soil/commit-hash.txt")"
	"$(SOURCE)/soil/host-shim.sh" did-all-succeed cpp-tarball
	install -Dm755 "$(SOURCE)/_bin/cxx-opt-sh/oils-for-unix.stripped" "$(HOST_OUTPUT)/ysh"
	"$(HOST_OUTPUT)/ysh" --version > "$(HOST_OUTPUT)/version.txt"
	printf 'IB_GREASE=%s/ysh\n' "$(HOST_OUTPUT)" >> "$(GITHUB_ENV)"
