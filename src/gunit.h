#ifndef GTEST_H
#define GTEST_H

#include <iostream>

namespace testing
{
class Test;
struct TestRegRecord {
  const char* name;
  TestRegRecord* next;
  Test* (*fn)();
  TestRegRecord(const char* name, Test* (*fn)());
};

class Test {
public:
  virtual void SetUp() {}
  virtual void Run() {}
  virtual void TearDown() {}
};

}

int RUN_ALL_TESTS();

#define TEST_STR_(S) #S
#define TEST_STR(S) TEST_STR_(S)
#define TEST_CLASS_NAME(CASE, TST) CASE##_##TST##_Test

#define TEST_(CASE, TST, BASE)                                        \
  class TEST_CLASS_NAME(CASE, TST) : public BASE {                    \
   public:                                                            \
    static Test* Create_() { return new TEST_CLASS_NAME(CASE, TST); } \
    void Run() override;                                              \
  };                                                                  \
  ::testing::TestRegRecord CASE##_##TST##reg(                         \
      TEST_STR(TEST_CLASS_NAME(CASE, TST)),                           \
      TEST_CLASS_NAME(CASE, TST)::Create_);                           \
  void TEST_CLASS_NAME(CASE, TST)::Run()

#define TEST(CASE, TST) TEST_(CASE, TST, ::testing::Test)
#define TEST_F(CASE, TST) TEST_(CASE, TST, CASE)

#define ASSERT_EQ(A, B)                                                     \
  {                                                                         \
    const auto& a_ = A;                                                     \
    const auto& b_ = B;                                                     \
    if (a_ != b_) {                                                         \
      std::cout << "[E]" << a_ << " != " << b_ << " at " << __FILE__ << ":" \
                << __LINE__ << std::endl;                                   \
      throw 1;                                                              \
    }                                                                       \
  }
#define ASSERT_NE(A, B)                                                     \
  {                                                                         \
    const auto& a_ = A;                                                     \
    const auto& b_ = B;                                                     \
    if (a_ == b_) {                                                         \
      std::cout << "[E]" << a_ << " == " << b_ << " at " << __FILE__ << ":" \
                << __LINE__ << std::endl;                                   \
      throw 1;                                                              \
    }                                                                       \
  }
#define ASSERT_LT(A, B)                                                     \
  {                                                                         \
    const auto& a_ = A;                                                     \
    const auto& b_ = B;                                                     \
    if (a_ >= b_) {                                                         \
      std::cout << "[E]" << a_ << " >= " << b_ << " at " << __FILE__ << ":" \
                << __LINE__ << std::endl;                                   \
      throw 1;                                                              \
    }                                                                       \
  }
#define ASSERT_LE(A, B)                                                     \
  {                                                                         \
    const auto& a_ = A;                                                     \
    const auto& b_ = B;                                                     \
    if (a_ > b_) {                                                         \
      std::cout << "[E]" << a_ << " > " << b_ << " at " << __FILE__ << ":" \
                << __LINE__ << std::endl;                                   \
      throw 1;                                                              \
    }                                                                       \
  }

struct dummy_stream
{
  template<typename T>
  dummy_stream& operator<< (const T&) { return *this; }
};

#define ASSERT_TRUE(A)                                                 \
  ([&]() {                                                             \
    const auto& a_ = A;                                                \
    if (!a_) {                                                         \
      std::cout << "[E]" << a_ << " false"                             \
                << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
      throw 1;                                                         \
    }                                                                  \
    return dummy_stream();                                             \
  })()

#define ASSERT_FALSE(A) ASSERT_TRUE(!(A))


#define EXPECT_EQ(A, B) ASSERT_EQ(A, B)
#define EXPECT_NE(A, B) ASSERT_NE(A, B)
#define EXPECT_LT(A, B) ASSERT_LT(A, B)
#define EXPECT_LE(A, B) ASSERT_LE(A, B)
#define EXPECT_TRUE(A) ASSERT_TRUE(A)
#define EXPECT_FALSE(A) ASSERT_FALSE(A)

#endif  // GTEST
