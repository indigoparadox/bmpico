
# vim: ft=make noexpandtab

all: icoconv icoprint

%: src/%.c
	gcc -static -Wunused -Werror -o $@ $<

install: icoconv icoprint
	install -s icoconv /usr/bin/icoconv
	install -s icoprint /usr/bin/icoprint

deb: icoconv icoprint
	checkinstall \
		--maintainer "indigo.repo@interfinitydynamics.info" \
		--pkgversion "`git tag | grep "^[0-9.]*$$" | tail -1`" \
		--install=no

clean:
	rm -rf icoconv icoprint doc-pak bmpico*.deb

