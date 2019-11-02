#include <iostream>

#include <thread>
#include <filesystem>
#include "reload/filesystem.h"

#include "solution.h"
#include "visual_studio.h"

#include "reload.h"

#include <cereal/types/unordered_map.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <fstream>

void test();

int main(int argc, char* argv[])
{
  test();
  //test4();
  //test3();
  //test2();
  //test();

  return 0;
}

struct Minimal
{
  std::string myData;
  std::string myData2;
  std::string myData3;

  template <class Archive>
  void serialize(Archive & ar)
  {
    ar(CEREAL_NVP(myData), CEREAL_NVP(myData3), CEREAL_NVP(myData2));
  }
};

struct Normal
{
  std::string myData;
  std::string myData2;
  std::string myData3;

  template <class Archive>
  void serialize(Archive & ar)
  {
    ar(CEREAL_NVP(myData));
  }
};

void test()
{
  std::stringstream ss;
  Minimal m = {"1", "2", "3"};
  Normal  n = {"4", "5", "6"};

  {
    cereal::JSONOutputArchive oar(ss);
    oar(CEREAL_NVP(m), CEREAL_NVP(n));

  }



  cereal::JSONInputArchive iar(ss);
  Minimal a;
  Normal b;

  iar(b);
  iar(a);

  ENABLE_HOT_RELOAD();
  //reload::runtime::current_project cp;
  //reload::build_target.initialize();
  reload::solution.initialize();
  reload::logger logger;
  reload::compiler.initialize(&logger);

  while (true)
  {
    reload::solution.update_source_tree();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
}

void test2()
{
  //  auto compiler = reload::compiler();
  //  compiler.initialize(wefwe);
}

#include <chrono>
#include <thread>
#include "solution.h"

/*
namespace std
{
using wall_clock = std::chrono::system_clock;
using wall_clock_high_res = std::chrono::high_resolution_clock;
using cpu_clock = std::clock_t;
}

std::clock_t startcputime = std::clock();
do_some_fancy_stuff();
double cpu_duration = (std::clock() - startcputime) / (double)CLOCKS_PER_SEC;
std::cout << "Finished in " << cpu_duration << " seconds [CPU Clock] " << std::endl;


auto wcts = std::chrono::system_clock::now();
do_some_fancy_stuff();
std::chrono::duration<double> wctduration = (std::chrono::system_clock::now() - wcts);
std::cout << "Finished in " << wctduration.count() << " seconds [Wall Clock]" << std::endl;
*/



void test4()
{

}
