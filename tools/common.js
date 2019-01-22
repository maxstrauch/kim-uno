module.exports = { toHex, toNumber, hexdump };

function toHex(val, omitPrefix) {
    const raw = Number((val>>>0) & 0xff).toString(16);
    return `${omitPrefix === true ? '':'0x'}${raw.length < 2 ? '0':''}${raw}`;
}

function toNumber(val) {
    if (val.indexOf('0x') > -1) {
        return parseInt(val, 16);
    } else {
        return parseInt(val, 10);
    }
}

function hexdump(mem) {
    console.log('      00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f');

    let lnBuf = '0000  ';
    for (let i = 0; i < mem.length; i++) {
        if (i%16 == 0 && i > 0) {
            console.log(lnBuf);
            lnBuf = '00' + toHex(i, true) + '  ';
        }
        
        lnBuf += `${toHex(mem[i], true)} `;
    }
    console.log(lnBuf);
}