#pragma once

#define TEST_WINDOW_WIDTH 800
#define TEST_WINDOW_HEIGHT 600

void
setup(void);

void
teardown(void);

void
test_render_frame(const char *testcase_name);