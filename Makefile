
# vim: ft=make noexpandtab

all: icoconv icoprint

%: %.c
	gcc -Wunused -Werror -o $@ $<

clean:
	rm -f icoconv icoprint

