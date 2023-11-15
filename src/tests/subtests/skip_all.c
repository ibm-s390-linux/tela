#include "tela.h"

int main(int argc, char *argv[])
{
	skip_all("%s", "reason");

	/* Should not happen. */
	fail("exit");

	return exit_status();
}
