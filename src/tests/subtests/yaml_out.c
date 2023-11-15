#include "tela.h"

int main(int argc, char *argv[])
{
	yaml("my: %s", "data");
	yaml("my2: %s", "data2");
	pass("pass");

	/* Report two testcases to catch residual YAML output. */
	pass("pass2");

	return 0;
}
