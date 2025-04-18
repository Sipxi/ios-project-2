#ifndef LOGGER_H
#define LOGGER_H

void init_log(const char *filename);
void log_test_result(const char *func_name, const char *assertion, int expected, int actual, int passed);
void close_log();

#endif // TESTS_ARGS_H