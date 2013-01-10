/*******************************************************************************
 * Copyright (c) 2012-2013 Emmanuel Goossaert
 * This file is part of FelixDB.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or any later
 * version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#include <gtest/gtest.h>

#include <string>
#include <sstream>

#include "util_test.h"
#include "status.h"
#include "logger.h"
#include "hashdb.h"
#include "data_manager.h"

namespace felixdb {


std::string concatenate(std::string const& str, int i)
{
    std::stringstream s;
    s << str << i;
    return s.str();
}



class HashDBTest : public ::testing::Test {
 public:
  HashDBTest() : num_items_(30000), step_(10000) {
  }
 protected:
  virtual void SetUp() {
    CleanUpFiles();
    db_.Open("file.db");
  }

  virtual void TearDown() {
    db_.Synchronize();
    CleanUpFiles();
  }

  felixdb::HashMap db_;
  int num_items_;
  int step_;
};

TEST_F(HashDBTest, SequentialUsage) {
  felixdb::Status s;
  std::string value_long("qwertyuiopasdfghjklzxcvbnm1234qwertyuiopasdfghjklzxcvbnm1234qwertyuiopasdfghjklzxcvbnm1234qwertyuiopasdfghjklzxcvbnm1234qwertyuiopasdfghjklzxcvbnm1234");
  std::string value_out("value_out");

  for (int i = 0; i < num_items_; i++) {
    s = db_.Put(concatenate("key", i), value_long);
    ASSERT_TRUE(s.IsOK());
  }

  for (int i = 0; i < num_items_; i++) {
    s = db_.Get(concatenate("key", i), &value_out);
    ASSERT_FALSE(s.IsNotFound());
  }

  for (int i = 0; i < num_items_; i++) {
    s = db_.Delete(concatenate("key", i));
    ASSERT_TRUE(s.IsOK());
  }

  for (int i = 0; i < num_items_; i++) {
    s = db_.Get(concatenate("key", i), &value_out);
    ASSERT_TRUE(s.IsNotFound());
  }
}




} // namespace felixdb

/*
int Factorial(int val)
{
  if (val <= 1) return 1;
  return Factorial(val-1) * val;
}

// Tests factorial of 0.
TEST(FactorialTest, Zero) {
  EXPECT_EQ(1, Factorial(0));
}

// Tests factorial of positive numbers.
TEST(FactorialTest, Positive) {
  EXPECT_EQ(1, Factorial(1));
  EXPECT_EQ(2, Factorial(2));
  EXPECT_EQ(6, Factorial(3));
  EXPECT_EQ(40320, Factorial(8));
}
*/
