#include <string.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <M5GFX.h>

M5GFX display;

#define MOUNT_POINT "/sdcard"
#define PIN_NUM_MISO  GPIO_NUM_13
#define PIN_NUM_MOSI  GPIO_NUM_12
#define PIN_NUM_CLK   GPIO_NUM_14
#define PIN_NUM_CS    GPIO_NUM_4

// ログ表示関数
void displayLog(const char* message) {
    static int yPos = 10;
    display.setCursor(10, yPos);
    display.print(message);
    display.display();  // 明示的に画面更新
    
    yPos += 30;
    if (yPos >= display.height()) {
        yPos = 10;
        display.clear();
    }
}

extern "C" void app_main(void)
{
    // M5GFX初期化
    display.init();
    display.setTextColor(TFT_BLACK, TFT_WHITE);
    display.setTextSize(2);
    display.clear();
    display.display();  // 追加: 画面更新を明示的に呼び出し

    displayLog("SDカード初期化開始");
    
    esp_err_t ret;
    // マウント設定
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    sdmmc_card_t *card;
    
    // SPI設定
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.max_freq_khz = 400;  // SPI通信速度を下げる

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
        .flags = 0
    };
    
    // バス初期化
    ret = spi_bus_initialize((spi_host_device_t)host.slot, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        displayLog("SPIバス初期化失敗");
        vTaskDelay(pdMS_TO_TICKS(2000));  // エラー表示のための遅延
        return;
    }
    displayLog("SPIバス初期化成功");
    
    // SDカードスロット設定
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = (spi_host_device_t)host.slot;
    
    // ファイルシステムマウント
    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        char error_msg[64];
        snprintf(error_msg, sizeof(error_msg), "マウント失敗: %s", esp_err_to_name(ret));
        displayLog(error_msg);
        vTaskDelay(pdMS_TO_TICKS(2000));  // エラー表示のための遅延
        return;
    }
    displayLog("ファイルシステムマウント成功");
    
    // ディレイを追加
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // ファイル作成
    const char *filename = MOUNT_POINT "/example.txt";
    FILE *f = fopen(filename, "w");
    if (f == NULL) {
        displayLog("ファイル作成失敗");
        esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
        vTaskDelay(pdMS_TO_TICKS(2000));  // エラー表示のための遅延
        return;
    }
    displayLog("ファイル作成成功");
    
    // ディレイを追加
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // ファイルに書き込み
    fprintf(f, "Hello, SDカード!");
    fclose(f);
    displayLog("ファイル書き込み完了");

    // ファイルから読み込み（デバッグ用）
    f = fopen(filename, "r");
    if (f != NULL) {
        char buffer[64];
        if (fgets(buffer, sizeof(buffer), f) != NULL) {
            char read_msg[128];
            snprintf(read_msg, sizeof(read_msg), "読み込み: %s", buffer);
            displayLog(read_msg);
        }
        fclose(f);
    }
    
    // クリーンアップ
    esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
    spi_bus_free((spi_host_device_t)host.slot);
    displayLog("全処理完了");

    // 最後に長めに待機して画面を表示
    vTaskDelay(pdMS_TO_TICKS(5000));
}
