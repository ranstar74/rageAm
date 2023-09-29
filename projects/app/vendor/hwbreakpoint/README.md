# hw-breakpoint
set hardware breakpoints programmaticaly - for windows x86/x64

## Features

* Works both on x86 and x64
* Multithreaded: ability to break every existed thread (at BP registration time), and future thread

## Usage
```c++
#include "breakpoint.h"

// let's assume you have variable "val" that you want to watch on it
SomeNativeType val;

// to set write breakpoint
HWBreakpoint::Set(&val, HWBreakpoint::Condition::Write);

// to set read and write breakpoint
HWBreakpoint::Set(&val, HWBreakpoint::Condition::ReadWrite);

// to clear the breakpoint
HWBreakpoint::Clear(&val);

// to cleanup all breakpoint
HWBreakpoint::ClearAll();
```
## Credit
The accessing debug register code is based on https://github.com/mmorearty/hardware-breakpoints
