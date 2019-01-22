const fs = require('fs');
const path = require('path');
const { toNumber, toHex, hexdump } = require('./common');

if (process.argv.length != 3) {
    console.error('\x1b[31m%s\x1b[0m', `Error: missing filename to simulate!`);
    console.error(`\n Usage: node ${path.basename(process.argv[1])} FILENAME\n`);
    process.exit(1);
}

// Read the file
const data = fs
    .readFileSync(process.argv[2])
    .toString();

let prgCode = data.match(/PROGMEM.*?\{([\s\S]+)\}/)[1];
if (!prgCode) {
    console.error('\x1b[31m%s\x1b[0m', `Error: file not containing program code!`);
    process.exit(1);
}

// Transform the code into an array of numbers: the main memory
const mem = prgCode
    .replace(/\/\*.*?\*\//g, '')
    .trim()
    .split(',')
    .map((val) => toNumber(val.trim()));

// Execution starts at location 0x0a
let pc = 10;
    
// Run the emulation
let steps = 0;
while(pc > 0) {
    steps++;
    console.log("--- (" + steps + ")");
    let a = mem[pc++] || 0;
    let b = mem[pc++] || 0;
    let c = mem[pc++] || 0;
    
    console.log(`DEBUG | a = ${a}, b = ${b}, c = ${c}`);
    console.log(`DEBUG | mem[a] = ${mem[a]}, mem[b] = ${mem[b]}, mem[c] = ${mem[c]}`);
    
    mem[b] = mem[b] - mem[a];
    
    console.log(`DEBUG | mem[b] = ${mem[b]}`);
    
    if (mem[b] <= 0) {
        console.log(`DEBUG | mem[b] <= 0 --> goto ${c}`);
        pc = c;
    }
}
    
console.log(`Done.\n`);

console.log(`Memory hexdump:`);
hexdump(mem);


console.log(`\nDisplay output:`);
console.log(toHex(mem[0x09], true) + toHex(mem[0x08], true) + " " + toHex(mem[0x07], true));
    