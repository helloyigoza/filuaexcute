# DX11 ImGui Lua Console (Educational Build)

> **Important:** This repository is published for reverse-engineering and graphics-hooking education.
> Do **not** use this project to violate game/server rules, EULAs, or local laws.

## What this project contains

This codebase demonstrates:
- DirectX11 present-hook integration patterns
- ImGui overlay rendering in a hooked process
- Text editor integration for runtime script input
- Internal resource abstraction experiments

## Public repository hardening

This public version includes:
- neutral project naming in UI labels
- community documentation and contribution guidelines
- a basic security policy for responsible disclosure
- repository hygiene files (`.gitignore`, `LICENSE`)

## Build notes

- Primary solution: `ImGui DirectX 11 Kiero Hook.sln`
- Toolchain: Visual Studio with C++ desktop workload
- External/runtime behavior may vary based on host environment and anti-cheat systems

## Responsible use

Use this repository only in controlled, authorized environments (e.g. private test apps, CTF labs, or your own software).

If you are looking for production-safe plugin systems, consider sandboxed scripting engines and explicit plugin APIs rather than runtime hooks.
