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

// --- Reset to pristine state ---

TEST(ControllerTest, ResetWipesEverything) {
    Controller ctrl;
    // Build up some state across all the things reset should clear.
    feed_chars(ctrl, "5"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "7"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "+");           // stack: [12], trail entries
    feed_chars(ctrl, "ssa");         // a := 12
    feed_chars(ctrl, "t3");          // q3 := 12
    feed_chars(ctrl, "d6");          // radix 16
    feed_chars(ctrl, "mr");          // angular mode radians
    feed_chars(ctrl, "I");           // inverse flag set
    feed_chars(ctrl, "12");          // input "12" active

    // Sanity: state is dirty.
    EXPECT_GT(ctrl.display().stack_depth, 0);
    EXPECT_GT(ctrl.display().trail_entries.size(), 0u);
    EXPECT_EQ(ctrl.stack().state().display_radix, 16);
    EXPECT_EQ(ctrl.stack().state().angular_mode, AngularMode::Radians);
    EXPECT_TRUE(ctrl.stack().state().inverse_flag);
    EXPECT_TRUE(ctrl.input().active());
    EXPECT_TRUE(ctrl.variables().exists("a"));

    ctrl.reset();

    EXPECT_EQ(ctrl.display().stack_depth, 0);
    EXPECT_EQ(ctrl.display().trail_entries.size(), 0u);
    EXPECT_EQ(ctrl.stack().state().display_radix, 10);
    EXPECT_EQ(ctrl.stack().state().precision, 12);
    EXPECT_EQ(ctrl.stack().state().angular_mode, AngularMode::Degrees);
    EXPECT_EQ(ctrl.stack().state().group_digits, 0);
    EXPECT_FALSE(ctrl.stack().state().inverse_flag);
    EXPECT_FALSE(ctrl.input().active());
    EXPECT_FALSE(ctrl.variables().exists("a"));
}

TEST(ControllerTest, ResetClearsUndoHistory) {
    Controller ctrl;
    feed_chars(ctrl, "5"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "3"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "+");
    EXPECT_EQ(ctrl.display().stack_entries.back(), "8");
    ctrl.reset();
    EXPECT_EQ(ctrl.display().stack_depth, 0);
    // Undo after reset must be a no-op — there's no history to revert to.
    feed_chars(ctrl, "U");
    EXPECT_EQ(ctrl.display().stack_depth, 0);
}

// --- Auto-grouping default per radix (3 for non-hex, 4 for hex) ---

TEST(ControllerTest, GroupingDefaultIsThreeInDecimal) {
    Controller ctrl;
    feed_chars(ctrl, "dg");                                  // toggle on
    EXPECT_EQ(ctrl.stack().state().group_digits, 3);
}

TEST(ControllerTest, GroupingDefaultBecomesFourInHex) {
    Controller ctrl;
    feed_chars(ctrl, "d6");                                  // radix=16, group still 0
    EXPECT_EQ(ctrl.stack().state().group_digits, 0);
    feed_chars(ctrl, "dg");                                  // toggle on -> 4
    EXPECT_EQ(ctrl.stack().state().group_digits, 4);
}

TEST(ControllerTest, RadixSwitchSwapsGroupingDefault) {
    Controller ctrl;
    feed_chars(ctrl, "dg");                                  // dec, on -> 3
    EXPECT_EQ(ctrl.stack().state().group_digits, 3);
    feed_chars(ctrl, "d6");                                  // -> hex, group auto -> 4
    EXPECT_EQ(ctrl.stack().state().group_digits, 4);
    feed_chars(ctrl, "d0");                                  // -> dec, group auto -> 3
    EXPECT_EQ(ctrl.stack().state().group_digits, 3);
    feed_chars(ctrl, "d8");                                  // -> oct, default is 3, no swap
    EXPECT_EQ(ctrl.stack().state().group_digits, 3);
    feed_chars(ctrl, "d6");                                  // -> hex, swap to 4
    EXPECT_EQ(ctrl.stack().state().group_digits, 4);
}

TEST(ControllerTest, RadixSwitchPreservesOffGrouping) {
    Controller ctrl;
    EXPECT_EQ(ctrl.stack().state().group_digits, 0);         // off by default
    feed_chars(ctrl, "d6");                                  // -> hex
    EXPECT_EQ(ctrl.stack().state().group_digits, 0);         // still off
    feed_chars(ctrl, "d0");                                  // -> dec
    EXPECT_EQ(ctrl.stack().state().group_digits, 0);
}

TEST(ControllerTest, RadixSwitchPreservesUserCustomGrouping) {
    Controller ctrl;
    // Programmatically pick a non-default group size; auto-swap should leave
    // it alone (encodes "user customized").
    ctrl.stack().state().group_digits = 5;
    feed_chars(ctrl, "d6");                                  // -> hex
    EXPECT_EQ(ctrl.stack().state().group_digits, 5);         // preserved
    feed_chars(ctrl, "d0");                                  // -> dec
    EXPECT_EQ(ctrl.stack().state().group_digits, 5);
}

// User-reported flow: while in radix 16, "10 d r" should set radix to 10.
// (The "10" is decimal because input is always base 10; the 'd' isn't
// consumed as a hex digit even though display radix is hex.)
TEST(ControllerTest, ChangeBackToRadix10FromHexViaDR) {
    Controller ctrl;
    feed_chars(ctrl, "16"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "dr");                                  // -> radix 16
    EXPECT_EQ(ctrl.stack().state().display_radix, 16);
    feed_chars(ctrl, "10");                                  // input "10" (decimal)
    feed_chars(ctrl, "dr");                                  // -> radix 10
    EXPECT_EQ(ctrl.stack().state().display_radix, 10);
}

// User-reported: 16 d r used to fail with "Unknown command: radix_n".
TEST(ControllerTest, ArbitraryRadixFromStack) {
    Controller ctrl;
    feed_chars(ctrl, "255"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "16");  feed_special(ctrl, "RET");
    feed_chars(ctrl, "dr");                 // d r: pop 16, set radix=16
    auto ds = ctrl.display();
    EXPECT_EQ(ds.stack_depth, 1);
    EXPECT_EQ(ds.stack_entries.back(), "16#FF");
    EXPECT_TRUE(ds.message.empty());
}

// --- Variable storage (single-letter names) ---

TEST(ControllerTest, StoreAndRecall) {
    Controller ctrl;
    feed_chars(ctrl, "42"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "ssa");                    // s s a: store top in 'a' (peek)
    EXPECT_EQ(ctrl.display().stack_depth, 1) << "store should not pop";
    feed_chars(ctrl, "DEL");                    // (defensive — DEL is fine)
    // Drop and recall
    feed_special(ctrl, "DEL");                  // drop the 42
    EXPECT_EQ(ctrl.display().stack_depth, 0);
    feed_chars(ctrl, "sra");                    // s r a: recall 'a'
    EXPECT_EQ(ctrl.display().stack_depth, 1);
    EXPECT_EQ(ctrl.display().stack_entries.back(), "42");
}

TEST(ControllerTest, StoreIntoPops) {
    Controller ctrl;
    feed_chars(ctrl, "7"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "stx");                    // s t x: pop and store in 'x'
    EXPECT_EQ(ctrl.display().stack_depth, 0);
    feed_chars(ctrl, "srx");
    EXPECT_EQ(ctrl.display().stack_entries.back(), "7");
}

TEST(ControllerTest, RecallUnknownVariableErrors) {
    Controller ctrl;
    feed_chars(ctrl, "srz");
    auto ds = ctrl.display();
    EXPECT_FALSE(ds.message.empty());
    EXPECT_EQ(ds.stack_depth, 0);
}

TEST(ControllerTest, ExchangeSwapsTopWithVariable) {
    Controller ctrl;
    feed_chars(ctrl, "1"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "ssa");                    // a := 1
    feed_chars(ctrl, "2"); feed_special(ctrl, "RET");
    EXPECT_EQ(ctrl.display().stack_entries.back(), "2");
    feed_chars(ctrl, "sxa");                    // exchange: top becomes 1, a becomes 2
    EXPECT_EQ(ctrl.display().stack_entries.back(), "1");
    feed_chars(ctrl, "sra");
    EXPECT_EQ(ctrl.display().stack_entries.back(), "2");
}

TEST(ControllerTest, UnstoreRemovesVariable) {
    Controller ctrl;
    feed_chars(ctrl, "5"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "ssa");
    feed_chars(ctrl, "sua");                    // unstore 'a'
    feed_chars(ctrl, "sra");                    // recall now fails
    EXPECT_FALSE(ctrl.display().message.empty());
}

TEST(ControllerTest, StoreArithmetic) {
    Controller ctrl;
    feed_chars(ctrl, "10"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "ssa");                    // a := 10  (stack: [10])
    feed_chars(ctrl, "3");  feed_special(ctrl, "RET");      // stack: [10, 3]
    feed_chars(ctrl, "s+a");                    // a := 10+3 = 13; stack unchanged
    EXPECT_EQ(ctrl.display().stack_depth, 2);
    EXPECT_EQ(ctrl.display().stack_entries.back(), "3");
    feed_special(ctrl, "DEL");                  // [10]
    feed_special(ctrl, "DEL");                  // []
    feed_chars(ctrl, "sra");
    EXPECT_EQ(ctrl.display().stack_entries.back(), "13");
}

TEST(ControllerTest, NonLetterCancelsVariableCommand) {
    Controller ctrl;
    feed_chars(ctrl, "1"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "ss");                     // start s s, now waiting for name
    EXPECT_EQ(ctrl.display().pending_prefix, "store ?");
    feed_chars(ctrl, "5");                      // not a letter — cancel
    EXPECT_FALSE(ctrl.display().message.empty());
    EXPECT_TRUE(ctrl.display().pending_prefix.empty());
}

// --- Vector ops (newly bound) ---

// Helper: push [a, b, c] vector via direct stack manipulation.
static void push_vec(Controller& ctrl, std::vector<int> elems) {
    std::vector<ValuePtr> v;
    for (int e : elems) v.push_back(Value::make_integer(e));
    ctrl.stack().push(Value::make_vector(std::move(v)));
}

TEST(ControllerTest, VectorSum) {
    Controller ctrl;
    push_vec(ctrl, {1, 2, 3, 4, 5});
    feed_chars(ctrl, "V+");
    EXPECT_EQ(ctrl.display().stack_entries.back(), "15");
}

TEST(ControllerTest, VectorProduct) {
    Controller ctrl;
    push_vec(ctrl, {2, 3, 4});
    feed_chars(ctrl, "V*");
    EXPECT_EQ(ctrl.display().stack_entries.back(), "24");
}

TEST(ControllerTest, VectorMaxMin) {
    Controller ctrl;
    push_vec(ctrl, {3, 1, 4, 1, 5, 9, 2, 6});
    feed_chars(ctrl, "VX");                     // max
    EXPECT_EQ(ctrl.display().stack_entries.back(), "9");
    feed_special(ctrl, "DEL");
    push_vec(ctrl, {3, 1, 4, 1, 5, 9, 2, 6});
    feed_chars(ctrl, "VN");                     // min
    EXPECT_EQ(ctrl.display().stack_entries.back(), "1");
}

TEST(ControllerTest, VectorSort) {
    Controller ctrl;
    push_vec(ctrl, {3, 1, 4, 1, 5, 9, 2, 6});
    feed_chars(ctrl, "vr");
    auto ds = ctrl.display();
    EXPECT_NE(ds.stack_entries.back().find("[1, 1, 2, 3, 4, 5, 6, 9]"), std::string::npos)
        << "got: " << ds.stack_entries.back();
}

TEST(ControllerTest, VectorIdentity) {
    Controller ctrl;
    feed_chars(ctrl, "3"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "vi");
    auto ds = ctrl.display();
    // 3x3 identity: [[1,0,0],[0,1,0],[0,0,1]]
    EXPECT_NE(ds.stack_entries.back().find("[1, 0, 0]"), std::string::npos);
}

TEST(ControllerTest, VectorIndex) {
    Controller ctrl;
    feed_chars(ctrl, "5"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "vx");
    EXPECT_NE(ctrl.display().stack_entries.back().find("[1, 2, 3, 4, 5]"),
              std::string::npos);
}

TEST(ControllerTest, VectorDot) {
    Controller ctrl;
    push_vec(ctrl, {1, 2, 3});
    push_vec(ctrl, {4, 5, 6});
    feed_chars(ctrl, "VO");
    EXPECT_EQ(ctrl.display().stack_entries.back(), "32");   // 4+10+18
}

// --- Bitwise (b prefix) ---

TEST(ControllerTest, BitwiseAnd) {
    Controller ctrl;
    feed_chars(ctrl, "12"); feed_special(ctrl, "RET");      // 0b1100
    feed_chars(ctrl, "10"); feed_special(ctrl, "RET");      // 0b1010
    feed_chars(ctrl, "ba");
    EXPECT_EQ(ctrl.display().stack_entries.back(), "8");    // 0b1000
}

TEST(ControllerTest, BitwiseOr) {
    Controller ctrl;
    feed_chars(ctrl, "12"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "10"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "bo");
    EXPECT_EQ(ctrl.display().stack_entries.back(), "14");   // 0b1110
}

TEST(ControllerTest, BitwiseXor) {
    Controller ctrl;
    feed_chars(ctrl, "12"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "10"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "bx");
    EXPECT_EQ(ctrl.display().stack_entries.back(), "6");    // 0b0110
}

TEST(ControllerTest, LeftShift) {
    Controller ctrl;
    feed_chars(ctrl, "1"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "8"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "bl");
    EXPECT_EQ(ctrl.display().stack_entries.back(), "256");
}

TEST(ControllerTest, RightShift) {
    Controller ctrl;
    feed_chars(ctrl, "256"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "3");   feed_special(ctrl, "RET");
    feed_chars(ctrl, "br");
    EXPECT_EQ(ctrl.display().stack_entries.back(), "32");
}

TEST(ControllerTest, WordSizeSetting) {
    Controller ctrl;
    feed_chars(ctrl, "8"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "bw");                                 // word size := 8
    EXPECT_EQ(ctrl.display().stack_depth, 0);
    // Verify by NOT-ing 0: should give 0xFF = 255 in an 8-bit word.
    feed_chars(ctrl, "0"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "bn");
    EXPECT_EQ(ctrl.display().stack_entries.back(), "255");
}

TEST(ControllerTest, WordSizeRejectsBadInputs) {
    Controller ctrl;
    feed_chars(ctrl, "0"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "bw");
    auto ds = ctrl.display();
    EXPECT_FALSE(ds.message.empty());
    EXPECT_EQ(ds.stack_depth, 1);                           // 0 restored
}

// --- Trail navigation (t prefix) ---

TEST(ControllerTest, TrailYankPushesEntryAtPointer) {
    Controller ctrl;
    feed_chars(ctrl, "5"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "7"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "+");                          // stack: [12], trail has 5,7,+
    EXPECT_EQ(ctrl.display().stack_entries.back(), "12");
    feed_chars(ctrl, "ty");                         // yank current trail entry
    // Pointer is at the latest entry (from the +), so yank pushes the result.
    EXPECT_EQ(ctrl.display().stack_depth, 2);
    EXPECT_EQ(ctrl.display().stack_entries.back(), "12");
}

TEST(ControllerTest, TrailNavigateAndYank) {
    Controller ctrl;
    feed_chars(ctrl, "5");  feed_special(ctrl, "RET");
    feed_chars(ctrl, "7");  feed_special(ctrl, "RET");
    feed_chars(ctrl, "11"); feed_special(ctrl, "RET");
    // Trail: [5, 7, 11], pointer = 2 (the latest entry).
    feed_chars(ctrl, "t[");                         // pointer to first
    EXPECT_EQ(ctrl.display().trail_pointer, 0);
    feed_chars(ctrl, "tn");                         // pointer down
    EXPECT_EQ(ctrl.display().trail_pointer, 1);
    feed_chars(ctrl, "tn");                         // pointer down
    EXPECT_EQ(ctrl.display().trail_pointer, 2);
    feed_chars(ctrl, "tp");                         // pointer up
    EXPECT_EQ(ctrl.display().trail_pointer, 1);
    feed_chars(ctrl, "ty");                         // yank entry at pointer (7)
    EXPECT_EQ(ctrl.display().stack_entries.back(), "7");
    // Yank itself adds a trail entry, so the trail grew by one and the
    // pointer now sits on that newest entry.
    EXPECT_EQ(ctrl.display().trail_entries.size(), 4u);
    EXPECT_EQ(ctrl.display().trail_pointer, 3);
    feed_chars(ctrl, "t[");                         // back to first
    EXPECT_EQ(ctrl.display().trail_pointer, 0);
    feed_chars(ctrl, "t]");                         // to last
    EXPECT_EQ(ctrl.display().trail_pointer, 3);
}

TEST(ControllerTest, TrailKillRemovesPointerEntry) {
    Controller ctrl;
    feed_chars(ctrl, "5"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "7"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "9"); feed_special(ctrl, "RET");
    int before = static_cast<int>(ctrl.display().trail_entries.size());
    feed_chars(ctrl, "tk");                         // delete entry at pointer
    int after = static_cast<int>(ctrl.display().trail_entries.size());
    EXPECT_EQ(after, before - 1);
}

TEST(ControllerTest, TrailYankOnEmptyReportsError) {
    Controller ctrl;
    feed_chars(ctrl, "ty");
    EXPECT_FALSE(ctrl.display().message.empty());
    EXPECT_EQ(ctrl.display().stack_depth, 0);
}

// --- Quick registers q0-q9 ---

TEST(ControllerTest, QuickStoreAndRecall) {
    Controller ctrl;
    feed_chars(ctrl, "42"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "t3");                 // store top in q3 (peek)
    EXPECT_EQ(ctrl.display().stack_depth, 1);
    feed_special(ctrl, "DEL");              // drop the 42
    feed_chars(ctrl, "r3");                 // recall q3
    EXPECT_EQ(ctrl.display().stack_entries.back(), "42");
}

TEST(ControllerTest, QuickRecallEmptyReportsError) {
    Controller ctrl;
    feed_chars(ctrl, "r7");
    auto ds = ctrl.display();
    EXPECT_FALSE(ds.message.empty());
    EXPECT_EQ(ds.stack_depth, 0);
}

TEST(ControllerTest, AllTenQuickRegisters) {
    Controller ctrl;
    // Push 10 distinct values, each into a different quick register.
    for (int i = 0; i < 10; ++i) {
        feed_chars(ctrl, std::to_string(i * 100));
        feed_special(ctrl, "RET");
        feed_chars(ctrl, std::string("t") + char('0' + i));
        feed_special(ctrl, "DEL");
    }
    // Recall in reverse order.
    for (int i = 9; i >= 0; --i) {
        feed_chars(ctrl, std::string("r") + char('0' + i));
        EXPECT_EQ(ctrl.display().stack_entries.back(), std::to_string(i * 100));
        feed_special(ctrl, "DEL");
    }
}

// User-reported: in radix 16, `d g` couldn't reach the grouping command
// because input_state.feed() was eagerly consuming `d` as a hex digit
// (starting a new number entry) before the keymap dispatcher saw it.
TEST(ControllerTest, PrefixCommandsWorkInHexRadix) {
    Controller ctrl;
    feed_chars(ctrl, "16"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "dr");                 // set radix=16
    EXPECT_EQ(ctrl.display().stack_depth, 0);
    feed_chars(ctrl, "dg");                 // toggle grouping (was failing)
    EXPECT_TRUE(ctrl.display().message.empty())
        << "got message: " << ctrl.display().message;
    // Verify grouping actually toggled: a big hex value should now have
    // separators.
    feed_chars(ctrl, "1048575"); feed_special(ctrl, "RET");
    auto ds = ctrl.display();
    EXPECT_NE(ds.stack_entries.back().find(','), std::string::npos)
        << "expected a thousands separator, got: " << ds.stack_entries.back();
}

// Hex literals can still be entered explicitly via the radix prefix `#`,
// even though bare letters now run their commands instead of starting a
// number entry.
TEST(ControllerTest, HexLiteralViaRadixPrefixStillWorks) {
    Controller ctrl;
    feed_chars(ctrl, "16#FF"); feed_special(ctrl, "RET");
    EXPECT_EQ(ctrl.display().stack_depth, 1);
    EXPECT_EQ(ctrl.display().stack_entries.back(), "255");
}

TEST(ControllerTest, ArbitraryRadixRejectsOutOfRange) {
    Controller ctrl;
    feed_chars(ctrl, "100"); feed_special(ctrl, "RET");  // > 36
    feed_chars(ctrl, "dr");
    auto ds = ctrl.display();
    EXPECT_FALSE(ds.message.empty());
    // Bad input must be restored to the stack, not silently dropped.
    EXPECT_EQ(ds.stack_depth, 1);
    EXPECT_EQ(ds.stack_entries.back(), "100");
}

TEST(ControllerTest, HexRadix) {
    Controller ctrl;
    feed_chars(ctrl, "255");
    feed_special(ctrl, "RET");
    feed_chars(ctrl, "d6"); // d 6 = hex
    auto ds = ctrl.display();
    EXPECT_EQ(ds.stack_entries.back(), "16#FF");
}

// User-reported: 16, 16, 16 then ^, ^ used to give 1 because the second
// ^ truncated 16^16 = 2^64 to 0 via mpz::get_si and computed 16^0 = 1.
TEST(ControllerTest, PowerOverflowReportsErrorAndRestoresStack) {
    Controller ctrl;
    feed_chars(ctrl, "16"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "16"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "16"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "^");  // 16^16 succeeds -> stack: [16, 2^64]
    EXPECT_EQ(ctrl.display().stack_depth, 2);
    feed_chars(ctrl, "^");  // 16^(2^64) overflows
    auto ds = ctrl.display();
    // The bad operation must NOT silently produce 1.
    EXPECT_FALSE(ds.message.empty()) << "expected an overflow error message";
    EXPECT_NE(ds.stack_entries.back(), "1");
    // And the stack must be restored to the pre-command state.
    EXPECT_EQ(ds.stack_depth, 2);
}

// Generic: an error in any binary op rolls the stack back to its
// pre-command state. Use 1/0 (division by zero) as a clean trigger.
TEST(ControllerTest, FailedBinaryOpRestoresStack) {
    Controller ctrl;
    feed_chars(ctrl, "1"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "0"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "/");
    auto ds = ctrl.display();
    EXPECT_FALSE(ds.message.empty());
    EXPECT_EQ(ds.stack_depth, 2);
    EXPECT_EQ(ds.stack_entries[0], "1");
    EXPECT_EQ(ds.stack_entries[1], "0");
}

// User-reported: undo couldn't roll back number-entry pushes because
// finalize_entry called Stack::push directly without begin/end_command.
TEST(ControllerTest, UndoUndoesNumberEntry) {
    Controller ctrl;
    feed_chars(ctrl, "5");  feed_special(ctrl, "RET");
    feed_chars(ctrl, "10"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "15"); feed_special(ctrl, "RET");
    EXPECT_EQ(ctrl.display().stack_depth, 3);
    feed_chars(ctrl, "U");          // undo
    EXPECT_EQ(ctrl.display().stack_depth, 2);
    EXPECT_EQ(ctrl.display().stack_entries.back(), "10");
    feed_chars(ctrl, "U");
    EXPECT_EQ(ctrl.display().stack_depth, 1);
    EXPECT_EQ(ctrl.display().stack_entries.back(), "5");
    feed_chars(ctrl, "U");
    EXPECT_EQ(ctrl.display().stack_depth, 0);
}

TEST(ControllerTest, RedoAfterUndoOfNumberEntry) {
    Controller ctrl;
    feed_chars(ctrl, "42"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "U");          // undo
    EXPECT_EQ(ctrl.display().stack_depth, 0);
    feed_chars(ctrl, "D");          // redo
    EXPECT_EQ(ctrl.display().stack_depth, 1);
    EXPECT_EQ(ctrl.display().stack_entries.back(), "42");
}

TEST(ControllerTest, UndoUndoesPushOfPi) {
    Controller ctrl;
    feed_chars(ctrl, "P");          // push pi
    EXPECT_EQ(ctrl.display().stack_depth, 1);
    feed_chars(ctrl, "U");          // undo
    EXPECT_EQ(ctrl.display().stack_depth, 0);
}

TEST(ControllerTest, UndoUndoesPrecisionPop) {
    Controller ctrl;
    feed_chars(ctrl, "20"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "p");          // sets precision to 20, pops the 20
    EXPECT_EQ(ctrl.display().stack_depth, 0);
    feed_chars(ctrl, "U");          // undoes the pop
    EXPECT_EQ(ctrl.display().stack_depth, 1);
    EXPECT_EQ(ctrl.display().stack_entries.back(), "20");
}

TEST(ControllerTest, UndoUndoesLastArgs) {
    Controller ctrl;
    feed_chars(ctrl, "2"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "3"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "+");                          // stack: [5]
    feed_special(ctrl, "M-RET");                    // stack: [5, 2, 3]
    EXPECT_EQ(ctrl.display().stack_depth, 3);
    feed_chars(ctrl, "U");                          // undo M-RET
    EXPECT_EQ(ctrl.display().stack_depth, 1);
    EXPECT_EQ(ctrl.display().stack_entries.back(), "5");
}

// Special cases: even with huge exponents, base 0/±1 have well-defined results.
TEST(ControllerTest, PowerSpecialCasesSurviveHugeExponent) {
    Controller ctrl;
    // 0 ^ (2^64) = 0
    feed_chars(ctrl, "0");  feed_special(ctrl, "RET");
    feed_chars(ctrl, "16"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "16"); feed_special(ctrl, "RET");
    feed_chars(ctrl, "^");  // builds 2^64
    feed_chars(ctrl, "^");  // 0 ^ (2^64) = 0
    EXPECT_EQ(ctrl.display().stack_entries.back(), "0");
    EXPECT_TRUE(ctrl.display().message.empty());
}
