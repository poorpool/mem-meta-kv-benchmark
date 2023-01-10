#include "utils.h"
#include <bits/stdint-intn.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
using std::cout;
using std::endl;
using std::ifstream;
using std::string;
using std::stringstream;

int64_t GetVmRssInB(int pid) {
  char buf[85];
  sprintf(buf, "/proc/%d/status", pid);
  ifstream is;
  is.open(buf);
  string str;

  while (getline(is, str)) {
    if (str.rfind("VmRSS", 0) != 0) {
      continue;
    }
    stringstream sst(str);
    string tmp;
    sst >> tmp;
    int64_t val;
    sst >> val;
    sst >> tmp;
    if (tmp == "kB") {
      val *= 1024;
    } else if (tmp == "MB") {
      val *= 1024;
      val *= 1024;
    } else if (tmp == "GB") {
      val *= 1024;
      val *= 1024;
      val *= 1024;
    } else {
      cout << "Unknown VmRSS unit " << tmp << endl;
      is.close();
      return 0;
    }
    is.close();
    return val;
  }

  is.close();
  cout << "Failed GetVmRssInB" << endl;
  return 0;
}