#pragma once

#include <FS.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

template<fs::FS &_FS>
class YandexDisk {
public:
  YandexDisk(const String &token);
  ~YandexDisk();

  bool upload(const String &path, Stream &stream, bool overwrite = false);
  bool upload(const String &path, const String &fileName, bool overwrite = false);
  bool download(const String &path, Stream &stream);
  bool download(const String &path, const String &fileName);

protected:
  void close();
  void prepare(bool auth);
  String encodeUrl(const String &url);
  String getUploadUrl(const String &path, bool overwrite);
  String getDownloadUrl(const String &path);

  String _token;
  WiFiClientSecure _client;
  HTTPClient *_http;
};

static const char *strchr_P(PGM_P str, char ch) {
  while (pgm_read_byte(str)) {
    if (pgm_read_byte(str) == ch)
      return str;
    ++str;
  }
  return nullptr;
}

/*
static bool copyStream(Stream &src, Stream &dest, uint32_t size) {
  uint8_t buf[1024];
  uint16_t readed, written;

  while (size) {
    readed = src.readBytes(buf, min((uint32_t)sizeof(buf), size));
    written = dest.write(buf, readed);
    if (written != readed)
      break;
    size -= readed;
  }
  return size == 0;
}
*/

template<fs::FS &_FS>
YandexDisk<_FS>::YandexDisk(const String &token) : _token(token), _http(nullptr) {
  _client.setInsecure();
}

template<fs::FS &_FS>
YandexDisk<_FS>::~YandexDisk() {
  if (_http)
    delete _http;
}

template<fs::FS &_FS>
bool YandexDisk<_FS>::upload(const String &path, Stream &stream, bool overwrite) {
  String url;
  bool result = false;

  url = getUploadUrl(path, overwrite);
  if (! url.isEmpty()) {
    _http = new HTTPClient();
    if (_http) {
      if (_http->begin(_client, url)) {
        char method[4];
        int code;

        prepare(false);
        strcpy_P(method, PSTR("PUT"));
        code = _http->sendRequest(method, &stream, stream.available());
        result = ((code == 201) || (code == 202));
        _http->end();
      }
      close();
    }
  }
  return result;
}

template<fs::FS &_FS>
bool YandexDisk<_FS>::upload(const String &path, const String &fileName, bool overwrite) {
  bool result = false;

  if (_FS.exists(fileName)) {
    char mode[2];

    mode[0] = 'r';
    mode[1] = '\0';
    fs::File f = _FS.open(fileName, mode);
    if (f) {
      result = upload(path, f, overwrite);
      f.close();
    }
  }
  return result;
}

template<fs::FS &_FS>
bool YandexDisk<_FS>::download(const String &path, Stream &stream) {
  String url;
  bool result = false;

  url = getDownloadUrl(path);
  if (! url.isEmpty()) {
    _http = new HTTPClient();
    if (_http) {
      if (_http->begin(_client, url)) {
        int code;

        prepare(false);
        code = _http->GET();
        if (code == 200) {
          result = _http->writeToStream(&stream) == _http->getSize();
/*
          result = copyStream(_http->getStream(), stream, _http->getSize());
*/
        }
        _http->end();
      }
      close();
    }
  }
  return result;
}

template<fs::FS &_FS>
bool YandexDisk<_FS>::download(const String &path, const String &fileName) {
  char mode[2];
  bool result = false;

  mode[0] = 'w';
  mode[1] = '\0';
  fs::File f = _FS.open(fileName, mode);
  if (f) {
    result = download(path, f);
    f.close();
  }
  return result;
}

template<fs::FS &_FS>
void YandexDisk<_FS>::close() {
  if (_http) {
//    _http->end();
    delete _http;
    _http = nullptr;
  }
}

template<fs::FS &_FS>
void YandexDisk<_FS>::prepare(bool auth) {
  _http->setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  _http->setTimeout(15000);
  if (auth)
    _http->addHeader(F("Authorization"), String(F("OAuth ")) + _token);
}

template<fs::FS &_FS>
String YandexDisk<_FS>::encodeUrl(const String &url) {
  String result;

//  result.reserve(url.length());
  for (uint16_t i = 0; i < url.length(); ++i) {
    char c = url[i];

    if (strchr_P(PSTR(";,/?:@&=+$"), c)) {
      char str[4];

      str[0] = '%';
      str[1] = (c >> 4) > 9 ? 'A' + (c >> 4) - 10 : '0' + (c >> 4);
      str[2] = (c & 0x0F) > 9 ? 'A' + (c & 0x0F) - 10 : '0' + (c & 0x0F);
      str[3] = '\0';
      result.concat(str);
    } else
      result.concat(c);
  }
  return result;
}

template<fs::FS &_FS>
String YandexDisk<_FS>::getUploadUrl(const String &path, bool overwrite) {
  static const char URL[] PROGMEM = "https://cloud-api.yandex.net/v1/disk/resources/upload?path=app%3A";
  static const char HREF[] PROGMEM = "href";

  String result;

  close();
  result = FPSTR(URL);
  if (path[0] != '/')
    result.concat(F("%2F"));
  result.concat(encodeUrl(path));
  if (overwrite)
    result.concat(F("&overwrite=true"));
  result.concat(F("&fields=href"));
  _http = new HTTPClient();
  if (_http) {
    if (_http->begin(_client, result)) {
      result.clear();
      prepare(true);
      if (_http->GET() == 200) {
        DynamicJsonDocument json(1024);

        if (deserializeJson(json, _http->getStream()) == DeserializationError::Ok) {
          result = (const char*)json[FPSTR(HREF)];
        }
      }
      _http->end();
    }
    close();
  }
  return result;
}

template<fs::FS &_FS>
String YandexDisk<_FS>::getDownloadUrl(const String &path) {
  static const char URL[] PROGMEM = "https://cloud-api.yandex.net/v1/disk/resources/download?path=app%3A";
  static const char HREF[] PROGMEM = "href";

  String result;

  close();
  result = FPSTR(URL);
  if (path[0] != '/')
    result.concat(F("%2F"));
  result.concat(encodeUrl(path));
  result.concat(F("&fields=href"));
  _http = new HTTPClient();
  if (_http) {
    if (_http->begin(_client, result)) {
      int code;

      result.clear();
      prepare(true);
      code = _http->GET();
      if (code == 200) {
        DynamicJsonDocument json(1024);

        if (deserializeJson(json, _http->getStream()) == DeserializationError::Ok) {
          result = (const char*)json[FPSTR(HREF)];
        }
      }
      _http->end();
    }
    close();
  }
  return result;
}
