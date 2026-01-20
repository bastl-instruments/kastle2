# FAQ

## Can I buy a Kastle 2 or Citadel kit?

Yes! Kits are available from the [official Bastl shop](http://shop.bastl-instruments.com).

## Will you update to RP2350?

All development and testing for Kastle 2 was completed using the RP2040, and production began before the RP2350 was announced. While the RP2350 offers more power, its pinout and compiled binaries are not directly compatible with the RP2040. To keep the Kastle 2 family unified, we chose to stick with the RP2040—a reliable, well-known chip that delivers good enough performance for Kastle 2. Of course, you’re welcome to create your own Kastle 2 derivative using newer hardware if you wish!

## Will you release Alchemist as Open Source?

Right now we don’t plan to release the Alchemist source code to the public, since it’s packed with a lot of Bastl’s secret sauce that we might use in future projects.

## Can you share the board/gerber/case/panel files?

As a small company, Bastl Instruments relies on manufacturing and selling assembled units and kits. We’ve open-sourced the schematics, but the board design remains proprietary to help prevent straightforward product copying. We encourage you to develop your own designs and improvements, rather than simply duplicating the original.

The same applies to panel and case files: we provide general outlines and blank templates so you can create your own unique Kastle 2.

If you make Kastle 2-derived instruments, please remember to follow the license terms described in the [README](README.md).

## How to contribute to the Kastle 2?

You are free to open issues and pull requests; please name your branches accordingly (`feature/...`, `bugfix/...`). Before contributing please make sure you read [Toolchain Install Guide](TOOLCHAIN_INSTALL.md), [Coding Style Guide](CODING_STYLE.md) and [Glossary & Examples](GLOSSARY_EXAMPLES.md).

## The compiler says...

### Undefined reference.
Make sure you added the *.cpp file to CmakeLists.txt

### tusb.h: No such file or directory
You probably don't have Pico SDK GIT submodules initialized.
Go to the Pico SDK directory and call:
```
git submodule init
git submodule update
```
Then you have to configure cmake again, by running VS Code task **Configure** or manually.