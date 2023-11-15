#include <stddef.h>
#include "tela.h"

int main(int argc, char *argv[])
{
	pass("pass");
	pass(NULL);
	fail("fail");
	fail(NULL);
	skip("skip1", "reason");
	skip("skip2", NULL);
	skip(NULL, "reason");
	skip(NULL, NULL);
	todo("todo1", "reason");
	todo("todo2", NULL);
	todo(NULL, "reason");
	todo(NULL, NULL);
	ok(0, "ok0");
	ok(1, "ok1");
	ok(-1, "ok-1");

	return exit_status();
}
