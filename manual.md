# Stackcalc — User Manual

Stackcalc is a stack-based, arbitrary-precision desktop calculator
modeled after Emacs's `M-x calc`. It uses **RPN** (Reverse Polish
Notation): you push values onto a stack, then apply operators that
consume from the stack. There is no `=` key — the result of an
operation appears immediately at the top of the stack.

This manual is a reference. Every keystroke that the program responds
to is documented here. A compact cheat sheet is at the end.

## Launching

After building, the executable lands at:

| Platform | Path |
|---|---|
| macOS | `build/bin/stackcalc.app` (also runnable via `open`) |
| Linux/Windows | `build/bin/stackcalc` |

The window opens at 960 × 640 logical pixels, split 50/50 between the
stack (left) and the trail (right). Everything is keyboard-driven; the
mouse is only used to resize the window or drag the splitter sash
between the two panes.

Stackcalc automatically persists its session (stack, variables, trail,
precision, word size, display settings, window size, splitter position)
when you quit, and restores it on next launch. The state file lives in
the platform-appropriate user data directory:

| Platform | Path |
|---|---|
| macOS | `~/Library/Application Support/stackcalc/state.scl` |
| Windows | `%APPDATA%\stackcalc\state.scl` |
| Linux | `$XDG_DATA_HOME/stackcalc/state.scl` (default `~/.local/share/stackcalc/state.scl`) |

To start fresh, use **File → Reset Calculator** (`Ctrl+R`), which wipes
the stack, variables, trail, and all modes back to defaults.

---

## 1. The display

```
┌────────────────────────────────────┐
│ message line (red, only on errors) │
├────────────────────────────────────┤
│  3:  [1, 2, 3]                     │
│  2:  3.14159265359                 │
│  1:  42                            │
│      .                             │   ← edit-point marker
│      127_                          │   ← entry line + blinking cursor
├────────────────────────────────────┤
│ 12 Deg   [I] [d-]                  │   ← mode line + flags + prefix
└────────────────────────────────────┘
```

| Region | Color | Notes |
|---|---|---|
| Stack entries | white values, gray `N:` labels | Numbered bottom-up; `1:` is the top of the stack. |
| `.` marker | gray | Marks where the next entered number will go. |
| Entry line | green | Only shown while you're typing a number. The trailing `_` blinks. |
| Mode line | cyan | `precision angular [Frac] [Radix N] [Fix/Sci/Eng]`. Default: `12 Deg`. |
| Modifier flags | yellow | `[I]` inverse, `[H]` hyperbolic, `[K]` keep-args. |
| Pending prefix | yellow | `[d-]`, `[m-]`, `store ?`, etc. — what stackcalc is waiting for. |
| Message | red | Last error or status message. Cleared by the next keystroke. |

The trail occupies the right pane of the main window and logs every
operation as it happens. See section 16.

---

## 2. Number entry

Typing a digit (or `.`) starts a new entry. The entry is committed to
the stack by `RET`, by another digit-after-RET, or implicitly by the
next operator.

| Syntax | What it means | Example |
|---|---|---|
| `0–9` | decimal digits | `42` |
| `.` | decimal point | `3.14` |
| `_` | negative sign (because `-` is a binary operator) | `_42` → `−42` |
| `e` / `E` | exponent; followed by optional `-` or `+` sign | `2.5e-3`, `1e+6` |
| `a:b` | fraction `a/b` | `1:3` → `1/3` |
| `h@m'sec"` | hours-minutes-seconds | `2@30'15"` |
| `R#NNN` | integer in custom radix `R` (2–36) | `16#FF` → `255` |
| `\b` | backspace — delete last char of entry | |

Number entry is **always decimal**, regardless of the display radix.
Typing `10` always means ten, even when the display is in hex. To
enter a value in another base, use the explicit `R#NNN` form (e.g.
`16#FF` for 255). This way command keys that double as hex digits
(`d`, `f`, `A`, `B`, `C`, `E`, etc.) keep working in any display
mode. Within a `R#` prefix entry, the appropriate digits of that
radix extend the entry.

---

## 3. Stack manipulation

| Key | Action |
|---|---|
| `RET` | Push the current entry. If no entry, duplicate the top. |
| `SPC` | Same as `RET`. |
| `DEL` | Drop the top of stack. |
| `Backspace` | Delete a character from the entry; or drop, if no entry is active. |
| `TAB` | Swap the top two stack entries. |
| `M-TAB` (Alt+Tab) | Roll the top three entries up. |
| `M-RET` (Alt+Enter) | Push the arguments of the last command back onto the stack ("last args"). |
| `Escape` | Cancel the current entry. |

`M-RET` is useful for redoing a calculation with a different operator:
after `2 RET 3 +` you have `1: 5`; type `M-RET` to recover `2` and
`3` on top, then type `*` to get `1: 6`.

---

## 4. Undo and redo

| Key | Action |
|---|---|
| `U` or `Ctrl+Z` | Undo the last stack-affecting command. |
| `D` or `Ctrl+Y` | Redo. |

Everything that touches the stack is undoable, including pushing a
freshly entered number. **Mode changes** (precision, word size,
display radix, angular mode, etc.) are *not* themselves reversed by
undo — but the stack pop that delivered them is, so the popped value
returns to the stack.

---

## 5. Arithmetic

| Key | Op | Notes |
|---|---|---|
| `+` | add | Works on any pair of compatible types. |
| `-` | subtract | |
| `*` | multiply | HMS × scalar is supported. |
| `/` | divide | Integer/integer in fraction mode produces a fraction. |
| `^` | power | Integer/fraction/float/complex/interval bases; integer or non-integer real exponent (non-integer goes through MPFR). Negative base ^ non-integer gives `nan`. |
| `%` | modulo | Floor-remainder for integers; fraction & float supported. |
| `\` | integer (floor) division | |
| `&` | reciprocal (`1/x`) | Complex `&` works (uses `conj/|z|²`). |
| `n` | negate (because `-` is binary) | |
| `Q` | square root | `sqrt(-1)` promotes to `i`. Complex `Q` is closed-form, no trig. |
| `A` | absolute value / magnitude | `|3+4i|` returns `5`. |

`I Q` (inverse + sqrt) means **square** (multiplies the top by itself).

Compound types (RectComplex, ErrorForm, Interval, ModuloForm,
Vector, HMS) auto-promote when mixed with reals; see section 17.

---

## 6. Modifier flags

These three keys *don't* run a command — they set a transient flag
that modifies the **next** command, then clear themselves.

| Key | Flag | Effect on the next command |
|---|---|---|
| `I` | inverse | Use the inverse function (e.g., `arcsin` instead of `sin`). |
| `H` | hyperbolic | Use the hyperbolic variant (e.g., `sinh` instead of `sin`). |
| `K` | keep-args | Don't consume the operands; leave them on the stack alongside the result. |

Examples:

| Sequence | Meaning |
|---|---|
| `S` | sin |
| `I S` | arcsin |
| `H S` | sinh |
| `I H S` | arsinh |
| `K +` | add, but leave the two original operands below the result |
| `I Q` | square (`x*x`) |
| `I F` | ceiling |
| `I R` | truncate |

Flag indicators appear in the mode line as `[I]`, `[H]`, `[K]` until
the next command consumes them.

---

## 7. Scientific functions

### Trig (angular-mode-aware)

| Key | Plain | `I` | `H` | `I H` |
|---|---|---|---|---|
| `S` | sin | arcsin | sinh | arsinh |
| `C` | cos | arccos | cosh | arcosh |
| `T` | tan | arctan | tanh | artanh |

### Logarithmic

| Key | Plain | `I` | `H` | `I H` |
|---|---|---|---|---|
| `L` | natural log | exp | log₁₀ | exp10 |
| `E` | exp | natural log | — | — |
| `B` | log_b(a) (binary, takes base from stack) | b^a (alog) | — | — |

### Combinatorics

| Key | Op |
|---|---|
| `!` | factorial |
| `k c` | binomial coefficient (n choose m) |
| `k d` | double factorial |
| `H k c` | permutations P(n, m) |

### Constants (`P`)

| Key | Constant |
|---|---|
| `P` | π |
| `H P` | e |
| `I P` | γ (Euler-Mascheroni) |
| `H I P` | φ (golden ratio) |

### Extra scientific (`f` prefix)

| Sequence | Function |
|---|---|
| `f T` | atan2(y, x) |
| `f E` | expm1(x) — `exp(x) − 1` accurate near 0 |
| `f L` | lnp1(x) — `ln(1+x)` accurate near 0 |
| `f I` | ilog(a, b) — `floor(log_b(a))`, integer args, exact |
| `f g` | Γ(x) — gamma function via MPFR |

---

## 8. Number theory and combinatorics — `k` prefix

| Sequence | Op | Notes |
|---|---|---|
| `k g` | gcd | Two integers. |
| `k l` | lcm | Two integers. |
| `k r` | random | Uniform integer in `[0, n)`. |
| `k n` | next prime | `I k n` gives the previous prime. |
| `k p` | prime test | Reports "Definitely prime" / "Probably prime" / "Not prime" in the message line; doesn't modify the stack. |
| `k f` | prime factors | Returns a vector of `[prime, exponent]` pairs. |
| `k t` | Euler's totient φ(n) | |
| `k e` | extended GCD | Returns `[gcd, x, y]` such that `ax + by = gcd`. |
| `k m` | modular power | `base^exp mod m` (3-arg, all integer). |
| `k c` | binomial | (also see section 7) |
| `k d` | double factorial | |

---

## 9. Vectors and matrices

Vectors are the only compound numeric type without a literal entry
syntax; build them via these commands:

### Vector construction / manipulation — `v` prefix

| Sequence | Op |
|---|---|
| `v x` | index vector `[1, 2, …, n]` from top-of-stack `n` |
| `v i` | n × n identity matrix from top-of-stack `n` |
| `v d` | diagonal matrix from top vector |
| `v t` | transpose |
| `v l` | length |
| `v v` | reverse |
| `v r` | sort ascending (`I v r` = descending) |
| `v h` | head (`I v h` = tail) |
| `v k` | cons (prepend) |
| `v &` | matrix inverse |

### Vector reduce / linear algebra — `V` prefix

| Sequence | Op |
|---|---|
| `V +` | sum of elements |
| `V *` | product of elements |
| `V X` | maximum |
| `V N` | minimum |
| `V O` | dot product |
| `V D` | determinant |
| `V T` | trace |
| `V C` | cross product (3-element vectors only) |

Element-wise arithmetic: `+`, `-`, `*`, `/` already broadcast over
vectors. Multiplying a scalar by a vector applies element-wise.

---

## 10. Bitwise — `b` prefix

These operate on integers, treating values as fixed-width words. The
**word size** lives in the calc state (default 32, range 1–1024) and
governs `NOT`, shifts, rotates, and clip.

| Sequence | Op |
|---|---|
| `b a` | AND |
| `b o` | OR |
| `b x` | XOR |
| `b n` | NOT (bitwise complement, masked to word size) |
| `b l` | left shift (top = shift amount, 2nd = value) |
| `b r` | right shift (arithmetic) |
| `b t` | rotate left (`I b t` = rotate right) |
| `b c` | clip to word size (mask off high bits) |
| `b w` | set word size from stack (1–1024) |

Tip: pair with `d 6` to see the result in hex (`16#…`).

---

## 11. Display modes — `d` prefix

### Number format

| Sequence | Format |
|---|---|
| `d n` | normal (default) — show all significant digits |
| `d f` | fixed-point |
| `d s` | scientific |
| `d e` | engineering |

### Radix

| Sequence | Radix |
|---|---|
| `d 0` | 10 (decimal, the default) |
| `d 2` | 2 (binary) |
| `d 8` | 8 (octal) |
| `d 6` | 16 (hexadecimal) |
| `d r` | pop top of stack as the radix (must be 2–36) |

### Other display toggles

| Sequence | Toggle |
|---|---|
| `d g` | toggle digit grouping (groups of 3 for decimal/binary/octal, 4 for hex; auto-switches when you change radix) |
| `d z` | leading zeros (pad to word size in non-decimal radix) |
| `d c` | complex as `(a, b)` pair (default) |
| `d i` | complex as `a + bi` |
| `d j` | complex as `a + bj` |

Display mode changes are not undoable.

---

## 12. Calc modes — `m` prefix

| Sequence | Mode |
|---|---|
| `m d` | degrees (default) |
| `m r` | radians |
| `m f` | toggle fraction mode (integer division → fraction vs. float) |
| `m s` | toggle symbolic mode |
| `m i` | toggle infinite mode |

Symbolic and infinite modes set their flag, but no current operation
consumes them — they're plumbing for future features.

---

## 13. Variables — `s` prefix

Variable names are a single ASCII letter (case-sensitive), giving 52
named slots. After the two-key sequence, stackcalc waits for the
name; the mode line shows `store ?`, `recall ?`, etc. Pressing a
non-letter cancels with a "Cancelled" message.

| Sequence | Action |
|---|---|
| `s s X` | store top of stack in `X` (peek; doesn't pop) |
| `s t X` | store top of stack in `X` and **pop** |
| `s r X` | push `X` onto the stack |
| `s x X` | exchange top with `X` (top gets old `X`, `X` gets old top) |
| `s u X` | unstore (delete) `X` |
| `s + X` | `X := X + top`, stack unchanged |
| `s - X` | `X := X − top`, stack unchanged |
| `s * X` | `X := X × top`, stack unchanged |
| `s / X` | `X := X ÷ top`, stack unchanged |

Recalling or arithmetic-storing an undefined variable yields a
`variable 'X' not stored` error message.

---

## 14. Quick registers

Quick registers are 10 nameless slots, `q0` through `q9`, accessed
without a name-entry step.

| Sequence | Action |
|---|---|
| `t N` | store top of stack in `qN` (peek; doesn't pop) |
| `r N` | recall `qN` |

`N` is a single digit `0–9`. Recalling an empty register reports
`qN is empty`.

---

## 15. Precision and word size

| Key | Action | Range |
|---|---|---|
| `p` | pop top as new precision (decimal digits) | 1–1000 |
| `b w` | pop top as new word size (bits, for bitwise ops) | 1–1024 |

Both validate; out-of-range or non-integer inputs are restored to
the stack and an error message is shown.

Default precision is 12 decimal digits, default word size is 32 bits.

---

## 16. Trail

The trail occupies the right pane of the main window and logs every
operation as it happens. Each line shows a short tag and the resulting
value:

```
ent: 2
ent: 3
add: 5
```

Plain number entries have no tag (just the value). Operations get
short tags like `add`, `sub`, `mul`, `sin`, `cos`, `lsh`, `rsh`,
`xor`, `det`, `cross`, `pi`, `gam`, `rcl-X`, `rcl-q3`, `sto-Y`,
`xch-Z`. Drag the splitter sash to resize the panes.

### Trail navigation — `t` prefix

The trail has a yellow-highlighted **pointer** (`>`) you can move
around to pick an entry. These commands share the `t` prefix with
quick-store (`t0`–`t9`); which one fires depends on the next key —
digits go to quick-store, letters/brackets go to the trail.

| Sequence | Command |
|---|---|
| `t y` | yank — push the value at the pointer onto the stack |
| `t [` | move pointer to the first entry |
| `t ]` | move pointer to the last entry |
| `t n` | next entry (pointer down) |
| `t p` | previous entry (pointer up) |
| `t h` | move pointer to the last entry (alias for `t ]`) |
| `t k` | kill (delete) the entry at the pointer |

Only `t y` and `t k` affect the stack or trail state (and are
undoable); the other navigation commands just move the pointer.

---

## 17. Value types

Stackcalc supports 12 value types in its stack. Compound types are
constructed by the operations that produce them.

| Type | Example display | Constructible from keyboard? |
|---|---|---|
| Integer | `42` | yes (digits) |
| Fraction | `1:3` | yes (`1:3`) |
| DecimalFloat | `3.14159265359` (or `16#3.243F6A8885` in hex display) | yes (`3.14`, `2.5e-3`) |
| RectComplex | `(3, 4)` or `3+4i` (display mode) | only via operations producing complex (e.g. `Q` of a negative) |
| PolarComplex | — | not currently constructible from keyboard |
| HMS | `2@ 30' 15"` | yes (`2@30'15"`) |
| DateForm | — | not constructible from keyboard |
| ModuloForm | `(3 mod 7)` | only programmatically |
| ErrorForm | `5 ± 0.2` | only programmatically |
| Interval | `[1 .. 3]` | only programmatically |
| Vector | `[1, 2, 3]` | yes, but only via `v x`, `v i`, `v d`, or other operations — there is no literal `[…]` input syntax |
| Infinity | `inf`, `−inf`, `uinf`, `nan` | only as a result |

---

## 18. Limitations and known gaps

These are honest gaps, not bugs:

- **Variable names**: single ASCII letter only. There is no
  minibuffer-style prompt for long names.
- **DateForm**: the type exists and the formatter handles it, but
  there is no input syntax and no command bindings. Effectively
  unreachable from the UI.
- **PolarComplex**: same — no entry syntax, no arithmetic dispatch.
  Rectangular complex covers everyday needs.
- **Symbolic and infinite modes**: settable via `m s` / `m i`, but no
  operation currently consumes the flags. They're scaffolding for
  future symbolic features.
- **Complex with non-integer exponent**: only integer-exponent
  complex powers are supported (e.g., `(1+i)^4`). Non-integer
  exponents on complex bases are not implemented.
- **Vector literal entry**: no `[1, 2, 3]` input syntax; build
  vectors via `v x` ([1..n]), `v i` (identity), `v d` (diagonal),
  or by accumulating values with `v k` (cons).
- **Mode changes are not undone**: undo restores the stack but does
  not roll back precision, word size, angular mode, display radix,
  etc. (Mode-affecting commands that *also* pop a value — like `p`,
  `b w`, `d r` — restore the popped value.)
- **Numeric ranges**: precision 1–1000, word size 1–1024.

---

## 19. Quick reference (cheat sheet)

### Top-level single keys

| Key | Command |
|---|---|
| `0–9` `.` `_` `e` `:` `@` `'` `"` `#` | number entry |
| `RET` `SPC` | enter / dup |
| `DEL` | drop |
| `TAB` | swap |
| `M-TAB` | roll up |
| `M-RET` | last args |
| `+` `-` `*` `/` `^` `%` `\` `&` | arithmetic |
| `n` | negate |
| `Q` | sqrt |
| `A` | abs |
| `F` | floor |
| `R` | round |
| `S` `C` `T` | sin, cos, tan |
| `L` | ln |
| `E` | exp |
| `B` | log_b |
| `!` | factorial |
| `P` | π (with H/I/HI for e/γ/φ) |
| `I` `H` `K` | inverse / hyperbolic / keep-args flags |
| `U` | undo (also `Ctrl+Z`) |
| `D` | redo (also `Ctrl+Y`) |
| `p` | set precision from stack |
| `Esc` | cancel input |
| `Ctrl+R` | reset calculator (File → Reset Calculator) |

### Prefix sequences

| Prefix | Use |
|---|---|
| `d` | display modes (format, radix, grouping, complex notation) |
| `m` | calc modes (angular, fraction, symbolic, infinite) |
| `s` | named variables (then a single letter for the name) |
| `t N` / `r N` | quick registers q0–q9 (`N` is a digit) |
| `t` (+ letter/bracket) | trail navigation (`t y`, `t [`, `t ]`, `t n`, `t p`, `t h`, `t k`) |
| `b` | bitwise |
| `f` | extra scientific (atan2, expm1, lnp1, ilog, gamma) |
| `k` | number theory and combinatorics |
| `v` | vector construction / manipulation |
| `V` | vector reduce / linear algebra |

### All `d` (display) sequences

| | |
|---|---|
| `d n` normal | `d f` fixed | `d s` scientific | `d e` engineering |
| `d 2` binary | `d 8` octal | `d 6` hex | `d 0` decimal |
| `d r` radix from stack | `d g` grouping | `d z` leading zeros | |
| `d c` complex (a,b) | `d i` complex a+bi | `d j` complex a+bj | |

### All `m` (mode) sequences

| `m d` deg | `m r` rad | `m f` toggle frac | `m s` symbolic | `m i` infinite |
|---|---|---|---|---|

### All `s` (variable) sequences

| `s s X` store | `s t X` store-pop | `s r X` recall | `s x X` exchange | `s u X` unstore |
|---|---|---|---|---|
| `s + X` | `s - X` | `s * X` | `s / X` | |

### All `b` (bitwise) sequences

| `b a` AND | `b o` OR | `b x` XOR | `b n` NOT |
|---|---|---|---|
| `b l` lshift | `b r` rshift | `b t` rotate (`I` for right) | `b c` clip |
| `b w` set word size | | | |

### All `f` (fancy scientific) sequences

| `f T` atan2 | `f E` expm1 | `f L` lnp1 | `f I` ilog | `f g` gamma |
|---|---|---|---|---|

### All `k` (number theory) sequences

| `k g` gcd | `k l` lcm | `k r` random | `k n` next prime |
|---|---|---|---|
| `k p` prime test | `k f` prime factors | `k t` totient | `k e` extended gcd |
| `k m` mod-pow | `k c` choose | `k d` double factorial | |

### All `v` (vector) sequences

| `v x` index | `v i` identity | `v d` diagonal | `v t` transpose |
|---|---|---|---|
| `v l` length | `v v` reverse | `v r` sort | `v h` head |
| `v k` cons | `v &` matrix inverse | | |

### All `V` (vector reduce) sequences

| `V +` sum | `V *` product | `V X` max | `V N` min |
|---|---|---|---|
| `V O` dot | `V D` determinant | `V T` trace | `V C` cross |

### All `t` (trail) sequences

| `t y` yank | `t [` first | `t ]` last | `t n` next |
|---|---|---|---|
| `t p` prev | `t h` here | `t k` kill | |

(Digits `t 0`…`t 9` are quick-store, not trail commands.)

---

*End of manual.*
