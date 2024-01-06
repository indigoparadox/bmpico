
# vim: ft=make noexpandtab

all: icoconv icoprint

%: %.c
	gcc -o $@ $<

clean:
	rm -f icoconv icoprint

