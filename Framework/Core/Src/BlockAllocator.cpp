#include "Precompiled.h"
#include "BlockAllocator.h"
#include "DebugUtil.h"

using namespace Engine;
using namespace Engine::Core;

BlockAllocator::BlockAllocator(const char* name, std::size_t blockSize, std::size_t capacity)
    : mName(name)
    , mFreeBlocks(capacity)
    , mBlockSize(blockSize)
    , mCapacity(capacity)
{
    ASSERT(blockSize > 0, "BlockAllocator: %s invalid block size!", mName.c_str());
    ASSERT(capacity > 0, "BlockAllocator: %s invalid capacity!", mName.c_str());

    mData = std::malloc(blockSize * capacity);

    for (std::size_t i = 0; i < capacity; ++i)
    {
        mFreeBlocks[i] = static_cast<uint8_t*>(mData) + (i * blockSize);
    }
    LOG("%s allocated %zu blocks, blocks size: %zu", mName.c_str(), capacity, blockSize);
}

BlockAllocator::~BlockAllocator()
{
    ASSERT(mBlocksAllocatedTotal == mBlocksFreed, "BlockAllocator: %s not all blocks are freed!", mName.c_str());
    std::free(mData);
    mData = nullptr;
    LOG("%s destructed, Allocated: %zu, Freed %zu, Highest: %zu",
        mName.c_str(), mBlocksAllocatedCurrent, mBlocksFreed, mBlocksHighest);
}

void* BlockAllocator::Allocate()
{
    if (mFreeBlocks.empty())
    {
        LOG("%s allocation failed, no free blocks available!", mName.c_str());
        return nullptr;
    }

    void* freeBlock = mFreeBlocks.back();
    mFreeBlocks.pop_back();

    ++mBlocksAllocatedTotal;
    ++mBlocksAllocatedCurrent;
    mBlocksHighest = std::max(mBlocksHighest, mBlocksAllocatedCurrent);

    LOG("%s allocated blocks at %p, Allocated: %zu, Freed %zu, Highest: %zu",
        mName.c_str(), freeBlock, mBlocksAllocatedCurrent, mBlocksFreed, mBlocksHighest);

    return freeBlock;
}

void BlockAllocator::Free(void* ptr)
{
    if (ptr == nullptr)
    {
        LOG("%s free failed, null pointer provided!", mName.c_str());
        return;
    }

    const uint8_t* start = static_cast<uint8_t*>(mData);
    const uint8_t* end = static_cast<uint8_t*>(mData) + (mBlockSize * mCapacity);
    const uint8_t* current = static_cast<uint8_t*>(ptr);
    const auto diff = current - start;
    ASSERT(current >= start && current < end && static_cast<std::size_t>(diff) % mBlockSize == 0,
        "BlockAllocator: %s free failed, Invalid address being freed!", mName.c_str());

    LOG("%s free %p", mName.c_str(), ptr);
    --mBlocksAllocatedCurrent;
    ++mBlocksFreed;
    mFreeBlocks.push_back(ptr);
}
