#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <range/v3/all.hpp>

using namespace std;


struct HomeObject
{
    int id;
    string name;
    bool isOn;

    virtual void SwitchOn() = 0;
    virtual void SwitchOff() = 0;

    virtual ~HomeObject() {}
};

struct Iron : public HomeObject
{
    void SwitchOn() override
    {
//        cout << "Iron is on" << endl;
        isOn = true;
    }

    void SwitchOff() override
    {
//        cout << "Iron is off" << endl;
        isOn = false;
    }
};

struct Plate : public HomeObject
{
    void SwitchOn() override
    {
//        cout << "Plate is on" << endl;
        isOn = true;
    }

    void SwitchOff() override
    {
//        cout << "Plate is off" << endl;
        isOn = false;
    }
};


int main()
{
    using HomeObjectPtr = shared_ptr<HomeObject>;

    std::srand(unsigned(std::time(0)));
    vector<uint64_t> times = {};

    vector<HomeObjectPtr> home_objects;
    home_objects.reserve(1000000);

    for (int i = 0; i < 1000000; ++i)
    {
        if (std::rand() % 2 == 1)
            home_objects.emplace_back(make_shared<Iron>());
        else
            home_objects.emplace_back(make_shared<Plate>());
    }

    for (auto i = 0; i < 1000; ++i)
    {
        auto start = chrono::system_clock::now();

        for_each(home_objects.begin(),
                 home_objects.end(),
                 [](HomeObjectPtr obj_ptr) { obj_ptr->SwitchOn(); });

        auto stop = chrono::system_clock::now();

        times.emplace_back(chrono::duration_cast<chrono::microseconds>(stop - start).count());
    }

    cout << accumulate(times.begin(), times.end(), 0) / 1000 << endl;

    return 0;
}