# Core compiler area

This directory mirrors the responsibility of CPython's core `Python/` area without copying Python implementation code. For Ric, the responsibility includes compiler orchestration, type checking, NIR lowering and optimization.

The current buildable files remain under `compiler/bootstrap/` during the bootstrap phase. They will move into dedicated Ric implementation units as the compiler ABI stabilizes.
