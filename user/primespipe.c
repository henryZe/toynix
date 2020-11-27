// Concurrent version of prime sieve of Eratosthenes.
// Invented by Doug McIlroy, inventor of Unix pipes.
// See http://swtch.com/~rsc/thread/.
// The picture halfway down the page and the text surrounding it
// explain what's going on here.
//
// Since NENV is 1024, we can print 1022 primes before running out.
// The remaining two environments are the integer generator at the bottom
// of main and user/idle.

#include <lib.h>

static unsigned int
primeproc(int fd)
{
	int i, id, p, pfd[2], wfd, ret;

	// fetch a prime from our left neighbor
top:
	ret = readn(fd, &p, 4);
	if (ret != 4)
		panic("primeproc could not read initial prime: %d, %e", ret, ret >= 0 ? 0 : ret);

	cprintf("%d\n", p);

	// fork a right neighbor to continue the chain
	i = pipe(pfd);
	if (i < 0)
		panic("pipe: %e", i);

	id = fork();
	if (id < 0)
		panic("fork: %e", id);

	if (id == 0) {
		close(fd);
		close(pfd[1]);
		fd = pfd[0];
		goto top;
	}

	close(pfd[0]);
	wfd = pfd[1];

	// filter out multiples of our prime
	for (;;) {
		ret = readn(fd, &i, 4);
		if (ret != 4)
			panic("primeproc %d readn %d %d %e", p, fd, ret, ret >= 0 ? 0 : ret);

		if (i % p) {
			ret = write(wfd, &i, 4);
			if (ret != 4)
				panic("primeproc %d write: %d %e", p, ret, ret >= 0 ? 0 : ret);
		}
	}
}

void
umain(int argc, char **argv)
{
	int i, id, p[2], ret;

	i = pipe(p);
	if (i < 0)
		panic("pipe: %e", i);

	// fork the first prime process in the chain
	id = fork();
	if (id < 0)
		panic("fork: %e", id);

	if (id == 0) {
		close(p[1]);
		primeproc(p[0]);
	}
	
	close(p[0]);

	// feed all the integers through
	for (i = 2; ; i++) {
		ret = write(p[1], &i, 4);
		if (ret != 4)
			panic("generator write: %d, %e", ret, ret >= 0 ? 0 : ret);
	}
}
