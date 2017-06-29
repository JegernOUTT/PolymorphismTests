#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>
#include <memory>
#include <range/v3/all.hpp>

#include "./staticpoly.hpp"

using namespace std;

template < typename User >
struct Operation
{
    void operator()(string_view message)
    {
        std::cout << "From " << User::name
                  << ": " << message << std::endl;
    }
};

struct Kolya
{
    static inline string name = "Kolya";
};
struct Sergey
{
    static inline string name = "Sergey";
};
struct Petya
{
    static inline string name = "Petya";
};


//template < typename T >
//concept bool WritableConcept = requires(T a)
//{
//    a.write(string_view{});
//};
//
//template <typename... Args> requires ((WritableConcept<Args>), ...)
//using WritableVariant = variant<Args...>;
//
//using Writable = WritableVariant<FileWriter, StreamWriter, SocketWriter>;


using OperationVariant = variant<Operation<Kolya>, Operation<Sergey>, Operation<Petya>>;

int main()
{
    vector<OperationVariant> operations;
    operations.emplace_back(Operation<Kolya> {});
    operations.emplace_back(Operation<Sergey>{});
    operations.emplace_back(Operation<Petya> {});
    operations.emplace_back(Operation<Sergey>{});
    operations.emplace_back(Operation<Kolya> {});
    operations.emplace_back(Operation<Petya> {});



    staticpoly::apply(operations.begin(), operations.end(),
                      [](auto& operation) { operation("Hello!"); });

    staticpoly::apply(operations.begin(), operations.end(),
                      [](Operation<Sergey> operation) { operation("Hello!"); });


//    staticpoly::apply(loggers.begin(), loggers.end(),
//                      [] (FileWriter& item)   { item.write("Write to file"); },
//                      [] (SocketWriter& item) { item.write("Write to sock"); });
//
//
//    vector<Writable> loggers_filtered;
//    staticpoly::filter(loggers.begin(), loggers.end(),
//                       back_inserter(loggers_filtered),
//                       [] (FileWriter&)   { return true; },
//                       [] (StreamWriter&) { return false; });
//
//
//    auto result = staticpoly::reduce(loggers.begin(), loggers.end(),
//                                     0,
//                                     [] (int acc, FileWriter& item)   { return item.a + acc; },
//                                     [] (int acc, StreamWriter& item) { return item.b + acc; });
//
//
//
//    vector<int> data;
//    staticpoly::map(loggers.begin(), loggers.end(),
//                    back_inserter(data),
//                    [] (FileWriter& item)   { return item.a; },
//                    [] (StreamWriter& item) { return item.b; },
//                    [] (SocketWriter& item) { return item.c; });
//
//    for (auto i: data)
//    {
//        std::cout << i << std::endl;
//    }


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


    return 0;
}