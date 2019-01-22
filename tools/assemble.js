const fs = require('fs');
const path = require('path');
const { toHex } = require('./common');

// Make sure that a filename is given which contains the
// code to assemble ...
if (process.argv.length != 3) {
    console.error('\x1b[31m%s\x1b[0m', `Error: missing filename to assemble!`);
    console.error(`\n Usage: node ${path.basename(process.argv[1])} FILENAME\n`);
    process.exit(1);
}

// Map containing all used variables, their location in memory and
// an initial value (optional). Z is the zero register and exists always
const variableMap = {
    'z': { loc: 0x00, init: 0 }
};

// Map of label name to position in memory where it starts, e.g.
// labelMap = { 'loop': 25 } // loop starts at memory location 25
const labelMap = { };

// Starting location where the first byte of the program is put
const offset = process.env.PRG_OFFSET || 0x0a;

// Step 1: parse the file and extract the raw program
// ---
const rawProgram = fs
    .readFileSync(process.argv[2])
    .toString()
    .trim()    
    .toLowerCase()
    // Split the text into an array of lines
    .split('\n')
    // Cleanup every line
    .map((ln) => ln.trim())
    // Remove comments
    .map((ln) => {
        let index;
        if ((index = ln.indexOf(';')) > -1) {
            return ln.substring(0, index).trim();
        }
        return ln;
    })
    // Save variable assignments (definitions)
    .map((ln) => {
        if (ln.startsWith('.def')) {
            const def = ln.substring(5).split(' ').filter((arg) => arg.trim().length > 0);
            variableMap[def[0]] = {
                loc: parseInt(def[1], def[1].indexOf('x') > -1 ? 16 : 10),
                init: def.length > 2 ? parseInt(def[2], def[2].indexOf('x') > -1 ? 16 : 10) : undefined
            }
            return '';
        }
        return ln;
    })
    // Remove all empty lines
    .filter((ln) => ln.length > 0)
    // Split all instructions into an array of arguments
    .map((ln) => ln.trim().split(/[ |,]/).filter((arg) => arg.length > 0));

// Step 2: transform all "macros" & instructions into the CPU instructions
// ---
let instructions = [];

// The memory pointer points to the memory address where the code
// is located at
let memPtr = 0 + offset; 

for (let i = 0; i < rawProgram.length; i++) {
    
    // Handle labels
    if (rawProgram[i][0].match(/([a-z_]+):/)) {
        const labelName = rawProgram[i][0].substring(0, rawProgram[i][0].indexOf(':')).trim();
        if (labelName in labelMap) {
            console.error('\x1b[31m%s\x1b[0m', `Error: label "${labelName}" already registered!`);
            process.exit(1);
        }
        labelMap[labelName] = memPtr;
    } else 
    // Pseudo instruction: move
    if (rawProgram[i][0] === 'mov') {
        // MOV a, b : copies data from mem[a] into mem[b]
        let a = parseVar(rawProgram[i][1]);
        let b = parseVar(rawProgram[i][2]);
        
        // subleq b, b, <next_instruction_location>
        instructions.push(b); memPtr++;
        instructions.push(b); memPtr++;
        instructions.push(++memPtr);
        // subleq a, Z, <next_instruction_location>
        instructions.push(a); memPtr++;
        instructions.push(variableMap.z.loc); memPtr++;
        instructions.push(++memPtr);
        // subleq Z, b, <next_instruction_location>
        instructions.push(variableMap.z.loc); memPtr++;
        instructions.push(b); memPtr++;
        instructions.push(++memPtr);
        // subleq Z, Z, <next_instruction_location>
        instructions.push(variableMap.z.loc); memPtr++;
        instructions.push(variableMap.z.loc); memPtr++;
        instructions.push(++memPtr);
    } else 
    // Pseudo instruction: add
    if (rawProgram[i][0] === 'add') {
        // ADD a, b : mem[b] = mem[a] + mem[b]
        let a = parseVar(rawProgram[i][1]);
        let b = parseVar(rawProgram[i][2]);
        
        // subleq a, Z, <next_instruction_location>
        instructions.push(a); memPtr++;
        instructions.push(variableMap.z.loc); memPtr++;
        instructions.push(++memPtr);
        // subleq Z, b, <next_instruction_location>
        instructions.push(variableMap.z.loc); memPtr++;
        instructions.push(b); memPtr++;
        instructions.push(++memPtr);
        // subleq Z, Z, <next_instruction_location>
        instructions.push(variableMap.z.loc); memPtr++;
        instructions.push(variableMap.z.loc); memPtr++;
        instructions.push(++memPtr);
    } else
    // Instruction subleq
    if (rawProgram[i][0] === 'subleq') {
        instructions.push(parseVar(rawProgram[i][1])); memPtr++;
        instructions.push(parseVar(rawProgram[i][2])); memPtr++;
        instructions.push(parseVar(rawProgram[i][3])); memPtr++;
    } else 
    // Pseudo instruction: halt
    if (rawProgram[i][0] === 'hlt') {
        // Halts the CPU: by definition a JMP to location -1 is a halt
        instructions.push(0); memPtr++;
        instructions.push(0); memPtr++;
        instructions.push(-1); memPtr++;
    } else {
        console.error('\x1b[31m%s\x1b[0m', `Error: unkown instruction: ${rawProgram[i][0]}`);
        process.exit(1);
    }
}

function parseVar(x) {
    // If a number is given, return it; either hex or decimal
    if (x.match(/^[0-9x]+$/)) {
        return parseInt(x, x.indexOf('x') > -1 ? 16 : 10);
    }
    
    // If the variable name exists, return the value
    return (x in variableMap) ? variableMap[x].loc : x;
}

// Step 3: now that all instructions are known, all labels and their exact
// positions are also known. The following loop replaces all lables with 
// the correct locations in memory from the label map
// ---
for (let i = 0; i < instructions.length; i++) {
    if ((typeof instructions[i]) === 'string') {
        instructions[i] = labelMap[instructions[i]];
        
        if (!instructions[i]) {
           console.error('\x1b[31m%s\x1b[0m', `Error: label "${x}" not defined!`);
           process.exit(1);
        }
    }
}

// Step 4: Create a virtual memory to put the program into
// ---
const mem = new Array(256);
for (let i = 0; i < mem.length; i++) {
    mem[i] = 0x00;
}

if (instructions.length > (256 - offset)) {
    console.error('\x1b[31m%s\x1b[0m', `Error: program too long for 256 bytes of memory!`);
    console.error('\x1b[31m%s\x1b[0m', `offset=${offset}, program length=${instructions.length}`);
    process.exit(1);
}

// Copy the program
for (let i = 0; i < instructions.length; i++) {
    mem[offset + i] = instructions[i];
}

// Copy the variables
for (let varname in variableMap) {
    if (variableMap[varname].init && variableMap[varname].init >= 0) {
        mem[variableMap[varname].loc] = variableMap[varname].init;
    }
}

// Step 5: Finished! We now only need to output the assembled program
// ---

console.log(`/*`)
console.log(` --- Report ---`);
console.log(`  Program starts at: ${toHex(offset)}`);
console.log(`  Code length: ${instructions.length} bytes`);
console.log(`\n  Variables:`);
for (let varname in variableMap) {
    console.log(`   - ${varname} at ${toHex(variableMap[varname].loc)} (initial value: ${variableMap[varname].init})`);
}

console.log(`\n  Labels:`);
for (let label in labelMap) {
    console.log(`   - ${label} at ${labelMap[label]}`);
}
console.log(`*/\n`);

// Output the data as C-header file
console.log(`unsigned const PROGMEM char prg[256] = {`);
console.log('/*        0x00  0x01  0x02  0x03  0x04  0x05  0x06  0x07  0x08  0x09  0x0a  0x0b  0x0c  0x0d  0x0e  0x0f  */');
    
let lnBuf = '/*0x00*/  ';
for (let i = 0; i < 256; i++) {
    if (i%16 == 0 && i > 0) {
        console.log(lnBuf);
        lnBuf = `/*${toHex(i)}*/  `;
    }
    
    lnBuf += `${toHex(mem[i])}, `;
}
console.log(lnBuf);
    
console.log(`};`);
