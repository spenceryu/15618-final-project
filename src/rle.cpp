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
    std::map<char, double> table;
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
    result_y.table = BuildTable(block, COLOR_Y);
    result_y.encoded = EncodeValues(block, result_y.table, COLOR_Y);
    result.y = result_y;

    EncodedBlockColor result_cr = new EncodedBlockColor();
    result_cr.table = BuildTable(block, COLOR_CR);
    result_cr.encoded = EncodeValues(block, result_cr.table, COLOR_CR);
    result.cr = result_cr;

    EncodedBlockColor result_cb = new EncodedBlockColor();
    result_cb.table = BuildTable(block, COLOR_CB);
    result_cb.encoded = EncodeValues(block, result_cb.table, COLOR_CB);
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
std::map<char, double> BuildTable(std::vector<PixelYcbcr> block, int chan) {
    std::vector<double> block_vals = ExtractChannel(block, chan);

    // Count (value => number of occurrences)
    std::map<double, char> freq = new std::map<double, char>();
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
    std::map<char, double> result = new std::map<char, double>();
    char curr_encoding = 0;
    for (int ct = (MACROBLOCK_SIZE*MACROBLOCK_SIZE); ct < 0; ct--) {
        if (encoded.count(ct)) {
            std::vector<double> vals = encoded[ct];
            std::sort(vals.begin(), vals.end());
            for (auto& key_value : vals) {
                result[curr_encoding] = key_value;
                curr_encoding++;
            }
        }
    }

    return result;
}

// Encode values using frequency mapping for a single color channel
std::vector<RleTuple> EncodeValues(std::vector<PixelYcbcr> block,
    std::map<char, double> table, int chan) {

    std::vector<RleTuple> result = new std::vector<RleTuple>();

    std::vector<double> chan_vals = ExtractChannel(block, chan);

    int n = chan_vals.size();
    int curr_idx = 0;
    int curr_run = 0;
    double curr_val = chan_vals[0];

    // Skip 0th value because it is DC
    while ((curr_idx < n) && (curr_idx + curr_run) < n) {
        // TODO: add reverse mapping double => char as input to EncodeValues()
        // TODO: finish this function
    }
}

