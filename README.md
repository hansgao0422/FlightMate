# FlightMate（飞行伴侣）

<img width="1200" height="420" alt="hero" src="https://github.com/user-attachments/assets/bb38b6de-b2b9-4cbe-9a4b-8c926fc98f7d" />




FlightMate 是一款面向 **LilyGo T-LoRa Pager V1.0** 的便携式飞行信息工具。它将附近机场、METAR 气象报文、GNSS 定位、离线地图和飞行日志集中在一台设备上，适合飞行前信息查看与旅途中辅助记录。

> FlightMate 仅用于信息参考，不可替代经认证的航空导航设备、官方气象资料或飞行程序。

当前版本：**1.07 MVP**

## 主要功能

- 显示附近机场、距离、方位和当前 UTC 时间。
- 通过 Wi-Fi 从 AviationWeather 获取机场最新 METAR 报文。
- 使用 GNSS 定位，并在 SD 卡离线 XYZ 地图上显示当前位置。
- 支持地图平移、1～9 级缩放和快速回到当前位置。
- 创建、查看和编辑飞行日志。
- 设置 Wi-Fi、屏幕亮度、键盘灯、屏幕超时、GPS 和低功耗待机。
- Wi-Fi 密码输入时显示实际字符，设置列表中仍以星号隐藏。

## 适用硬件

| 项目 | 规格 |
| --- | --- |
| 开发板 | LilyGo T-LoRa Pager V1.0 |
| 主控 | ESP32-S3 |
| Flash | 16 MB |
| PSRAM | 8 MB |
| 屏幕 | ST7796U，480 × 222 |
| 存储 | microSD 卡，用于离线地图等数据 |

## 快速开始

### 1. 准备离线地图

FlightMate 可以直接使用 TrailMate 已存放在 SD 卡中的地图瓦片，无需移动文件。支持以下两个目录：

```text
/maps/base/osm/{z}/{x}/{y}.png
/FlightMate/maps/base/osm/{z}/{x}/{y}.png
```

地图瓦片要求：

- 256 × 256 PNG；
- Web Mercator 投影；
- XYZ 目录结构；
- 缩放级别 1～9。

`manifest.json` 为可选文件。没有该文件时，固件会直接按上述目录读取瓦片。本项目发布包不包含实际地图数据。

### 2. 烧录固件

解压 `FlightMate-1.07-MVP-release.zip`，在 Windows 命令提示符或 PowerShell 中进入发布包目录，然后执行：

```powershell
FLASH_FLIGHTMATE_1.07.bat COM7
```

将 `COM7` 替换为设备实际串口。默认方式升级应用并保留设备设置，建议优先使用。

如需清空设备并进行全新烧录：

```powershell
FLASH_FLIGHTMATE_1.07.bat COM7 fresh
```

`fresh` 会擦除原有 Flash 数据和设置，请先确认确实需要清空设备。

也可以使用下列地址手动烧录：

| 文件 | 烧录地址 |
| --- | ---: |
| `FlightMate-1.07-bootloader.bin` | `0x0` |
| `FlightMate-1.07-partitions.bin` | `0x8000` |
| `FlightMate-1.07-boot_app0.bin` | `0xE000` |
| `FlightMate-1.07-app.bin` | `0x10000` |
| `FlightMate-1.07-merged-16MB.bin` | `0x0`，仅用于全新烧录 |

发布包：`FlightMate-1.07-MVP-release.zip`  
SHA-256：`c5a4e887e2d3667e1be55478656b942347f7e3db18da21827be5e1a813b24d64`

## 操作说明

### 通用按键

| 操作 | 按键 |
| --- | --- |
| 返回首页 | `H` 或 `Backspace` |
| 切换键盘灯 | `B` |
| 切换字符层 | 短按黄色键 |
| 进入低功耗待机 | 长按黄色键约 2 秒 |

### 各页面操作

| 页面 | 操作 |
| --- | --- |
| 首页 | `W/A`、`S/D` 或旋钮选择，`Enter` 打开 |
| 机场信息 | `R` 刷新 METAR，`L` 调节亮度，`Q` 返回 |
| 地图 | `W/A/S/D` 平移，`Q/E` 缩放，`C` 回到当前位置，`L` 调节亮度 |
| 日志列表 | `N` 新建，`W/S` 选择，`Enter` 打开 |
| 日志编辑 | `W/S` 选择字段，`A/D` 切换步骤，`Enter` 编辑，`Q` 退出 |
| 设置 | `W/S` 或旋钮选择，`Enter` 修改 |

## METAR 数据说明

- 数据来源为 [AviationWeather](https://aviationweather.gov/)。
- FlightMate 只保留并显示每个机场最新一条 METAR。
- 最近 2 小时没有报文时，程序会自动回查最近 24 小时。
- `NO REPORT` 表示数据源当前没有该机场的可用报文，不代表设备 Wi-Fi 或网络请求失败。
- METAR 更新依赖可正常访问数据源的 Wi-Fi 网络。

## 从源码构建

源码入口位于 `FlightMate/FlightMate.ino`。当前已验证的构建基线如下：

| 工具或库 | 版本 |
| --- | --- |
| Arduino CLI | 1.5.1 |
| Arduino-ESP32 | 3.3.10 |
| ESP-IDF | 5.5.4 |
| LilyGoLib | 0.2.0 |
| ArduinoJson | 7.4.2 |
| LVGL | 9.4.0 |
| Adafruit GFX Library | 1.12.0 |
| esptool | 5.3.0 |
| 分区方案 | `app3M_fat9M_16MB` |

建议使用与上述版本一致的环境构建，以减少开发板支持包或图形库 API 差异造成的问题。

## 1.07 更新摘要

- 放大机场页面右侧 METAR 正文字体，并调整显示行数。
- 在机场信息区域增加 `UTC HH:MM` 时间。
- 修正部分机场 ICAO 代码。
- METAR 查询增加 24 小时自动回查和更明确的无报文状态。
- 增加 METAR HTTP 状态、响应长度及网络错误串口日志。
- Wi-Fi 密码编辑改为显示输入字符。

## 验证状态

FlightMate 1.07 已完成独立全量编译、合并固件和静态布局检查。最终屏幕显示、键盘输入、GNSS、SD 卡地图、各地网络访问以及低功耗待机效果仍应在实际设备上验证。

## 参考项目

- [vicliu624/trail-mate](https://github.com/vicliu624/trail-mate)：离线地图瓦片目录与读取方式参考。
- [hansgao0422/T-LoRa-Pager-Flight-Radar](https://github.com/hansgao0422/T-LoRa-Pager-Flight-Radar)：飞行信息功能与界面设计参考。
