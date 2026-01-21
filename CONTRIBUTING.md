# Contributing to Kastle 2

Before contributing, please read these guides:

- **[Toolchain Install Guide](TOOLCHAIN_INSTALL.md)** - Set up your development environment
- **[Coding Style Guide](CODING_STYLE.md)** - Follow our C++ conventions  
- **[Glossary & Examples](GLOSSARY_EXAMPLES.md)** - Understand the app architecture
- **[FAQ](FAQ.md)** - Common issues and solutions

## Development Workflow

1. **Fork** the repository and create a branch
2. **Install** the toolchain following the guide and configure the CMake
3. **Build** your changes
4. **Test** on real hardware using USB, J-Link, or Picoprobe methods

## Branch Naming

Use descriptive branch names with prefixes:
- `feature/new-audio-effect` 
- `bugfix/lfo-timing-issue`
- `docs/update-api-examples`

## Reporting Issues

When reporting bugs, please include:
- App name and version
- Hardware variant (Kastle 2 vs Citadel)
- Steps to reproduce the issue
- Expected vs actual behavior

## Questions?

Check the [FAQ](FAQ.md) or discuss your experience on our [Discord server](https://discord.com/invite/C4RXRjgYJ6).