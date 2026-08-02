# FlightMate V1.13

<img width="1200" height="420" alt="hero" src="https://github.com/user-attachments/assets/ed4aefce-e0be-4f93-a3bc-772fb138a715" />


FlightMate 是为 LilyGO T-LoRa Pager 制作的便携飞行信息工具，提供机场与 METAR、离线地图、GPS 飞行数据、飞行日志和实时航班雷达。

## 适用硬件与固件环境

| 项目 | 规格 |
| --- | --- |
| 开发板 | LilyGO T-LoRa Pager V1.0 |
| 主控 | ESP32-S3 |
| Flash / PSRAM | 16 MB / 8 MB |
| 屏幕 | ST7796U，480×222 |
| 构建环境 | Arduino CLI 1.5.1、Arduino-ESP32 3.3.10、ESP-IDF 5.5.4 |
| 分区方案 | `app3M_fat9M_16MB` |

## 主要功能

<img width="960" height="444" alt="Codex 图像 2026年8月3日 00_29_43" src="https://github.com/user-attachments/assets/140fea02-7d53-47cd-848b-a7c33ac9edfa" />

### 主界面

- 四个入口：`AIRPORT`、`FLIGHT`、`LOGBOOK`、`SETTINGS`。
- 顶部中央显示设置时区对应的当地时间；时间未同步时显示 `--:--`。
- 支持旋钮、WASD 和 ENTER 操作。

### 机场看板与 Radar

- 显示最近机场、距离、方位、UTC 时间和最新 METAR。
- 机场卡片中的 `RADAR` 按钮可进入 Flight Radar。
- Radar 以有效 GPS 坐标为中心；无有效定位时使用设置中的手动经纬度。
- OpenSky 提供附近航班位置、呼号、高度和航迹；未填写 OAuth 凭据时使用匿名模式。
- TYPE 优先通过 ADSBDB 的 ICAO24 查询获得，OpenSky category 作为回退。
- 扫描动画采用原 FlightRadar 风格的 34° 线状绿色拖影，节奏为 50 ms / 3°。
- 雷达半径可选 `40 / 80 / 120 / 160 / 250 NM`，默认 `80 NM`。

### Flight 飞行页

- 左侧显示 SD 卡离线地图，右侧显示速度、高度、航向、经纬度、卫星数和 HDOP。
- `KM/H`、`M`、`DEG` 使用同一右边界对齐。
- 设置航班号后，航班号显示在地图定位点旁，并自动避开地图边界。
- 支持 WASD 平移、Q/E 缩放、C 回到当前位置。

### 飞行日志

- 支持浏览、创建和编辑飞行日志。
- 航班号设置与日志数据彼此独立，不改变原有日志字段。

### 熄屏与低功耗

<img width="960" height="444" alt="Codex 图像 2026年8月3日 00_29_15" src="https://github.com/user-attachments/assets/d05305e5-cac5-4453-893c-31405d893964" />

- 自动熄屏后进入最低背光的信息页，显示当地时间、航班号和唤醒提示。
- 熄屏字体颜色为正常界面的约 50%，系统分钟变化时立即刷新时间。
- 熄屏页不显示产品名、版本号或 `SCREEN OFF`；任意按键或旋钮操作可唤醒。
- `LOW POWER STANDBY` 是独立的深睡眠模式，按 BOOT 唤醒。

> V1.13 的自动熄屏页仍保持 LCD 工作，因此功耗高于 V1.10 的完全关闭屏幕。需要长时间待机时请使用 `LOW POWER STANDBY`。

## 与 V1.10 的关系

V1.13 保留 V1.10 的 Wi-Fi 隔离、航班号、机场/METAR、离线地图和飞行日志功能，并累计增加 Radar、OpenSky/ADSBDB、熄屏信息页及多项界面修正。完整差异和升级步骤见 [UPGRADE-FROM-V1.10.md](UPGRADE-FROM-V1.10.md)。

## 首次使用

1. 刷写固件并重启设备。
2. 进入 `SETTINGS`，设置 `WI-FI SSID`、`WI-FI PASSWORD`、`TIMEZONE` 和 `FLIGHT NUMBER`。
3. 确认 `GPS` 为 `ON`；室外等待定位，或填写 `MANUAL LAT`、`MANUAL LON` 作为 Radar 回退中心。
4. OpenSky 凭据可留空使用匿名模式；需要认证模式时填写 `OPENSKY CLIENT ID` 和 `OPENSKY SECRET`。
5. 设置 `RADAR RADIUS`，返回机场页并按 ENTER 打开 Radar。

## 常用按键

| 页面 | 操作 |
| --- | --- |
| 全局 | `H` 返回主界面，`B` 切换键盘灯 |
| 主界面 | 旋钮或 WASD 选择，ENTER 打开 |
| 机场 | ENTER 打开 Radar，`R` 刷新 METAR，`L` 调节屏幕亮度 |
| Radar | `R` 刷新航班，`S` 进入 Radar 设置，`Q/H` 返回 |
| Flight | WASD 平移，`Q/E` 缩放，`C` 回中，`L` 调节亮度 |
| 日志列表 | `N` 新建，W/S 或旋钮选择，ENTER 查看 |
| 设置 | W/S 或旋钮选择，ENTER 修改 |

## 离线地图目录

固件会依次尝试以下瓦片路径：

```text
/FlightMate/maps/base/osm/{z}/{x}/{y}.png
/maps/base/osm/{z}/{x}/{y}.png
```

manifest 可放在：

```text
/FlightMate/maps/manifest.json
/FlightMate/manifest.json
/maps/manifest.json
/manifest.json
```

地图缩放范围为 1～9。manifest 缺失时仍会尝试直接读取瓦片。

## 从 V1.10 升级

推荐仅更新应用，保留 Wi-Fi、航班号、时区、亮度、GPS 和手动坐标等 NVS 设置：

```bat
FLASH_APP_ONLY_FLIGHTMATE_V1.13.bat COM7
```

将 `COM7` 替换为设备实际串口。首次进入 V1.13 后，再补充 OpenSky 凭据和 Radar 半径即可。

## 其他刷写方式

写入 bootloader、分区表、boot_app0 和应用，但不主动擦除 NVS：

```bat
FLASH_FLIGHTMATE_V1.13.bat COM7
```

清除整个 Flash 后重新安装：

```bat
ERASE_AND_FLASH_FLIGHTMATE_V1.13.bat COM7
```

完整恢复镜像 `FlightMate-V1.13-merged-16MB.bin` 从 `0x0` 写入，会覆盖设备中的设置和数据，仅建议用于故障恢复或全新安装。

## 镜像地址

| 文件 | 地址 |
| --- | ---: |
| `FlightMate-V1.13-bootloader.bin` | `0x0` |
| `FlightMate-V1.13-partitions.bin` | `0x8000` |
| `FlightMate-V1.13-boot_app0.bin` | `0xE000` |
| `FlightMate-V1.13-app.bin` | `0x10000` |
| `FlightMate-V1.13-merged-16MB.bin` | `0x0` |

## Radar 网络与日志

- 航班位置和 TYPE 查询需要 Wi-Fi；GPS 定位本身仍在设备本地工作。
- 认证 OpenSky 的刷新周期为 60 秒，匿名模式为 240 秒；也可在 Radar 页按 `R` 手动刷新。
- 每轮最多查询前 4 架未缓存飞机的 TYPE，内存缓存容量为 12 架，避免连续请求外部接口。
- 串口波特率为 `115200`。TYPE 排查时关注：

```text
[RADAR] state icao=... fields=... category=... fallback=...
[RADAR] type icao=... http=... resolved=...
```

- 串口输入 `help` 可查看诊断命令，包含 `status`、`gps`、`sd`、`map`、`heap`、`tasks`、`weather refresh`、`radar refresh` 和 `logs rebuild`。

## 构建与校验

- Arduino CLI 构建通过：程序存储 1,695,657 字节（53%），全局 RAM 66,952 字节（20%）。
- App BIN 为 1,695,904 字节；合并镜像为 16,777,216 字节。
- bootloader 与 App 已通过 esptool checksum 和 validation hash 检查。
- 合并镜像在 `0x0 / 0x8000 / 0xE000 / 0x10000` 与四个独立镜像逐字节匹配。
- 文件 SHA-256 见 `SHA256SUMS.txt`；ZIP 校验见外层 `ZIP-SHA256SUMS.txt`。

V1.12 的实机反馈已用于 V1.13 修正；V1.13 已完成构建、镜像和封包验证，但本轮未连接设备完成最终实机验收。刷写后请重点确认熄屏时间跨分钟更新、HOME 当地时间、单位右对齐和 `[RADAR] type ... resolved=...` 日志。
