#include "../include/GView.hpp"
#include <zlib.h>
#include <iostream>

namespace GView::ZLIB
{
bool Decompress(const Buffer& input, uint64 inputSize, Buffer& output, uint64 outputSize)
{
    CHECK(input.IsValid(), false, "");
    CHECK(inputSize > 0, false, "");
    CHECK(outputSize > inputSize, false, "");

    output.Resize(outputSize);

    uint64 outputSizeCopy = outputSize;
    int32 ret             = uncompress(output.GetData(), (uLongf*) &outputSizeCopy, input.GetData(), static_cast<uLong>(inputSize));
    CHECK(outputSize == outputSizeCopy, false, "ZLIB error: %d!", ret);
    CHECK(ret == Z_OK, false, "ZLIB error: %d!", ret);

    return true;
}
bool DecompressStream(const Buffer& input, uint64 inputSize, Buffer& output, uint64 outputSize)
{
    CHECK(input.IsValid(), false, "");
    CHECK(inputSize > 0, false, "");
    CHECK(outputSize > inputSize, false, "");

    output.Resize(outputSize);
    int32 ret = uncompress(output.GetData(), (uLongf*) &outputSize, input.GetData(), static_cast<uLong>(inputSize));

    CHECK(ret == Z_OK, false, "ZLIB error: %d!", ret);
    output.Resize(outputSize);
    return true;
}
} // namespace GView::ZLIB
