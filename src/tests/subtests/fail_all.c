#include "tela.h"

int main(int argc, char *argv[])
{
	fail_all();

	/* Should not happen. */
	fail("exit");

	return exit_status();
}
