#include <buffer.h>
#include "CuTest.h"

void TestNewBuffer(CuTest* tc)
{
  struct abuf b = ABUF_INIT;
  CuAssertPtrEquals(tc, NULL, b.b);
  CuAssertIntEquals(tc, 0, b.len);
}

void TestAppendBuffer(CuTest* tc)
{
  struct abuf b = ABUF_INIT;
  abAppend(&b, "hello", 6);
  CuAssertPtrNotNull(tc, b.b);
  CuAssertIntEquals(tc, 6, b.len);
  CuAssertStrEquals(tc, "hello", b.b);
}

CuSuite* TestBufferGetSuite(void)
{
	CuSuite* suite = CuSuiteNew();

  SUITE_ADD_TEST(suite, TestNewBuffer);
	SUITE_ADD_TEST(suite, TestAppendBuffer);

	return suite;
}
