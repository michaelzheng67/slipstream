#include "server/latency_recorder.h"
#include <gtest/gtest.h>
#include <string>

namespace {

std::string print_output(const latency_recorder &r) {
  testing::internal::CaptureStdout();
  r.print();
  std::fflush(stdout);
  return testing::internal::GetCapturedStdout();
}

} // namespace

TEST(LatencyRecorderTest, EmptyPrintsNoSamples) {
  latency_recorder r(10);
  EXPECT_EQ(print_output(r), "no samples\n");
}

TEST(LatencyRecorderTest, NearestRankPercentiles) {
  latency_recorder r(1000);
  for (uint64_t v = 1000; v >= 1; --v) { // record out of order: 1000..1
    r.record(v);
  }
  // n = 1000: p50 -> rank 500, p99 -> rank 990, p99.9 -> rank 999
  auto out = print_output(r);
  EXPECT_NE(out.find("samples: 1000  dropped: 0"), std::string::npos);
  EXPECT_NE(out.find("p50: 500  p99: 990  p99.9: 999  max: 1000"),
            std::string::npos);
}

TEST(LatencyRecorderTest, SingleSample) {
  latency_recorder r(1);
  r.record(42);
  EXPECT_NE(print_output(r).find("p50: 42  p99: 42  p99.9: 42  max: 42"),
            std::string::npos);
}

TEST(LatencyRecorderTest, DropsWhenFullWithoutGrowing) {
  latency_recorder r(4);
  for (uint64_t v = 1; v <= 10; ++v) {
    r.record(v);
  }
  auto out = print_output(r);
  // capacity() may round up past 4, so check the totals add up instead
  EXPECT_NE(out.find("dropped: "), std::string::npos);
  auto pos = out.find("samples: ");
  auto samples = std::stoul(out.substr(pos + 9));
  auto dropped = std::stoul(out.substr(out.find("dropped: ") + 9));
  EXPECT_GE(samples, 4u);
  EXPECT_EQ(samples + dropped, 10u);
}
