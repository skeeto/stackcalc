#include <gtest/gtest.h>
#include "controller.h"

using namespace sc;

// Helper: feed a sequence of characters as key events
static void feed_chars(Controller& ctrl, const std::string& chars) {
    for (char c : chars) {
        ctrl.process_key(KeyEvent::character(c));
    }
}

// Helper: feed a special key
static void feed_special(Controller& ctrl, const std::string& name) {
    ctrl.process_key(KeyEvent::special(name));
}

TEST(ControllerTest, EnterNumber) {
    Controller ctrl;
    feed_chars(ctrl, "42");
    feed_special(ctrl, "RET");
    auto ds = ctrl.display();
    EXPECT_EQ(ds.stack_depth, 1);
    EXPECT_EQ(ds.stack_entries.back(), "42");
}

TEST(ControllerTest, AddTwoNumbers) {
    Controller ctrl;
    feed_chars(ctrl, "2");
    feed_special(ctrl, "RET");
    feed_chars(ctrl, "3");
    feed_special(ctrl, "RET");
    feed_chars(ctrl, "+");
    auto ds = ctrl.display();
    EXPECT_EQ(ds.stack_depth, 1);
    EXPECT_EQ(ds.stack_entries.back(), "5");
}

TEST(ControllerTest, SubtractNumbers) {
    Controller ctrl;
    feed_chars(ctrl, "10");
    feed_special(ctrl, "RET");
    feed_chars(ctrl, "3");
    feed_special(ctrl, "RET");
    feed_chars(ctrl, "-");
    auto ds = ctrl.display();
    EXPECT_EQ(ds.stack_depth, 1);
    EXPECT_EQ(ds.stack_entries.back(), "7");
}

TEST(ControllerTest, Multiply) {
    Controller ctrl;
    feed_chars(ctrl, "6");
    feed_special(ctrl, "RET");
    feed_chars(ctrl, "7");
    feed_special(ctrl, "RET");
    feed_chars(ctrl, "*");
    auto ds = ctrl.display();
    EXPECT_EQ(ds.stack_depth, 1);
    EXPECT_EQ(ds.stack_entries.back(), "42");
}

TEST(ControllerTest, Negate) {
    Controller ctrl;
    feed_chars(ctrl, "5");
    feed_special(ctrl, "RET");
    feed_chars(ctrl, "n");
    auto ds = ctrl.display();
    EXPECT_EQ(ds.stack_entries.back(), "-5");
}

TEST(ControllerTest, DropAndSwap) {
    Controller ctrl;
    feed_chars(ctrl, "1");
    feed_special(ctrl, "RET");
    feed_chars(ctrl, "2");
    feed_special(ctrl, "RET");
    feed_special(ctrl, "TAB"); // swap
    auto ds = ctrl.display();
    EXPECT_EQ(ds.stack_entries[0], "2");
    EXPECT_EQ(ds.stack_entries[1], "1");

    feed_special(ctrl, "DEL"); // drop top
    ds = ctrl.display();
    EXPECT_EQ(ds.stack_depth, 1);
    EXPECT_EQ(ds.stack_entries.back(), "2");
}

TEST(ControllerTest, Undo) {
    Controller ctrl;
    feed_chars(ctrl, "5");
    feed_special(ctrl, "RET");
    feed_chars(ctrl, "3");
    feed_special(ctrl, "RET");
    feed_chars(ctrl, "+");
    // stack: [8]
    feed_chars(ctrl, "U"); // undo
    auto ds = ctrl.display();
    EXPECT_EQ(ds.stack_depth, 2);
}

TEST(ControllerTest, Sin90Degrees) {
    Controller ctrl;
    // Default is degrees
    feed_chars(ctrl, "90");
    feed_special(ctrl, "RET");
    feed_chars(ctrl, "S");
    auto ds = ctrl.display();
    EXPECT_EQ(ds.stack_depth, 1);
    // sin(90) = 1.0 approximately
    // Should be a float very close to 1
}

TEST(ControllerTest, InverseSinArcsin) {
    Controller ctrl;
    feed_chars(ctrl, "1");
    feed_special(ctrl, "RET");
    feed_chars(ctrl, "I"); // set inverse
    feed_chars(ctrl, "S"); // arcsin
    auto ds = ctrl.display();
    EXPECT_EQ(ds.stack_depth, 1);
    // arcsin(1) = 90 degrees
}

TEST(ControllerTest, PiConstant) {
    Controller ctrl;
    feed_chars(ctrl, "P");
    auto ds = ctrl.display();
    EXPECT_EQ(ds.stack_depth, 1);
    // Should start with 3.14...
    EXPECT_TRUE(ds.stack_entries.back().substr(0, 4) == "3.14") << ds.stack_entries.back();
}

TEST(ControllerTest, Factorial) {
    Controller ctrl;
    feed_chars(ctrl, "5");
    feed_special(ctrl, "RET");
    feed_chars(ctrl, "!");
    auto ds = ctrl.display();
    EXPECT_EQ(ds.stack_entries.back(), "120");
}

TEST(ControllerTest, ModeLine) {
    Controller ctrl;
    auto ds = ctrl.display();
    EXPECT_EQ(ds.mode_line, "12 Deg");
}

TEST(ControllerTest, ChangeAngularMode) {
    Controller ctrl;
    feed_chars(ctrl, "mr"); // m r = radians mode
    auto ds = ctrl.display();
    EXPECT_TRUE(ds.mode_line.find("Rad") != std::string::npos) << ds.mode_line;
}

TEST(ControllerTest, ChangeDisplayFormat) {
    Controller ctrl;
    feed_chars(ctrl, "ds"); // d s = scientific
    auto ds = ctrl.display();
    EXPECT_TRUE(ds.mode_line.find("Sci") != std::string::npos);
}

TEST(ControllerTest, FloatEntry) {
    Controller ctrl;
    feed_chars(ctrl, "3.14");
    feed_special(ctrl, "RET");
    auto ds = ctrl.display();
    EXPECT_EQ(ds.stack_depth, 1);
    EXPECT_TRUE(ds.stack_entries.back().find("3.14") != std::string::npos);
}

TEST(ControllerTest, OperatorFinalizesEntry) {
    // Entering "2 RET 3 +" should give 5, not require extra RET before +
    Controller ctrl;
    feed_chars(ctrl, "2");
    feed_special(ctrl, "RET");
    feed_chars(ctrl, "3");
    feed_chars(ctrl, "+"); // should finalize 3 and then add
    auto ds = ctrl.display();
    EXPECT_EQ(ds.stack_depth, 1);
    EXPECT_EQ(ds.stack_entries.back(), "5");
}

TEST(ControllerTest, HyperbolicSinh) {
    Controller ctrl;
    feed_chars(ctrl, "1");
    feed_special(ctrl, "RET");
    feed_chars(ctrl, "H"); // set hyperbolic
    feed_chars(ctrl, "S"); // sinh
    auto ds = ctrl.display();
    EXPECT_EQ(ds.stack_depth, 1);
    // sinh(1) ~= 1.1752...
}

TEST(ControllerTest, SqrtAndSquare) {
    Controller ctrl;
    feed_chars(ctrl, "9");
    feed_special(ctrl, "RET");
    feed_chars(ctrl, "Q"); // sqrt
    auto ds = ctrl.display();
    // sqrt(9) = 3
    EXPECT_EQ(ds.stack_depth, 1);

    // Now square it: I Q
    feed_chars(ctrl, "IQ");
    ds = ctrl.display();
    EXPECT_EQ(ds.stack_depth, 1);
    // Should be 9 again
}

TEST(ControllerTest, EntryText) {
    Controller ctrl;
    feed_chars(ctrl, "12");
    auto ds = ctrl.display();
    EXPECT_EQ(ds.entry_text, "12");

    feed_special(ctrl, "RET");
    ds = ctrl.display();
    EXPECT_TRUE(ds.entry_text.empty());
}

TEST(ControllerTest, ErrorMessageOnBadOp) {
    Controller ctrl;
    // Try to add with empty stack
    feed_chars(ctrl, "+");
    auto ds = ctrl.display();
    EXPECT_FALSE(ds.message.empty());
}

TEST(ControllerTest, SetPrecision) {
    Controller ctrl;
    feed_chars(ctrl, "20");
    feed_special(ctrl, "RET");
    feed_chars(ctrl, "p");
    auto ds = ctrl.display();
    EXPECT_TRUE(ds.mode_line.find("20") != std::string::npos);
}

TEST(ControllerTest, HexRadix) {
    Controller ctrl;
    feed_chars(ctrl, "255");
    feed_special(ctrl, "RET");
    feed_chars(ctrl, "d6"); // d 6 = hex
    auto ds = ctrl.display();
    EXPECT_EQ(ds.stack_entries.back(), "16#FF");
}
