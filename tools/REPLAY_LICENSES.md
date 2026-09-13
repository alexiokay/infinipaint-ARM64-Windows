# Replay utility source and licenses

pen_replay is an InfiniPaint diagnostic tool, distributed under GPL-3.0-or-later.
The included COPYING file contains the GPL. Complete corresponding tool source,
CMake instructions and host helpers are in the commit associated with the Actions
run that produced this artifact:

https://github.com/alexiokay/infinipaint-Custom

Build instructions are in docs/BRUSH_PIPELINE.md. The executable is statically
linked to the Microsoft C++ runtime; no separate custom service or library is needed.

External MIT-licensed source compiled into this tool:

- pen-stabilizer v0.1.0, adbdce4e902433fcd14fba16e08863f2ec909f79:
  https://github.com/PenTraceTools/pen-stabilizer/tree/adbdce4e902433fcd14fba16e08863f2ec909f79
  License included at deps/pen-stabilizer/LICENSE.
- PenTraceLab recording/processing helpers, e13be1ccf06039c46ae9997beb8810fc28bf5929:
  https://github.com/PenTraceTools/pen-trace-lab/tree/e13be1ccf06039c46ae9997beb8810fc28bf5929
  MIT notice included at assets/data/third_party_licenses/PenTraceLab.

This tool is not a full InfiniPaint build or a replacement for PenTraceLab.
