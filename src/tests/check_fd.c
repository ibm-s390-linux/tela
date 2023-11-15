/* Ensure that standard file descriptors are available, and no other
 * file descriptors are leaked to a compiled test executable. */

#include <dirent.h>
#include <err.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define	PROC	"/proc/self/fd"

int main(void)
{
	char filename[PATH_MAX], link[PATH_MAX];
	DIR *dir;
	struct dirent *de;
	int num_open = 0;
	struct stat buf;

	if (fstat(STDIN_FILENO, &buf) != 0)
		err(1, "Error: Missing stdin file descriptor");
	if (fstat(STDOUT_FILENO, &buf) != 0)
		err(1, "Error: Missing stdout file descriptor");
	if (fstat(STDERR_FILENO, &buf) != 0)
		err(1, "Error: Missing stderr file descriptor");

	dir = opendir(PROC);
	if (!dir)
		err(1, "Error: Could not open %s", PROC);

	while ((de = readdir(dir))) {
		if (strcmp(de->d_name, ".") == 0 ||
		    strcmp(de->d_name, "..") == 0)
			continue;

		/* Skip fd from opendir(). */
		if (atoi(de->d_name) == dirfd(dir))
			continue;

		snprintf(filename, PATH_MAX, "%s/%s", PROC, de->d_name);
		memset(link, 0, PATH_MAX);
		if (readlink(filename, link, PATH_MAX)  < 0)
			err(1, "Error: Could not read link %s", filename);

		printf("fd %s => %s\n", de->d_name, link);
		num_open++;
	}

	closedir(dir);

	if (num_open > 3)
		errx(1, "Error: Found leaked file descriptors");

	printf("Success\n");

	return 0;
}
