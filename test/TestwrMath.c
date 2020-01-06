#include "unity.h"
#include "wrMath.h"

// macros
//void test__Abs(void){
//    TEST_ASSERT_EQUAL_FLOAT(1.0, _Abs( 1.0));
//    TEST_ASSERT_EQUAL_FLOAT(1.0, _Abs(-1.0));
//    TEST_ASSERT_EQUAL_FLOAT(99999.0, _Abs( 99999.0));
//    TEST_ASSERT_EQUAL_FLOAT(99999.0, _Abs(-99999.0));
//
//    TEST_ASSERT_EQUAL_INT(1, _Abs( 1));
//    TEST_ASSERT_EQUAL_INT(1, _Abs(-1));
//}
//
//void test__Max(void){
//    TEST_ASSERT_EQUAL_FLOAT(1.0, _Max( 0.5, 1.0));
//    TEST_ASSERT_EQUAL_FLOAT(1.0, _Max( 1.0, 0.0));
//    TEST_ASSERT_EQUAL_FLOAT(1.0, _Max( 1.0, -1.0));
//    TEST_ASSERT_EQUAL_FLOAT(-1.0, _Max(-99999.0, -1.0));
//
//    TEST_ASSERT_EQUAL_INT( 1, _Max( 0,  1));
//    TEST_ASSERT_EQUAL_INT( 1, _Max( 1,  0));
//    TEST_ASSERT_EQUAL_INT( 1, _Max( 1, -1));
//    TEST_ASSERT_EQUAL_INT(-1, _Max(-99999, -1));
//}
//
//void test__Min(void){
//    TEST_ASSERT_EQUAL_FLOAT(0.5, _Min( 0.5, 1.0));
//    TEST_ASSERT_EQUAL_FLOAT(0.0, _Min( 1.0, 0.0));
//    TEST_ASSERT_EQUAL_FLOAT(-1.0, _Min( 1.0, -1.0));
//    TEST_ASSERT_EQUAL_FLOAT(-99999.0, _Min(-99999.0, -1.0));
//
//    TEST_ASSERT_EQUAL_INT( 0, _Min( 0,  1));
//    TEST_ASSERT_EQUAL_INT( 0, _Min( 1,  0));
//    TEST_ASSERT_EQUAL_INT(-1, _Min( 1, -1));
//    TEST_ASSERT_EQUAL_INT(-99999, _Min(-99999, -1));
//}
//
//void test__Swap(void){
//    int a = 1, b = -1;
//    _Swap(a,b)
//    TEST_ASSERT_EQUAL_INT(1, b);
//    TEST_ASSERT_EQUAL_INT(-1, a);
//}
//
//void test__Lim01(void){
//    TEST_ASSERT_EQUAL_FLOAT(0.5,  _Lim01( 0.5 ));
//    TEST_ASSERT_EQUAL_FLOAT(0.0,  _Lim01( -1.0));
//    TEST_ASSERT_EQUAL_FLOAT(1.0,  _Lim01( 9.0));
//
//    TEST_ASSERT_EQUAL_INT(0,  _Lim01( 0 ));
//    TEST_ASSERT_EQUAL_INT(0,  _Lim01( -1));
//    TEST_ASSERT_EQUAL_INT(1,  _Lim01( 1));
//    TEST_ASSERT_EQUAL_INT(1,  _Lim01( 10));
//}
//
//void test_max_f(void){
//    TEST_ASSERT_EQUAL_FLOAT(1.0, max_f(0.0, 1.0));
//    TEST_ASSERT_EQUAL_FLOAT(1.0, max_f(1.0, 0.0));
//    TEST_ASSERT_EQUAL_FLOAT(99.0, max_f(-100.0, 99.0));
//    TEST_ASSERT_EQUAL_FLOAT(0.0, max_f(-100.0, 0.0));
//    TEST_ASSERT_EQUAL_FLOAT(-8.0, max_f(-100.0, -8.0));
//}

void test_add_vf_f(void){
    float src[] = {-1.0, 0.0, 1.0};
    float expect[] = {0.0, 1.0, 2.0};
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(expect, add_vf_f(src, 1.0, 3), 3);
}

int main(void)
{
    UNITY_BEGIN();

    //// macros
    //RUN_TEST(test__Abs);
    //RUN_TEST(test__Max);
    //RUN_TEST(test__Min);
    //RUN_TEST(test__Swap);
    //RUN_TEST(test__Lim01);

    //// singleton
    //RUN_TEST(test_max_f);

    // vectors
    RUN_TEST(test_add_vf_f);

    return UNITY_END();
}
