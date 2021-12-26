const fs = require("fs");
const omg = require("omggif");
const { execSync } = require('child_process');

execSync("rm -rf data_compress")

// load gif
const gif_raw = fs.readFileSync("../assets/out-60.gif");
const gif_data = new omg.GifReader(gif_raw);
const num_pixel = gif_data.width * gif_data.height;
const num_frames = gif_data.numFrames();

const pallete = [];
var maxLength = 0;
var pallete_name;

var buffer = new Uint8Array(num_pixel * 4)
var prevFrame = new Uint8Array(num_pixel * 4)
var buffer_formatted = new Uint16Array(num_pixel);

for (let i=0; i<num_frames; i++) {
// for (let i=0; i<2; i++) {

    gif_data.decodeAndBlitFrameRGBA(i, buffer);

    let i3 = 0
    for (let i2=4; i2<buffer.length; i2 += 4) {
        // remove transparent
        if (buffer[i2-1] != 0) {
            prevFrame[i2-1] = buffer[i2-1]
            prevFrame[i2-2] = buffer[i2-2]
            prevFrame[i2-3] = buffer[i2-3]
            prevFrame[i2-4] = buffer[i2-4]
        }

        // pallete_name = `${prevFrame[i2-4]},${prevFrame[i2-3]},${prevFrame[i2-2]},${prevFrame[i2-1]}`

        // quick check if there is a non opaque pallete
        // if (buffer[i2-1] != 255 && buffer[i2-1] != 0) {
        //     // throw some error maybe? idk
        //     console.log(pallete_name)
        // }

        // add pallte if not exist
        // if (pallete.indexOf(pallete_name) == -1) pallete.push(pallete_name);
        
        buffer_formatted[i3] = rgb8(prevFrame[i2-4], prevFrame[i2-3], prevFrame[i2-2])

        i3++
    }

    // console.log(count1, count2)

    // // Write temp frame
    const temp = fs.openSync('temp', "w");
    fs.writeSync(temp, Buffer.from(buffer_formatted.buffer));
    fs.closeSync(temp);

    // // compress temp frame
    execSync("./cpp/lzss temp")

    const temp_new = fs.readFileSync("temp");
    maxLength = Math.max(temp_new.length, maxLength);
    fs.appendFileSync("data_compress", temp_new);

    console.log(`Convert frames ${i+1} of ${num_frames}`);
}

execSync("cp data_compress ../nitrofiles/")

// console.log(buffer_final)
// console.log(pallete)
console.log("Num pallete:", pallete.length)
console.log(maxLength);

function rgb8(r,g,b) {
    return (((r)>>3)|(((g)>>3)<<5)|(((b)>>3)<<10))
}