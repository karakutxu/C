#ifndef TEST_COMPONENT_PREPARE_FRAME_H
#define TEST_COMPONENT_PREPARE_FRAME_H

#include <stdint.h>

void test_prepare_frame_success(void);
void test_prepare_frame_makeframe_failure(void);
void test_prepare_frame_transaction_id_preserved(void);
void test_prepare_frame_schedule_copied(void);
void test_prepare_frame_payload_zero_initialised(void);
void test_prepare_frame_statistics_incremented(void);
void test_prepare_frame_multiple_requests(void);
void test_prepare_frame_correct_frame_length(void);

#endif /* TEST_COMPONENT_PREPARE_FRAME_H */