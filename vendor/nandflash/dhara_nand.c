/**
 * @file        nand.c
 * @author      Andrew Loebs
 * @brief       "Glue" layer between dhara and spi_nand module
 *
 */

#include "dhara/nand.h"
#include "dhara/journal.h"
#include "nandflash.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

//#define DHARA_DEBUG_PRINTF printf
#define DHARA_DEBUG_PRINTF

typedef struct {
    bool valid;
    dhara_page_t page;
    size_t offset;
    uint8_t data[DHARA_META_SIZE];
} metadata_cache_entry;

#define NUM_CACHE_ENTRIES 16
metadata_cache_entry metadata_cache[NUM_CACHE_ENTRIES];
int cache_index;
int last_cache_hit;


int cache_idx(dhara_page_t page, size_t offset) {
    for (int j=0; j<NUM_CACHE_ENTRIES; j++) {
        int idx = last_cache_hit + j;
        if (idx >= NUM_CACHE_ENTRIES) idx -= NUM_CACHE_ENTRIES;
        if (!metadata_cache[idx].valid) continue;
        if (metadata_cache[idx].page == page && metadata_cache[idx].offset == offset) {
            last_cache_hit = idx;
            return idx;
        }
    }
    return -1;
}

void add_to_cache(dhara_page_t page, size_t offset, uint8_t *data) {
    metadata_cache[cache_index].valid = true;
    metadata_cache[cache_index].page = page;
    metadata_cache[cache_index].offset = offset;
    memcpy(metadata_cache[cache_index].data, data, DHARA_META_SIZE);
    cache_index++;
    if (cache_index == NUM_CACHE_ENTRIES) cache_index = 0;
}





// public function definitions
int dhara_nand_is_bad(const struct dhara_nand *n, dhara_block_t b)
{
    // construct row address
    row_address_t row = {.block = b, .page = 0};
    // call spi_nand layer for block status
    bool is_bad;
    int ret = nandflash_block_is_bad(row, &is_bad);
    DHARA_DEBUG_PRINTF("dhara: nand_is_bad(%d): %d (bad=%d)\n", b, ret, is_bad);
    if (SPI_NAND_RET_OK != ret) {
        // if we get a bad return, we'll just call this block bad
        is_bad = true;
    }

    return (int)is_bad;
}

void dhara_nand_mark_bad(const struct dhara_nand *n, dhara_block_t b)
{
    DHARA_DEBUG_PRINTF("dhara: nand_mark_bad(%d)\n", b);
    // construct row address
    row_address_t row = {.block = b, .page = 0};
    // call spi_nand layer
    nandflash_block_mark_bad(row); // ignore ret
}

int dhara_nand_erase(const struct dhara_nand *n, dhara_block_t b, dhara_error_t *err)
{
    // construct row address
    row_address_t row = {.block = b, .page = 0};
    // call spi_nand layer
    int ret = nandflash_block_erase(row);
    DHARA_DEBUG_PRINTF("dhara: nand_erase(%d): %d\n", b, ret);
    if (SPI_NAND_RET_OK == ret) { // success
        return 0;
    }
    else if (SPI_NAND_RET_E_FAIL == ret) { // failed internally on nand
        *err = DHARA_E_BAD_BLOCK;
        return -1;
    }
    else { // failed for some other reason
        return -1;
    }
}

int dhara_nand_prog(const struct dhara_nand *n, dhara_page_t p, const uint8_t *data,
                    dhara_error_t *err)
{
    //// Invalidate cache
    //if (p == cached_page_address) cached_page_address = INVALID_ADDRESS;

    // construct row address -- dhara's page address is identical to an MT29F row address
    row_address_t row = {.whole = p};
    // call spi_nand layer
    int ret = nandflash_page_program(row, 0, data, SPI_NAND_PAGE_SIZE);
    DHARA_DEBUG_PRINTF("dhara: nand_prog(%d): %d\n", p, ret);
    if (SPI_NAND_RET_OK == ret) { // success
        return 0;
    }
    else if (SPI_NAND_RET_P_FAIL == ret) { // failed internally on nand
        *err = DHARA_E_BAD_BLOCK;
        return -1;
    }
    else { // failed for some other reason
        return -1;
    }
}

int dhara_nand_is_free(const struct dhara_nand *n, dhara_page_t p)
{
    // construct row address -- dhara's page address is identical to an MT29F row address
    row_address_t row = {.whole = p};
    // call spi_nand layer
    bool is_free;
    int ret = nandflash_page_is_free(row, &is_free);
    DHARA_DEBUG_PRINTF("dhara: nand_is_free(%d): %d\n", p, ret);
    if (SPI_NAND_RET_OK != ret) {
        // if we get a bad return, we'll report the page as "not free"
        is_free = false;
    }

    return (int)is_free;
}

int dhara_nand_read(const struct dhara_nand *n, dhara_page_t p, size_t offset, size_t length,
                    uint8_t *data, dhara_error_t *err)
{
    row_address_t row = {.whole = p};

    if (length == DHARA_META_SIZE) {
        int cidx = cache_idx(p, offset);
        if (cidx == -1) {
            // Not in cache
            DHARA_DEBUG_PRINTF("dhara: read (%d, %d) not in cache\n", p, offset);
            int ret = nandflash_page_read(row, offset, data, DHARA_META_SIZE);
            if (ret != SPI_NAND_RET_OK) {
                if (ret == SPI_NAND_RET_ECC_ERR) *err = DHARA_E_ECC;
                return -1;
            }
            add_to_cache(p, offset, data);
            return 0;
        } else {
            DHARA_DEBUG_PRINTF("dhara: read (%d, %d) [cached]\n", p, offset);
            memcpy(data, metadata_cache[cidx].data, DHARA_META_SIZE);
            return 0;
        }
    }

    // call spi_nand layer
    int ret = nandflash_page_read(row, offset, data, length);
    DHARA_DEBUG_PRINTF("dhara: nand_read(%d, offs=%d, len=%d): %d\n", p, offset, length, ret);
    if (SPI_NAND_RET_OK == ret) { // success
        return 0;
    }
    else if (SPI_NAND_RET_ECC_ERR == ret) { // ECC failure
        *err = DHARA_E_ECC;
        return -1;
    }
    else { // failed for some other reason
        return -1;
    }
}

/* Read a page from one location and reprogram it in another location.
 * This might be done using the chip's internal buffers, but it must use
 * ECC.
 */
int dhara_nand_copy(const struct dhara_nand *n, dhara_page_t src, dhara_page_t dst,
                    dhara_error_t *err)
{
    // construct row addresses -- dhara's page address is identical to an MT29F row address
    row_address_t source = {.whole = src};
    row_address_t dest = {.whole = dst};
    // call spi_nand layer
    int ret = nandflash_page_copy(source, dest);
    DHARA_DEBUG_PRINTF("dhara: nand_copy(%d->%d): %d\n", src, dst, ret);
    if (SPI_NAND_RET_OK == ret) { // success
        return 0;
    }
    else if (SPI_NAND_RET_ECC_ERR == ret) { // ECC failure on read
        *err = DHARA_E_ECC;
        return -1;
    }
    else if (SPI_NAND_RET_P_FAIL == ret) { // program failure
        *err = DHARA_E_BAD_BLOCK;
        return -1;
    }
    else { // failed for some other reason
        return -1;
    }
}
