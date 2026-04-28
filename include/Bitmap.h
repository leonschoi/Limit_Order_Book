#pragma once
#include <bit>
#include <bitset>
#include <cassert>
#include <climits>
#include <cstdint>
#include <iostream>


// WORD_SIZE must be a power of 2 for the bit manipulation to work correctly
constexpr size_t WORD_SIZE = 64;              // bits per word (size_t)
constexpr size_t WORD_SIZE_1 = WORD_SIZE - 1; // mask for bit index within word

// Number of bits needed to index within a word (6 for 64-bit words) = log2(WORD_SIZE)
// [0, WORD_SIZE) can be represented with WORD_INDEX_BITS bits
// Bits used in WORD_SIZE_1 are exactly WORD_INDEX_BITS
// This works because WORD_SIZE is a power of 2
constexpr size_t WORD_INDEX_BITS = std::bit_width(WORD_SIZE_1);


struct Bitmap {
    static_assert((WORD_SIZE & (WORD_SIZE-1)) == 0, "WORD_SIZE must be a power of 2");

    size_t bitmap_size;
    size_t* words;

    Bitmap(size_t price_levels) : bitmap_size((price_levels + WORD_SIZE_1) / WORD_SIZE) {
        words = new size_t[bitmap_size]{}; 
    }
    ~Bitmap() {
        delete[] words;
    }

    void set(size_t idx) {
        // words[idx / WORD_SIZE] |= (1ULL << (idx % WORD_SIZE));
        words[idx >> WORD_INDEX_BITS] |= (1ULL << (idx & WORD_SIZE_1));
    }

    void clear(size_t idx) {
        // words[idx / WORD_SIZE] &= ~(1ULL << (idx % WORD_SIZE));
        words[idx >> WORD_INDEX_BITS] &= ~(1ULL << (idx & WORD_SIZE_1));
    }

    bool test(size_t idx) const {
        // return words[idx / WORD_SIZE] & (1ULL << (idx % WORD_SIZE));
        return words[idx >> WORD_INDEX_BITS] & (1ULL << (idx & WORD_SIZE_1));
    }

    int first() const {
        for (int w = 0; w < bitmap_size; ++w) {
            if (words[w]) {
                // return w * WORD_SIZE + __builtin_ctzll(words[w]);
                return (w << WORD_INDEX_BITS) + lsb(words[w]);
            }
        }
        return -1;
    }

    int last() const {
        for (int w = bitmap_size-1; w >= 0; --w) {            
            if (words[w]) {
                // return w * WORD_SIZE + ((WORD_SIZE-1) - __builtin_clzll(words[w]));
                return (w << WORD_INDEX_BITS) + msb(words[w]);
            }
        }
        return -1;
    }

    int lsb(size_t x) const { return std::countr_zero(x); }
    int msb(size_t x) const { return WORD_SIZE_1 - std::countl_zero(x); }
    /*
    // for WORD_SIZE < 64
    int msb(size_t x) const { 
        int unused_left_bits = sizeof(x) * CHAR_BIT - WORD_SIZE;
        return WORD_SIZE_1 - (std::countl_zero(x) - unused_left_bits); 
    }
    */
};


struct Bitmap2Level {
    static_assert((WORD_SIZE & (WORD_SIZE-1)) == 0, "WORD_SIZE must be a power of 2");

    size_t bitmap_size;
    size_t* level1;                     // actual bits
    size_t level0;                      // summary of non-empty words

    // 64 * 64 = 4096 price levels max with 2-level bitmap
    Bitmap2Level(size_t price_levels) : bitmap_size((price_levels + WORD_SIZE_1) / WORD_SIZE) {
        assert(price_levels <= 4096 && "price_levels must be <= 4096 for 2-level bitmap");

        level1 = new size_t[bitmap_size]{};
        level0 = 0;
    }
    ~Bitmap2Level() {
        delete[] level1;
    }

    void set(size_t idx) {
        /*
        size_t w = idx / WORD_SIZE;
        size_t b = idx % WORD_SIZE;

        level1[w] |= (1ULL << b);
        level0    |= (1ULL << w);
        */
        size_t w = idx >> WORD_INDEX_BITS;  // word index
        size_t b = idx & WORD_SIZE_1;       // bit index within word
        level1[w] |= (1ULL << b);
        level0    |= (1ULL << w);
    }    

    void clear(size_t idx) {
        /*
        size_t w = idx / WORD_SIZE;
        size_t b = idx % WORD_SIZE;

        level1[w] &= ~(1ULL << b);

        if (level1[w] == 0)
            level0 &= ~(1ULL << w);
        */
        size_t w = idx >> WORD_INDEX_BITS; // word index
        size_t b = idx & WORD_SIZE_1;      // bit index within word
        level1[w] &= ~(1ULL << b);

        if (level1[w] == 0) 
            level0 &= ~(1ULL << w);
    }    

    bool test(size_t idx) const {
        // return level1[idx / WORD_SIZE] & (1ULL << (idx % WORD_SIZE));
        size_t w = idx >> WORD_INDEX_BITS;    // word index
        size_t b = idx & WORD_SIZE_1;     // bit index within word

        return (level1[w] & (1ULL << b)) && 
               (level0 & (1ULL << w)); // both level1 bit and level0 summary bit must be set

    }

    int first() const {
        if (!level0) return -1;

        size_t w = lsb(level0);

        // return w * WORD_SIZE + __builtin_ctzll(words[w]);
        return (w << WORD_INDEX_BITS) + lsb(level1[w]);        
    }

    int last() const {
        if (!level0) return -1;
        
        size_t w = msb(level0);
        
        // return w * WORD_SIZE + ((WORD_SIZE-1) - __builtin_clzll(words[w]));
        return (w << WORD_INDEX_BITS) + msb(level1[w]);
    }

    int lsb(size_t x) const { return std::countr_zero(x); }
    int msb(size_t x) const { return WORD_SIZE_1 - std::countl_zero(x); }
    /*
    // for WORD_SIZE < 64
    int msb(size_t x) const { 
        int unused_left_bits = sizeof(x) * CHAR_BIT - WORD_SIZE;
        return WORD_SIZE_1 - (std::countl_zero(x) - unused_left_bits); 
    }
    */
};




struct Bitmap3Level {
    size_t* level2; // Data bits (Leaf)
    size_t* level1; // Summary of level 2
    size_t  level0; // Summary of level 1 (Root)

    size_t l2_size;
    size_t l1_size;

    // Supports up to 64^3 = 262,144 levels
    Bitmap3Level(size_t price_levels) {
        assert(price_levels <= (WORD_SIZE * WORD_SIZE * WORD_SIZE));

        l2_size = (price_levels + WORD_SIZE_1) / WORD_SIZE;
        l1_size = (l2_size + WORD_SIZE_1) / WORD_SIZE;

        level2 = new size_t[l2_size]{};
        level1 = new size_t[l1_size]{};
        level0 = 0;
    }

    ~Bitmap3Level() {
        delete[] level2;
        delete[] level1;
    }

    void set(size_t idx) {
        size_t w2 = idx >> WORD_INDEX_BITS;             // Word in level 2
        size_t b2 = idx & WORD_SIZE_1;                  // Bit in level 2 word
        
        size_t w1 = w2 >> WORD_INDEX_BITS;              // Word in level 1
        size_t b1 = w2 & WORD_SIZE_1;                   // Bit in level 1 word
        
        size_t b0 = w1;                                 // Bit in level 0 (root)

        level2[w2] |= (1ULL << b2);
        level1[w1] |= (1ULL << b1);
        level0     |= (1ULL << b0);
    }

    void clear(size_t idx) {
        size_t w2 = idx >> WORD_INDEX_BITS;
        size_t b2 = idx & WORD_SIZE_1;
        
        size_t w1 = w2 >> WORD_INDEX_BITS;
        size_t b1 = w2 & WORD_SIZE_1;
        
        size_t b0 = w1;

        level2[w2] &= ~(1ULL << b2);
        
        if (level2[w2] == 0) {
            level1[w1] &= ~(1ULL << b1);
            if (level1[w1] == 0) {
                level0 &= ~(1ULL << b0);
            }
        }
    }

    bool test(size_t idx) const {
        size_t w2 = idx >> WORD_INDEX_BITS;
        size_t b2 = idx & WORD_SIZE_1;
        return (level2[w2] & (1ULL << b2)) != 0;
    }

    int first() const {
        if (!level0) return -1;

        size_t w1 = lsb(level0);
        size_t w2 = (w1 << WORD_INDEX_BITS) | lsb(level1[w1]);
        return (w2 << WORD_INDEX_BITS) | lsb(level2[w2]);
    }

    int last() const {
        if (!level0) return -1;

        size_t w1 = msb(level0);
        size_t w2 = (w1 << WORD_INDEX_BITS) | msb(level1[w1]);
        return (w2 << WORD_INDEX_BITS) | msb(level2[w2]);
    }

private:
    int lsb(size_t x) const { return std::countr_zero(x); }
    int msb(size_t x) const { return WORD_SIZE_1 - std::countl_zero(x); }
};