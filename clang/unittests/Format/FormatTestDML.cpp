//===- unittest/Format/FormatTestDML.cpp ---------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "FormatTestUtils.h"
#include "clang/Format/Format.h"
#include "gtest/gtest.h"

namespace clang {
namespace format {

class FormatTestDML : public testing::Test {
protected:
  static std::string format(StringRef Code) {
    FormatStyle Style = getLLVMStyle(FormatStyle::LK_DML);
    std::vector<tooling::Range> Ranges(1, tooling::Range(0, Code.size()));
    auto Replacements = reformat(Style, Code, Ranges);
    auto Result = applyAllReplacements(Code, Replacements);
    EXPECT_TRUE(static_cast<bool>(Result));
    return *Result;
  }

  static void verifyFormat(StringRef Code) {
    EXPECT_EQ(Code, format(Code)) << "Expected code is not stable";
    EXPECT_EQ(Code, format(test::messUp(Code)));
  }

  static void verifyFormat(StringRef Expected, StringRef Input) {
    EXPECT_EQ(Expected, format(Input));
  }
};

TEST_F(FormatTestDML, DefaultStyle) {
  const auto Style = getLLVMStyle(FormatStyle::LK_DML);
  EXPECT_EQ(Style.IndentWidth, 4u);
  EXPECT_EQ(Style.TabWidth, 4u);
  EXPECT_EQ(Style.ColumnLimit, 79u);
  EXPECT_EQ(Style.PointerAlignment, FormatStyle::PAS_Right);
  EXPECT_EQ(Style.SortIncludes.Enabled, false);
}

TEST_F(FormatTestDML, FormatsDeviceObjectsAndMethods) {
  verifyFormat("device simple {\n"
               "    param size = 4;\n"
               "    import \"common.dml\";\n"
               "    bank regs {\n"
               "        register status {\n"
               "            field ready {\n"
               "                method read() -> (uint64 value) {\n"
               "                    return 1;\n"
               "                }\n"
               "            }\n"
               "        }\n"
               "    }\n"
               "    template common {}\n"
               "}");
}

TEST_F(FormatTestDML, PointerParametersInExplicitObjectExtensions) {
  verifyFormat("template foo {\n"
               "    in bank registers {\n"
               "        method my_method(transaction_t *t) {\n"
               "        }\n"
               "    }\n"
               "}");
}

TEST_F(FormatTestDML, ContinuedLogStringsUseStatementIndentation) {
  verifyFormat("template foo {\n"
               "    method my_method(transaction_t *t) {\n"
               "        log info, 1: \"A loooooooooooooong log string that "
               "says that my_method\"\n"
               "            + \" was called\";\n"
               "    }\n"
               "}");
}

TEST_F(FormatTestDML, IndentsBlocksAndAttachesBraces) {
  verifyFormat("device d {\n"
               "    param value = 1;\n"
               "    method read() {\n"
               "        return value;\n"
               "    }\n"
               "}",
               "device d\n"
               "{\n"
               "param value=1;\n"
               "method read()\n"
               "{\n"
               "return value;\n"
               "}\n"
               "}");
}

TEST_F(FormatTestDML, FormatsControlFlowAndCalls) {
  verifyFormat("device d {\n"
               "    method read() {\n"
               "        if (x == 1) {\n"
               "            foo(a, b);\n"
               "        }\n"
               "    }\n"
               "}",
               "device d{method read(){if(x==1){foo (a,b);}}}");
}

TEST_F(FormatTestDML, FormatsReservedInBeforeForeachCondition) {
  verifyFormat("device d {\n"
               "    foreach x in (values) { foo(x); }\n"
               "}",
               "device d{foreach x in(values){foo(x);}}");
  verifyFormat("template foo {\n"
               "    in each (index) {}\n"
               "}",
               "template foo{in each(index){}}");
}

TEST_F(FormatTestDML, FormatsExplicitMethodDeclarationOperators) {
  verifyFormat("template foo {\n"
               "    method bar() :{\n"
               "    }\n"
               "    method baz() :default {\n"
               "    }\n"
               "}",
               "template foo {\n"
               "    method bar() :{\n"
               "    }\n"
               "    method baz() : default {\n"
               "    }\n"
               "}");
}

TEST_F(FormatTestDML, FormatsDMLPunctuation) {
  verifyFormat("device d {\n"
               "    register r size 4 @ 0x10;\n"
               "    register bits @ [7:0];\n"
               "    bank b[i < 4] {}\n"
               "    param p is (a, b);\n"
               "    method f() -> (int r) {\n"
               "        return r;\n"
               "    }\n"
               "    param casted = cast(x, uint8 *);\n"
               "    param choice = a #? b #: c;\n"
               "    log info, 1: \"x\";\n"
               "    after 1.5 s: f();\n"
               "}",
               "device d{register r size 4@0x10;register bits@[7:0];"
               "bank b[i<4]{}param p is(a,b);"
               "method f()->(int r){return r;}"
               "param casted=cast(x,uint8*);"
               "param choice=a#?b#:c;"
               "log info,1:\"x\";after 1.5 s:f();}");
}

TEST_F(FormatTestDML, WrapsBeforeBinaryOperators) {
  verifyFormat("device d {\n"
               "    param value = alpha + beta + gamma + delta + epsilon + "
               "zeta + eta + theta\n"
               "                  + iota + kappa + lambda;\n"
               "}",
               "device d {\n"
               "param value=alpha+beta+gamma+delta+epsilon+zeta+eta+theta+"
               "iota+kappa+lambda;\n"
               "}");
}

TEST_F(FormatTestDML, PreservesHashConditionalBlocks) {
  EXPECT_EQ("device d {\n"
            "    param before = 1;\n"
            "#if (c) {  x=1; } #else { y=2; }\n"
            "    param after = 2;\n"
            "}",
            format("device d{\n"
                   "param before=1;\n"
                   "#if (c) {  x=1; } #else { y=2; }\n"
                   "param after=2;\n"
                   "}"));
  EXPECT_EQ("device d {\n"
            "#foreach x in (0..3) {  x=1; }\n"
            "    param after = 2;\n"
            "}",
            format("device d{\n"
                   "#foreach x in (0..3) {  x=1; }\n"
                   "param after=2;\n"
                   "}"));
  EXPECT_EQ("device d {\n"
            "#select (kind) {  x=1; }\n"
            "    param after = 2;\n"
            "}",
            format("device d{\n"
                   "#select (kind) {  x=1; }\n"
                   "param after=2;\n"
                   "}"));
}

TEST_F(FormatTestDML, CaseLabelsUseSwitchIndentation) {
  verifyFormat("device d {\n"
               "    method f() {\n"
               "        switch (x) {\n"
               "        case 0:\n"
               "            return;\n"
               "        default:\n"
               "            break;\n"
               "        }\n"
               "    }\n"
               "}");
}

} // namespace format
} // namespace clang
