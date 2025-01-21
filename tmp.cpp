#include <unordered_map>

// void basic(std::unordered_map<int, int> &map) {
//   map.at(0) = 1;
//   map.at(0) = 2;
// }

// void basic2(std::unordered_map<int, int> &map) {
//   if (map.count(0)) {
//     ++map[0];
//   }
// }

// void basic3(std::unordered_map<int, int> &map) {
//   if (map.count(0)) {
//     map.erase(0);
//   }

//   map.erase(map.begin());
// }

struct A {
  A();
  A(int);
  A(int, int);
  A(int, int, int);
  A(int, int, int, int);
};

void try_emplace() {
  std::unordered_map<int, A> map;
  map.try_emplace(0, 1);
  map.try_emplace(0, 1, 2);
  map.try_emplace(0, 1, 2, 3);
  map.try_emplace(0, 1, 2, 3, 4);
}
