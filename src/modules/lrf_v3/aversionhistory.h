#ifndef AVERSIONHISTORY_H
#define AVERSIONHISTORY_H

#include <map>
#include <string>

template<typename T>
class AVersionHistory {
  std::map<std::string, const T> versions;
  const T *latest = nullptr;
public:
  bool isEmpty() const { return versions.empty(); }
  const std::map<std::string, const T> &getAll() const { return versions; }
  const T *getLatest() const { return latest; }
  const T *get(const std::string &version_name) const {
    auto it = versions.find(version_name);
    return it != versions.end() ? &it->second : nullptr;
  }

  const std::string &append(const T &&new_version)
  {
    size_t i = versions.size();
    std::string name = "Version "+std::to_string(i);
    while(versions.find(name) != versions.end())
      name = "Version "+std::to_string(++i);
    const auto &key_val = versions.emplace(name, new_version).first;
    latest = &(key_val->second);
    return key_val->first;
  }

  bool remove(const std::string &version_name)
  {
    auto it = versions.find(version_name);
    if(it != versions.end()) {
      versions.erase(it);
      return true;
    }
    return false;
  }

  bool rename(const std::string &old_name, const std::string &new_name)
  {
    auto old_it = versions.find(old_name);
    if(old_it == versions.end()) //old must exist
      return false;
    auto new_it = versions.find(new_name);
    if(new_it != versions.end()) //new must not exist
      return false;
    //move old to new
    versions.emplace(new_name, std::move(old_it->second));
    versions.erase(old_it);
    return true;
  }

};

#endif // AVERSIONHISTORY_H
