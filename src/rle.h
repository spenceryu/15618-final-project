#include <vector>
#include <map>
#include <memory>
#include "image.h"

// Store (encoded, count) structs. Since macroblocks are 8x8 the char datatype
// (-128 to 127) is sufficient for storing this information.
struct RleTuple {
    char encoded;
    char count;
};

struct EncodedBlockColor {
    std::vector<RleTuple> encoded;
    std::map<char, double> decode_table;
    std::map<double, char> encode_table;
};

struct EncodedBlock {
    std::shared_ptr<EncodedBlockColor> y;
    std::shared_ptr<EncodedBlockColor> cr;
    std::shared_ptr<EncodedBlockColor> cb;
};

std::shared_ptr<EncodedBlock> RLE(std::vector<std::shared_ptr<PixelYcbcr>> block, int block_size);
std::vector<double> extractChannel(std::vector<std::shared_ptr<PixelYcbcr>> block, int chan);
void buildTable(std::vector<std::shared_ptr<PixelYcbcr>> block, int chan, std::map<double, char> freq, std::map<char, double> encodingTable, int block_size);
void encodeValues(std::vector<std::shared_ptr<PixelYcbcr>> block, std::shared_ptr<EncodedBlockColor> color, int chan);

std::vector<std::shared_ptr<PixelYcbcr>> DecodeRLE(std::shared_ptr<EncodedBlock> encoded, int block_size);
