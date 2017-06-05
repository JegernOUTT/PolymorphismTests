#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>
#include <memory>
#include <range/v3/all.hpp>
#include <boost/asio.hpp>

#include "staticpoly"

using namespace std;
namespace ba = boost::asio;

struct FileWriter
{
    FileWriter(string_view filename) : file(filename.data()) { }

    void write(string_view data) noexcept
    {
        file << data;
    }

private:
    ofstream file;
};

struct StreamWriter
{
    StreamWriter(ostream& str) : stream(str) {}

    void write(string_view data)
    {
        stream << data;
    }
private:
    ostream& stream;
};

struct SocketWriter
{
    SocketWriter(ba::io_service& svc, string_view host, string_view port) : sock(svc)
    {
        ba::ip::tcp::resolver resolver(svc);
        ba::ip::tcp::resolver::iterator endpoint = resolver.resolve(
            ba::ip::tcp::resolver::query(host.data(), port.data()));
        ba::connect(sock, endpoint);
    }

    void write(string_view data)
    {
        sock.send(ba::buffer(string{ data }));
    }

private:
    ba::ip::tcp::socket sock;
};

template < typename T >
concept bool WritableConcept = requires(T a)
{
    a.write(string_view{});
};

template <typename... Args> requires ((WritableConcept<Args>), ...)
using WritableVariant = variant<Args...>;

using Writable = WritableVariant<FileWriter, StreamWriter, SocketWriter>;

int main()
{
    vector<Writable> loggers;
    loggers.reserve(1000);

    ba::io_service service;

    loggers.emplace_back(FileWriter { "test1.data" });
    loggers.emplace_back(FileWriter { "test2.data" });
    loggers.emplace_back(StreamWriter { cout });
    loggers.emplace_back(SocketWriter { service, "10.54.6.9", "5743" });
    loggers.emplace_back(SocketWriter { service, "10.54.6.9", "5744" });
    loggers.emplace_back(SocketWriter { service, "10.54.6.9", "5745" });
    loggers.emplace_back(FileWriter { "test3.data" });
    loggers.emplace_back(StreamWriter { cerr });

    thread th { [&service] { service.run(); } };

    staticpoly::apply(loggers.begin(), loggers.end(),
                      [](auto& item) { item.write("Hello"); });

    staticpoly::apply(loggers.begin(), loggers.end(),
                      [](FileWriter& item) { item.write("Write to file"); },
                      [](StreamWriter& item) { item.write("Write to stream"); },
                      [](SocketWriter& item) { item.write("Write to sock"); });


//    vector<Switchable> filtered;
//    staticpoly::filter(home_objects.begin(), home_objects.end(),
//                       back_inserter(filtered),
//                       [] (pair<int, Iron>& item) { return item.second.id > 50 and item.second.temperature == 500; },
//                       [] (pair<int, Plate>& item) { return item.second.id > 650 and item.second.plates_count == 4; });
//    cout << filtered.size() << endl;


//    staticpoly::apply(home_objects.begin(),
//                      home_objects.end(),
//                      [](auto& item) { item.SwitchOn(); });
//
//    staticpoly::apply(home_objects.begin(),
//                      home_objects.end(),
//                      [](auto item) { assert(item.isOn); });
//
//
//    auto result = staticpoly::reduce(home_objects.begin(), home_objects.end(),
//                                     0,
//                                     [] (auto acc, auto item) { return item.isOn ? ++acc : acc; });
//
//
//    vector<int> irons;
//    staticpoly::map(home_objects.begin(), home_objects.end(),
//                    back_inserter(irons),
//                    [] (Plate item) { return item.id; });
//
//    for (auto i: irons)
//    {
//        std::cout << i << std::endl;
//    }
//

//    for (auto i = 0; i < 1000; ++i)
//    {
//        auto start = chrono::system_clock::now();
//
//
//        auto stop = chrono::system_clock::now();
//
//        times.emplace_back(chrono::duration_cast<chrono::microseconds>(stop - start).count());
//    }
//
//    cout << accumulate(times.begin(), times.end(), 0) / 1000 << endl;


    th.join();
    return 0;
}