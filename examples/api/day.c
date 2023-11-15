/*
 * Check if today is a workday, weekend, sunday or holiday.
 */

#include <time.h>
#include "tela.h"

int main(int argc, char *argv[])
{
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	bool is_weekend = (tm->tm_wday == 0 || tm->tm_wday == 6);

	diag("Got tm_wday=%d", tm->tm_wday);

	/* Testcase workday */
	ok(!is_weekend, "workday");

	/* Testcase weekend */
	if (is_weekend)
		pass("weekend");
	else
		fail("weekend");

	/* Testcase sunday */
	if (!is_weekend)
		skip("sunday", "Sunday is on the weekend");
	else
		ok(tm->tm_wday == 0, "sunday");

	/* Testcase holiday */
	todo("holiday", "Need table of holidays");

	return exit_status();
}
