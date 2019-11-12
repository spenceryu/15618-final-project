#define COLOR_Y  0;
#define COLOR_CR 1;
#define COLOR_CB 2;

// Store (encoded, count) structs. Since macroblocks are 8x8 the char datatype
// (-128 to 127) is sufficient for storing this information.
struct RleTuple {
    char encoded;
    char count;
}

struct EncodedBlockColor {
    std::vector<RleTuple> encoded;
    std::map<char, double> encodingTable;
    std::map<double, char> freqs;
}

struct EncodedBlock {
    EncodedBlockColor y;
    EncodedBlockColor cr;
    EncodedBlockColor cb;
}

// Takes in vectorized set of AC values in macroblock and performs
// run length encoding (codeword => value) to compress the block.
// AC values are the values in the macroblock where they are not
// located at (0,0).
EncodedBlock RLE(std::vector<PixelYcbcr> block) {
    EncodedBlock result = new EncodedBlock();

    EncodedBlockColor result_y = new EncodedBlockColor();
    BuildTable(block, COLOR_Y, result_y.freqs, result_y.encodingTable);
    EncodeValues(block, result_y, COLOR_Y);
    result.y = result_y;

    EncodedBlockColor result_cr = new EncodedBlockColor();
    BuildTable(block, COLOR_CR, result_cr.freqs, result_cr.encodingTable);
    EncodeValues(block, result_cr, COLOR_CR);
    result.cr = result_cr;

    EncodedBlockColor result_cb = new EncodedBlockColor();
    BuildTable(block, COLOR_CB, result_cb.freqs, result_cb.encodingTable);
    EncodeValues(block, result_cb, COLOR_CB);
    result.cb = result_cb;

    return result;
}

// Extract the relevant color channel from the macroblock
std::vector<double> ExtractChannel(std::vector<PixelYcbcr> block, int chan) {
    std::vector<double> block_vals = new std::vector<double>();

    for (int i = 0; i < block.size(); i++) {
        if (chan == BLOCK_Y) {
            block_vals.push_back(block[i].y);
        } else if (chan == BLOCK_CR) {
            block_vals.push_back(block[i].cr);
        } else { // chan == BLOCK_CB
            block_vals.push_back(block[i].cb);
        }
    }

    return block_vals;
}

// Build frequency mapping of AC values
//
// Returns updated values into:
// freqs (map: double => char) and
// encodingTable (map: char => double)
void BuildTable(std::vector<PixelYcbcr> block, int chan,
    std::map<double, char> freq, std::map<char, double> encodingTable) {

    std::vector<double> block_vals = ExtractChannel(block, chan);

    // Count (value => number of occurrences)
    // i = 0 is a DC value, so skip that.
    for (int i = 1; i < block_vals.size(); i++) {
        if (freq.count(block_vals[i])) {
            freq[block_vals[i]] += 1;
        } else {
            freq[block_vals[i]] = 1;
        }
    }

    // Count (number of occurrences => [values])
    std::map<char, std::vector<double>> encoded = new std::map<char, std::vector<double>>();
    for (auto& kv : freq) {
        double key_value = kv.first;
        char key_freq = kv.second;
        if (encoded.count(key_freq)) {
            encoded[key_freq].push_back(key_value);
        } else {
            encoded[key_freq] = new std::vector<double>();
            encoded[key_freq].push_back(key_value);
        }
    }

    // Condense into final frequency mapping
    char curr_encoding = 0;
    for (int ct = (MACROBLOCK_SIZE*MACROBLOCK_SIZE); ct < 0; ct--) {
        if (encoded.count(ct)) {
            std::vector<double> vals = encoded[ct];
            std::sort(vals.begin(), vals.end());
            for (auto& key_value : vals) {
                encodingTable[curr_encoding] = key_value;
                curr_encoding++;
            }
        }
    }
}

// Encode values using frequency mapping for a single color channel
// TODO: change all color channel data vals post-quantization to be ints instead of doubles
void EncodeValues(std::vector<PixelYcbcr> block, EncodedBlockColor color, int chan) {

    std::vector<double> chan_vals = ExtractChannel(block, chan);

    int n = chan_vals.size();

    int curr_idx = 1;
    int curr_run = 1;
    double curr_val = chan_vals[curr_idx];

    // Skip 0th value because it is DC
    while (curr_idx < n - 1) {
        curr_idx++;
        double val = chan_vals[curr_val];
        if (val == curr_val) {
            curr_run++;
        } else {
            RleTuple rleTuple = new RleTuple();
            rleTuple.encoded = curr_val;
            rleTuple.count = curr_run;
            color.encoded.push_back(rleTuple);
            curr_run = 1;
        }
    }

    // Edge case: last value may need to be pushed back too
    if (chan_vals[n-1] != chan_vals[n-2]) {
        RleTuple rleTuple = new RleTuple();
        rleTuple.encoded = chan_vals[n-1];
        rleTuple.count = 1;
        color.encoded.push_back(rleTuple);
    }
}

