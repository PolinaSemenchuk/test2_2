
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <windows.h>
#include <iostream>
#include <time.h>
#include <nlohmann/json.hpp>
#include <vector>
#include <fstream>
#include <stdio.h>
#include <sstream> 
#include <conio.h>
#include <httplib.h>
#include <thread>
#include <mutex>
//#include "Server.h"

using namespace std;
using json = nlohmann::json;

string global_port = "";
string global_speed = "";
string global_data = "";
json j_global{};
httplib::Server svr;


//функции для работы с данными от датчика
struct Wind {
    string sensor_name;
    int hour, minute, second;
    string wind_speed;
    string wind_direction;
};
vector<Wind> deserialization(const string& filename) {

    ifstream ifile(filename);
    if (!ifile.is_open()) {
        cerr << "error" << endl;
    }

    json j;
    try {
        ifile >> j;
    }
    catch (exception) {
        cerr << "error2" << endl;
    }

    vector<Wind> data;
    Wind w;
    for (auto& i : j) {

        w.sensor_name = i.value("sensor name", "");
        w.wind_speed = i.value("wind speed", "");
        w.wind_direction = i.value("direction of the wind", "");
        w.hour = i["time"].value("hour:", 0);
        w.minute = i["time"].value("minute:", 0);
        w.second = i["time"].value("second", 0);
        data.push_back(w);
    }
    return data;
}    
static json toJSON(string ws, string wd, struct tm now) {

    json j{};
    j["time"] = { {"hour:",now.tm_hour}, {"minute:",now.tm_min}, {"second",now.tm_sec} };
    j["sensor name"] = "WMT700";
    j["wind speed"] = ws;
    j["direction of the wind"] = wd;
    //cout << j << endl;
    return j;
}
static bool saveToFile(json& j, const string& filename) {
    json array;
    ifstream ifile(filename);
    setlocale(LC_ALL, "ru");

    if (ifile.is_open()) {
        try {
            ifile >> array;
        }
        catch (const json::parse_error&) {
            cerr << "Файл пуст, создастся пустой массив" << endl;
        }
        ifile.close();
    }
    else {
        cerr << "Ошибка открытия файла для чтения" << endl;
    }

    array.push_back(j);

    ofstream file(filename, ofstream::trunc);
    if (!file.is_open()) {
        cerr << "Ошибка открытия файла для записи: " << filename << endl;
        return false;
    }
    file << array.dump(4);
    file.close();

    if (!file) {
        cerr << "Ошибка записи в файл" << endl;
        return false;
    }

    return true;
}

//функкии для работы с сервером
string read_file(const string& path) {
    ifstream file(path);
    if (!file.is_open()) {
        throw runtime_error("Could not open file");
    }
    return string((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
}
void recieveConnetion(const httplib::Request& req, httplib::Response& res) {
    try {
        auto json_data = json::parse(req.body);

        global_port = json_data["port"];
        global_speed = json_data["speed"];

    }
    catch (const json::exception& e) {
        res.status = 400;
        res.set_content("Invalid JSON format", "text/plain");
    }
    catch (const exception& e) {
        res.status = 500;
        res.set_content("Error: " + string(e.what()), "text/plain");
    }
}
void sendACSII(const httplib::Request& req, httplib::Response& res) {

    res.set_content(global_data, "text/plain");
}
void outPut(const httplib::Request& req, httplib::Response& res) {

    cout << "PLEASE WORK 2" << j_global << endl;
    res.set_content(j_global.dump(), "application/json");
}
void startServer() {
    

    // Обработка GET-запроса для корневого URL
    svr.Get("/", [](const httplib::Request& req, httplib::Response& res) {
        try {
            string html = read_file("D:/popitka/public/index.html");
            res.set_content(html, "text/html");
        }
        catch (const exception& e) {
            res.status = 500;
            res.set_content("Error: " + string(e.what()), "text/plain");
        }
        });

    // Обработка GET-запроса для index.js
    svr.Get("/bundle.js", [](const httplib::Request& req, httplib::Response& res) {
        try {
            string js = read_file("D:/popitka/dist/bundle.js");
            res.set_content(js, "application/javascript");
        }
        catch (const exception& e) {
            res.status = 500;
            res.set_content("Error: " + string(e.what()), "text/plain");
        }
        });

    svr.Post("/submit", recieveConnetion);
    svr.Get("/data", sendACSII);
    svr.Get("/data2", outPut);
    cout << "Сервер запущен..." << endl;  
    svr.listen("0.0.0.0", 8080);  

}

//функции для преобразованй типов
wstring stringToWstring(string& str) {
    return wstring(str.begin(), str.end());
}
DWORD convertStringToBaudRate(const std::string& speed_str) {
    try {
        int speed = std::stoi(speed_str);
        switch (speed) {
        case 110: return CBR_110;
        case 300: return CBR_300;
        case 600: return CBR_600;
        case 1200: return CBR_1200;
        case 2400: return CBR_2400;
        case 4800: return CBR_4800;
        case 9600: return CBR_9600;
        case 14400: return CBR_14400;
        case 19200: return CBR_19200;
        case 38400: return CBR_38400;
        case 57600: return CBR_57600;
        case 115200: return CBR_115200;
        default:
            throw std::invalid_argument("Unsupported baud rate");
        }
    }
    catch (const std::invalid_argument& e) {
        std::cerr << "Invalid baud rate: " << e.what() << std::endl;
        throw; // rethrow the exception
    }
}

int main() {

    setlocale(LC_ALL, "ru");
    vector<Wind> w;
    string choose;
    cout << "Введите 1, чтобы посмотреть json файл. \nВведите 2, чтобы принять данные с датчика" << endl;
    cin >> choose;
    if (choose == "1") {
        w = deserialization("puk.json");
        for (auto& i : w) {
            cout << "Sensor name: " << i.sensor_name << endl;
            cout << "Wind speed: " << i.wind_speed << endl;
            cout << "Wind direction: " << i.wind_direction << endl;
            cout << "Time :" << i.hour << ":" << i.minute << ":" << i.second << endl;
            cout << "_______________________" << endl;
        }
    }
    else if (choose == "2") {

        thread th(startServer);

        // LPCWSTR portName = L"COM3";
       // LPCWSTR portName = L"COM4";
        while (global_port.empty()) {                         //подождать пока пол-ль введет порт и скорость
            this_thread::sleep_for(chrono::milliseconds(100));
        }
        wstring wstr = stringToWstring(global_port);
        LPCWSTR portName = wstr.c_str();  //присоединиться к порту 

        HANDLE hSerial = CreateFile(
            portName,                 // Имя порта
            GENERIC_READ | GENERIC_WRITE, // Режим доступа
            0,                        // Открытие в режиме одиночного доступа
            NULL,                     // Без атрибутов безопасности
            OPEN_EXISTING,            // Открытие существующего порта
            0,                        // Флаги и атрибуты
            NULL                      // Без шаблона
        );


        if (hSerial == INVALID_HANDLE_VALUE) {

            DWORD dwError = GetLastError();
            cerr << "Ошибка при открытии порта COM4.  " << dwError << endl;
            return 1;
        }
        else {
            cout << "Подключение к порту COM4 успешно установлено." << endl;
        }
        // настройка параметров порта
        DCB dcbSerialParams = { 0 };
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
        if (!GetCommState(hSerial, &dcbSerialParams))
        {
            cerr << "Ошибка состояния\n";
        }
        // dcbSerialParams.BaudRate = CBR_9600;
        dcbSerialParams.BaudRate = convertStringToBaudRate(global_speed);  //скорость, к-ую указал пол-ль
        dcbSerialParams.ByteSize = 8;
        dcbSerialParams.StopBits = ONESTOPBIT;
        dcbSerialParams.Parity = NOPARITY;
        if (!SetCommState(hSerial, &dcbSerialParams))
        {
            cerr << "Ошибка установки состояния последовательного порта\n";
        }

        // настройка тайм-аутов чтения/записи
        COMMTIMEOUTS timeouts = { 0 };
        timeouts.ReadIntervalTimeout = 50;
        timeouts.ReadTotalTimeoutConstant = 50;
        timeouts.ReadTotalTimeoutMultiplier = 10;
        timeouts.WriteTotalTimeoutConstant = 50;
        timeouts.WriteTotalTimeoutMultiplier = 10;

        if (!SetCommTimeouts(hSerial, &timeouts)) {
            cerr << "Ошибка настройки тайм-аутов порта." << endl;
            CloseHandle(hSerial);
            return 1;
        }
        char szBuff[1024] = { 0 };
        DWORD dwBytesRead = 0;

        while (true) {

        wait:
            time_t mytime = time(NULL);
            struct tm now;
            localtime_s(&now, &mytime);

            //получение данных от датчика
            if (!ReadFile(hSerial, szBuff, sizeof(szBuff), &dwBytesRead, NULL)) {
                cerr << "Ошибка чтения из порта." << endl;
                CloseHandle(hSerial);
                return 1;
            }

            if (dwBytesRead != 0) {

                string data = szBuff;
                size_t caretka = data.find('\r');
                data = data.substr(0, caretka);

                global_data = data + "<r><n>";

                data.erase(remove_if(data.begin(), data.end(), [](char c) {
                    return !(isdigit(c) || c == ',' || c == '.'); // убрать все лишние знаки
                    }), data.end());
                string delimiter = ",";
                string ws = data.substr(0, data.find(delimiter));
                string wd = data.substr(data.find(delimiter) + delimiter.length());

                // string to float, чтобы в json записывалось как 9.0, а не 09.00
                locale::global(locale("C")); // чтобы . была разделителем в числе с плавающей точкой
                float ws_f = stof(ws);
                float wd_f = stof(wd);

                stringstream s;
                s << ws_f;
                string ws_s = s.str();
                s.clear();
                s.str("");
                s << wd_f;
                string wd_s = s.str();
                json j = toJSON(ws_s, wd_s, now);

                

                if (!saveToFile(j, "puk.json")) {
                    cerr << "Ошибка сохранения файла " << endl;
                }
                else {
                    cout << "Полученные данные сохранены в файл." << endl;
                }

                // для вывода на странице 
                ostringstream oss;
                oss << setw(2) << setfill('0') << now.tm_hour << ":"
                    << setw(2) << setfill('0') << now.tm_min << ":"
                    << setw(2) << setfill('0') << now.tm_sec;

                
                string time_str = oss.str();


                j_global["time"] = time_str;
                j_global["windSpeed"] = ws_s;
                j_global["windDirection"] = wd_s;
            }
            else {
                goto wait; //если не были прочитаны новые байты (когда статус тестера stopped(ничего не отпарвляется), на порт
                //все равно поступают значения, которые были переданы до стопа.
                th.join();

            }

        }
    }
    else {
        cout << "Неверный ввод" << endl;

        return 0;
    }

}
