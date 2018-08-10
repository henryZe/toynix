#include <lib.h>

void
umain(int argc, char **argv)
{
	int i, fd, ret;

	fd = opendebug();
	if (fd < 0)
		return;

	if (argc == 2) {
		ret = write(fd, argv[1], strlen(argv[1]));
		if (ret < 0)
			goto out;

		read(fd, NULL, 0);

	} else {
		for (i = 0; i < MAXDEBUGOPT; i++) {
			ret = write(fd, debug_option[i], strlen(debug_option[i]));
			if (ret < 0)
				goto out;

			ret = read(fd, NULL, 0);
			if (ret < 0)
				goto out;
		}
	}

out:
	close(fd);
	return;
}