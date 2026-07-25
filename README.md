# FlightMate 1.07 MVP

适用硬件：LilyGo T-LoRa Pager V1.0（ESP32-S3、16 MB Flash、8 MB PSRAM、480×222 ST7796U）。

## 1.07 更新

- 机场页右侧 METAR 正文改为 2 倍字体，最多显示 6 行。
- 左侧机场卡片增加 `UTC HH:MM` 时间。
- 更正拉萨、泉州、海口、郑州的 ICAO 代码。
- 最近 2 小时无 METAR 时自动回查 24 小时；仍无数据时显示 `NO REPORT`，不再误报请求失败。
- 增加 METAR 请求的 HTTP 状态、响应长度和网络错误串口日志。
- Wi-Fi 密码编辑时直接显示输入字符；设置列表仍显示为星号。

在线核对 54 个机场时，当前有 48 个可从 AviationWeather 获得最近 2 小时 METAR。`ZBDS`、`ZLXN`、`ZLIC`、`ZULS`、`ZSCN`、`ZSQZ` 当时返回 HTTP 204，表示数据源没有报文，而不是设备联网失败。

## 烧录

默认升级并保留设置：

    FLASH_FLIGHTMATE_1.07.bat COM7

全新擦除烧录：

    FLASH_FLIGHTMATE_1.07.bat COM7 fresh

请将 `COM7` 换成设备实际串口。建议先使用默认升级，不需要擦除 NVS。

## 固件地址

| 文件 | 地址 |
| --- | ---: |
| `FlightMate-1.07-bootloader.bin` | `0x0` |
| `FlightMate-1.07-partitions.bin` | `0x8000` |
| `FlightMate-1.07-boot_app0.bin` | `0xE000` |
| `FlightMate-1.07-app.bin` | `0x10000` |
| `FlightMate-1.07-merged-16MB.bin` | `0x0`，全新烧录 |

文件校验值见 `SHA256SUMS.txt`。本版本已完成独立全新编译、镜像和静态布局验证；实际字体效果、UTC 显示及各机场联网结果仍需烧录后真机确认。
