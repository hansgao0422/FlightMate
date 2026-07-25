# FlightMate changelog

## 1.07

- 机场页 METAR 正文改为 2 倍字体、最多 6 行，左侧增加 UTC 时间。
- 更正 `ZLSN→ZULS`、`ZGMX→ZSQZ`、`ZGHK→ZJHK`、`ZLHW→ZHCC`。
- 2 小时无 METAR 时回查 24 小时，并区分 `NO REPORT`、`NETWORK ERROR`、TLS 和 HTTP 错误。
- Wi-Fi 密码编辑时显示明文，设置列表继续隐藏已保存密码。

## 1.06

- 修复进入地图页时 LVGL PNG 解码器未初始化导致的 `PNG error 83`。
- 按 LVGL 9.4 的 `lv_draw_buf_t` 返回格式读取像素，并使用对应接口释放解码缓冲。
- 继续直接兼容 TrailMate `/maps/base/osm/{z}/{x}/{y}.png`。

## 1.05

- 同时兼容 `/FlightMate/maps/base/osm/...` 与 TrailMate 标准 `/maps/base/osm/...` 瓦片目录。
- 地图右侧增加具体读取和 PNG 错误提示。

## 1.04

- 地图缩放范围扩展为 1～9，manifest 缺失时启用 Direct tiles。

## 1.03

- 机场面板仅显示最新一条 METAR，修复文字错位出界。

## 1.02

- 修复 Wi-Fi 密码输入后编辑框显示空白的问题。

## 1.01

- 首个 MVP。
