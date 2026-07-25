#include "flightmate_app.h"

#include <Adafruit_GFX.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <HTTPClient.h>
#include <LilyGoLib.h>
#include <Preferences.h>
#include <SD.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <esp_heap_caps.h>
#include <esp_sleep.h>
#include <esp_sntp.h>
#include <lvgl.h>
#define LODEPNG_NO_COMPILE_CPP
#include <libs/lodepng/lodepng.h>
#include <math.h>
#include <sys/time.h>
#include <time.h>

#include "config.h"

namespace {

constexpr char APP_NAME[] = "FlightMate";
constexpr char APP_VERSION[] = "1.07";
constexpr uint16_t SCREEN_W = 480;
constexpr uint16_t SCREEN_H = 222;
constexpr uint16_t TOP_H = 24;
constexpr uint16_t BOTTOM_H = 24;
constexpr uint16_t CONTENT_Y = TOP_H;
constexpr uint16_t CONTENT_H = SCREEN_H - TOP_H - BOTTOM_H;
constexpr uint8_t MAP_MIN_ZOOM = 1;
constexpr uint8_t MAP_MAX_ZOOM = 9;
constexpr uint16_t MAP_W = 340;
constexpr uint16_t MAP_H = CONTENT_H;
constexpr uint8_t TILE_CACHE_SIZE = 6;
constexpr uint8_t KEY_QUEUE_SIZE = 24;
constexpr uint8_t YELLOW_KEY_RAW = 21;
constexpr uint32_t POWER_HOLD_MS = 2000;
constexpr uint32_t BATTERY_REFRESH_MS = 30000;
constexpr uint32_t AIRPORT_REFRESH_MS = 1800000;
constexpr uint32_t WEATHER_REFRESH_MS = 1800000;
constexpr uint32_t WEATHER_RETRY_MS = 60000;
constexpr uint32_t UI_REFRESH_MS = 1000;
constexpr uint32_t MAP_REFRESH_MS = 250;
constexpr size_t MAX_PNG_BYTES = 1024U * 1024U;
constexpr uint32_t INDEX_MAGIC = 0x464D4C47; // FMLG

constexpr uint16_t C_BG = 0x0862;
constexpr uint16_t C_PANEL = 0x10E3;
constexpr uint16_t C_PRIMARY = 0x3F31;
constexpr uint16_t C_ACCENT = 0x563F;
constexpr uint16_t C_WARNING = 0xFDA8;
constexpr uint16_t C_DANGER = 0xFACD;
constexpr uint16_t C_TEXT = 0xEFFE;
constexpr uint16_t C_MUTED = 0x7CCF;
constexpr uint16_t C_GRID = 0x1A65;
constexpr uint16_t C_BLACK = 0x0000;

constexpr uint8_t DISPLAY_LEVELS[] = {4, 8, 12, 16};
constexpr uint8_t DISPLAY_PERCENT[] = {25, 50, 75, 100};
constexpr uint8_t KEYBOARD_LEVELS[] = {0, 128, 255};
constexpr uint32_t SCREEN_TIMEOUTS[] = {30000, 60000, 300000, 600000, 0};
constexpr char METAR_URL_PREFIX[] =
    "https://aviationweather.gov/api/data/metar?format=raw&taf=false&hours=";

constexpr char DIGICERT_GLOBAL_ROOT_G2[] PROGMEM = R"CERT(-----BEGIN CERTIFICATE-----
MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI
2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx
1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ
q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz
tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ
vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP
BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV
5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY
1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4
NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG
Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91
8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe
pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl
MrY=
-----END CERTIFICATE-----
)CERT";

class PsramCanvas16 : public GFXcanvas16 {
public:
    PsramCanvas16(uint16_t width, uint16_t height)
        : GFXcanvas16(width, height, false), canvasWidth(width), canvasHeight(height) {}

    bool begin()
    {
        if (buffer) return true;
        buffer = static_cast<uint16_t *>(heap_caps_malloc(
            static_cast<size_t>(canvasWidth) * canvasHeight * sizeof(uint16_t),
            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
        if (buffer) memset(buffer, 0, static_cast<size_t>(canvasWidth) * canvasHeight * sizeof(uint16_t));
        return buffer != nullptr;
    }

    ~PsramCanvas16() override
    {
        if (buffer) heap_caps_free(buffer);
        buffer = nullptr;
    }

private:
    uint16_t canvasWidth;
    uint16_t canvasHeight;
};

enum class Page : uint8_t {
    Home,
    Airport,
    Flight,
    LogList,
    LogDetail,
    LogEdit,
    Settings,
    WifiList
};

struct Settings {
    char wifiSsid[33] = {};
    char wifiPassword[65] = {};
    char timezone[33] = {};
    double manualLat = DEFAULT_LATITUDE;
    double manualLon = DEFAULT_LONGITUDE;
    uint8_t brightness = 2;
    uint8_t keyboardBrightness = 1;
    uint8_t screenTimeout = 2;
    bool gpsEnabled = true;
};

struct GpsSnapshot {
    bool hardwareReady = false;
    bool valid = false;
    bool stale = true;
    double latitude = 0;
    double longitude = 0;
    double speedKmh = 0;
    double courseDeg = 0;
    double altitudeM = 0;
    uint32_t satellites = 0;
    double hdop = 0;
    uint32_t lastLocationMs = 0;
    uint32_t charsProcessed = 0;
};

struct Airport {
    char icao[5];
    float latitude;
    float longitude;
    const char *name;
};

// MVP read-only airport data set. It intentionally contains airports with
// published ICAO codes only; no online lookup or guessed codes are used.
constexpr Airport AIRPORTS[] = {
    {"ZBAA", 40.0799F, 116.6031F, "Beijing Capital"},
    {"ZBAD", 39.5098F, 116.4105F, "Beijing Daxing"},
    {"ZSSS", 31.1979F, 121.3363F, "Shanghai Hongqiao"},
    {"ZSPD", 31.1443F, 121.8083F, "Shanghai Pudong"},
    {"ZGGG", 23.3924F, 113.2988F, "Guangzhou Baiyun"},
    {"ZGSZ", 22.6393F, 113.8107F, "Shenzhen Baoan"},
    {"ZUUU", 30.5785F, 103.9471F, "Chengdu Shuangliu"},
    {"ZUTF", 30.3125F, 104.4413F, "Chengdu Tianfu"},
    {"ZUCK", 29.7192F, 106.6417F, "Chongqing Jiangbei"},
    {"ZPPP", 25.1019F, 102.9292F, "Kunming Changshui"},
    {"ZLXY", 34.4471F, 108.7516F, "Xian Xianyang"},
    {"ZHHH", 30.7838F, 114.2081F, "Wuhan Tianhe"},
    {"ZSHC", 30.2295F, 120.4344F, "Hangzhou Xiaoshan"},
    {"ZSNJ", 31.7420F, 118.8620F, "Nanjing Lukou"},
    {"ZSQD", 36.3619F, 120.0883F, "Qingdao Jiaodong"},
    {"ZBTJ", 39.1244F, 117.3462F, "Tianjin Binhai"},
    {"ZBDS", 39.4934F, 109.8614F, "Ordos Ejin Horo"},
    {"ZBYN", 37.7469F, 112.6284F, "Taiyuan Wusu"},
    {"ZBHH", 40.8514F, 111.8241F, "Hohhot Baita"},
    {"ZYTX", 41.6398F, 123.4834F, "Shenyang Taoxian"},
    {"ZYTL", 38.9657F, 121.5386F, "Dalian Zhoushuizi"},
    {"ZYCC", 43.9962F, 125.6853F, "Changchun Longjia"},
    {"ZYHB", 45.6234F, 126.2503F, "Harbin Taiping"},
    {"ZWWW", 43.9071F, 87.4742F, "Urumqi Diwopu"},
    {"ZLLL", 36.5152F, 103.6208F, "Lanzhou Zhongchuan"},
    {"ZLXN", 36.5275F, 102.0430F, "Xining Caojiabao"},
    {"ZLIC", 38.3228F, 106.3932F, "Yinchuan Hedong"},
    {"ZULS", 29.2978F, 90.9119F, "Lhasa Gonggar"},
    {"ZGHA", 28.1892F, 113.2196F, "Changsha Huanghua"},
    {"ZSCN", 28.8650F, 115.9000F, "Nanchang Changbei"},
    {"ZSFZ", 25.9351F, 119.6633F, "Fuzhou Changle"},
    {"ZSAM", 24.5440F, 118.1277F, "Xiamen Gaoqi"},
    {"ZSQZ", 24.7964F, 118.5897F, "Quanzhou Jinjiang"},
    {"ZJHK", 19.9349F, 110.4590F, "Haikou Meilan"},
    {"ZJSY", 18.3029F, 109.4123F, "Sanya Phoenix"},
    {"ZUGY", 26.5385F, 106.8007F, "Guiyang Longdongbao"},
    {"ZGNN", 22.6083F, 108.1724F, "Nanning Wuxu"},
    {"ZHCC", 34.5197F, 113.8409F, "Zhengzhou Xinzheng"},
    {"ZBSJ", 38.2807F, 114.6973F, "Shijiazhuang Zhengding"},
    {"VHHH", 22.3080F, 113.9185F, "Hong Kong International"},
    {"VMMC", 22.1496F, 113.5915F, "Macau International"},
    {"RCTP", 25.0777F, 121.2328F, "Taiwan Taoyuan"},
    {"RJTT", 35.5494F, 139.7798F, "Tokyo Haneda"},
    {"RJAA", 35.7720F, 140.3929F, "Tokyo Narita"},
    {"RKSI", 37.4602F, 126.4407F, "Seoul Incheon"},
    {"WSSS", 1.3644F, 103.9915F, "Singapore Changi"},
    {"VTBS", 13.6900F, 100.7501F, "Bangkok Suvarnabhumi"},
    {"WMKK", 2.7456F, 101.7099F, "Kuala Lumpur"},
    {"OMDB", 25.2532F, 55.3657F, "Dubai International"},
    {"EGLL", 51.4700F, -0.4543F, "London Heathrow"},
    {"LFPG", 49.0097F, 2.5479F, "Paris Charles de Gaulle"},
    {"EDDF", 50.0379F, 8.5622F, "Frankfurt"},
    {"KJFK", 40.6413F, -73.7781F, "New York JFK"},
    {"KLAX", 33.9416F, -118.4085F, "Los Angeles"}
};

struct MapTile {
    bool valid = false;
    uint8_t zoom = 0;
    int32_t x = 0;
    int32_t y = 0;
    uint32_t usedAt = 0;
    uint16_t *pixels = nullptr;
};

struct MapState {
    uint8_t zoom = 7;
    int32_t panX = 0;
    int32_t panY = 0;
    bool follow = true;
    bool manifestValid = false;
    bool manifestChecked = false;
    char manifestName[48] = "Not checked";
    char tileStatus[32] = "";
    bool tileStatusError = false;
};

struct RawKeyEvent {
    bool ready = false;
    bool pressed = false;
    uint8_t raw = 0;
};

struct LogRecord {
    char date[11] = {};
    char departureLocal[6] = {};
    char arrivalLocal[6] = {};
    char flightNo[13] = {};
    char departureIcao[5] = {};
    char arrivalIcao[5] = {};
    char airline[25] = {};
    char aircraftType[13] = {};
    char registration[13] = {};
    char engine[17] = {};
    char ageYears[7] = {};
    char cabin[13] = "ECONOMY";
    char seatNo[9] = {};
    char departureRunway[9] = {};
    char arrivalRunway[9] = {};
    char gate[9] = {};
    char standType[13] = "JET_BRIDGE";
    char pushbackLocal[6] = {};
    char takeoffLocal[6] = {};
    char landingLocal[6] = {};
    char onBlockLocal[6] = {};
    char cruiseAltitudeM[9] = {};
    char durationMin[9] = {};
    char distanceKm[11] = {};
    char cruiseSpeedKmh[9] = {};
    char mealRating[3] = {};
    char serviceRating[3] = {};
    char notes[97] = {};
};

#pragma pack(push, 1)
struct LogIndexHeader {
    uint32_t magic;
    uint16_t schema;
    uint16_t count;
};

struct LogIndexEntry {
    char path[96];
    char date[11];
    char departureLocal[6];
    char flightNo[13];
    char departureIcao[5];
    char arrivalIcao[5];
};
#pragma pack(pop)

struct WeatherState {
    char icao[5] = "----";
    char raw[256] = "No cached METAR";
    time_t fetchedUtc = 0;
    char status[48] = "OFFLINE CACHE";
};

PsramCanvas16 canvas(SCREEN_W, SCREEN_H);
Preferences preferences;
Settings settings;
GpsSnapshot gps;
MapState mapState;
MapTile tileCache[TILE_CACHE_SIZE];
int8_t mapTileRootIndex = -1;
WeatherState weather;
SemaphoreHandle_t weatherMutex = nullptr;
volatile bool weatherFetchRunning = false;
volatile bool weatherFetchRequested = false;
char weatherRequestIcao[5] = "----";

Page page = Page::Home;
uint8_t homeSelection = 0;
uint8_t settingsSelection = 0;
uint8_t wifiSelection = 0;
String scannedSsids[10];
int32_t scannedRssi[10] = {};
uint8_t scannedCount = 0;
bool editText = false;
String editBuffer;
uint8_t editTarget = 0;
bool symbolMode = false;
bool capsMode = false;
enum class EditOwner : uint8_t { None, Settings, Log };
EditOwner editOwner = EditOwner::None;
char *editDestination = nullptr;
size_t editCapacity = 0;
bool editUppercase = false;
bool editSecret = false;
String editLabel;
RawKeyEvent keyQueue[KEY_QUEUE_SIZE];
volatile uint8_t keyReadIndex = 0;
volatile uint8_t keyWriteIndex = 0;
portMUX_TYPE keyMux = portMUX_INITIALIZER_UNLOCKED;

bool sdReady = false;
bool screenOff = false;
bool dirty = true;
bool rotaryPressed = false;
bool yellowKeyHeld = false;
uint32_t yellowKeyPressedAt = 0;
bool shutdownTriggered = false;
bool wifiScanRunning = false;
volatile bool rtcSyncPending = false;
uint8_t batteryPercent = 255;
uint32_t lastBatteryRefresh = 0;
uint32_t lastUiRefresh = 0;
uint32_t lastMapRefresh = 0;
uint32_t lastInputAt = 0;
uint32_t lastWifiAttempt = 0;
uint32_t lastAirportRefresh = 0;
uint32_t lastWeatherRequest = 0;
uint32_t lastGpsChars = 0;
uint32_t gpsCharsChangedAt = 0;
String transientStatus;
uint32_t transientStatusUntil = 0;
String serialLine;

const Airport *nearestAirport = nullptr;
float nearestAirportKm = -1;
float nearestAirportBearing = 0;

LogIndexEntry logEntries[64] = {};
uint16_t logCount = 0;
uint16_t logSelection = 0;
LogRecord logRecord;
char currentLogPath[96] = {};
bool logIsNew = false;
uint8_t logStep = 0;
uint8_t logField = 0;
bool gpsInitialized = false;

const char *pageName()
{
    switch (page) {
    case Page::Home: return "HOME";
    case Page::Airport: return "AIRPORT";
    case Page::Flight: return "FLIGHT";
    case Page::LogList: return "LOGBOOK";
    case Page::LogDetail: return "LOG DETAIL";
    case Page::LogEdit: return "LOG EDIT";
    case Page::Settings: return "SETTINGS";
    case Page::WifiList: return "WIFI SCAN";
    }
    return "";
}

void copyText(char *destination, size_t size, const String &source)
{
    if (!size) return;
    strlcpy(destination, source.c_str(), size);
}

void copyText(char *destination, size_t size, const char *source)
{
    if (!size) return;
    strlcpy(destination, source ? source : "", size);
}

String latestMetarReport(String response)
{
    response.replace("\r", "\n");
    int start = 0;
    while (start < static_cast<int>(response.length())) {
        int end = response.indexOf('\n', start);
        if (end < 0) end = response.length();
        String report = response.substring(start, end);
        report.replace("\t", " ");
        while (report.indexOf("  ") >= 0) report.replace("  ", " ");
        report.trim();
        if (report.length()) return report;
        start = end + 1;
    }
    return "";
}

void setStatus(const String &message, uint32_t duration = 3000)
{
    transientStatus = message;
    transientStatusUntil = millis() + duration;
    dirty = true;
    Serial.printf("[APP] %s\n", message.c_str());
}

uint16_t swap565(uint16_t value)
{
    return static_cast<uint16_t>((value << 8) | (value >> 8));
}

void flushDisplay()
{
    uint16_t *pixels = canvas.getBuffer();
    const size_t count = static_cast<size_t>(SCREEN_W) * SCREEN_H;
    for (size_t i = 0; i < count; ++i) pixels[i] = swap565(pixels[i]);
    instance.pushColors(0, 0, SCREEN_W, SCREEN_H, pixels);
    for (size_t i = 0; i < count; ++i) pixels[i] = swap565(pixels[i]);
}

void drawText(const String &text, int16_t x, int16_t y, uint16_t color = C_TEXT, uint8_t size = 1)
{
    canvas.setTextWrap(false);
    canvas.setTextSize(size);
    canvas.setTextColor(color);
    canvas.setCursor(x, y);
    canvas.print(text);
}

void drawCentered(const String &text, int16_t centerX, int16_t y, uint16_t color = C_TEXT, uint8_t size = 1)
{
    int16_t x1, y1;
    uint16_t w, h;
    canvas.setTextSize(size);
    canvas.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
    drawText(text, centerX - static_cast<int16_t>(w) / 2, y, color, size);
}

void drawPanel(int16_t x, int16_t y, int16_t w, int16_t h, bool selected = false)
{
    canvas.fillRoundRect(x, y, w, h, 7, C_PANEL);
    canvas.drawRoundRect(x, y, w, h, 7, selected ? C_PRIMARY : C_GRID);
    if (selected) canvas.drawRoundRect(x + 1, y + 1, w - 2, h - 2, 6, C_PRIMARY);
}

void applyBrightness()
{
    if (!screenOff) instance.setBrightness(DISPLAY_LEVELS[settings.brightness]);
    instance.kb.setBrightness(KEYBOARD_LEVELS[settings.keyboardBrightness]);
}

void saveSettings()
{
    preferences.begin("flightmate", false);
    preferences.putUShort("schema", 1);
    preferences.putString("ssid", settings.wifiSsid);
    preferences.putString("password", settings.wifiPassword);
    preferences.putString("timezone", settings.timezone);
    preferences.putDouble("manual_lat", settings.manualLat);
    preferences.putDouble("manual_lon", settings.manualLon);
    preferences.putUChar("brightness", settings.brightness);
    preferences.putUChar("kb_light", settings.keyboardBrightness);
    preferences.putUChar("timeout", settings.screenTimeout);
    preferences.putBool("gps", settings.gpsEnabled);
    preferences.end();
}

void loadSettings()
{
    preferences.begin("flightmate", true);
    copyText(settings.wifiSsid, sizeof(settings.wifiSsid),
             preferences.getString("ssid", DEFAULT_WIFI_SSID));
    copyText(settings.wifiPassword, sizeof(settings.wifiPassword),
             preferences.getString("password", DEFAULT_WIFI_PASSWORD));
    copyText(settings.timezone, sizeof(settings.timezone),
             preferences.getString("timezone", DEFAULT_TIMEZONE));
    settings.manualLat = preferences.getDouble("manual_lat", DEFAULT_LATITUDE);
    settings.manualLon = preferences.getDouble("manual_lon", DEFAULT_LONGITUDE);
    settings.brightness = min<uint8_t>(preferences.getUChar("brightness", 2), 3);
    settings.keyboardBrightness = min<uint8_t>(preferences.getUChar("kb_light", 1), 2);
    settings.screenTimeout = min<uint8_t>(preferences.getUChar("timeout", 2), 4);
    settings.gpsEnabled = preferences.getBool("gps", true);
    preferences.end();
    setenv("TZ", settings.timezone, 1);
    tzset();
}

void updateBattery()
{
    if (lastBatteryRefresh && millis() - lastBatteryRefresh < BATTERY_REFRESH_MS) return;
    lastBatteryRefresh = millis();
    if (!instance.gauge.refresh()) {
        batteryPercent = 255;
        return;
    }
    const uint16_t value = instance.gauge.getStateOfCharge();
    batteryPercent = value <= 100 ? static_cast<uint8_t>(value) : 255;
}

void setGpsEnabled(bool enabled, bool persist = true)
{
    if (gpsInitialized && enabled == settings.gpsEnabled &&
        (!enabled || gps.hardwareReady)) return;
    gpsInitialized = true;
    settings.gpsEnabled = enabled;
    if (enabled) {
        instance.powerControl(POWER_GPS, true);
        delay(20);
        gps.hardwareReady = instance.initGPS();
        gpsCharsChangedAt = millis();
        Serial.printf("[GPS] enabled, init=%s model=%s\n",
                      gps.hardwareReady ? "OK" : "FAIL", instance.gps.getModel().c_str());
        setStatus(gps.hardwareReady ? "GPS ENABLED" : "GPS INIT FAILED");
    } else {
        Serial1.end();
        instance.powerControl(POWER_GPS, false);
        gps = GpsSnapshot{};
        setStatus("GPS DISABLED");
    }
    if (persist) saveSettings();
}

int64_t daysFromCivil(int year, unsigned month, unsigned day)
{
    year -= month <= 2;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yearOfEra = static_cast<unsigned>(year - era * 400);
    const unsigned dayOfYear = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
    const unsigned dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
    return static_cast<int64_t>(era) * 146097 + static_cast<int64_t>(dayOfEra) - 719468;
}

time_t utcEpoch(const struct tm &utc)
{
    return static_cast<time_t>(daysFromCivil(utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday) * 86400 +
                               utc.tm_hour * 3600 + utc.tm_min * 60 + utc.tm_sec);
}

void syncClockFromGps()
{
    if (!instance.gps.date.isValid() || !instance.gps.time.isValid()) return;
    struct tm utc = {};
    utc.tm_year = instance.gps.date.year() - 1900;
    utc.tm_mon = instance.gps.date.month() - 1;
    utc.tm_mday = instance.gps.date.day();
    utc.tm_hour = instance.gps.time.hour();
    utc.tm_min = instance.gps.time.minute();
    utc.tm_sec = instance.gps.time.second();
    const time_t current = time(nullptr);
    const time_t candidate = utcEpoch(utc);
    if (candidate > 1700000000 && (current < 1700000000 || llabs(candidate - current) > 2)) {
        timeval tv = {.tv_sec = candidate, .tv_usec = 0};
        settimeofday(&tv, nullptr);
        rtcSyncPending = true;
        Serial.printf("[TIME] synchronized from GNSS: %lld\n", static_cast<long long>(candidate));
    }
}

void updateGps()
{
    if (!settings.gpsEnabled || !gps.hardwareReady) return;
    gps.charsProcessed = instance.gps.loop(false);
    if (gps.charsProcessed != lastGpsChars) {
        lastGpsChars = gps.charsProcessed;
        gpsCharsChangedAt = millis();
    }
    gps.valid = instance.gps.location.isValid() && instance.gps.location.age() < 3000;
    gps.stale = !gps.valid;
    if (instance.gps.location.isValid()) {
        gps.latitude = instance.gps.location.lat();
        gps.longitude = instance.gps.location.lng();
        gps.lastLocationMs = millis() - min<uint32_t>(instance.gps.location.age(), millis());
    }
    if (instance.gps.speed.isValid()) gps.speedKmh = instance.gps.speed.kmph();
    if (instance.gps.course.isValid()) gps.courseDeg = instance.gps.course.deg();
    if (instance.gps.altitude.isValid()) gps.altitudeM = instance.gps.altitude.meters();
    if (instance.gps.satellites.isValid()) gps.satellites = instance.gps.satellites.value();
    if (instance.gps.hdop.isValid()) gps.hdop = instance.gps.hdop.hdop();
    syncClockFromGps();
}

bool initSd()
{
    sdReady = instance.installSD();
    Serial.printf("[SD] mount=%s", sdReady ? "OK" : "FAIL");
    if (sdReady) Serial.printf(" size=%lluMB", SD.cardSize() / (1024ULL * 1024ULL));
    Serial.println();
    return sdReady;
}

bool refreshSdState()
{
    const bool current = instance.isCardReady();
    if (current != sdReady) {
        sdReady = current;
        dirty = true;
        Serial.printf("[SD] state changed: %s\n", sdReady ? "READY" : "MISSING");
    }
    return sdReady;
}

double degreesToRadians(double degrees)
{
    return degrees * PI / 180.0;
}

float distanceKm(double lat1, double lon1, double lat2, double lon2)
{
    constexpr double radiusKm = 6371.0;
    const double dLat = degreesToRadians(lat2 - lat1);
    const double dLon = degreesToRadians(lon2 - lon1);
    const double a = sin(dLat / 2) * sin(dLat / 2) +
                     cos(degreesToRadians(lat1)) * cos(degreesToRadians(lat2)) *
                     sin(dLon / 2) * sin(dLon / 2);
    return static_cast<float>(radiusKm * 2 * atan2(sqrt(a), sqrt(1 - a)));
}

float bearingDegrees(double lat1, double lon1, double lat2, double lon2)
{
    const double y = sin(degreesToRadians(lon2 - lon1)) * cos(degreesToRadians(lat2));
    const double x = cos(degreesToRadians(lat1)) * sin(degreesToRadians(lat2)) -
                     sin(degreesToRadians(lat1)) * cos(degreesToRadians(lat2)) *
                         cos(degreesToRadians(lon2 - lon1));
    double result = atan2(y, x) * 180.0 / PI;
    if (result < 0) result += 360;
    return static_cast<float>(result);
}

void currentPosition(double &latitude, double &longitude)
{
    if (gps.valid) {
        latitude = gps.latitude;
        longitude = gps.longitude;
    } else {
        latitude = settings.manualLat;
        longitude = settings.manualLon;
    }
}

void updateNearestAirport(bool force = false)
{
    if (!force && lastAirportRefresh && millis() - lastAirportRefresh < AIRPORT_REFRESH_MS) return;
    lastAirportRefresh = millis();
    double latitude, longitude;
    currentPosition(latitude, longitude);
    nearestAirport = nullptr;
    nearestAirportKm = 151.0F;
    for (const Airport &airport : AIRPORTS) {
        const float distance = distanceKm(latitude, longitude, airport.latitude, airport.longitude);
        if (distance < nearestAirportKm) {
            nearestAirport = &airport;
            nearestAirportKm = distance;
        }
    }
    if (!nearestAirport || nearestAirportKm > 150.0F) {
        nearestAirport = nullptr;
        nearestAirportKm = -1;
        nearestAirportBearing = 0;
        Serial.printf("[AIRPORT] no ICAO airport within 150km at %.5f,%.5f\n", latitude, longitude);
        return;
    }
    nearestAirportBearing = bearingDegrees(latitude, longitude,
                                           nearestAirport->latitude, nearestAirport->longitude);
    Serial.printf("[AIRPORT] %s %s %.1fkm bearing %.0f\n", nearestAirport->icao,
                  nearestAirport->name, nearestAirportKm, nearestAirportBearing);
    dirty = true;
}

void loadWeatherCache()
{
    Preferences cache;
    cache.begin("weather-cache", true);
    if (cache.getUShort("schema", 0) == 1) {
        copyText(weather.icao, sizeof(weather.icao), cache.getString("icao", "----"));
        copyText(weather.raw, sizeof(weather.raw), cache.getString("raw", "No cached METAR"));
        weather.fetchedUtc = static_cast<time_t>(cache.getLong64("fetched", 0));
        copyText(weather.status, sizeof(weather.status), "CACHED");
    }
    cache.end();
}

void saveWeatherCache(const WeatherState &state)
{
    Preferences cache;
    cache.begin("weather-cache", false);
    cache.putUShort("schema", 1);
    cache.putString("icao", state.icao);
    cache.putString("raw", state.raw);
    cache.putLong64("fetched", static_cast<int64_t>(state.fetchedUtc));
    cache.putString("source", "aviationweather.gov");
    cache.end();
}

int fetchMetarWindow(const char *icao, uint8_t hours, String &report, String &detail)
{
    report = "";
    detail = "";
    WiFiClientSecure client;
    client.setCACert(DIGICERT_GLOBAL_ROOT_G2);
    HTTPClient http;
    const String url = String(METAR_URL_PREFIX) + String(hours) + "&ids=" + icao;
    http.setConnectTimeout(8000);
    http.setTimeout(12000);
    http.setUserAgent(String(APP_NAME) + "/" + APP_VERSION);
    if (!http.begin(client, url)) {
        detail = "TLS connection could not start";
        Serial.printf("[WEATHER] request icao=%s hours=%u begin=FAIL\n", icao, hours);
        return 0;
    }

    http.addHeader("Accept", "text/plain");
    const int code = http.GET();
    const int contentLength = http.getSize();
    if (code == HTTP_CODE_OK) {
        report = latestMetarReport(http.getString());
    } else if (code < 0) {
        detail = HTTPClient::errorToString(code);
    }
    Serial.printf("[WEATHER] request icao=%s hours=%u http=%d bytes=%d report=%u detail=%s\n",
                  icao, hours, code, contentLength, static_cast<unsigned>(report.length()),
                  detail.c_str());
    http.end();
    return code;
}

void weatherTask(void *)
{
    char icao[5];
    copyText(icao, sizeof(icao), weatherRequestIcao);
    WeatherState result;
    copyText(result.icao, sizeof(result.icao), icao);
    copyText(result.status, sizeof(result.status), "FETCH FAILED");
    copyText(result.raw, sizeof(result.raw), "METAR request failed");

    if (WiFi.status() != WL_CONNECTED) {
        copyText(result.status, sizeof(result.status), "OFFLINE");
        copyText(result.raw, sizeof(result.raw), "Wi-Fi is not connected");
    } else if (time(nullptr) < 1700000000) {
        copyText(result.status, sizeof(result.status), "WAITING FOR CLOCK");
        copyText(result.raw, sizeof(result.raw), "TLS requires GNSS or NTP time");
    } else {
        String report;
        String detail;
        int code = fetchMetarWindow(icao, 2, report, detail);
        bool olderReport = false;
        if (code == HTTP_CODE_NO_CONTENT || (code == HTTP_CODE_OK && report.length() <= 8)) {
            code = fetchMetarWindow(icao, 24, report, detail);
            olderReport = true;
        }

        if (code == HTTP_CODE_OK && report.length() > 8) {
            copyText(result.raw, sizeof(result.raw), report);
            copyText(result.status, sizeof(result.status), olderReport ? "OLDER 24H" : "LIVE");
            result.fetchedUtc = time(nullptr);
            saveWeatherCache(result);
        } else if (code == HTTP_CODE_NO_CONTENT || (code == HTTP_CODE_OK && report.length() <= 8)) {
            copyText(result.status, sizeof(result.status), "NO REPORT");
            copyText(result.raw, sizeof(result.raw), "No METAR available in the last 24 hours");
            result.fetchedUtc = time(nullptr);
        } else if (code == 0) {
            copyText(result.status, sizeof(result.status), "TLS START FAILED");
            copyText(result.raw, sizeof(result.raw), detail);
        } else if (code < 0) {
            copyText(result.status, sizeof(result.status), "NETWORK ERROR");
            copyText(result.raw, sizeof(result.raw), detail);
        } else {
            snprintf(result.status, sizeof(result.status), "HTTP %d", code);
            copyText(result.raw, sizeof(result.raw),
                     detail.length() ? detail : String("AviationWeather request failed"));
        }
    }

    if (weatherMutex && xSemaphoreTake(weatherMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        weather = result;
        xSemaphoreGive(weatherMutex);
    }
    Serial.printf("[WEATHER] %s %s\n", result.icao, result.status);
    weatherFetchRunning = false;
    dirty = true;
    vTaskDelete(nullptr);
}

void requestWeather(bool force = false)
{
    if (!nearestAirport || weatherFetchRunning) return;
    if (!force && lastWeatherRequest && millis() - lastWeatherRequest < WEATHER_RETRY_MS) return;
    copyText(weatherRequestIcao, sizeof(weatherRequestIcao), nearestAirport->icao);
    lastWeatherRequest = millis();
    weatherFetchRunning = true;
    if (xTaskCreate(weatherTask, "metar", 8192, nullptr, 2, nullptr) != pdPASS) {
        weatherFetchRunning = false;
        setStatus("METAR TASK FAILED");
    } else {
        setStatus("METAR REFRESH STARTED", 1500);
    }
}

void connectWifi()
{
    if (!settings.wifiSsid[0]) {
        WiFi.mode(WIFI_OFF);
        return;
    }
    lastWifiAttempt = millis();
    WiFi.mode(WIFI_STA);
    WiFi.begin(settings.wifiSsid, settings.wifiPassword);
    configTzTime(settings.timezone, "pool.ntp.org", "time.cloudflare.com");
    Serial.printf("[NET] connecting SSID=%s\n", settings.wifiSsid);
}

void updateNetwork()
{
    if (settings.wifiSsid[0] && WiFi.status() != WL_CONNECTED &&
        millis() - lastWifiAttempt > 30000) connectWifi();
    if (page == Page::Airport && nearestAirport && !weatherFetchRunning) {
        const bool airportChanged = strcmp(weather.icao, nearestAirport->icao) != 0;
        const bool stale = weather.fetchedUtc == 0 || time(nullptr) - weather.fetchedUtc > WEATHER_REFRESH_MS / 1000;
        if (WiFi.status() == WL_CONNECTED && (airportChanged || stale) &&
            millis() - lastWeatherRequest > WEATHER_RETRY_MS) requestWeather(false);
    }
}

double mercatorWorldX(double longitude, uint8_t zoom)
{
    return (longitude + 180.0) / 360.0 * (1UL << zoom) * 256.0;
}

double mercatorWorldY(double latitude, uint8_t zoom)
{
    latitude = constrain(latitude, -85.05112878, 85.05112878);
    const double latRad = degreesToRadians(latitude);
    return (1.0 - log(tan(latRad) + 1.0 / cos(latRad)) / PI) / 2.0 *
           (1UL << zoom) * 256.0;
}

int32_t wrapTileX(int32_t x, uint8_t zoom)
{
    const int32_t count = 1L << zoom;
    x %= count;
    if (x < 0) x += count;
    return x;
}

MapTile *findTile(uint8_t zoom, int32_t x, int32_t y)
{
    for (MapTile &tile : tileCache) {
        if (tile.valid && tile.zoom == zoom && tile.x == x && tile.y == y) {
            tile.usedAt = millis();
            return &tile;
        }
    }
    return nullptr;
}

MapTile *tileSlot()
{
    MapTile *slot = &tileCache[0];
    for (MapTile &tile : tileCache) {
        if (!tile.valid) return &tile;
        if (tile.usedAt < slot->usedAt) slot = &tile;
    }
    return slot;
}

void clearTileCache()
{
    for (MapTile &tile : tileCache) {
        tile.valid = false;
        if (tile.pixels) heap_caps_free(tile.pixels);
        tile.pixels = nullptr;
    }
}

void setMapTileStatus(const char *status, bool error)
{
    if (strcmp(mapState.tileStatus, status) != 0 || mapState.tileStatusError != error) dirty = true;
    copyText(mapState.tileStatus, sizeof(mapState.tileStatus), status);
    mapState.tileStatusError = error;
}

bool validateMapManifest()
{
    mapState.manifestChecked = true;
    mapState.manifestValid = false;
    mapState.tileStatus[0] = '\0';
    mapState.tileStatusError = false;
    mapTileRootIndex = -1;
    copyText(mapState.manifestName, sizeof(mapState.manifestName), "SD missing");
    if (!refreshSdState() && !initSd()) {
        mapState.manifestChecked = false;
        return false;
    }
    copyText(mapState.manifestName, sizeof(mapState.manifestName), "Manifest missing");
    const char *paths[] = {"/FlightMate/maps/manifest.json", "/FlightMate/manifest.json",
                           "/maps/manifest.json", "/manifest.json"};
    if (!instance.lockSPI(pdMS_TO_TICKS(1000))) return false;
    File file;
    for (const char *path : paths) {
        file = SD.open(path, FILE_READ);
        if (file) break;
    }
    if (!file) {
        instance.unlockSPI();
        mapState.manifestValid = true;
        copyText(mapState.manifestName, sizeof(mapState.manifestName), "Direct tiles");
        Serial.println("[MAP] manifest=NOT_FOUND mode=DIRECT_TILES");
        return true;
    }
    JsonDocument doc;
    const DeserializationError error = deserializeJson(doc, file);
    file.close();
    instance.unlockSPI();
    if (error) {
        copyText(mapState.manifestName, sizeof(mapState.manifestName), "Manifest JSON invalid");
        return false;
    }
    const int minZoom = doc["minZoom"] | MAP_MIN_ZOOM;
    const int maxZoom = doc["maxZoom"] | MAP_MAX_ZOOM;
    const bool valid = doc["schema"].as<int>() == 1 && doc["tileSize"].as<int>() == 256 &&
                       strcmp(doc["format"] | "", "png") == 0 &&
                       strcmp(doc["scheme"] | "", "xyz") == 0 &&
                       minZoom >= 0 && minZoom <= maxZoom && maxZoom <= 22;
    mapState.manifestValid = valid;
    copyText(mapState.manifestName, sizeof(mapState.manifestName),
             valid ? String(doc["name"] | "FlightMate map") : String("Manifest incompatible"));
    Serial.printf("[MAP] manifest=%s name=%s\n", valid ? "OK" : "FAIL", mapState.manifestName);
    return valid;
}

MapTile *loadMapTile(uint8_t zoom, int32_t x, int32_t y)
{
    x = wrapTileX(x, zoom);
    if (y < 0 || y >= (1L << zoom) || !mapState.manifestValid) return nullptr;
    if (!sdReady) {
        setMapTileStatus("SD missing", true);
        return nullptr;
    }
    if (MapTile *cached = findTile(zoom, x, y)) return cached;

    static const char *pathPatterns[] = {
        "/FlightMate/maps/base/osm/%u/%ld/%ld.png",
        "/maps/base/osm/%u/%ld/%ld.png"
    };
    char path[96] = {};
    if (!instance.lockSPI(pdMS_TO_TICKS(1500))) {
        setMapTileStatus("SPI busy", true);
        return nullptr;
    }
    File file;
    size_t size = 0;
    int8_t selectedRoot = -1;
    bool invalidSize = false;
    for (uint8_t attempt = 0; attempt < 2; ++attempt) {
        const uint8_t root = mapTileRootIndex >= 0
                                 ? static_cast<uint8_t>(attempt == 0 ? mapTileRootIndex : 1 - mapTileRootIndex)
                                 : attempt;
        snprintf(path, sizeof(path), pathPatterns[root], zoom,
                 static_cast<long>(x), static_cast<long>(y));
        file = SD.open(path, FILE_READ);
        if (!file) continue;
        size = file.size();
        if (size == 0 || size > MAX_PNG_BYTES) {
            invalidSize = true;
            file.close();
            continue;
        }
        selectedRoot = static_cast<int8_t>(root);
        break;
    }
    if (selectedRoot < 0) {
        instance.unlockSPI();
        setMapTileStatus(invalidSize ? "PNG size invalid" : "Tile not found", true);
        static uint32_t lastMissingLog = 0;
        if (lastMissingLog == 0 || millis() - lastMissingLog >= 5000) {
            lastMissingLog = millis();
            Serial.printf("[MAP] tile unavailable z%u/%ld/%ld tried=FlightMate,/maps size_invalid=%s\n",
                          zoom, static_cast<long>(x), static_cast<long>(y),
                          invalidSize ? "YES" : "NO");
        }
        return nullptr;
    }
    mapTileRootIndex = selectedRoot;
    uint8_t *compressed = static_cast<uint8_t *>(heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!compressed) {
        file.close();
        instance.unlockSPI();
        setMapTileStatus("No PSRAM input", true);
        return nullptr;
    }
    const size_t read = file.read(compressed, size);
    file.close();
    instance.unlockSPI();
    if (read != size) {
        heap_caps_free(compressed);
        setMapTileStatus("SD read failed", true);
        return nullptr;
    }

    lv_draw_buf_t *decoded = nullptr;
    unsigned width = 0;
    unsigned height = 0;
    const unsigned error = lodepng_decode32(reinterpret_cast<unsigned char **>(&decoded),
                                            &width, &height, compressed, size);
    heap_caps_free(compressed);
    if (error || width != 256 || height != 256 || !decoded || !decoded->data) {
        if (decoded) lv_draw_buf_destroy(decoded);
        Serial.printf("[MAP] decode failed z%u/%ld/%ld error=%u size=%ux%u\n",
                      zoom, static_cast<long>(x), static_cast<long>(y), error, width, height);
        if (error) {
            char status[32];
            snprintf(status, sizeof(status), "PNG error %u", error);
            setMapTileStatus(status, true);
        } else {
            setMapTileStatus("PNG not 256x256", true);
        }
        return nullptr;
    }

    MapTile *slot = tileSlot();
    if (!slot->pixels) {
        slot->pixels = static_cast<uint16_t *>(heap_caps_malloc(256U * 256U * sizeof(uint16_t),
                                                               MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    }
    if (!slot->pixels) {
        lv_draw_buf_destroy(decoded);
        setMapTileStatus("No PSRAM tile", true);
        return nullptr;
    }
    const uint8_t *rgba = decoded->data;
    for (size_t index = 0; index < 256U * 256U; ++index) {
        const uint8_t red = rgba[index * 4];
        const uint8_t green = rgba[index * 4 + 1];
        const uint8_t blue = rgba[index * 4 + 2];
        slot->pixels[index] = static_cast<uint16_t>(((red & 0xF8) << 8) |
                                                    ((green & 0xFC) << 3) | (blue >> 3));
    }
    lv_draw_buf_destroy(decoded);
    slot->valid = true;
    slot->zoom = zoom;
    slot->x = x;
    slot->y = y;
    slot->usedAt = millis();
    setMapTileStatus(selectedRoot == 0 ? "Tiles FlightMate" : "Tiles /maps", false);
    Serial.printf("[MAP] loaded z%u/%ld/%ld path=%s\n", zoom, static_cast<long>(x),
                  static_cast<long>(y), path);
    return slot;
}

void keyboardRawCallback(bool pressed, uint8_t raw)
{
    portENTER_CRITICAL(&keyMux);
    const uint8_t next = static_cast<uint8_t>((keyWriteIndex + 1) % KEY_QUEUE_SIZE);
    if (next != keyReadIndex) {
        keyQueue[keyWriteIndex].pressed = pressed;
        keyQueue[keyWriteIndex].raw = raw;
        keyWriteIndex = next;
    }
    portEXIT_CRITICAL(&keyMux);
}

char rawKeyToCharacter(uint8_t raw)
{
    static constexpr char normal[4][10] = {
        {'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p'},
        {'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', '\n'},
        {'\0', 'z', 'x', 'c', 'v', 'b', 'n', 'm', '\0', '\b'},
        {' ', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'}
    };
    static constexpr char symbols[4][10] = {
        {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0'},
        {'*', '/', '+', '-', '=', ':', '\'', '"', '@', '\n'},
        {'\0', '_', '$', ';', '?', '!', ',', '.', '\0', '\b'},
        {' ', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'}
    };
    if (!raw || raw > 40) return '\0';
    const uint8_t index = raw - 1;
    char value = symbolMode ? symbols[index / 10][index % 10] : normal[index / 10][index % 10];
    if (capsMode && value >= 'a' && value <= 'z') value = static_cast<char>(toupper(value));
    return value;
}

void wakeScreen()
{
    lastInputAt = millis();
    if (!screenOff) return;
    screenOff = false;
    instance.wakeupDisplay();
    applyBrightness();
    dirty = true;
}

void cycleDisplayBrightness()
{
    settings.brightness = (settings.brightness + 1) % 4;
    applyBrightness();
    saveSettings();
    setStatus("DISPLAY " + String(DISPLAY_PERCENT[settings.brightness]) + "%");
}

void cycleKeyboardBrightness()
{
    settings.keyboardBrightness = (settings.keyboardBrightness + 1) % 3;
    applyBrightness();
    saveSettings();
    const char *names[] = {"OFF", "MEDIUM", "HIGH"};
    setStatus("KEYBOARD " + String(names[settings.keyboardBrightness]));
}

[[noreturn]] void enterDeepSleep()
{
    canvas.fillScreen(C_BLACK);
    drawCentered("LOW POWER STANDBY", SCREEN_W / 2, 78, C_WARNING, 2);
    drawCentered("Press BOOT to wake", SCREEN_W / 2, 122, C_PRIMARY, 1);
    flushDisplay();
    Serial.println("[POWER] entering deep sleep; BOOT wake source");
    Serial.flush();
    WiFi.disconnect(true, false);
    WiFi.mode(WIFI_OFF);
    instance.kb.setBrightness(0);
    instance.setBrightness(0);
    instance.sleep(WAKEUP_SRC_BOOT_BUTTON, false, 0);
    while (true) delay(1000);
}

String clippedText(const String &value, size_t maximum)
{
    if (value.length() <= maximum) return value;
    if (maximum < 4) return value.substring(0, maximum);
    return value.substring(0, maximum - 3) + "...";
}

String localClockText()
{
    const time_t now = time(nullptr);
    if (now < 1700000000) return "--:--";
    struct tm local = {};
    localtime_r(&now, &local);
    char value[20] = {};
    strftime(value, sizeof(value), "%H:%M", &local);
    return value;
}

String utcClockText()
{
    const time_t now = time(nullptr);
    if (now < 1700000000) return "--:--";
    struct tm utc = {};
    gmtime_r(&now, &utc);
    char value[20] = {};
    strftime(value, sizeof(value), "%H:%M", &utc);
    return value;
}

void drawChrome(const String &hints)
{
    canvas.fillRect(0, 0, SCREEN_W, TOP_H, C_BLACK);
    canvas.drawFastHLine(0, TOP_H - 1, SCREEN_W, C_GRID);
    drawText(String(APP_NAME) + " " + APP_VERSION, 8, 7, C_PRIMARY);
    drawCentered(pageName(), SCREEN_W / 2, 7, C_TEXT);
    drawText(localClockText(), 362, 7, C_MUTED);
    const String battery = batteryPercent <= 100 ? String(batteryPercent) + "%" : "--%";
    drawText(battery, 438, 7, batteryPercent != 255 && batteryPercent < 20 ? C_DANGER : C_TEXT);

    canvas.fillRect(0, SCREEN_H - BOTTOM_H, SCREEN_W, BOTTOM_H, C_BLACK);
    canvas.drawFastHLine(0, SCREEN_H - BOTTOM_H, SCREEN_W, C_GRID);
    String footer = hints;
    if (transientStatus.length() && static_cast<int32_t>(transientStatusUntil - millis()) > 0) {
        footer = transientStatus;
    }
    drawCentered(clippedText(footer, 72), SCREEN_W / 2, SCREEN_H - 16, C_MUTED);
}

void drawHome()
{
    canvas.fillScreen(C_BG);
    static const char *titles[] = {"AIRPORT", "FLIGHT", "LOGBOOK", "SETTINGS"};
    static const char *subtitles[] = {"Nearest + METAR", "GNSS + offline map",
                                      "Browse and edit", "Network and device"};
    for (uint8_t index = 0; index < 4; ++index) {
        const int16_t x = index % 2 ? 246 : 18;
        const int16_t y = index / 2 ? 113 : 36;
        drawPanel(x, y, 216, 64, homeSelection == index);
        drawText(titles[index], x + 14, y + 14, homeSelection == index ? C_PRIMARY : C_TEXT, 2);
        drawText(subtitles[index], x + 14, y + 43, C_MUTED);
    }
    drawChrome("ROTATE/WASD Select  ENTER Open  B Key light");
}

void drawAirport()
{
    canvas.fillScreen(C_BG);
    WeatherState snapshot;
    if (weatherMutex && xSemaphoreTake(weatherMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        snapshot = weather;
        xSemaphoreGive(weatherMutex);
    } else {
        snapshot = weather;
    }
    drawPanel(14, 35, 142, 145);
    drawPanel(166, 35, 300, 145);
    const char *icao = nearestAirport ? nearestAirport->icao : "----";
    drawCentered(icao, 85, 50, nearestAirport ? C_PRIMARY : C_WARNING, 3);
    drawCentered(nearestAirport ? clippedText(nearestAirport->name, 22) : "NO AIRPORT <150KM",
                 85, 87, C_TEXT);
    if (nearestAirport) {
        drawCentered(String(nearestAirportKm, 1) + " KM", 85, 112, C_ACCENT, 2);
        drawCentered(String(nearestAirportBearing, 0) + " DEG", 85, 140, C_MUTED);
    }
    drawCentered("UTC " + utcClockText(), 85, 159, C_PRIMARY);
    drawText("METAR / " + String(snapshot.status), 178, 48, C_PRIMARY);
    String report = latestMetarReport(snapshot.raw);
    for (uint8_t line = 0; line < 6 && report.length(); ++line) {
        const size_t width = 23;
        size_t end = min(report.length(), width);
        if (end < report.length()) {
            const int split = report.substring(0, end).lastIndexOf(' ');
            if (split > 8) end = split;
        }
        drawText(report.substring(0, end), 178, 65 + line * 17, C_TEXT, 2);
        report.remove(0, min(report.length(), end + (end < report.length() && report[end] == ' ' ? 1U : 0U)));
    }
    if (snapshot.fetchedUtc > 0) {
        struct tm local = {};
        localtime_r(&snapshot.fetchedUtc, &local);
        char fetched[32] = {};
        strftime(fetched, sizeof(fetched), "UPDATED %m-%d %H:%M", &local);
        drawText(fetched, 178, 170, C_MUTED);
    }
    drawChrome("R Refresh METAR  L Display  Q/H Home  B Key light");
}

void blitTile(const MapTile &tile, int32_t destinationX, int32_t destinationY)
{
    const int32_t clipLeft = max<int32_t>(0, destinationX);
    const int32_t clipTop = max<int32_t>(CONTENT_Y, destinationY);
    const int32_t clipRight = min<int32_t>(MAP_W, destinationX + 256);
    const int32_t clipBottom = min<int32_t>(CONTENT_Y + MAP_H, destinationY + 256);
    if (clipLeft >= clipRight || clipTop >= clipBottom) return;
    uint16_t *destination = canvas.getBuffer();
    for (int32_t y = clipTop; y < clipBottom; ++y) {
        const size_t sourceOffset = static_cast<size_t>(y - destinationY) * 256 +
                                    static_cast<size_t>(clipLeft - destinationX);
        const size_t destinationOffset = static_cast<size_t>(y) * SCREEN_W + clipLeft;
        memcpy(destination + destinationOffset, tile.pixels + sourceOffset,
               static_cast<size_t>(clipRight - clipLeft) * sizeof(uint16_t));
    }
}

void drawMissingTile(int32_t x, int32_t y, int32_t tileX, int32_t tileY)
{
    const int16_t left = max<int32_t>(0, x);
    const int16_t top = max<int32_t>(CONTENT_Y, y);
    const int16_t right = min<int32_t>(MAP_W, x + 256);
    const int16_t bottom = min<int32_t>(CONTENT_Y + MAP_H, y + 256);
    if (left >= right || top >= bottom) return;
    canvas.fillRect(left, top, right - left, bottom - top, C_PANEL);
    for (int16_t gridX = left; gridX < right; gridX += 32) {
        canvas.drawFastVLine(gridX, top, bottom - top, C_GRID);
    }
    for (int16_t gridY = top; gridY < bottom; gridY += 32) {
        canvas.drawFastHLine(left, gridY, right - left, C_GRID);
    }
    drawText("z" + String(mapState.zoom) + "/" + String(tileX) + "/" + String(tileY),
             left + 6, top + 8, C_MUTED);
}

void drawFlight()
{
    canvas.fillScreen(C_BG);
    double latitude, longitude;
    currentPosition(latitude, longitude);
    const double centerWorldX = mercatorWorldX(longitude, mapState.zoom) + mapState.panX;
    const double centerWorldY = mercatorWorldY(latitude, mapState.zoom) + mapState.panY;
    const double leftWorld = centerWorldX - MAP_W / 2.0;
    const double topWorld = centerWorldY - MAP_H / 2.0;
    const int32_t firstTileX = static_cast<int32_t>(floor(leftWorld / 256.0));
    const int32_t lastTileX = static_cast<int32_t>(floor((leftWorld + MAP_W - 1) / 256.0));
    const int32_t firstTileY = static_cast<int32_t>(floor(topWorld / 256.0));
    const int32_t lastTileY = static_cast<int32_t>(floor((topWorld + MAP_H - 1) / 256.0));

    for (int32_t tileY = firstTileY; tileY <= lastTileY; ++tileY) {
        for (int32_t tileX = firstTileX; tileX <= lastTileX; ++tileX) {
            const int32_t screenX = static_cast<int32_t>(tileX * 256.0 - leftWorld);
            const int32_t screenY = CONTENT_Y + static_cast<int32_t>(tileY * 256.0 - topWorld);
            MapTile *tile = loadMapTile(mapState.zoom, tileX, tileY);
            if (tile) blitTile(*tile, screenX, screenY);
            else drawMissingTile(screenX, screenY, wrapTileX(tileX, mapState.zoom), tileY);
        }
    }

    const int16_t markerX = MAP_W / 2 - mapState.panX;
    const int16_t markerY = CONTENT_Y + MAP_H / 2 - mapState.panY;
    if (markerX >= 4 && markerX < MAP_W - 4 && markerY >= CONTENT_Y + 4 && markerY < CONTENT_Y + MAP_H - 4) {
        canvas.fillCircle(markerX, markerY, 5, gps.valid ? C_PRIMARY : C_WARNING);
        canvas.drawCircle(markerX, markerY, 8, C_BLACK);
    }
    canvas.fillRect(MAP_W, CONTENT_Y, SCREEN_W - MAP_W, CONTENT_H, C_PANEL);
    canvas.drawFastVLine(MAP_W, CONTENT_Y, CONTENT_H, C_GRID);
    drawText(gps.valid ? "GNSS FIX" : (gps.hardwareReady ? "GNSS SEARCH" : "GNSS OFF"),
             352, 34, gps.valid ? C_PRIMARY : C_WARNING);
    drawText(gps.valid ? String(gps.speedKmh, 1) : "---", 351, 57, C_TEXT, 2);
    drawText("KM/H", 421, 65, C_MUTED);
    drawText(gps.valid ? String(gps.altitudeM, 0) : "---", 351, 88, C_ACCENT, 2);
    drawText("GPS ALT M", 351, 111, C_MUTED);
    drawText("SAT " + String(gps.satellites) + "  HDOP " +
             (gps.hdop > 0 ? String(gps.hdop, 1) : String("--")), 351, 134, C_TEXT);
    const bool hasTileStatus = mapState.tileStatus[0] != '\0';
    const String mapStatus = hasTileStatus
                                 ? String(mapState.tileStatus)
                                 : (mapState.manifestValid ? clippedText(mapState.manifestName, 17)
                                                           : String(mapState.manifestName));
    const bool mapStatusError = hasTileStatus ? mapState.tileStatusError : !mapState.manifestValid;
    drawText("Z" + String(mapState.zoom) + "  " + clippedText(mapStatus, 17),
             351, 157, mapStatusError ? C_DANGER : C_MUTED);
    drawText("(c) OSM contributors", 8, 183, C_BLACK);
    drawChrome("WASD Pan  Q/E Zoom  C Center  H Home  L Display");
}

time_t logCreatedUtc = 0;
bool confirmDiscard = false;

void jsonValueToText(JsonVariantConst value, char *destination, size_t capacity, uint8_t decimals = 0)
{
    if (value.is<const char *>()) {
        copyText(destination, capacity, value.as<const char *>());
    } else if (value.is<double>()) {
        copyText(destination, capacity,
                 String(value.as<double>(), static_cast<unsigned int>(decimals)));
    } else if (value.is<long>()) {
        copyText(destination, capacity, String(value.as<long>()));
    } else {
        copyText(destination, capacity, "");
    }
}

bool ensureLogDirectories()
{
    if (!refreshSdState() || !instance.lockSPI(pdMS_TO_TICKS(1500))) return false;
    bool ok = SD.exists("/FlightLog") || SD.mkdir("/FlightLog");
    if (ok) ok = SD.exists("/FlightLog/.tmp") || SD.mkdir("/FlightLog/.tmp");
    instance.unlockSPI();
    return ok;
}

void cleanupLogTemps()
{
    if (!ensureLogDirectories() || !instance.lockSPI(pdMS_TO_TICKS(1500))) return;
    File directory = SD.open("/FlightLog/.tmp");
    if (directory) {
        File item = directory.openNextFile();
        while (item) {
            const String path = item.path();
            const bool removable = !item.isDirectory() &&
                                   (path.endsWith(".tmp") || path.endsWith(".bak"));
            item.close();
            if (removable) SD.remove(path);
            item = directory.openNextFile();
        }
        directory.close();
    }
    instance.unlockSPI();
}

bool readLogRecordLocked(const char *path, LogRecord &record, time_t &createdUtc)
{
    File file = SD.open(path, FILE_READ);
    if (!file) return false;
    JsonDocument doc;
    const DeserializationError error = deserializeJson(doc, file);
    file.close();
    if (error || doc["schema"].as<int>() != 1 || !doc["flight"].is<JsonObject>()) return false;

    record = LogRecord{};
    createdUtc = static_cast<time_t>(doc["createdUtc"].as<int64_t>());
    copyText(record.date, sizeof(record.date), doc["flight"]["date"] | "");
    copyText(record.flightNo, sizeof(record.flightNo), doc["flight"]["flightNo"] | "");
    copyText(record.departureIcao, sizeof(record.departureIcao), doc["flight"]["departureIcao"] | "");
    copyText(record.arrivalIcao, sizeof(record.arrivalIcao), doc["flight"]["arrivalIcao"] | "");
    copyText(record.departureLocal, sizeof(record.departureLocal), doc["flight"]["departureLocal"] | "");
    copyText(record.arrivalLocal, sizeof(record.arrivalLocal), doc["flight"]["arrivalLocal"] | "");
    copyText(record.airline, sizeof(record.airline), doc["aircraft"]["airline"] | "");
    copyText(record.aircraftType, sizeof(record.aircraftType), doc["aircraft"]["type"] | "");
    copyText(record.registration, sizeof(record.registration), doc["aircraft"]["registration"] | "");
    copyText(record.engine, sizeof(record.engine), doc["aircraft"]["engine"] | "");
    jsonValueToText(doc["aircraft"]["ageYears"], record.ageYears, sizeof(record.ageYears), 1);
    copyText(record.cabin, sizeof(record.cabin), doc["seat"]["cabin"] | "ECONOMY");
    copyText(record.seatNo, sizeof(record.seatNo), doc["seat"]["seatNo"] | "");
    copyText(record.departureRunway, sizeof(record.departureRunway), doc["operations"]["departureRunway"] | "");
    copyText(record.arrivalRunway, sizeof(record.arrivalRunway), doc["operations"]["arrivalRunway"] | "");
    copyText(record.gate, sizeof(record.gate), doc["operations"]["gate"] | "");
    copyText(record.standType, sizeof(record.standType), doc["operations"]["standType"] | "JET_BRIDGE");
    copyText(record.pushbackLocal, sizeof(record.pushbackLocal), doc["operations"]["pushbackLocal"] | "");
    copyText(record.takeoffLocal, sizeof(record.takeoffLocal), doc["operations"]["takeoffLocal"] | "");
    copyText(record.landingLocal, sizeof(record.landingLocal), doc["operations"]["landingLocal"] | "");
    copyText(record.onBlockLocal, sizeof(record.onBlockLocal), doc["operations"]["onBlockLocal"] | "");
    jsonValueToText(doc["metrics"]["cruiseAltitudeM"], record.cruiseAltitudeM, sizeof(record.cruiseAltitudeM));
    const int64_t durationSec = doc["metrics"]["durationSec"] | 0;
    copyText(record.durationMin, sizeof(record.durationMin), durationSec > 0 ? String(durationSec / 60) : String(""));
    jsonValueToText(doc["metrics"]["distanceKm"], record.distanceKm, sizeof(record.distanceKm), 1);
    jsonValueToText(doc["metrics"]["cruiseSpeedKmh"], record.cruiseSpeedKmh, sizeof(record.cruiseSpeedKmh), 1);
    jsonValueToText(doc["experience"]["mealRating"], record.mealRating, sizeof(record.mealRating));
    jsonValueToText(doc["experience"]["serviceRating"], record.serviceRating, sizeof(record.serviceRating));
    copyText(record.notes, sizeof(record.notes), doc["experience"]["notes"] | "");
    return true;
}

bool loadLogRecord(const char *path)
{
    if (!refreshSdState() || !instance.lockSPI(pdMS_TO_TICKS(1500))) return false;
    LogRecord loaded;
    time_t created = 0;
    const bool ok = readLogRecordLocked(path, loaded, created);
    instance.unlockSPI();
    if (!ok) return false;
    logRecord = loaded;
    logCreatedUtc = created;
    copyText(currentLogPath, sizeof(currentLogPath), path);
    logIsNew = false;
    return true;
}

int compareLogEntries(const LogIndexEntry &left, const LogIndexEntry &right)
{
    int result = strcmp(right.date, left.date);
    if (!result) result = strcmp(right.departureLocal, left.departureLocal);
    if (!result) result = strcmp(right.path, left.path);
    return result;
}

bool writeLogIndexLocked()
{
    const char *temporary = "/FlightLog/.tmp/index.bin.tmp";
    const char *finalPath = "/FlightLog/index.bin";
    const char *backup = "/FlightLog/.tmp/index.bin.bak";
    SD.remove(temporary);
    File file = SD.open(temporary, FILE_WRITE);
    if (!file) return false;
    const LogIndexHeader header = {INDEX_MAGIC, 1, logCount};
    bool ok = file.write(reinterpret_cast<const uint8_t *>(&header), sizeof(header)) == sizeof(header);
    if (ok && logCount) {
        const size_t bytes = static_cast<size_t>(logCount) * sizeof(LogIndexEntry);
        ok = file.write(reinterpret_cast<const uint8_t *>(logEntries), bytes) == bytes;
    }
    file.flush();
    file.close();
    if (!ok) {
        SD.remove(temporary);
        return false;
    }
    SD.remove(backup);
    const bool hadFinal = SD.exists(finalPath);
    if (hadFinal && !SD.rename(finalPath, backup)) {
        SD.remove(temporary);
        return false;
    }
    if (!SD.rename(temporary, finalPath)) {
        if (hadFinal) SD.rename(backup, finalPath);
        return false;
    }
    SD.remove(backup);
    return true;
}

bool rebuildLogIndex()
{
    if (!ensureLogDirectories() || !instance.lockSPI(pdMS_TO_TICKS(3000))) return false;
    logCount = 0;
    File directory = SD.open("/FlightLog");
    if (directory) {
        File item = directory.openNextFile();
        while (item && logCount < 64) {
            const String path = item.path();
            const bool candidate = !item.isDirectory() && path.endsWith(".json");
            item.close();
            if (candidate) {
                LogRecord record;
                time_t created = 0;
                if (readLogRecordLocked(path.c_str(), record, created)) {
                    LogIndexEntry &entry = logEntries[logCount++];
                    memset(&entry, 0, sizeof(entry));
                    copyText(entry.path, sizeof(entry.path), path);
                    copyText(entry.date, sizeof(entry.date), record.date);
                    copyText(entry.departureLocal, sizeof(entry.departureLocal), record.departureLocal);
                    copyText(entry.flightNo, sizeof(entry.flightNo), record.flightNo);
                    copyText(entry.departureIcao, sizeof(entry.departureIcao), record.departureIcao);
                    copyText(entry.arrivalIcao, sizeof(entry.arrivalIcao), record.arrivalIcao);
                }
            }
            item = directory.openNextFile();
        }
        directory.close();
    }
    for (uint16_t i = 1; i < logCount; ++i) {
        LogIndexEntry value = logEntries[i];
        int j = i - 1;
        while (j >= 0 && compareLogEntries(logEntries[j], value) > 0) {
            logEntries[j + 1] = logEntries[j];
            --j;
        }
        logEntries[j + 1] = value;
    }
    const bool ok = writeLogIndexLocked();
    instance.unlockSPI();
    logSelection = logCount ? min<uint16_t>(logSelection, logCount - 1) : 0;
    Serial.printf("[LOG] index rebuilt count=%u status=%s\n", logCount, ok ? "OK" : "FAIL");
    return ok;
}

bool loadLogIndex()
{
    if (!ensureLogDirectories() || !instance.lockSPI(pdMS_TO_TICKS(1500))) return false;
    File file = SD.open("/FlightLog/index.bin", FILE_READ);
    LogIndexHeader header = {};
    bool ok = file && file.read(reinterpret_cast<uint8_t *>(&header), sizeof(header)) == sizeof(header) &&
              header.magic == INDEX_MAGIC && header.schema == 1 && header.count <= 64;
    if (ok && header.count) {
        const size_t bytes = static_cast<size_t>(header.count) * sizeof(LogIndexEntry);
        ok = file.read(reinterpret_cast<uint8_t *>(logEntries), bytes) == bytes;
    }
    if (file) file.close();
    instance.unlockSPI();
    if (!ok) return rebuildLogIndex();
    logCount = header.count;
    logSelection = logCount ? min<uint16_t>(logSelection, logCount - 1) : 0;
    return true;
}

bool validDate(const char *value)
{
    if (strlen(value) != 10 || value[4] != '-' || value[7] != '-') return false;
    for (uint8_t i = 0; i < 10; ++i) {
        if (i != 4 && i != 7 && !isdigit(static_cast<unsigned char>(value[i]))) return false;
    }
    const int month = atoi(value + 5);
    const int day = atoi(value + 8);
    return month >= 1 && month <= 12 && day >= 1 && day <= 31;
}

bool validClock(const char *value)
{
    if (!value[0]) return true;
    if (strlen(value) != 5 || value[2] != ':') return false;
    return isdigit(value[0]) && isdigit(value[1]) && isdigit(value[3]) && isdigit(value[4]) &&
           atoi(value) < 24 && atoi(value + 3) < 60;
}

bool validIcao(const char *value)
{
    if (strlen(value) != 4) return false;
    for (uint8_t index = 0; index < 4; ++index) {
        if (!isalnum(static_cast<unsigned char>(value[index]))) return false;
    }
    return true;
}

bool validateLogRecord(String &reason)
{
    if (!validDate(logRecord.date)) reason = "DATE MUST BE YYYY-MM-DD";
    else if (!validIcao(logRecord.departureIcao) || !validIcao(logRecord.arrivalIcao)) reason = "ICAO MUST BE 4 CHARACTERS";
    else if (!validClock(logRecord.departureLocal) || !validClock(logRecord.arrivalLocal) ||
             !validClock(logRecord.pushbackLocal) || !validClock(logRecord.takeoffLocal) ||
             !validClock(logRecord.landingLocal) || !validClock(logRecord.onBlockLocal)) reason = "TIME MUST BE HH:MM";
    else if ((logRecord.mealRating[0] && (atoi(logRecord.mealRating) < 1 || atoi(logRecord.mealRating) > 5)) ||
             (logRecord.serviceRating[0] && (atoi(logRecord.serviceRating) < 1 || atoi(logRecord.serviceRating) > 5))) {
        reason = "RATINGS MUST BE 1-5";
    } else return true;
    return false;
}

String sanitizedFlightNumber()
{
    String result;
    for (const char *p = logRecord.flightNo; *p && result.length() < 12; ++p) {
        const char value = static_cast<char>(toupper(static_cast<unsigned char>(*p)));
        if (isalnum(static_cast<unsigned char>(value)) || value == '-' || value == '_') result += value;
    }
    return result.length() ? result : String("UNKNOWN");
}

String createLogPath()
{
    char stamp[24] = {};
    const time_t now = time(nullptr);
    if (now >= 1700000000) {
        struct tm local = {};
        localtime_r(&now, &local);
        strftime(stamp, sizeof(stamp), "%Y%m%d-%H%M%S", &local);
    } else {
        snprintf(stamp, sizeof(stamp), "UNSYNC-%010lu", static_cast<unsigned long>(millis()));
    }
    return String("/FlightLog/") + stamp + "-" + sanitizedFlightNumber() + ".json";
}

void buildLogJson(JsonDocument &doc, const String &id, time_t now)
{
    doc["schema"] = 1;
    doc["id"] = id;
    doc["createdUtc"] = static_cast<int64_t>(logCreatedUtc ? logCreatedUtc : now);
    doc["updatedUtc"] = static_cast<int64_t>(now);
    JsonObject flight = doc["flight"].to<JsonObject>();
    flight["date"] = logRecord.date;
    flight["flightNo"] = logRecord.flightNo;
    flight["departureIcao"] = logRecord.departureIcao;
    flight["arrivalIcao"] = logRecord.arrivalIcao;
    flight["departureLocal"] = logRecord.departureLocal;
    flight["arrivalLocal"] = logRecord.arrivalLocal;
    JsonObject aircraft = doc["aircraft"].to<JsonObject>();
    aircraft["airline"] = logRecord.airline;
    aircraft["type"] = logRecord.aircraftType;
    aircraft["registration"] = logRecord.registration;
    aircraft["engine"] = logRecord.engine;
    if (logRecord.ageYears[0]) aircraft["ageYears"] = atof(logRecord.ageYears);
    else aircraft["ageYears"] = nullptr;
    JsonObject seat = doc["seat"].to<JsonObject>();
    seat["cabin"] = logRecord.cabin;
    seat["seatNo"] = logRecord.seatNo;
    JsonObject operations = doc["operations"].to<JsonObject>();
    operations["departureRunway"] = logRecord.departureRunway;
    operations["arrivalRunway"] = logRecord.arrivalRunway;
    operations["gate"] = logRecord.gate;
    operations["standType"] = logRecord.standType;
    operations["pushbackLocal"] = logRecord.pushbackLocal;
    operations["takeoffLocal"] = logRecord.takeoffLocal;
    operations["landingLocal"] = logRecord.landingLocal;
    operations["onBlockLocal"] = logRecord.onBlockLocal;
    JsonObject metrics = doc["metrics"].to<JsonObject>();
    metrics["cruiseAltitudeM"] = atol(logRecord.cruiseAltitudeM);
    metrics["durationSec"] = atol(logRecord.durationMin) * 60;
    metrics["distanceKm"] = atof(logRecord.distanceKm);
    metrics["cruiseSpeedKmh"] = atof(logRecord.cruiseSpeedKmh);
    JsonObject experience = doc["experience"].to<JsonObject>();
    if (logRecord.mealRating[0]) experience["mealRating"] = atoi(logRecord.mealRating);
    else experience["mealRating"] = nullptr;
    if (logRecord.serviceRating[0]) experience["serviceRating"] = atoi(logRecord.serviceRating);
    else experience["serviceRating"] = nullptr;
    experience["notes"] = logRecord.notes;
}

bool saveLogRecord()
{
    String reason;
    if (!validateLogRecord(reason)) {
        setStatus(reason, 5000);
        return false;
    }
    if (!ensureLogDirectories()) {
        setStatus("SD / LOG DIRECTORY FAILED");
        return false;
    }
    const String finalPath = currentLogPath[0] ? String(currentLogPath) : createLogPath();
    const int slash = finalPath.lastIndexOf('/');
    const String baseName = finalPath.substring(slash + 1);
    const String temporary = String("/FlightLog/.tmp/") + baseName + ".tmp";
    const String backup = String("/FlightLog/.tmp/") + baseName + ".bak";
    const time_t now = time(nullptr) >= 1700000000 ? time(nullptr) : 0;
    const String id = baseName.substring(0, baseName.length() - 5);
    JsonDocument doc;
    buildLogJson(doc, id, now);

    if (!instance.lockSPI(pdMS_TO_TICKS(4000))) {
        setStatus("SD BUS BUSY");
        return false;
    }
    SD.remove(temporary);
    File file = SD.open(temporary, FILE_WRITE);
    bool ok = file && serializeJson(doc, file) > 0;
    if (file) {
        file.flush();
        file.close();
    }
    if (ok) {
        File verify = SD.open(temporary, FILE_READ);
        JsonDocument check;
        ok = verify && !deserializeJson(check, verify) && check["schema"].as<int>() == 1 &&
             check["id"].as<String>() == id;
        if (verify) verify.close();
    }
    const bool hadFinal = SD.exists(finalPath);
    if (ok) {
        SD.remove(backup);
        if (hadFinal) ok = SD.rename(finalPath, backup);
    }
    if (ok) ok = SD.rename(temporary, finalPath);
    if (!ok) {
        SD.remove(temporary);
        if (hadFinal && !SD.exists(finalPath) && SD.exists(backup)) SD.rename(backup, finalPath);
    } else {
        SD.remove(backup);
    }
    instance.unlockSPI();
    if (!ok) {
        setStatus("LOG SAVE FAILED", 5000);
        return false;
    }

    copyText(currentLogPath, sizeof(currentLogPath), finalPath);
    logCreatedUtc = logCreatedUtc ? logCreatedUtc : now;
    logIsNew = false;
    rebuildLogIndex();
    setStatus("LOG SAVED");
    Serial.printf("[LOG] saved %s\n", currentLogPath);
    return true;
}

struct LogFieldRef {
    const char *label;
    char *value;
    size_t capacity;
    bool uppercase;
};

uint8_t logFieldCount(uint8_t step)
{
    static constexpr uint8_t counts[] = {6, 5, 4, 6, 4, 3};
    return step < 6 ? counts[step] : 0;
}

LogFieldRef logFieldAt(uint8_t step, uint8_t field)
{
    if (step == 0) {
        switch (field) {
        case 0: return {"DATE", logRecord.date, sizeof(logRecord.date), false};
        case 1: return {"FLIGHT", logRecord.flightNo, sizeof(logRecord.flightNo), true};
        case 2: return {"FROM ICAO", logRecord.departureIcao, sizeof(logRecord.departureIcao), true};
        case 3: return {"TO ICAO", logRecord.arrivalIcao, sizeof(logRecord.arrivalIcao), true};
        case 4: return {"DEPART", logRecord.departureLocal, sizeof(logRecord.departureLocal), false};
        case 5: return {"ARRIVE", logRecord.arrivalLocal, sizeof(logRecord.arrivalLocal), false};
        }
    } else if (step == 1) {
        switch (field) {
        case 0: return {"AIRLINE", logRecord.airline, sizeof(logRecord.airline), false};
        case 1: return {"AIRCRAFT", logRecord.aircraftType, sizeof(logRecord.aircraftType), true};
        case 2: return {"REGISTRATION", logRecord.registration, sizeof(logRecord.registration), true};
        case 3: return {"ENGINE", logRecord.engine, sizeof(logRecord.engine), false};
        case 4: return {"AGE YEARS", logRecord.ageYears, sizeof(logRecord.ageYears), false};
        }
    } else if (step == 2) {
        switch (field) {
        case 0: return {"CABIN", logRecord.cabin, sizeof(logRecord.cabin), true};
        case 1: return {"SEAT", logRecord.seatNo, sizeof(logRecord.seatNo), true};
        case 2: return {"GATE", logRecord.gate, sizeof(logRecord.gate), true};
        case 3: return {"STAND", logRecord.standType, sizeof(logRecord.standType), true};
        }
    } else if (step == 3) {
        switch (field) {
        case 0: return {"DEP RWY", logRecord.departureRunway, sizeof(logRecord.departureRunway), true};
        case 1: return {"ARR RWY", logRecord.arrivalRunway, sizeof(logRecord.arrivalRunway), true};
        case 2: return {"PUSHBACK", logRecord.pushbackLocal, sizeof(logRecord.pushbackLocal), false};
        case 3: return {"TAKEOFF", logRecord.takeoffLocal, sizeof(logRecord.takeoffLocal), false};
        case 4: return {"LANDING", logRecord.landingLocal, sizeof(logRecord.landingLocal), false};
        case 5: return {"ON BLOCK", logRecord.onBlockLocal, sizeof(logRecord.onBlockLocal), false};
        }
    } else if (step == 4) {
        switch (field) {
        case 0: return {"ALTITUDE M", logRecord.cruiseAltitudeM, sizeof(logRecord.cruiseAltitudeM), false};
        case 1: return {"DURATION MIN", logRecord.durationMin, sizeof(logRecord.durationMin), false};
        case 2: return {"DISTANCE KM", logRecord.distanceKm, sizeof(logRecord.distanceKm), false};
        case 3: return {"SPEED KMH", logRecord.cruiseSpeedKmh, sizeof(logRecord.cruiseSpeedKmh), false};
        }
    } else if (step == 5) {
        switch (field) {
        case 0: return {"MEAL 1-5", logRecord.mealRating, sizeof(logRecord.mealRating), false};
        case 1: return {"SERVICE 1-5", logRecord.serviceRating, sizeof(logRecord.serviceRating), false};
        case 2: return {"NOTES", logRecord.notes, sizeof(logRecord.notes), false};
        }
    }
    return {nullptr, nullptr, 0, false};
}

void startNewLog()
{
    logRecord = LogRecord{};
    const time_t now = time(nullptr);
    if (now >= 1700000000) {
        struct tm local = {};
        localtime_r(&now, &local);
        strftime(logRecord.date, sizeof(logRecord.date), "%Y-%m-%d", &local);
    }
    if (nearestAirport) copyText(logRecord.departureIcao, sizeof(logRecord.departureIcao), nearestAirport->icao);
    currentLogPath[0] = '\0';
    logCreatedUtc = 0;
    logIsNew = true;
    logStep = 0;
    logField = 0;
    confirmDiscard = false;
    page = Page::LogEdit;
    dirty = true;
}

void drawLogList()
{
    canvas.fillScreen(C_BG);
    drawText("N NEW FLIGHT", 16, 32, C_PRIMARY);
    if (!sdReady) {
        drawCentered("SD CARD REQUIRED", SCREEN_W / 2, 100, C_WARNING, 2);
    } else if (!logCount) {
        drawCentered("NO FLIGHT LOGS", SCREEN_W / 2, 92, C_MUTED, 2);
        drawCentered("PRESS N TO CREATE", SCREEN_W / 2, 124, C_TEXT);
    } else {
        const uint16_t first = logSelection > 5 ? logSelection - 5 : 0;
        for (uint16_t row = 0; row < 6 && first + row < logCount; ++row) {
            const uint16_t index = first + row;
            const int16_t y = 51 + row * 23;
            drawPanel(12, y, 456, 20, index == logSelection);
            const LogIndexEntry &entry = logEntries[index];
            drawText(entry.date[0] ? entry.date : "----------", 22, y + 6, C_MUTED);
            drawText(entry.flightNo[0] ? entry.flightNo : "UNKNOWN", 119, y + 6,
                     index == logSelection ? C_PRIMARY : C_TEXT);
            drawText(String(entry.departureIcao) + " > " + entry.arrivalIcao, 240, y + 6, C_TEXT);
        }
    }
    drawChrome("N New  W/S/ROTATE Select  ENTER Detail  Q/H Home");
}

void drawLogDetail()
{
    canvas.fillScreen(C_BG);
    drawPanel(12, 34, 456, 145);
    drawText(String(logRecord.flightNo[0] ? logRecord.flightNo : "UNKNOWN") + "  " + logRecord.date,
             24, 46, C_PRIMARY, 2);
    drawText(String(logRecord.departureIcao) + " " + logRecord.departureLocal + "  >  " +
             logRecord.arrivalIcao + " " + logRecord.arrivalLocal, 24, 74, C_TEXT, 2);
    drawText(String("AIRCRAFT  ") + logRecord.aircraftType + "  " + logRecord.registration, 24, 105, C_MUTED);
    drawText(String("SEAT  ") + logRecord.cabin + " / " + logRecord.seatNo + "    GATE  " + logRecord.gate,
             24, 125, C_TEXT);
    drawText(String("DIST  ") + logRecord.distanceKm + " KM    DURATION  " + logRecord.durationMin + " MIN",
             24, 145, C_TEXT);
    drawText(clippedText(logRecord.notes, 62), 24, 164, C_MUTED);
    drawChrome("E Edit  Q Back to list  H Home");
}

void beginTextEdit(EditOwner owner, const char *label, char *destination, size_t capacity,
                   bool uppercase = false, bool secret = false)
{
    editOwner = owner;
    editLabel = label;
    editDestination = destination;
    editCapacity = capacity;
    editUppercase = uppercase;
    editSecret = secret;
    editBuffer = destination ? String(destination) : String();
    editText = true;
    dirty = true;
}

void closeTextEdit()
{
    editText = false;
    editOwner = EditOwner::None;
    editDestination = nullptr;
    editCapacity = 0;
    editLabel = "";
    dirty = true;
}

bool commitTextEdit()
{
    if (!editDestination || editCapacity == 0) return false;
    if (editUppercase) editBuffer.toUpperCase();
    copyText(editDestination, editCapacity, editBuffer);
    closeTextEdit();
    return true;
}

void drawTextEditor()
{
    canvas.fillRect(38, 55, 404, 112, C_BLACK);
    canvas.drawRoundRect(38, 55, 404, 112, 7, C_PRIMARY);
    drawText("EDIT " + editLabel, 54, 70, C_PRIMARY);
    String visible;
    if (editSecret) {
        visible.reserve(editBuffer.length() + 1);
        for (size_t index = 0; index < editBuffer.length(); ++index) visible += '*';
    } else {
        visible = editBuffer;
    }
    visible += '_';
    drawPanel(54, 94, 372, 35, true);
    drawText(clippedText(visible, 56), 66, 106, C_TEXT);
    drawText(String(capsMode ? "CAPS " : "") + (symbolMode ? "123/SYM" : "abc"), 54, 142, C_MUTED);
    drawText("ENTER Save  BACKSPACE Delete", 207, 142, C_MUTED);
}

void drawLogEdit()
{
    canvas.fillScreen(C_BG);
    static const char *stepNames[] = {
        "1 FLIGHT", "2 AIRCRAFT", "3 SEAT", "4 OPERATIONS", "5 METRICS", "6 EXPERIENCE"
    };
    drawText(stepNames[logStep], 16, 33, C_PRIMARY);
    drawText("A/D STEP", 390, 33, C_MUTED);
    const uint8_t count = logFieldCount(logStep);
    for (uint8_t index = 0; index < count; ++index) {
        const LogFieldRef field = logFieldAt(logStep, index);
        const int16_t y = 51 + index * 22;
        drawPanel(12, y, 456, 20, logField == index);
        drawText(field.label, 22, y + 6, C_MUTED);
        drawText(clippedText(field.value && field.value[0] ? field.value : "--", 45),
                 155, y + 6, logField == index ? C_PRIMARY : C_TEXT);
    }
    if (logStep == 5) {
        const int16_t y = 51 + count * 22;
        drawPanel(12, y, 456, 20, logField == count);
        drawCentered("SAVE FLIGHT LOG", SCREEN_W / 2, y + 6,
                     logField == count ? C_PRIMARY : C_TEXT);
    }
    if (confirmDiscard) {
        canvas.fillRoundRect(80, 65, 320, 88, 8, C_BLACK);
        canvas.drawRoundRect(80, 65, 320, 88, 8, C_WARNING);
        drawCentered("UNSAVED CHANGES", SCREEN_W / 2, 82, C_WARNING, 2);
        drawCentered("S Save   D Discard   C Continue", SCREEN_W / 2, 121, C_TEXT);
    }
    drawChrome("W/S Select  ENTER Edit  A/D Step  Q Exit");
}

enum SettingsRow : uint8_t {
    ROW_WIFI_SCAN,
    ROW_WIFI_SSID,
    ROW_WIFI_PASSWORD,
    ROW_TIMEZONE,
    ROW_DISPLAY,
    ROW_KEYBOARD,
    ROW_TIMEOUT,
    ROW_GPS,
    ROW_MANUAL_LAT,
    ROW_MANUAL_LON,
    ROW_RECONNECT,
    ROW_STANDBY,
    SETTINGS_ROW_COUNT
};

const char *settingsLabel(uint8_t row)
{
    static const char *labels[] = {
        "SCAN WI-FI", "WI-FI SSID", "WI-FI PASSWORD", "TIMEZONE",
        "DISPLAY", "KEYBOARD LIGHT", "SCREEN TIMEOUT", "GPS",
        "MANUAL LAT", "MANUAL LON", "RECONNECT / SYNC", "LOW POWER STANDBY"
    };
    return row < SETTINGS_ROW_COUNT ? labels[row] : "";
}

String settingsValue(uint8_t row)
{
    switch (row) {
    case ROW_WIFI_SCAN: return "ENTER";
    case ROW_WIFI_SSID: return settings.wifiSsid[0] ? String(settings.wifiSsid) : String("NOT SET");
    case ROW_WIFI_PASSWORD: return settings.wifiPassword[0] ? String("********") : String("NOT SET");
    case ROW_TIMEZONE: return settings.timezone;
    case ROW_DISPLAY: return String(DISPLAY_PERCENT[settings.brightness]) + "%";
    case ROW_KEYBOARD: {
        static const char *values[] = {"OFF", "MEDIUM", "HIGH"};
        return values[settings.keyboardBrightness];
    }
    case ROW_TIMEOUT: {
        static const char *values[] = {"30 SEC", "60 SEC", "5 MIN", "10 MIN", "ALWAYS ON"};
        return values[settings.screenTimeout];
    }
    case ROW_GPS: return settings.gpsEnabled ? "ON" : "OFF";
    case ROW_MANUAL_LAT: return String(settings.manualLat, 6);
    case ROW_MANUAL_LON: return String(settings.manualLon, 6);
    case ROW_RECONNECT: return WiFi.status() == WL_CONNECTED ? "CONNECTED" : "ENTER";
    case ROW_STANDBY: return "HOLD YELLOW OR ENTER";
    }
    return "";
}

void drawSettings()
{
    canvas.fillScreen(C_BG);
    const uint8_t first = settingsSelection > 5 ? settingsSelection - 5 : 0;
    for (uint8_t visible = 0; visible < 6 && first + visible < SETTINGS_ROW_COUNT; ++visible) {
        const uint8_t row = first + visible;
        const int16_t y = 35 + visible * 25;
        drawPanel(12, y, 456, 22, row == settingsSelection);
        drawText(settingsLabel(row), 22, y + 7, C_MUTED);
        drawText(clippedText(settingsValue(row), 35), 235, y + 7,
                 row == settingsSelection ? C_PRIMARY : C_TEXT);
    }
    drawText(String("SD ") + (sdReady ? "READY" : "MISSING") + "  MAP " +
             (mapState.manifestValid ? "READY" : "NOT READY") + "  LOGS " + String(logCount),
             16, 187, C_MUTED);
    drawChrome("W/S/ROTATE Select  ENTER Change  Q/H Home");
}

void drawWifiList()
{
    canvas.fillScreen(C_BG);
    drawText(wifiScanRunning ? "SCANNING..." : "SELECT NETWORK", 16, 34, C_PRIMARY);
    if (!wifiScanRunning && !scannedCount) {
        drawCentered("NO NETWORKS FOUND", SCREEN_W / 2, 102, C_MUTED, 2);
    }
    for (uint8_t row = 0; row < scannedCount && row < 6; ++row) {
        const int16_t y = 53 + row * 23;
        drawPanel(12, y, 456, 20, row == wifiSelection);
        drawText(clippedText(scannedSsids[row], 48), 22, y + 6,
                 row == wifiSelection ? C_PRIMARY : C_TEXT);
        drawText(String(scannedRssi[row]) + " dBm", 390, y + 6, C_MUTED);
    }
    drawChrome("W/S/ROTATE Select  ENTER Use SSID  Q Back");
}

void beginSettingsEdit(uint8_t row)
{
    editTarget = row;
    switch (row) {
    case ROW_WIFI_SSID:
        beginTextEdit(EditOwner::Settings, settingsLabel(row), settings.wifiSsid,
                      sizeof(settings.wifiSsid));
        break;
    case ROW_WIFI_PASSWORD:
        beginTextEdit(EditOwner::Settings, settingsLabel(row), settings.wifiPassword,
                      sizeof(settings.wifiPassword));
        break;
    case ROW_TIMEZONE:
        beginTextEdit(EditOwner::Settings, settingsLabel(row), settings.timezone,
                      sizeof(settings.timezone));
        break;
    case ROW_MANUAL_LAT:
        editOwner = EditOwner::Settings;
        editTarget = row;
        editLabel = settingsLabel(row);
        editBuffer = String(settings.manualLat, 6);
        editDestination = nullptr;
        editCapacity = 20;
        editUppercase = false;
        editSecret = false;
        editText = true;
        break;
    case ROW_MANUAL_LON:
        editOwner = EditOwner::Settings;
        editTarget = row;
        editLabel = settingsLabel(row);
        editBuffer = String(settings.manualLon, 6);
        editDestination = nullptr;
        editCapacity = 20;
        editUppercase = false;
        editSecret = false;
        editText = true;
        break;
    default:
        break;
    }
    dirty = true;
}

bool commitActiveEdit()
{
    if (editOwner == EditOwner::Log) return commitTextEdit();
    if (editOwner != EditOwner::Settings) return false;
    if (editTarget == ROW_MANUAL_LAT || editTarget == ROW_MANUAL_LON) {
        char *end = nullptr;
        const double value = strtod(editBuffer.c_str(), &end);
        const bool validNumber = end && *end == '\0';
        const bool inRange = editTarget == ROW_MANUAL_LAT ?
                             (value >= -90.0 && value <= 90.0) :
                             (value >= -180.0 && value <= 180.0);
        if (!validNumber || !inRange) {
            setStatus("INVALID COORDINATE");
            return false;
        }
        if (editTarget == ROW_MANUAL_LAT) settings.manualLat = value;
        else settings.manualLon = value;
        closeTextEdit();
    } else if (!commitTextEdit()) {
        return false;
    }
    setenv("TZ", settings.timezone, 1);
    tzset();
    saveSettings();
    updateNearestAirport(true);
    setStatus("SETTING SAVED");
    return true;
}

void startWifiScan()
{
    WiFi.mode(WIFI_STA);
    WiFi.scanDelete();
    scannedCount = 0;
    wifiSelection = 0;
    const int result = WiFi.scanNetworks(true, true);
    wifiScanRunning = result == WIFI_SCAN_RUNNING;
    if (!wifiScanRunning && result >= 0) {
        scannedCount = min<int>(result, 10);
        for (uint8_t index = 0; index < scannedCount; ++index) {
            scannedSsids[index] = WiFi.SSID(index);
            scannedRssi[index] = WiFi.RSSI(index);
        }
    }
    page = Page::WifiList;
    dirty = true;
}

void updateWifiScan()
{
    if (!wifiScanRunning) return;
    const int result = WiFi.scanComplete();
    if (result == WIFI_SCAN_RUNNING) return;
    wifiScanRunning = false;
    scannedCount = result > 0 ? min<int>(result, 10) : 0;
    for (uint8_t index = 0; index < scannedCount; ++index) {
        scannedSsids[index] = WiFi.SSID(index);
        scannedRssi[index] = WiFi.RSSI(index);
    }
    dirty = true;
    Serial.printf("[NET] scan complete count=%u\n", scannedCount);
}

void goHome()
{
    if (page == Page::Flight) clearTileCache();
    if (wifiScanRunning) {
        WiFi.scanDelete();
        wifiScanRunning = false;
    }
    editText = false;
    confirmDiscard = false;
    page = Page::Home;
    dirty = true;
}

void enterLogList()
{
    if (!ensureLogDirectories()) {
        setStatus("SD CARD REQUIRED");
    } else {
        cleanupLogTemps();
        loadLogIndex();
    }
    page = Page::LogList;
    logSelection = logCount ? min<uint16_t>(logSelection, logCount - 1) : 0;
    dirty = true;
}

void openSelectedLog()
{
    if (!logCount || logSelection >= logCount) return;
    if (loadLogRecord(logEntries[logSelection].path)) {
        page = Page::LogDetail;
        dirty = true;
    } else {
        setStatus("LOG FILE INVALID");
    }
}

void activateSelection()
{
    if (page == Page::Home) {
        switch (homeSelection) {
        case 0: page = Page::Airport; updateNearestAirport(true); break;
        case 1:
            page = Page::Flight;
            if (!mapState.manifestChecked) validateMapManifest();
            break;
        case 2: enterLogList(); return;
        case 3: page = Page::Settings; loadLogIndex(); break;
        }
    } else if (page == Page::LogList) {
        openSelectedLog();
        return;
    } else if (page == Page::LogEdit) {
        const uint8_t count = logFieldCount(logStep);
        if (logStep == 5 && logField == count) {
            if (saveLogRecord()) page = Page::LogDetail;
        } else if (logField < count) {
            const LogFieldRef field = logFieldAt(logStep, logField);
            beginTextEdit(EditOwner::Log, field.label, field.value, field.capacity, field.uppercase);
        }
    } else if (page == Page::Settings) {
        switch (settingsSelection) {
        case ROW_WIFI_SCAN: startWifiScan(); return;
        case ROW_WIFI_SSID:
        case ROW_WIFI_PASSWORD:
        case ROW_TIMEZONE:
        case ROW_MANUAL_LAT:
        case ROW_MANUAL_LON:
            beginSettingsEdit(settingsSelection);
            return;
        case ROW_DISPLAY: cycleDisplayBrightness(); break;
        case ROW_KEYBOARD: cycleKeyboardBrightness(); break;
        case ROW_TIMEOUT:
            settings.screenTimeout = (settings.screenTimeout + 1) % 5;
            saveSettings();
            break;
        case ROW_GPS: setGpsEnabled(!settings.gpsEnabled); break;
        case ROW_RECONNECT:
            WiFi.disconnect(false, false);
            connectWifi();
            setStatus("WI-FI RECONNECTING");
            break;
        case ROW_STANDBY: enterDeepSleep(); break;
        }
    } else if (page == Page::WifiList && scannedCount) {
        copyText(settings.wifiSsid, sizeof(settings.wifiSsid), scannedSsids[wifiSelection]);
        saveSettings();
        page = Page::Settings;
        settingsSelection = ROW_WIFI_PASSWORD;
        setStatus("SSID SELECTED - SET PASSWORD");
    }
    dirty = true;
}

void moveSelection(int8_t delta)
{
    if (page == Page::Home) {
        homeSelection = static_cast<uint8_t>((homeSelection + 4 + delta) % 4);
    } else if (page == Page::LogList && logCount) {
        logSelection = static_cast<uint16_t>((logSelection + logCount + delta) % logCount);
    } else if (page == Page::LogEdit) {
        const uint8_t count = logFieldCount(logStep) + (logStep == 5 ? 1 : 0);
        logField = static_cast<uint8_t>((logField + count + delta) % count);
    } else if (page == Page::Settings) {
        settingsSelection = static_cast<uint8_t>((settingsSelection + SETTINGS_ROW_COUNT + delta) %
                                                 SETTINGS_ROW_COUNT);
    } else if (page == Page::WifiList && scannedCount) {
        wifiSelection = static_cast<uint8_t>((wifiSelection + scannedCount + delta) % scannedCount);
    }
    dirty = true;
}

void changeLogStep(int8_t delta)
{
    logStep = static_cast<uint8_t>((logStep + 6 + delta) % 6);
    const uint8_t count = logFieldCount(logStep) + (logStep == 5 ? 1 : 0);
    if (logField >= count) logField = count - 1;
    dirty = true;
}

void changeMapZoom(int8_t delta)
{
    const int zoom = constrain(static_cast<int>(mapState.zoom) + delta,
                               static_cast<int>(MAP_MIN_ZOOM), static_cast<int>(MAP_MAX_ZOOM));
    if (zoom == mapState.zoom) return;
    mapState.zoom = static_cast<uint8_t>(zoom);
    mapState.panX = 0;
    mapState.panY = 0;
    clearTileCache();
    dirty = true;
}

void handleCharacter(char key)
{
    if (editText) {
        if (key == '\n') {
            if (!commitActiveEdit()) setStatus("INVALID VALUE");
        } else if (key == '\b') {
            if (editBuffer.length()) editBuffer.remove(editBuffer.length() - 1);
        } else if (key >= 32 && key <= 126 && editBuffer.length() + 1 < editCapacity) {
            editBuffer += key;
        }
        dirty = true;
        return;
    }

    key = static_cast<char>(tolower(static_cast<unsigned char>(key)));
    if (confirmDiscard) {
        if (key == 's') {
            if (saveLogRecord()) {
                confirmDiscard = false;
                page = Page::LogDetail;
            }
        } else if (key == 'd') {
            confirmDiscard = false;
            if (logIsNew) enterLogList();
            else page = Page::LogDetail;
        } else if (key == 'c' || key == 'q') {
            confirmDiscard = false;
        }
        dirty = true;
        return;
    }
    if (key == 'b') {
        cycleKeyboardBrightness();
        return;
    }
    if (key == 'h' || key == '\b') {
        goHome();
        return;
    }

    switch (page) {
    case Page::Home:
        if (key == 'w' || key == 'a') moveSelection(-1);
        else if (key == 's' || key == 'd') moveSelection(1);
        else if (key == '\n') activateSelection();
        break;
    case Page::Airport:
        if (key == 'q') goHome();
        else if (key == 'r') requestWeather(true);
        else if (key == 'l') cycleDisplayBrightness();
        break;
    case Page::Flight:
        if (key == 'w') mapState.panY -= 96;
        else if (key == 's') mapState.panY += 96;
        else if (key == 'a') mapState.panX -= 96;
        else if (key == 'd') mapState.panX += 96;
        else if (key == 'q') changeMapZoom(-1);
        else if (key == 'e') changeMapZoom(1);
        else if (key == 'c') {
            mapState.panX = 0;
            mapState.panY = 0;
            mapState.follow = true;
        } else if (key == 'l') cycleDisplayBrightness();
        dirty = true;
        break;
    case Page::LogList:
        if (key == 'q') goHome();
        else if (key == 'w') moveSelection(-1);
        else if (key == 's') moveSelection(1);
        else if (key == 'n') startNewLog();
        else if (key == '\n') activateSelection();
        break;
    case Page::LogDetail:
        if (key == 'q') enterLogList();
        else if (key == 'e') {
            logStep = 0;
            logField = 0;
            page = Page::LogEdit;
            dirty = true;
        }
        break;
    case Page::LogEdit:
        if (key == 'q') {
            confirmDiscard = true;
            dirty = true;
        } else if (key == 'w') moveSelection(-1);
        else if (key == 's') moveSelection(1);
        else if (key == 'a') changeLogStep(-1);
        else if (key == 'd') changeLogStep(1);
        else if (key == '\n') activateSelection();
        break;
    case Page::Settings:
        if (key == 'q') goHome();
        else if (key == 'w') moveSelection(-1);
        else if (key == 's') moveSelection(1);
        else if (key == '\n') activateSelection();
        break;
    case Page::WifiList:
        if (key == 'q') {
            page = Page::Settings;
            dirty = true;
        } else if (key == 'w') moveSelection(-1);
        else if (key == 's') moveSelection(1);
        else if (key == '\n') activateSelection();
        break;
    }
}

void handleRawKey(bool pressed, uint8_t raw)
{
    wakeScreen();
    if (raw == YELLOW_KEY_RAW) {
        if (pressed) {
            yellowKeyHeld = true;
            shutdownTriggered = false;
            yellowKeyPressedAt = millis();
        } else if (yellowKeyHeld) {
            const bool longPress = millis() - yellowKeyPressedAt >= POWER_HOLD_MS;
            yellowKeyHeld = false;
            if (longPress) enterDeepSleep();
            symbolMode = !symbolMode;
            setStatus(symbolMode ? "SYMBOL MODE" : "LETTER MODE", 1200);
        }
        return;
    }
    if (!pressed) return;
    if (raw == 29) {
        capsMode = !capsMode;
        setStatus(capsMode ? "CAPS ON" : "CAPS OFF", 1200);
        return;
    }
    const char key = rawKeyToCharacter(raw);
    if (key) handleCharacter(key);
}

void pollKeyboard()
{
    char ignored;
    instance.kb.getKey(&ignored);
    while (true) {
        RawKeyEvent event;
        portENTER_CRITICAL(&keyMux);
        if (keyReadIndex != keyWriteIndex) {
            event = keyQueue[keyReadIndex];
            event.ready = true;
            keyReadIndex = static_cast<uint8_t>((keyReadIndex + 1) % KEY_QUEUE_SIZE);
        }
        portEXIT_CRITICAL(&keyMux);
        if (!event.ready) break;
        handleRawKey(event.pressed, event.raw);
    }
    if (yellowKeyHeld && !shutdownTriggered && millis() - yellowKeyPressedAt >= POWER_HOLD_MS) {
        shutdownTriggered = true;
        enterDeepSleep();
    }
}

void pollRotary()
{
    const RotaryMsg_t message = instance.getRotary();
    const bool moved = message.dir != ROTARY_DIR_NONE;
    const bool clicked = message.centerBtnPressed && !rotaryPressed;
    rotaryPressed = message.centerBtnPressed;
    if (!moved && !clicked) return;
    const bool wasOff = screenOff;
    wakeScreen();
    if (wasOff) return;
    if (moved) {
        const int8_t delta = message.dir == ROTARY_DIR_UP ? 1 : -1;
        if (page == Page::Flight) changeMapZoom(delta);
        else moveSelection(delta);
    }
    if (clicked && !editText) activateSelection();
}

void renderPage()
{
    if (screenOff) return;
    switch (page) {
    case Page::Home: drawHome(); break;
    case Page::Airport: drawAirport(); break;
    case Page::Flight: drawFlight(); break;
    case Page::LogList: drawLogList(); break;
    case Page::LogDetail: drawLogDetail(); break;
    case Page::LogEdit: drawLogEdit(); break;
    case Page::Settings: drawSettings(); break;
    case Page::WifiList: drawWifiList(); break;
    }
    if (editText) drawTextEditor();
    flushDisplay();
    dirty = false;
}

void printDiagnostics()
{
    Serial.printf("[APP] version=%s page=%s uptime=%lus wifi=%s battery=%s\n",
                  APP_VERSION, pageName(), static_cast<unsigned long>(millis() / 1000),
                  WiFi.status() == WL_CONNECTED ? "CONNECTED" : "OFFLINE",
                  batteryPercent <= 100 ? String(batteryPercent).c_str() : "--");
    Serial.printf("[GPS] enabled=%s ready=%s fix=%s chars=%lu lat=%.6f lon=%.6f sat=%lu\n",
                  settings.gpsEnabled ? "YES" : "NO", gps.hardwareReady ? "YES" : "NO",
                  gps.valid ? "YES" : "NO", static_cast<unsigned long>(gps.charsProcessed),
                  gps.latitude, gps.longitude, static_cast<unsigned long>(gps.satellites));
    Serial.printf("[SD] ready=%s size=%lluMB logs=%u map=%s\n", sdReady ? "YES" : "NO",
                  sdReady ? SD.cardSize() / (1024ULL * 1024ULL) : 0, logCount,
                  mapState.manifestValid ? "READY" : mapState.manifestName);
    Serial.printf("[APP] heap=%u min=%u psram=%u min_psram=%u tasks=%u\n",
                  ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getFreePsram(),
                  ESP.getMinFreePsram(), uxTaskGetNumberOfTasks());
}

void processSerialCommand(String command)
{
    command.trim();
    command.toLowerCase();
    if (!command.length()) return;
    if (command == "status" || command == "gps" || command == "sd" ||
        command == "heap" || command == "tasks") {
        printDiagnostics();
    } else if (command == "weather refresh") {
        requestWeather(true);
    } else if (command == "logs rebuild") {
        rebuildLogIndex();
    } else if (command == "map") {
        Serial.printf("[MAP] zoom=%u pan=%ld,%ld manifest=%s name=%s\n", mapState.zoom,
                      static_cast<long>(mapState.panX), static_cast<long>(mapState.panY),
                      mapState.manifestValid ? "OK" : "FAIL", mapState.manifestName);
    } else if (command == "help") {
        Serial.println("[APP] commands: status gps sd map heap tasks weather refresh logs rebuild help");
    } else {
        Serial.printf("[APP] unknown command: %s\n", command.c_str());
    }
}

void pollSerial()
{
    while (Serial.available()) {
        const char value = static_cast<char>(Serial.read());
        if (value == '\r') continue;
        if (value == '\n') {
            processSerialCommand(serialLine);
            serialLine = "";
        } else if (value >= 32 && value <= 126 && serialLine.length() < 96) {
            serialLine += value;
        }
    }
}

void updateScreenPower()
{
    const uint32_t timeout = SCREEN_TIMEOUTS[settings.screenTimeout];
    if (!screenOff && timeout && millis() - lastInputAt >= timeout) {
        screenOff = true;
        instance.setBrightness(0);
        instance.sleepDisplay();
        Serial.println("[POWER] display timeout");
    }
}

void timeSyncCallback(struct timeval *)
{
    rtcSyncPending = true;
}

void updateRtcSync()
{
    if (!rtcSyncPending) return;
    rtcSyncPending = false;
    if (!(instance.getDeviceProbe() & HW_RTC_ONLINE) || time(nullptr) < 1700000000) return;
    const int result = instance.rtc.hwClockWrite();
    Serial.printf("[TIME] RTC write=%s\n", result == 0 ? "OK" : "FAIL");
}

} // namespace

void flightMateSetup()
{
    Serial.begin(115200);
    Serial.printf("\n[APP] %s %s starting\n", APP_NAME, APP_VERSION);
    const uint32_t skip = NO_SCAN_I2C_DEV | NO_HW_SENSOR | NO_HW_NFC | NO_HW_DRV |
                          NO_HW_GPS | NO_HW_LORA | NO_HW_SD | NO_HW_MIC |
                          NO_HW_CODEC | NO_INIT_FATFS;
    instance.begin(skip);
    instance.setRotation(0);
    if (!lv_is_initialized()) lv_init();
    sntp_set_time_sync_notification_cb(timeSyncCallback);
    loadSettings();
    applyBrightness();
    instance.kb.setRawCallback(keyboardRawCallback);
    instance.kb.setRepeat(false);
    weatherMutex = xSemaphoreCreateMutex();
    loadWeatherCache();
    updateBattery();
    initSd();
    if (sdReady) validateMapManifest();
    setGpsEnabled(settings.gpsEnabled, false);
    if (settings.wifiSsid[0]) connectWifi();

    Serial.printf("[APP] display=%ux%u psram=%lu bytes\n", instance.width(), instance.height(),
                  static_cast<unsigned long>(ESP.getPsramSize()));
    if (instance.width() != SCREEN_W || instance.height() != SCREEN_H || !canvas.begin()) {
        Serial.println("[APP] fatal display or PSRAM initialization failure");
        while (true) delay(1000);
    }
    updateNearestAirport(true);
    lastInputAt = millis();
    drawHome();
    flushDisplay();
    dirty = false;
}

void flightMateLoop()
{
    pollKeyboard();
    pollRotary();
    pollSerial();
    updateWifiScan();
    updateGps();
    updateBattery();
    updateNearestAirport();
    updateNetwork();
    updateRtcSync();
    static uint32_t lastSdRefresh = 0;
    if (millis() - lastSdRefresh >= 2000) {
        lastSdRefresh = millis();
        refreshSdState();
    }
    updateScreenPower();
    if (static_cast<int32_t>(millis() - lastUiRefresh) >= static_cast<int32_t>(UI_REFRESH_MS)) {
        lastUiRefresh = millis();
        dirty = true;
    }
    if (page == Page::Flight && millis() - lastMapRefresh >= MAP_REFRESH_MS) {
        lastMapRefresh = millis();
        dirty = true;
    }
    if (transientStatus.length() && static_cast<int32_t>(millis() - transientStatusUntil) >= 0) {
        transientStatus = "";
        dirty = true;
    }
    if (dirty) renderPage();
    delay(5);
}
