#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "tela.h"

typedef struct {
	char *name;
	char *result;
} testresult_t;

testresult_t testresult = { NULL, NULL };

void callback(const char *name, const char *result, void *data)
{
	((testresult_t *) data)->name = strdup(name);
	((testresult_t *) data)->result = strdup(result);
}

int main(int argc, char *argv[])
{
	atresult(&callback, &testresult);
	pass("test");
	if (strcmp(testresult.name, "test"))
		return 1;
	if (strcmp(testresult.result, "pass"))
		return 1;
	free(testresult.name);
	free(testresult.result);
	return 0;
}
