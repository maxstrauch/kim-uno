# KIM Uno

_A portable, software defined dev kit for (retro) microprocessors_

![KIM Uno](https://raw.github.com/maxstrauch/kim-uno/master/images/frontshot.jpg)

Back in late 2018 it came to my mind, that I wanted to build a small portable microprocessor dev kit, just like the famous [KIM-1](https://en.wikipedia.org/wiki/KIM-1) from MOS Technology, Inc. and designed by Chuck Peddle who was also involved in creating the 6502 CPU.

Therefore, I designed the KIM Uno as a portable device, which fits in one hand and is powered by two CR2032 batteries. It uses the ATMega328p ("Arduino") microcontroller running at 8 MHz to emulate (or simulate) a desired CPU. This architecture also makes sure, that the emulated CPUs are interchangeable to anything which fits inside the microcontroller's flash memory. So it is a multi-purpose device.

Currently this project emulates an [OISC](https://en.wikipedia.org/wiki/One_instruction_set_computer) with just one instruction `subleq`. The idea came to me when I watched [The Ultimate Apollo Guidance Computer Talk](https://www.youtube.com/watch?v=xx7Lfh5SKUQ) and then discovered [this GitHub repo](https://github.com/davidar/subleq).

There is another pretty interesting project, called the same (KIM Uno), which does a real replica of the 6502 KIM Uno. Check it out [here](https://obsolescence.wixsite.com/obsolescence/kim-uno-summary-c1uuh). The creator even sells the kit. So if you are interested in 6502 and like this project, you should take a look there!

# Links

You can find:

 - a very detailed series of build instructions on Instructable: https://www.instructables.com/id/The-KIM-Uno-a-5-Microprocessor-Dev-Kit-Emulator/
 - a video demonstrating the device on YouTube: https://youtu.be/oWd1w513VB8

# Contents of this repository

This repository contains all needed documents, to create your own KIM Uno. In the root of the repository you can find the following important documents:

 - `bom_kim-uno-rev1.ods` - A bill of materials and rough calculation of the material cost (it's about 4,75 € as of 2018) and links to the components on Ebay. The offers on Ebay might be closed (depends on when you read this here) but this might be a good starting point for sourcing the components.
 - `kim-uno-rev1_2018-12-12_gerbers.zip` - A zip file containing the latest Gerber and drill files for production. I send exactly those files out to PCBWay to produce the PCBs. The files are exported to the specifications of PCBWay (see https://www.pcbway.com/blog/help_center/Generate_Gerber_file_from_Kicad.html) but might work also for another manufacturer.
 - `kim-uno-rev1-schematic.pdf` - The schematic of the project. [Show schematics.](https://raw.github.com/maxstrauch/kim-uno/master/kim-uno-rev1-schematic.pdf)
 - `arduino-isp.ino` - An Arduino sketch of the ISP code I use on my Arduino UNO to program the microcontrollers

This are the essential three documents you'll need to reproduce my KIM Uno. Furthermore, the following directories can be found in the repository:

 - `images` - Just images of the project
 - `kicad` - The original KiCad files, so that you can make modifications as you like (see also the License section at the end)
 - `src` - The source code for the microcontroller firmware; you can either use `src/main.hex` and burn it onto your ATMega328p or you can modify `src/main.c`, recompile it (see `src/Makefile`) and then burn it with your own changes
 - `tools` - A little NodeJS based assembler and simulator of the KIM Uno

# Flashing the firmware

In the directory `src/` you can find the source code for the emulator along with an already compiled `src/main.hex` which you can directly upload to the microcontroller.

Before you can actually use it, you need to set the fuse bits of the microcontroller, so that it uses the internal 8 MHz clock without dividing it in half. You can get the job done with the following command:

    avrdude -c stk500v1 -b 9600 -v -v -P /dev/cu.usbmodem1421 -p m328p -U lfuse:w:0xe2:m -U hfuse:w:0xd9:m -U efuse:w:0xff:m

After this you can flash the firmware onto the microcontroller with this command:

    avrdude -c stk500v1 -b 9600 -v -v -P /dev/cu.usbmodem1421 -p m328p -U flash:w:main.hex

Maybe you need to change the argument for `-P`.

You can also use the `src/Makefile` to do this tasks. Run `make all` to compile (GNU AVR toolchain required to be installed) and `make install` to flash it. You can use `make fuse` to set the fuses correctly.

# Theory of operation

## Basics

The KIM Uno does emulate a One Instruction Set Computer (OISC) and provides the subleq instruction to perform computation.

The subleq instruction stands for subtract and branch if less than or equal to zero. In pseudo-code this looks like the following:

    subleq A B C: mem[B] = mem [B] - mem[A]; 
                  if (mem[B] <= 0) goto C;

Since the KIM Uno emulates a 8-bit machine, all arguments A, B and C are 8 bit values and therefore it can address a total main memory of 256 byte. Obviously this can be extended, by making A, B and C multi-byte values. But for now let's keep it simple.

The KIM Uno has also "peripherals": the display and keyboard. It uses a memory mapped architecture to interface those peripherals, although the memory map is very simple:

 - 0x00 = the Z register (zero) and should be kept zero.
 - 0x01 - 0x06 = six bytes which represent the value of every of the display segments (from right to left). A value <= 0xf (16) can be placed in one of this memory locations and it will appear on the seven segment display. There are some special characters to show which can be displayed setting a value > 0xf - see the source code (main.c) for more details.
 - 0x07, 0x08, 0x09 = three bytes where every byte represents two seven segment displays (from right to left). This memory locations allow simply displaying a result without splitting up the result into two nibbles to place it in the single digit memory locations 0x01 - 0x06.
 - 0x0a+ = A program starts at 0x0a. Currently the "Go" key executes from 0x0a fixed.

## Pseudo instructions

There are three "pseudo instructions" which make life easier when programming for the KIM Uno. They are composed of multiple `subleq` instructions:

**MOV a, b - copy data at location a to b can be composed of:**

    subleq b, b, 2 (next instruction)
    subleq a, Z, 3 (next instruction)
    subleq Z, b, 4 (next instruction)
    subleq Z, Z, e.g. 5 (next instruction)

Using the subtraction feature of `subleq`, which `does mem[b] - mem[a]` and overwrites `mem[b]` with the result, the value is copied using the zero register. And `subleq Z, Z, ...` simply resets the zero register to 0, regardless of the value of Z.

**ADD a, b - adds the values a + b and stores the sum in b can be composed of:**

    subleq a, Z, 2 (next instruction)
    subleq Z, b, 3 (next instruction)
    subleq Z, Z, e.g. 4 (next instruction)

This instruction simply calculates `mem[b] - (- mem[a])` which is `mem[b] + mem[a]` by also using the subtraction feature.

**HLT - halts the CPU and ends the execution:**

By definition the emulator knows that the CPU wants to terminate if it jumps to 0xff (or -1 if singed). So a simple

    subleq Z, Z, -1

does the job and indicates to the emulator, that it should end emulation.

## Fibonacci example program

With this information one can now write a program in assembler, for example Fibonacci's algorithm:


    .def n   0xfa 6             ; Input n for fibonacci
    .def a   0xfc 0
    .def b   0xfd 1
    .def out 0x07 0             ; Output register for display

    ; Temp registeres, needed since the subleq instructions modify the original
    ; registers (the program might be optimized to use only one tmp register)
    .def tmp1 0xfe 0
    .def tmp2 0xff 0

    ; Constant values. Since subleq has no immediate instructions we store
    ; those values in a register
    .def Z   0x00 0
    .def one 0xfb 1
    .def two 0xf9 2

        mov n, tmp1             ; Copy n to tmp1 to perform calculation on it
        subleq two, tmp1, done  ; If n <= 2, we're done and goto the end
        subleq one, n, loop     ; Start computation with n := n-1

    loop:
        mov a, tmp1             ; Save a and ...
        mov b, tmp2             ; ... b to perform the addition on it
        add tmp1, tmp2          ; Save the sum of a and b on tmp2
        mov b, a                ; Set a to the value of b
        mov tmp2, b             ; Set b to tmp2 which is a + b
        subleq one, n, done     ; Decrement n
        subleq Z, Z, loop       ; Check if n > 0 and continue computation

    done:
        mov b, out              ; Finished ;-) - output the value on b
        hlt                     ; Halt the CPU

This is basically the content of `tools/fibonacci/fibonacci.s`. This program can now be turned into `subleq` CPU instructions and entered into the memory of the KIM Uno and then executed. Since there is only one instruction, only the arguments (A, B and C) are entered. So after three memory locations the next instruction arguments begin and so forth.

But there is a faster way, shown in the next section.

## Further tools

The `tools/` directory contains some basic assembler and simulator for the KIM Uno. They are written in [NodeJS](https://nodejs.org/en/) - make sure that you have installed it.

**Assembling programs**

If you take a look at "tools/fibonacci/fibonacci.s" you'll find that it is the source code for the discussed fibonacci implementation. To assemble it and make a program out of it, that the KIM Uno can run, you enter the following command (in `tools/`):

    node assemble.js fibonacci/fibonacci.s

and it will either print an error if you made a mistake or spill out the resulting program. To save it, you can copy the output and save it to a file or simply run this command:

    node assemble.js fibonacci/fibonacci.s > yourfile.h

The output is formatted in a way that it can be directly included in the KIM Uno firmware as a C header file, but the simulator can also use it to simulate. Simply enter:

    node sim.js yourfile.h

And you will be presented with the simulation result and the output expected from the KIM Uno on the display.

# Conclusion

## Design flaws in the PCB design

After completing this project I found a couple of points which are noteworthy and should be addressed in a new revision of the PCB board:

- the ATMega328p's silk screen has not the usual notch where the first pin is located. The DIP-28 footprint does not even have a square pad where the first pin is located. This should definitely be improved with a more detailed silkscreen to prevent confusion
- the ISP header has no connection labels on the silk screen. This makes it difficult to recognize how to connect it to the ISP
- the ISP header could be changed into a 2x6 pin header with a standard pin layout to prevent any confusion

Apart from those points I'm pretty happy how it turned out and worked on the first try.

## Further uses

But the journey does not end here - there are an infinite number of options how you could modify the KIM Uno and customize it to your needs and liking.

For example the KIM Uno could be equipped with a "real" retro CPU emulator which might emulate the famous MOS 6502 or Intel 8085, 8086 or 8088. Then it would go the way to my initial vision, before I learned about OISCs.

But there are possible other uses, since the hardware design is pretty generic. The KIM Uno could be used as:

- a remote controller e.g. for CNCs or other devices. Maybe wired or equipped with an IR diode or any other wireless sender
- a (hexadecimal) pocket calculator. The firmware can be adapted very easily and the board design does not need to be changed very much. Maybe the silkscreen can be adapted with math operations and the gap between the segments can be removed. Apart from this, it is already ready for this transformation


_**Have fun with it. Cheers!**_


# License

Creative Commons Attribution-ShareAlike 4.0 International (CC BY-SA 4.0). Click [here](http://creativecommons.org/licenses/by-sa/4.0/) for details. The license applies to the entire source code.

`This program is distributed WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.`
