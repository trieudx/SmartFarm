#ifndef __RBOOT_API_H__
#define __RBOOT_API_H__

/** @defgroup rboot rBoot API
 *  @brief      rBoot for ESP8266 API allows runtime code to access the rBoot configuration.
 *              Configuration may be read to use within the main firmware or updated to
 *              affect next boot behavior.
 *  @copyright  2015 Richard A Burton
 *  @author     richardaburton@gmail.com
 *  @author     OTA code based on SDK sample from Espressif
 *  @license    See licence.txt for license terms.
 *  @{
 */

#include <rboot.h>
#include "stdint.h"
#include "stdbool.h"
#include "stddef.h"

#ifdef __cplusplus
extern "C"
{
#endif

  /**	@brief  Structure defining flash write status
   *  @note   The user application should not modify the contents of this
   *          structure.
   *	@see    rboot_write_flash
   */
  typedef struct
  {
      uint32_t start_addr;
      uint32_t start_sector;
      //uint32_t max_sector_count;
      int32_t last_sector_erased;
      uint8_t extra_count;
      uint8_t extra_bytes[4];
  } rboot_write_status;

  /**	@brief	Read rBoot configuration from flash
   *	@retval rboot_config Copy of the rBoot configuration
   *  @note   Returns rboot_config (defined in rboot.h) allowing you to modify any values
   *          in it, including the ROM layout.
   */
  rboot_config rboot_get_config(void);

  /**	@brief	Write rBoot configuration to flash memory
   *	@param	conf pointer to a rboot_config structure containing configuration to save
   *	@retval bool True on success
   *  @note   Saves the rboot_config structure back to configuration sector (BOOT_CONFIG_SECTOR)
   *          of the flash, while maintaining the contents of the rest of the sector.
   *          You can use the rest of this sector for your app settings, as long as you
   *          protect this structure when you do so.
   */
  bool rboot_set_config(rboot_config *conf);

  /** @brief  Get index of current ROM
   *  @retval uint8 Index of the current ROM
   *  @note   Get the currently selected boot ROM (this will be the currently
   *          running ROM, as long as you haven't changed it since boot or rBoot
   *          booted the rom in temporary boot mode, see rboot_get_last_boot_rom).
   */
  uint8_t rboot_get_current_rom(void);

  /**	@brief	Set the index of current ROM
   *	@param	rom The index of the ROM to use on next boot
   *	@retval	bool True on success
   *  @note   Set the current boot ROM, which will be used when next restarted.
   *  @note   This function re-writes the whole configuration to flash memory (not just the current ROM index)
   */
  bool rboot_set_current_rom(uint8_t rom);

  /**	@brief  Initialise flash write process
   *	@param  start_addr Address on the SPI flash to begin write to
   *  @note   Call once before starting to pass data to write to flash memory with rboot_write_flash function.
   *          start_addr is the address on the SPI flash to write from. Returns a status structure which
   *          must be passed back on each write. The contents of the structure should not
   *          be modified by the calling code.
   */
  rboot_write_status rboot_write_init(uint32_t start_addr);

  /**	@brief  Write data to flash memory
   *	@param  status Pointer to rboot_write_status structure defining the write status
   *  @param  data Pointer to a block of uint8 data elements to be written to flash
   *  @param  len Quantity of uint8 data elements to write to flash
   *  @note   Call repeatedly to write data to the flash, starting at the address
   *  specified on the prior call to rboot_write_init. Current write position is
   *  tracked automatically. This method is likely to be called each time a packet
   *  of OTA data is received over the network.
   *  @note   Call rboot_write_init before calling this function to get the rboot_write_status structure
   */
  bool rboot_write_flash(rboot_write_status *status, uint8_t *data, uint16_t len);

#ifdef BOOT_RTC_ENABLED
  /** @brief  Get rBoot status/control data from RTC data area
   *  @param  rtc Pointer to a rboot_rtc_data structure to be populated
   *  @retval bool True on success, false if no data/invalid checksum (in which
   *          case do not use the contents of the structure)
   */
  bool rboot_get_rtc_data(rboot_rtc_data *rtc);

  /** @brief  Set rBoot status/control data in RTC data area
   *  @param  rtc pointer to a rboot_rtc_data structure
   *  @retval bool True on success
   *  @note   The checksum will be calculated automatically for you.
   */
  bool rboot_set_rtc_data(rboot_rtc_data *rtc);

  /** @brief  Set temporary rom for next boot
   *  @param  rom Rom slot number for next boot
   *  @retval bool True on success
   *  @note   This call will tell rBoot to temporarily boot the specified rom on
   *          the next boot. This is does not update the stored rBoot config on
   *          the flash, so after another reset it will boot back to the original
   *          rom.
   */
  bool rboot_set_temp_rom(uint8_t rom);

  /** @brief  Get the last booted rom slot number
   *  @param  rom Pointer to rom slot number variable to populate
   *  @retval bool True on success, false if no data/invalid checksum
   *  @note   This will find the currently running rom, even if booted as a
   *          temporary rom.
   */
  bool rboot_get_last_boot_rom(uint8_t *rom);

  /** @brief  Get the last boot mode
   *  @param  mode Pointer to mode variable to populate
   *  @retval bool True on success, false if no data/invalid checksum
   *  @note   This will indicate the type of boot: MODE_STANDARD, MODE_GPIO_ROM or
   *          MODE_TEMP_ROM.
   */
  bool rboot_get_last_boot_mode(uint8_t *mode);
#endif

  /* ADDITIONS TO RBOOT-API FOR ESP-OPEN-RTOS FOLLOW */

  /* Returns offset of given rboot slot, or (uint32_t)-1 if slot is invalid.
   */
  uint32_t rboot_get_slot_offset(uint8_t slot);

  /** @description Verify basic image parameters - headers, CRC8 checksum.

   @param Offset of image to verify. Can use rboot_get_slot_offset() to find.
   @param Optional pointer will return the total valid length of the image.
   @param Optional pointer to a static human-readable error message if fails.

   @return True for valid, False for invalid.
   **/
  bool rboot_verify_image(uint32_t offset, uint32_t *image_length,
                          const char **error_message);

  /* @description Digest callback prototype, designed to be compatible with
   mbedtls digest functions (SHA, MD5, etc.)

   See the ota_basic example to see an example of calculating the
   SHA256 digest of an OTA image.
   */
  typedef void (*rboot_digest_update_fn)(void * ctx, void *data,
                                         size_t data_len);

  /** @description Calculate a digest over the image at the offset specified

   @note This function is actually a generic function that hashes SPI
   flash contents, doesn't know anything about rboot image format. Use
   rboot_verify_image to ensure a well-formed OTA image.

   @param offset - Starting offset of image to hash (should be 4 byte aligned.)

   @param image_length - Length of image to hash (should be 4 byte aligned.)

   @param update_fn - Function to update digest (see rboot_digest_update_fn for details)

   @param update_ctx - Context argument for digest update function.

   @return True if digest completes successfully, false if digest function failed part way through
   **/
  bool rboot_digest_image(uint32_t offset, uint32_t image_length,
                          rboot_digest_update_fn update_fn, void *update_ctx);

#ifdef __cplusplus
}
#endif

/** @} */
#endif
