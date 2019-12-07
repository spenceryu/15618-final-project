#include <algorithm>
#include "rle.h"

// Takes in vectorized set of AC values in macroblock and performs
// run length encoding (codeword => value) to compress the block.
// AC values are the values in the macroblock where they are not
// located at (0,0).
std::shared_ptr<EncodedBlock> RLE(std::vector<std::shared_ptr<PixelYcbcr>> block, int block_size) {

    std::shared_ptr<EncodedBlock> result(new EncodedBlock());

    std::shared_ptr<EncodedBlockColor> result_y = buildTable(block, COLOR_Y, block_size);
    encodeValues(block, result_y, COLOR_Y);
    result->y = result_y;

    /*
    // Print statement: verify buildTable() is actually writing (fixed now...)
    std::shared_ptr<std::map<char, double>> tb = result_y->decode_table;
    std::map<char, double> tb_val = *(tb.get());
    for (std::map<char, double>::iterator iter = tb_val.begin();
        iter != tb_val.end(); ++iter) {
        fprintf(stdout, "Encoding: %d (encoded) %f (raw)\n", iter->first, iter->second);
    }
    */

    // Print statement: verify RleTuples are being encoded (Fixed now...)
    /*
    for (RleTuple a: (*(result_y->encoded).get())) {
        fprintf(stdout, "RleTuple: encoded %d count %d\n", a.encoded, a.count);
    }
    */

    std::shared_ptr<EncodedBlockColor> result_cr = buildTable(block, COLOR_CR, block_size);
    encodeValues(block, result_cr, COLOR_CR);
    result->cr = result_cr;

    std::shared_ptr<EncodedBlockColor> result_cb = buildTable(block, COLOR_CB, block_size);
    encodeValues(block, result_cb, COLOR_CB);
    result->cb = result_cb;

    return result;
}

std::vector<std::shared_ptr<PixelYcbcr>> decodeRLE(
    std::shared_ptr<EncodedBlock> encoded,
    int block_size) {

    std::vector<std::shared_ptr<PixelYcbcr>> result(block_size * block_size);
    for (unsigned int i = 0; i < result.size(); i++) {
        result[i] = std::make_shared<PixelYcbcr>();
    }

    // Decode y channel
    unsigned int y_idx = 0;
    std::shared_ptr<EncodedBlockColor> y_channel = encoded->y;
    std::shared_ptr<std::vector<RleTuple>> tups = y_channel->encoded;

    // Decode DC value first
    result[y_idx]->y = y_channel->dc_val;
    y_idx++;

    // Decode AC values after
    for (RleTuple tup : *tups) {
        std::shared_ptr<std::map<char, double>> decode_table = y_channel->decode_table;
        std::map<char, double> decode_table_ptr = *decode_table;
        char encoded = tup.encoded;
        char freq = tup.count;
        double decoded_val = decode_table_ptr[encoded];
        for (char c = 0; c < freq; c++) {
            result[y_idx]->y = decoded_val;
            y_idx++;
        }
    }

    // Decode cr channel
    unsigned int cr_idx = 0;
    std::shared_ptr<EncodedBlockColor> cr_channel = encoded->cr;
    tups = cr_channel->encoded;

    // Decode DC value first
    result[cr_idx]->cr = cr_channel->dc_val;
    cr_idx++;

    // Decode AC values after
    for (RleTuple tup : *tups) {
        std::shared_ptr<std::map<char, double>> decode_table = cr_channel->decode_table;
        std::map<char, double> decode_table_ptr = *decode_table;
        char encoded = tup.encoded;
        char freq = tup.count;
        double decoded_val = decode_table_ptr[encoded];
        for (char c = 0; c < freq; c++) {
            result[cr_idx]->cr = decoded_val;
            cr_idx++;
        }
    }

    // Decode cb channel
    unsigned int cb_idx = 0;
    std::shared_ptr<EncodedBlockColor> cb_channel = encoded->cb;
    tups = cb_channel->encoded;

    // Decode DC value first
    result[cb_idx]->cb = cb_channel->dc_val;
    cb_idx++;

    // Decode AC values after
    for (RleTuple tup : *tups) {
        std::shared_ptr<std::map<char, double>> decode_table = cb_channel->decode_table;
        std::map<char, double> decode_table_ptr = *decode_table;
        char encoded = tup.encoded;
        char freq = tup.count;
        double decoded_val = decode_table_ptr[encoded];
        for (char c = 0; c < freq; c++) {
            result[cb_idx]->cb = decoded_val;
            cb_idx++;
        }
    }

    return result;
}

// Extract the relevant color channel from the macroblock
std::vector<double> extractChannel(std::vector<std::shared_ptr<PixelYcbcr>> block, int chan) {
    std::vector<double> block_vals;

    for (unsigned int i = 0; i < block.size(); i++) {
        double val;
        if (chan == COLOR_Y) {
            val = block[i]->y;
        } else if (chan == COLOR_CR) {
            val = block[i]->cr;
        } else { // chan == COLOR_CB
            val = block[i]->cb;
        }

        if (val == -0.0) {
            val = 0.0;
        }
        block_vals.push_back(val);
    }

    return block_vals;
}

// Build frequency mapping of AC values
//
// Returns updated values into:
// freqs (map: double => char) and
// encodingTable (map: char => double)
std::shared_ptr<EncodedBlockColor> buildTable(
    std::vector<std::shared_ptr<PixelYcbcr>> block,
    int chan,
    int block_size) {

    std::shared_ptr<EncodedBlockColor> result = std::make_shared<EncodedBlockColor>();
    result->encoded = std::make_shared<std::vector<RleTuple>>();
    result->decode_table = std::make_shared<std::map<char,double>>();
    result->encode_table = std::make_shared<std::map<double,char>>();

    std::vector<double> block_vals = extractChannel(block, chan);

    // Count (value => number of occurrences)
    // i = 0 is a DC value, so skip that.
    std::map<double, char> freq;
    for (unsigned int i = 1; i < block_vals.size(); i++) {
        if (freq.count(block_vals[i])) {
            freq[block_vals[i]] += 1;
        } else {
            freq[block_vals[i]] = 1;
        }
    }

    // Count (number of occurrences => [values])
    std::map<char, std::vector<double>> encoded;
    char num_chars = 0; // number of unique vals to encode in the block
    std::vector<char> key_freqs;
    for (auto& kv : freq) {
        double key_value = kv.first;
        char key_freq = kv.second;
        encoded[key_freq].push_back(key_value);
        key_freqs.push_back(key_freq);
        num_chars++;
    }

    // sort in descending order
    std::sort(key_freqs.begin(), key_freqs.end(), std::greater<char>());

    // Condense into final frequency mapping
    // Encoded Value => Value
    char curr_encoding = 0;
    for (std::map<char, std::vector<double>>::iterator iter = encoded.begin(); iter != encoded.end(); ++iter) {
        // char key_freq = iter->first;
        std::vector<double> vals_to_encode = iter->second;
        // iterate through each key_freq
        for (double val_to_encode : vals_to_encode) {
            (*result->encode_table)[val_to_encode] = curr_encoding;
            (*result->decode_table)[curr_encoding] = val_to_encode;
            curr_encoding++;
        }
    }

    return result;
}


// Encode values using frequency mapping for a single color channel
void encodeValues(std::vector<std::shared_ptr<PixelYcbcr>> block, std::shared_ptr<EncodedBlockColor> color, int chan) {

    std::vector<double> chan_vals = extractChannel(block, chan);

    int n = chan_vals.size();
    std::shared_ptr<std::vector<RleTuple>> encoded_ptr = color->encoded;

    // Encode the DC value
    color->dc_val = chan_vals[0];

    int curr_idx = 1;
    int curr_run = 1;
    double curr_val = chan_vals[curr_idx];

    // Skip 0th value because it is DC
    while (curr_idx < n - 1) {
        curr_idx++;
        double val = chan_vals[curr_idx];
        if (val == curr_val) {
            curr_run++;
        } else {
            RleTuple rleTuple;
            rleTuple.encoded = (*color->encode_table)[curr_val];
            rleTuple.count = curr_run;
            (*encoded_ptr).push_back(rleTuple);
            curr_run = 1;
            curr_val = chan_vals[curr_idx];
        }
    }

    // Edge case: pushing back last value
    // Case 1: last value is different
    RleTuple rleTuple;
    rleTuple.encoded = (*color->encode_table)[chan_vals[n-1]];
    if (chan_vals[n-1] != chan_vals[n-2]) {
        rleTuple.count = 1;
    } else { // Case 2: last value is the same
        rleTuple.count = curr_run;
    }
    (*color->encoded).push_back(rleTuple);
}

// Write the encoded blocks  without pointers from the
// worker to buffer for MPI send back to master
void writeToBuffer(
    EncodedBlockNoPtr* encodedBlockBuffer,
    std::vector<std::shared_ptr<EncodedBlock>> encodedBlocks,
    int idx, int chan) {

    // Get encoded block from vector of encoded blocks
    std::shared_ptr<EncodedBlock> encodedBlock = encodedBlocks[idx];

    // Write the DC value to the buffer
    encodedBlockBuffer[idx].y.dc_val = encodedBlock.dc_val;

    for (unsigned int i = 0; i < encodedBlock.size(); i++) {
        if (chan == COLOR_Y) {
            // Write the encoded channel values to the buffer
            for (unsigned int j = 0; j < encodedBlock.y.encoded.size(); j++) {
                encodedBlockBuffer[idx].y.encoded[j].encoded = encodedBlock->y->encoded[j].encoded;
                encodedBlockBuffer[idx].y.encoded[j].count = encodedBlock->y->encoded[j].count;
            }
            // Write the encoded channel length to the buffer
            encodedBlockBuffer[idx].y.encoded_len = encodedBlock.y.encoded.size();
            // Write the dict: chars + doubles pairs to the buffer
            char kv_idx = 0;
            for (std::map<char, double>::iterator iter = encodedBlock->encode_table.begin();
                iter != encodedBlock->encode_table.end(); ++iter) {
                char char_val = iter->first;
                double double_val = iter->second;
                encodedBlockBuffer[idx].y.char_vals[kv_idx] = char_val;
                encodedBlockBuffer[idx].y.double_vals[kv_idx] = double_val;
                kv_idx++;
            }
            // Write the table size to the buffer
            encodedBlockBuffer[idx].y.table_size = kv_idx;
        }
    } // TODO add two more color switches

}
