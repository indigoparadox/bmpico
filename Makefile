
# vim: ft=make noexpandtab

all: icoconv icoprint

%: src/%.c
	gcc -static -Wunused -Werror -o $@ $<

install:
	install -s icoconv /usr/bin/icoconv
	install -s icoprint /usr/bin/icoprint

deb:
	checkinstall \
		--maintainer "indigo.repo@interfinitydynamics.info" \
		--pkgversion "`git tag | grep "^[0-9.]*$$" | tail -1`"

clean:
	rm -rf icoconv icoprint doc-pak bmpico*.deb

