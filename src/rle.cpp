#define COLOR_Y  0
#define COLOR_CR 1
#define COLOR_CB 2

#include <algorithm>
#include "rle.h"

// Takes in vectorized set of AC values in macroblock and performs
// run length encoding (codeword => value) to compress the block.
// AC values are the values in the macroblock where they are not
// located at (0,0).
std::shared_ptr<EncodedBlock> RLE(std::vector<std::shared_ptr<PixelYcbcr>> block, int block_size) {

    std::shared_ptr<EncodedBlock> result(new EncodedBlock());

    std::shared_ptr<EncodedBlockColor> result_y(new EncodedBlockColor());
    buildTable(block, COLOR_Y, result_y->freqs, result_y->encodingTable, block_size);
    encodeValues(block, result_y, COLOR_Y);
    result->y = result_y;

    std::shared_ptr<EncodedBlockColor> result_cr(new EncodedBlockColor());
    buildTable(block, COLOR_CR, result_cr->freqs, result_cr->encodingTable, block_size);
    encodeValues(block, result_cr, COLOR_CR);
    result->cr = result_cr;

    std::shared_ptr<EncodedBlockColor> result_cb(new EncodedBlockColor());
    buildTable(block, COLOR_CB, result_cb->freqs, result_cb->encodingTable, block_size);
    encodeValues(block, result_cb, COLOR_CB);
    result->cb = result_cb;

    return result;
}

std::vector<std::shared_ptr<PixelYcbcr>> DecodeRLE(std::shared_ptr<EncodedBlock> encoded, int block_size) {

    std::vector<std::shared_ptr<PixelYcbcr>> result(block_size * block_size);
    for (unsigned int i = 0; i < result.size(); i++) {
        result[i] = std::make_shared<PixelYcbcr>();
    }

    /*
    // Decode y channel
    unsigned int y_idx = 0;
    for (RleTuple tup : encoded->y->encoded) {
        double decoded_val = encoded->y->encodingTable[tup.encoded];
        char freq = tup.count;
        for (char c = 0; c < freq; c++) {
            result[y_idx]->y = decoded_val;
            y_idx++;
        }
    }

    // Decode cr channel
    unsigned int cr_idx = 0;
    for (RleTuple tup : encoded->cr->encoded) {
        double decoded_val = encoded->cr->encodingTable[tup.encoded];
        char freq = tup.count;
        for (char c = 0; c < freq; c++) {
            result[y_idx]->cr = decoded_val;
            cr_idx++;
        }
    }

    // Decode cb channel
    unsigned int cb_idx = 0;
    for (RleTuple tup : encoded->cb->encoded) {
        double decoded_val = encoded->cb->encodingTable[tup.encoded];
        char freq = tup.count;
        for (char c = 0; c < freq; c++) {
            result[cb_idx]->cb = decoded_val;
            cb_idx++;
        }
    }
    */

    return result;
}

// Extract the relevant color channel from the macroblock
std::vector<double> extractChannel(std::vector<std::shared_ptr<PixelYcbcr>> block, int chan) {
    std::vector<double> block_vals;

    for (unsigned int i = 0; i < block.size(); i++) {
        if (chan == COLOR_Y) {
            block_vals.push_back(block[i]->y);
        } else if (chan == COLOR_CR) {
            block_vals.push_back(block[i]->cr);
        } else { // chan == COLOR_CB
            block_vals.push_back(block[i]->cb);
        }
    }

    return block_vals;
}

// Build frequency mapping of AC values
//
// Returns updated values into:
// freqs (map: double => char) and
// encodingTable (map: char => double)
void buildTable(std::vector<std::shared_ptr<PixelYcbcr>> block, int chan, std::map<double, char> freq, std::map<char, double> encodingTable, int block_size) {

    std::vector<double> block_vals = extractChannel(block, chan);

    // Count (value => number of occurrences)
    // i = 0 is a DC value, so skip that.
    for (unsigned int i = 1; i < block_vals.size(); i++) {
        if (freq.count(block_vals[i])) {
            freq[block_vals[i]] += 1;
        } else {
            freq[block_vals[i]] = 1;
        }
    }

    // Count (number of occurrences => [values])
    std::map<char, std::vector<double>> encoded;
    for (auto& kv : freq) {
        double key_value = kv.first;
        char key_freq = kv.second;
        encoded[key_freq].push_back(key_value);
    }

    // Condense into final frequency mapping
    char curr_encoding = 0;
    for (int ct = (block_size*block_size); ct < 0; ct--) {
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
void encodeValues(std::vector<std::shared_ptr<PixelYcbcr>> block, std::shared_ptr<EncodedBlockColor> color, int chan) {

    std::vector<double> chan_vals = extractChannel(block, chan);

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
            RleTuple rleTuple;
            rleTuple.encoded = curr_val;
            rleTuple.count = curr_run;
            color->encoded.push_back(rleTuple);
            curr_run = 1;
        }
    }

    // Edge case: last value may need to be pushed back too
    if (chan_vals[n-1] != chan_vals[n-2]) {
        RleTuple rleTuple;
        rleTuple.encoded = chan_vals[n-1];
        rleTuple.count = 1;
        color->encoded.push_back(rleTuple);
    }
}

