#include "tela.h"

int main(int argc, char *argv[])
{
	yaml_file("./yaml_text", 0, NULL, false);
	pass("text");

	yaml_file("./yaml_text2", 2, "multiline", false);
	pass("multiline");

	yaml_file("./yaml_text3", 0, "hex", true);
	pass("hex");

	yaml_file("/dev/null", 0, "empty", true);
	pass("empty");

	/* Report final testcase to catch residual YAML output. */
	pass("residual");

	return 0;
}
