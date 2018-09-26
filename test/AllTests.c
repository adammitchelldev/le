#include <stdio.h>

#include "CuTest.h"

CuSuite* TestBufferGetSuite();

int RunAllTests(void)
{
	CuString *output = CuStringNew();
	CuSuite* suite = CuSuiteNew();

	CuSuiteAddSuite(suite, TestBufferGetSuite());

	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);
	printf("%s\n", output->buffer);

	int failCount = 0;
	failCount += suite->failCount;

	return failCount;
}

int main(void)
{
	return RunAllTests() > 0 ? 1 : 0;
}
