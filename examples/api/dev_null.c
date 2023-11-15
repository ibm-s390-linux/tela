/*
 * Attempt to open /dev/null.
 */

#include <err.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	FILE *fd;

	fd = fopen("/dev/null", "r");
	if (!fd)
		err(1, "Could not open /dev/null");
	fclose(fd);

	return 0;
}
