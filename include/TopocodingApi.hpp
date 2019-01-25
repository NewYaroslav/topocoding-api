#ifndef TOPOCODINGAPI_HPP_INCLUDED
#define TOPOCODINGAPI_HPP_INCLUDED
//------------------------------------------------------------------------------
#include <curl/curl.h>
#include <vector>
#include <string>
#include <cmath>
//------------------------------------------------------------------------------
/** \brief Класс для хранения координат
 */
class TopocodingApiCoordinates {
public:
    double lat = 0; /**< Широта */
    double lon = 0; /**< Долгота */

    TopocodingApiCoordinates() {};

    /** \brief Инициализировать класс
     * \param _lat широта
     * \param _lon долгота
     */
    TopocodingApiCoordinates(double _lat, double _lon) {
        lat = _lat;
        lon = _lon;
    }
};
//------------------------------------------------------------------------------
/** \brief Класс для взаимодействия с API topocoding.com
 */
class TopocodingApi {
public:
    enum ErrorType {
        OK = 0,
        NO_INIT = -1,
        INIT_ERROR = -2,
        DECOMPRESSION_ERROR = -3,
        PARSER_ERROR = -4,
        SUBSTRING_NOT_FOUND = -5,
    };
private:
    std::string topoKey = "YAKPLYRKEDSKPTW";
//------------------------------------------------------------------------------
    double topoToFeets(double meters) {
      return meters * 3.28084;
    }
//------------------------------------------------------------------------------
    double topoComputeDistance(double lat1, double lon1, double lat2, double lon2) {
        const float PI = 3.1415926535897932384626433832795;
        lat1 = lat1 / 180 * PI;
        lon1 = lon1 / 180 * PI;
        lat2 = lat2 / 180 * PI;
        lon2 = lon2 / 180 * PI;
        double dlon = lon2 - lon1;
        double dlat = lat2 - lat1;
        double a = sin(dlat/2) * sin(dlat/2) + cos(lat1) * cos(lat2) * sin(dlon/2) * sin(dlon/2);
        double c = 2 * atan2(sqrt(a), sqrt(1-a));
        double R = 6378.137;
        return R * c;
    }
//------------------------------------------------------------------------------
    std::vector<double> topoComputeIntermediate(double lat1, double lon1, double lat2, double lon2, double fraction, double dist = -1.0) {
        if(dist <= 0.0 )
            dist = topoComputeDistance(lat1, lon1, lat2, lon2);

        const double R = 6378.137;
        const float PI = 3.1415926535897932384626433832795;
        lat1 = lat1 / 180 * PI;
        lon1 = lon1 / 180 * PI;
        lat2 = lat2 / 180 * PI;
        lon2 = lon2 / 180 * PI;
        dist = dist / R;

        double A = sin((1-fraction)*dist)/sin(dist);
        double B = sin(fraction*dist)/sin(dist);
        double x = A*cos(lat1)*cos(lon1) +  B*cos(lat2)*cos(lon2);
        double y = A*cos(lat1)*sin(lon1) +  B*cos(lat2)*sin(lon2);
        double z = A*sin(lat1)           +  B*sin(lat2);
        double lat = atan2(z, sqrt(x*x+y*y));
        double lon = atan2(y,x);

        lat = lat * 180 / PI;
        lon = lon * 180 / PI;
        std::vector<double> _array(3);
        _array[0] = lat;
        _array[1] = lon;
        _array[2] = dist * fraction * R;
        return _array;
    }
//------------------------------------------------------------------------------
    // Функция доступа к серверу
    template <class T>
    std::string topoEncodeCoordinates(std::vector<T> coordinates) {
        std::string result;

        std::string topoUrlChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.!*()";
        double topoUrlCharsLength = topoUrlChars.size();
        double topoUrlCharsSqrt = std::floor(std::sqrt(topoUrlCharsLength));
        size_t topoUrlMaxCoordinates = 280;

        if(coordinates.size() > topoUrlMaxCoordinates) {
            std::cout << "Too many points to fit into the safe URL length limit." << std::endl;
            return "";
        }

        for (size_t j = 0; j < coordinates.size(); j++) {
            double lat = coordinates[j].lat;
            double lon = coordinates[j].lon;
            if (lat < 0.0)
                lat = 180 + lat;
            lat = lat / 180.0;
            lat = lat - floor(lat);
            if (lon < 0.0)
                lon = 360 + lon;
            lon = lon / 360;
            lon = lon - floor(lon);
            for (int i = 0; i < 3; i++) {
                lat = lat * topoUrlCharsLength;
                int index = floor(lat);
                lat = lat - index;
                result += topoUrlChars.substr(index, 1 );
                lon = lon * topoUrlCharsLength;
                index = floor(lon);
                lon = lon - index;
                result += topoUrlChars.substr(index, 1 );
            }
            lat = lat * topoUrlCharsSqrt;
            lon = lon * topoUrlCharsSqrt;
            int index = std::floor(lat) * topoUrlCharsSqrt + std::floor(lon);
            result += topoUrlChars.substr(index, 1);
        }
        return result;
    }
//------------------------------------------------------------------------------
    static int writer(char *data, size_t size, size_t nmemb, std::string *buffer) {
        int result = 0;
        if (buffer != NULL) {
            buffer->append(data, size * nmemb);
            result = size * nmemb;
        }
        return result;
    }
//------------------------------------------------------------------------------
    template <class T>
    int do_get_request(std::vector<T> coordinates) {
        CURL *curl;
        curl = curl_easy_init();
        if(!curl) {
            return INIT_ERROR;
        }
        std::cout << topoEncodeCoordinates(coordinates) << std::endl;
        std::string url = "http://topocoding.com/api/altitude_v1.php?id=x&key=" + topoKey + "&l=" + topoEncodeCoordinates(coordinates);
        char error_buffer[CURL_ERROR_SIZE];
        const int TIME_OUT = 60;
        std::string buffer;

        //curl_easy_setopt(curl, CURLOPT_GET, 1); // делаем пост запрос
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        //curl_easy_setopt(curl, CURLOPT_CAINFO, sert_file.c_str());
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
        curl_easy_setopt(curl, CURLOPT_HEADER, 1); // отключаем заголовок в ответе
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writer);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);

        struct curl_slist *http_headers = NULL;
        http_headers = curl_slist_append(http_headers, "User-Agent: Internet Explorer/6.0 (X11; U; Windows XP SP2; en-US; rv:1.7.2) Gecko/20040804");
        http_headers = curl_slist_append(http_headers, "Host: www.vhfdx.ru");
        http_headers = curl_slist_append(http_headers, "X-Forwarded-For: 192.168.1.1");
        //http_headers = curl_slist_append(http_headers, "Accept: application/json, text/javascript, */*; q=0.01");
        //http_headers = curl_slist_append(http_headers, "Accept-Language: ru-RU,ru;q=0.8,en-US;q=0.5,en;q=0.3");
        //http_headers = curl_slist_append(http_headers, "Accept-Encoding: gzip, deflate, br");
        //http_headers = curl_slist_append(http_headers, "Content-Type: application/x-www-form-urlencoded; charset=UTF-8");
        //http_headers = curl_slist_append(http_headers, "X-Requested-With: XMLHttpRequest");
        //http_headers = curl_slist_append(http_headers, "Connection: keep-alive");
        //http_headers = curl_slist_append(http_headers, "Cache-Control: no-cache");
        //http_headers = curl_slist_append(http_headers, "Pragma: no-cache");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, http_headers);
        ///curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.c_str());

        curl_easy_setopt(curl, CURLOPT_TIMEOUT, TIME_OUT);

        CURLcode result;
        result = curl_easy_perform(curl);
        curl_slist_free_all(http_headers);
        http_headers = NULL;

        curl_easy_cleanup(curl);
        if (result == CURLE_OK) {
#               if (1)
            std::cout << buffer << std::endl;
            std::cout << "size: " << buffer.size();
#               endif
            return OK;
        } else {
            std::cout << "Error: [" << result << "] - " << error_buffer;
            return result;
        }
    }
//------------------------------------------------------------------------------
public:
//------------------------------------------------------------------------------
    TopocodingApi() {

    };
//------------------------------------------------------------------------------
    int getAltitudes(double lat, double lon) {
        //TopocodingApiCoordinates point(lat, lon)
        std::vector<TopocodingApiCoordinates> coordinates;
        coordinates.push_back(TopocodingApiCoordinates(lat, lon));
        return do_get_request<TopocodingApiCoordinates>(coordinates);
    }
//------------------------------------------------------------------------------
};
//------------------------------------------------------------------------------
#endif // TOPOCODINGAPI_HPP_INCLUDED
