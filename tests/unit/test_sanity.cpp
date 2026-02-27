#include "PrimeStage/PrimeStage.h"

#include "third_party/doctest.h"

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

TEST_CASE("PrimeStage exposes version") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path cmakePath = repoRoot / "CMakeLists.txt";
  REQUIRE(std::filesystem::exists(cmakePath));

  std::ifstream cmakeInput(cmakePath);
  REQUIRE(cmakeInput.good());
  std::string cmake((std::istreambuf_iterator<char>(cmakeInput)),
                    std::istreambuf_iterator<char>());
  REQUIRE(!cmake.empty());

  std::string marker = "project(PrimeStage VERSION ";
  size_t markerPos = cmake.find(marker);
  REQUIRE(markerPos != std::string::npos);
  size_t versionStart = markerPos + marker.size();
  size_t versionEnd = cmake.find_first_of(" \t\r\n)", versionStart);
  REQUIRE(versionEnd != std::string::npos);
  std::string expectedVersion = cmake.substr(versionStart, versionEnd - versionStart);

  unsigned expectedMajor = 0u;
  unsigned expectedMinor = 0u;
  unsigned expectedPatch = 0u;
  REQUIRE(std::sscanf(expectedVersion.c_str(),
                      "%u.%u.%u",
                      &expectedMajor,
                      &expectedMinor,
                      &expectedPatch) == 3);

  PrimeStage::Version version = PrimeStage::getVersion();
  CHECK(version.major == static_cast<uint32_t>(expectedMajor));
  CHECK(version.minor == static_cast<uint32_t>(expectedMinor));
  CHECK(version.patch == static_cast<uint32_t>(expectedPatch));
  CHECK(std::string(PrimeStage::getVersionString()) == expectedVersion);
}
