// GSLAM - A general SLAM framework and benchmark
// Copyright 2018 PILAB Inc. All rights reserved.
// https://github.com/zdzhaoyong/GSLAM
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name of Google Inc. nor the names of its contributors may be
//   used to endorse or promote products derived from this software without
//   specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Author: zd5945@126.com (Yong Zhao) 353184965@qq.com(Guochen Liu)
//
// Svar: A light-weight, efficient, thread-safe parameter setting, dynamic
// variable sharing and command calling util class.
// Features:
// * Arguments parsing with help information
// * Support a very tiny script language with variable, function and condition
// * Thread-safe variable binding and sharing
// * Function binding and calling with Scommand
// * Support tree structure presentation, save&load with XML, JSON and YAML
// formats

#ifndef GSLAM_SVAR_H
#define GSLAM_SVAR_H
#include <assert.h>
#include <cxxabi.h>
#include <deque>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>
#include <typeinfo>
#include <vector>

#define svar GSLAM::Svar::instance()

namespace GSLAM {

namespace streamable_check {
typedef char yes;
typedef char (&no)[2];

struct anyx {
  template <class T>
  anyx(const T&);
};

no operator<<(const anyx&, const anyx&);
no operator>>(const anyx&, const anyx&);

template <class T>
yes check(T const&);
no check(no);

template <typename StreamType, typename T>
struct has_loading_support {
  static StreamType& stream;
  static T& x;
  static const bool value = sizeof(check(stream >> x)) == sizeof(yes);
};

template <typename StreamType, typename T>
struct has_saving_support {
  static StreamType& stream;
  static T& x;
  static const bool value = sizeof(check(stream << x)) == sizeof(yes);
};

template <typename StreamType, typename T>
struct has_stream_operators {
  static const bool can_load = has_loading_support<StreamType, T>::value;
  static const bool can_save = has_saving_support<StreamType, T>::value;
  static const bool value = can_load && can_save;
};
}

class Svar;
class SvarHolder;
template <typename VarType = void*, typename KeyType = std::string>
class SvarMapHolder;
template <typename VarType = Svar>
class SvarVecHolder;
class SvarLanguage;
template <typename VarType = void*, typename KeyType = std::string>
using SvarWithType = SvarMapHolder<VarType, KeyType>;
typedef SvarLanguage Scommand;

/** The class Svar will be shared in the same process, it help users to
 transform paraments use a name id,
 all paraments with a same name will got the same data. One can change it in all
 threads from assigned var,
 files and stream.
 */
class Svar {
 public:
  typedef std::map<std::string, std::string> SvarMap;

  struct ArgumentInfo {
    std::string type, def, introduction;
    friend std::ostream& operator<<(std::ostream& ost,
                                    const ArgumentInfo& arg) {
      ost << Svar::typeName(arg.type) << "(" << arg.def << ")";
    }
  };

 public:
  Svar();
  ~Svar();

  /** This gives us singletons instance. \sa enter */
  static Svar& instance();

  template <typename T,
            typename std::enable_if<!std::is_array<T>::value, int>::type = 0>
  void Set(const std::string& name, const T& def, bool overwrite = true);

  template <size_t N>
  void Set(const std::string& name, const char (&def)[N],
           bool overwrite = true) {
    return Set<std::string>(name, def);
  }

  template <class T,
            typename std::enable_if<
                streamable_check::has_loading_support<std::istream, T>::value,
                int>::type = 0>
  T& Get(const std::string& name, T def = T());

  template <class T,
            typename std::enable_if<
                !streamable_check::has_loading_support<std::istream, T>::value,
                int>::type = 0>
  T& Get(const std::string& name, T def = T());

  template <class T>
  T& operator()(const std::string& name, T def = T()) {
    return Get<T>(name, def);
  }

  bool empty() const;

  template <class T = std::string>
  bool exist(const std::string& name);

  template <class T = std::string>
  bool erase(const std::string& name);

  /** \brief clear Svar data
*/
  template <class T = void>
  void clear();

  template <class T>
  SvarMapHolder<T>& as() {
    return *holder<SvarMapHolder<T>>();
  }

  template <class T>
  std::shared_ptr<T> holder(bool create = true);

  int& GetInt(const std::string& name, int defaut = 0);
  double& GetDouble(const std::string& name, double defaut = 0);
  std::string& GetString(const std::string& name,
                         const std::string& defaut = "");
  void*& GetPointer(const std::string& name, const void* p = NULL);

  /// Argument parsing related things
  std::vector<std::string> ParseMain(int argc, char** argv);

  template <typename T>
  T& Arg(const std::string& name, T def, const std::string& info);

  std::string help();

  void setUsage(const std::string& usage) { GetString("Usage") = usage; }

  /// Svar language related things
  SvarLanguage& language();
  bool ParseLine(std::string s, bool bSilentFailure = false);
  bool ParseStream(std::istream& is);
  bool ParseFile(std::string sFileName);

  Svar GetChild(const std::string& name);
  void AddChild(const std::string& name, const Svar& child);
  std::list<std::pair<std::string, Svar>> Children();

  /** \brief other utils
 */
  std::string getStatsAsText(const std::string& topic = "Svar",
                             const size_t column_width = 80);
  void dumpAllVars();
  bool save(std::string filename = "");

  template <typename T>
  static std::string toString(const T& v);

  template <typename T>
  static T fromString(const std::string& str);

  static std::string getFolderPath(const std::string& path);
  static std::string getBaseName(const std::string& path);
  static std::string getFileName(const std::string& path);
  static std::string typeName(std::string name);
  static std::string printTable(std::vector<std::pair<int, std::string>> line);
  static bool fileExists(const std::string& filename);

  bool operator==(const Svar& b) { return holders == b.holders; }

 private:
  std::shared_ptr<SvarMapHolder<std::shared_ptr<SvarHolder>>> holders;
};  // end of class Svar

class SvarHolder {
 public:
  virtual ~SvarHolder() {}
  virtual void clear() {}
  virtual std::string getStatsAsText(const size_t column_width = 80) = 0;
};


/**@ingroup gInterface
 @brief The class Svar will be shared in the same process, it help users to
 transform paraments use a name id,
 all paraments with a same name will got the same data. One can change it in all
 threads from assigned var,
 files and stream.
 */
template <typename VarType, typename KeyType>
class SvarMapHolder : public SvarHolder {
  friend class Svar;

 public:
  typedef std::map<KeyType, VarType> DataMap;
  typedef typename DataMap::iterator DataIter;
  typedef std::pair<DataIter, bool> InsertRet;

 public:
  SvarMapHolder() {}

  /** This gives us singletons instance. \sa enter */
  static SvarMapHolder& instance() {
    static std::shared_ptr<SvarMapHolder> inst(new SvarMapHolder());
    return *inst;
  }

  bool empty() const { return data.empty(); }

  inline bool exist(const KeyType& name) {
    std::unique_lock<std::mutex> lock(mMutex);
    return data.find(name) != data.end();
  }

  inline bool erase(const KeyType& name) {
    std::unique_lock<std::mutex> lock(mMutex);
    data.erase(name);
    return true;
  }

  virtual inline void clear() {
    std::unique_lock<std::mutex> lock(mMutex);
    data.clear();
  }

  /** This insert a named var to the map,you can overwrite or not if the var has
 * exist. \sa enter
*/
  inline bool insert(const KeyType& name, const VarType& var,
                     bool overwrite = false) {
    std::unique_lock<std::mutex> lock(mMutex);
    DataIter it;
    it = data.find(name);
    if (it == data.end()) {
      data.insert(std::pair<std::string, VarType>(name, var));
      return true;
    } else {
      if (overwrite) it->second = var;
      return false;
    }
  }

  /** function get_ptr() returns the pointer of the map or the var pointer when
 * name is supplied,
 * when the var didn't exist,it will return NULL or insert the default var and
 * return the var pointer in the map
 */
  inline DataMap* get_ptr() { return &data; }

  inline DataMap get_data() {
    std::unique_lock<std::mutex> lock(mMutex);
    return data;
  }

  inline VarType* get_ptr(const KeyType& name) {
    std::unique_lock<std::mutex> lock(mMutex);
    DataIter it;
    it = data.find(name);
    if (it == data.end()) {
      return NULL;
    } else {
      return &(it->second);
    }
  }

  inline VarType* get_ptr(const KeyType& name, const VarType& def) {
    std::unique_lock<std::mutex> lock(mMutex);
    DataIter it;
    it = data.find(name);
    if (it == data.end()) {
      InsertRet ret = data.insert(std::pair<KeyType, VarType>(name, def));
      if (ret.second)
        return &(ret.first->second);
      else
        return NULL;
    } else {
      return &(it->second);
    }
  }

  /** function get_var() return the value found in the map,\sa enter.
 */
  inline VarType get_var(const KeyType& name, const VarType& def) {
    std::unique_lock<std::mutex> lock(mMutex);
    DataIter it;
    it = data.find(name);
    if (it == data.end()) {
      InsertRet ret = data.insert(std::pair<KeyType, VarType>(name, def));
      if (ret.second)
        return (ret.first->second);
      else
        return def;
    } else {
      return it->second;
    }
  }

  /** this function can be used to assign or get the var use corrospond name,\sa
 * enter.
 */
  inline VarType& operator[](const KeyType& name) {
    std::unique_lock<std::mutex> lock(mMutex);
    DataIter it;
    it = data.find(name);
    if (it == data.end()) {
      while (1) {
        InsertRet ret =
            data.insert(std::pair<KeyType, VarType>(name, VarType()));
        if (ret.second) return (ret.first->second);
      }
      //            else return def;//UNSAFE!!!!!
    } else {
      return it->second;
    }
  }

  template <typename STREAM>
  void dump(STREAM& stream, const size_t column_width = 80,
            typename std::enable_if<streamable_check::has_saving_support<
                STREAM, VarType>::value>::type* = 0) {
    std::unique_lock<std::mutex> lock(mMutex);

    for (DataIter it = data.begin(); it != data.end(); it++)
      stream << Svar::printTable(
          {{column_width / 2 - 1, Svar::toString(it->first)},
           {column_width / 2, Svar::toString(it->second)}});
  }

  template <typename STREAM>
  void dump(STREAM& stream, const size_t column_width = 80,
            typename std::enable_if<!streamable_check::has_saving_support<
                STREAM, VarType>::value>::type* = 0) {
    std::unique_lock<std::mutex> lock(mMutex);

    for (DataIter it = data.begin(); it != data.end(); it++)
      stream << Svar::printTable(
          {{column_width / 2 - 1, Svar::toString(it->first)},
           {column_width / 2, "Unstreamable"}});
  }

  virtual std::string getStatsAsText(const size_t column_width = 80) {
    std::ostringstream ost;
    std::string key_name = Svar::typeName(typeid(KeyType).name());
    std::string type_name = Svar::typeName(typeid(VarType).name());
    type_name = "map<" + key_name + "," + type_name + ">";
    int gap = std::max<int>((column_width - type_name.size()) / 2 - 1, 0);
    for (int i = 0; i < gap; i++) ost << '-';
    ost << type_name;
    for (int i = 0; i < gap; i++) ost << '-';
    ost << std::endl;
    dump(ost, column_width);
    return ost.str();
  }

 protected:
  DataMap data;
  std::mutex mMutex;
};  // end of class SvarWithType

template <typename VarType>
class SvarVecHolder : public SvarHolder {
 public:
  SvarVecHolder() : _impl(new Data()) {}

  VarType& operator[](const size_t& idx) {
    std::unique_lock<std::mutex> lock(_impl->_mutex);
    return _impl->_data[idx];
  }

  size_t size() const { return _impl->_data.size(); }

  std::vector<VarType> toVec() {
    std::unique_lock<std::mutex> lock(_impl->_mutex);
    return _impl->_data;
  }

  void push_back(const VarType& var) {
    std::unique_lock<std::mutex> lock(_impl->_mutex);
    _impl->_data.push_back(var);
  }

  virtual void clear() {
    std::unique_lock<std::mutex> lock(_impl->_mutex);
    _impl->_data.clear();
  }

  template <typename STREAM>
  void dump(STREAM& stream, const size_t column_width = 80,
            typename std::enable_if<streamable_check::has_saving_support<
                STREAM, VarType>::value>::type* = 0) {
    std::string type_name = Svar::typeName(typeid(VarType).name());
    type_name = "vector<" + type_name + ">";
    int gap = std::max<int>((column_width - type_name.size()) / 2 - 1, 0);
    for (int i = 0; i < gap; i++) stream << '-';
    stream << type_name;
    for (int i = 0; i < gap; i++) stream << '-';
    stream << std::endl;
    std::unique_lock<std::mutex> lock(_impl->_mutex);
    for (auto it = _impl->_data.begin(); it != _impl->_data.end(); it++)
      stream << (it == _impl->_data.begin() ? "[" : "") << Svar::toString(*it)
             << (it + 1 == _impl->_data.end() ? "]" : ",");
  }

  template <typename STREAM>
  void dump(STREAM& stream, const size_t column_width = 80,
            typename std::enable_if<!streamable_check::has_saving_support<
                STREAM, VarType>::value>::type* = 0) {
    std::string type_name = Svar::typeName(typeid(VarType).name());
    type_name = "vector<" + type_name + ">";
    int gap = std::max<int>((column_width - type_name.size()) / 2 - 1, 0);
    for (int i = 0; i < gap; i++) stream << '-';
    stream << type_name;
    for (int i = 0; i < gap; i++) stream << '-';
    stream << std::endl;
  }

  virtual std::string getStatsAsText(const size_t column_width = 80) {
    std::ostringstream ost;
    dump(ost, column_width);
    return ost.str();
  }

  friend std::ostream& operator<<(std::ostream& ost, const SvarVecHolder& hd) {
    ost << hd.size();
  }

 protected:
  struct Data {
    std::vector<VarType> _data;
    std::string _name;
    std::mutex _mutex;
  };
  std::shared_ptr<Data> _impl;
};

inline Svar::Svar()
    : holders(new SvarMapHolder<std::shared_ptr<SvarHolder>>()) {}

inline Svar::~Svar() {}

inline Svar& Svar::instance() {
  static std::shared_ptr<Svar> global_svar(new Svar);
  return *global_svar;
}

template <typename T,
          typename std::enable_if<!std::is_array<T>::value, int>::type>
void Svar::Set(const std::string& name, const T& def, bool overwrite) {
  auto idx = name.find_last_of(".");
  if (idx != std::string::npos) {
    return GetChild(name.substr(0, idx))
        .Set(name.substr(idx + 1), def, overwrite);
  }
  as<T>().insert(name, def, overwrite);
}

template <class T>
inline void Svar::clear() {
  holders->erase(typeid(SvarMapHolder<T>).name());
}

template <>
inline void Svar::clear<void>() {
  holders->clear();
}

template <class T>
inline bool Svar::erase(const std::string& name) {
  auto idx = name.find_last_of(".");
  if (idx != std::string::npos) {
    return GetChild(name.substr(0, idx)).as<T>().erase(name.substr(idx + 1));
  }
  as<T>().erase(name);
  return true;
}

template <class T>
inline bool Svar::exist(const std::string& name) {
  auto idx = name.find_last_of(".");
  if (idx != std::string::npos) {
    return GetChild(name.substr(0, idx)).as<T>().exist(name.substr(idx + 1));
  }
  return as<T>().exist(name);
}

template <class T>
std::shared_ptr<T> Svar::holder(bool create) {
  auto ptr = holders->get_ptr(typeid(T).name());
  if (ptr) return std::dynamic_pointer_cast<T>(*ptr);
  if (!create) return std::shared_ptr<T>();
  auto hd = holders->get_var(typeid(T).name(), std::shared_ptr<T>(new T()));
  return std::dynamic_pointer_cast<T>(hd);
}

template <class T,
          typename std::enable_if<
              streamable_check::has_loading_support<std::istream, T>::value,
              int>::type>
T& Svar::Get(const std::string& name, T def) {
  auto idx = name.find(".");
  if (idx != std::string::npos) {
    return GetChild(name.substr(0, idx)).Get(name.substr(idx + 1), def);
  }
  // First: Use the var from SvarWithType, this is a fast operation
  SvarMapHolder<T>& typed_map = as<T>();

  T* ptr = typed_map.get_ptr(name);
  if (ptr) return *ptr;
  auto strHolder = holder<SvarWithType<std::string>>(false);
  std::string envStr;
  if (strHolder && strHolder->exist(name))
    envStr = (*strHolder)[name];
  else if (char* envcstr = getenv(name.c_str()))
    envStr = envcstr;
  if (!envStr.empty())  // Second: Use the var from Svar
  {
    def = fromString<T>(envStr);
  }

  ptr = typed_map.get_ptr(name, def);
  return *ptr;
}

template <class T,
          typename std::enable_if<
              !streamable_check::has_loading_support<std::istream, T>::value,
              int>::type>
T& Svar::Get(const std::string& name, T def) {
  auto idx = name.find(".");
  if (idx != std::string::npos) {
    return GetChild(name.substr(0, idx)).Get(name.substr(idx + 1), def);
  }

  T* ptr = as<T>().get_ptr(name, def);
  return *ptr;
}

template <>
inline void*& Svar::Get(const std::string& name, void* def) {
  return GetPointer(name, def);
}

inline bool Svar::fileExists(const std::string& filename) {
  std::ifstream f(filename.c_str());
  return f.good();
}

inline std::string Svar::getFolderPath(const std::string& path) {
  auto idx = std::string::npos;
  if ((idx = path.find_last_of('/')) == std::string::npos)
    idx = path.find_last_of('\\');
  if (idx != std::string::npos)
    return path.substr(0, idx);
  else
    return "";
}

inline std::string Svar::getBaseName(const std::string& path) {
  std::string filename = getFileName(path);
  auto idx = filename.find_last_of('.');
  if (idx == std::string::npos)
    return filename;
  else
    return filename.substr(0, idx);
}

inline std::string Svar::getFileName(const std::string& path) {
  auto idx = std::string::npos;
  if ((idx = path.find_last_of('/')) == std::string::npos)
    idx = path.find_last_of('\\');
  if (idx != std::string::npos)
    return path.substr(idx + 1);
  else
    return path;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline std::string Svar::toString(const T& def) {
  std::ostringstream sst;
  sst << def;
  return sst.str();
}

template <>
inline std::string Svar::toString(const std::string& def) {
  return def;
}

template <>
inline std::string Svar::toString(const double& def) {
  using namespace std;
  ostringstream ost;
  ost << setiosflags(ios::fixed) << setprecision(12) << def;
  return ost.str();
}

template <>
inline std::string Svar::toString(const bool& def) {
  return def ? "true" : "false";
}

template <typename T>
inline T Svar::fromString(const std::string& str) {
  std::istringstream sst(str);
  T def;
  try {
    sst >> def;
  } catch (std::exception e) {
    std::cerr << "Failed to read value from " << str << std::endl;
  }
  return def;
}

template <>
inline std::string Svar::fromString(const std::string& str) {
  return str;
}

template <>
inline bool Svar::fromString<bool>(const std::string& str) {
  if (str.empty()) return false;
  if (str == "0") return false;
  if (str == "false") return false;
  return true;
}

inline std::string Svar::typeName(std::string name) {
  static std::map<std::string, std::string> decode = {
      {typeid(int32_t).name(), "int32_t"},
      {typeid(int64_t).name(), "int64_t"},
      {typeid(uint32_t).name(), "uint32_t"},
      {typeid(uint64_t).name(), "uint64_t"},
      {typeid(u_char).name(), "u_char"},
      {typeid(char).name(), "char"},
      {typeid(float).name(), "float"},
      {typeid(double).name(), "double"},
      {typeid(std::string).name(), "string"},
      {typeid(bool).name(), "bool"},
  };
  auto it = decode.find(name);
  if (it != decode.end()) return it->second;

  int status;
  char* realname = abi::__cxa_demangle(name.c_str(), 0, 0, &status);
  std::string result(realname);
  free(realname);
  return result;
}

inline bool Svar::empty() const { return holders->empty(); }

inline std::string Svar::help() {
  std::stringstream sst;
  int width = GetInt("COLUMNS", 80);
  int namePartWidth = width / 5 - 1;
  int statusPartWidth = width * 2 / 5 - 1;
  int introPartWidth = width * 2 / 5;
  std::string usage = GetString("Usage", "");
  if (usage.empty()) {
    usage = "Usage:\n" + GetString("ProgramName", "exe") +
            " [--help] [-conf configure_file]"
            " [-arg_name arg_value]...\n";
  }
  sst << usage << std::endl;

  std::string desc;
  if (!GetString("Version").empty())
    desc += "Version: " + GetString("Version") + ", ";
  desc +=
      "Using Svar supported argument parsing. The following table listed "
      "several argument introductions.\n";
  sst << printTable({{width, desc}});

  Arg<std::string>("conf", "Default.cfg",
                   "The default configure file going to parse.");
  Arg<bool>("help", false, "Show the help information.");

  auto& inst = as<ArgumentInfo>();
  sst << printTable({{namePartWidth, "Argument"},
                     {statusPartWidth, "Type(default->setted)"},
                     {introPartWidth, "Introduction"}});
  for (int i = 0; i < width; i++) sst << "-";
  sst << std::endl;

  for (const auto& it : inst.get_data()) {
    ArgumentInfo info = it.second;
    std::string setted = Get<std::string>(it.first);
    if (setted != info.def)
      setted = "->" + setted;
    else
      setted.clear();
    std::string name = "-" + it.first;
    std::string status = typeName(info.type) + "(" + info.def + setted + ")";
    std::string intro = info.introduction;
    sst << printTable({{namePartWidth, name},
                       {statusPartWidth, status},
                       {introPartWidth, intro}});
  }
  return sst.str();
}

inline std::string Svar::printTable(
    std::vector<std::pair<int, std::string>> line) {
  std::stringstream sst;
  while (true) {
    size_t emptyCount = 0;
    for (auto& it : line) {
      size_t width = it.first;
      std::string& str = it.second;
      if (str.size() <= width) {
        sst << std::setw(width) << std::setiosflags(std::ios::left) << str
            << " ";
        str.clear();
        emptyCount++;
      } else {
        sst << str.substr(0, width) << " ";
        str = str.substr(width);
      }
    }
    sst << std::endl;
    if (emptyCount == line.size()) break;
  }
  return sst.str();
}

inline Svar Svar::GetChild(const std::string& name) {
  auto idx = name.find('.');
  if (idx != std::string::npos) {
    return GetChild(name.substr(0, idx)).GetChild(name.substr(idx + 1));
  }

  return Get<Svar>(name);
}

inline void Svar::AddChild(const std::string& name, const Svar& child) {
  auto& childMap = as<Svar>();
  if (childMap.insert(name, child)) return;

  SvarVecHolder<Svar>& hd = Get<SvarVecHolder<Svar>>(name);
  if (hd.size() == 0) {
    hd.push_back(childMap[name]);
  }
  hd.push_back(child);
}

inline std::list<std::pair<std::string, Svar>> Svar::Children() {
  std::list<std::pair<std::string, Svar>> result;
  auto childMap = as<Svar>().get_data();
  result.insert(result.end(), childMap.begin(), childMap.end());
  auto hd = holder<SvarWithType<SvarVecHolder<Svar>>>(false);
  if (!hd) return result;
  for (auto it : hd->get_data()) {
    for (auto vit : it.second.toVec()) {
      result.push_back(std::make_pair(it.first, vit));
    }
  }
  return result;
}

inline int& Svar::GetInt(const std::string& name, int def) {
  return Get<int>(name, def);
}

inline double& Svar::GetDouble(const std::string& name, double def) {
  return Get<double>(name, def);
}

inline std::string& Svar::GetString(const std::string& name,
                                    const std::string& def) {
  return Get<std::string>(name, def);
}

inline void*& Svar::GetPointer(const std::string& name, const void* ptr) {
  return *(as<void*>().get_ptr(name, (void*)ptr));
}

inline bool Svar::save(std::string filename) {
  using namespace std;
  if (filename.size() == 0) filename = GetString("Config_File", "Default.cfg");
  ofstream ofs(filename.c_str());
  if (!ofs.is_open()) return false;

  SvarMap data_copy;
  { data_copy = as<std::string>().get_data(); }
  for (auto it = data_copy.begin(); it != data_copy.end(); it++) {
    ofs << it->first << " = " << it->second << endl;
  }
  return true;
}

inline std::string Svar::getStatsAsText(const std::string& topic,
                                        const size_t column_width) {
  std::ostringstream ost;
  int name_width = column_width / 2 - 1;
  int value_width = column_width / 2;

  int gap = std::max<int>((column_width - topic.size()) / 2 - 1, 0);
  for (int i = 0; i < gap; i++) ost << '=';
  ost << topic;
  for (int i = 0; i < gap; i++) ost << '=';
  ost << std::endl;
  ost << printTable({{name_width, "Name"}, {value_width, "Value"}});
  for (std::pair<std::string, std::shared_ptr<SvarHolder>> h :
       holders->get_data()) {
    std::string str = h.second->getStatsAsText(column_width);
    ost << str;
  }

  for (auto child : Children()) {
    ost << child.second.getStatsAsText(topic + "." + child.first, column_width);
  }

  return ost.str();
}

inline void Svar::dumpAllVars() {
  using namespace std;
  cout << endl << getStatsAsText();
}

class SvarLanguage : public SvarHolder {
  typedef std::function<void(std::string sCommand, std::string sParams)>
      CallbackProc;
  typedef std::vector<CallbackProc> CallbackVector;

 public:
  SvarLanguage(Svar var) : svar_(var) {
    using namespace std::placeholders;
    RegisterCommand("include",
                    [&](std::string sParams) { svar_.ParseFile(sParams); }, _2);
    RegisterCommand("parse",
                    [&](std::string sParams) { svar_.ParseFile(sParams); }, _2);
    RegisterCommand(
        "echo", [&](std::string sParams) { std::cout << sParams << std::endl; },
        _2);
    RegisterCommand("GetInt",
                    [&](std::string name, std::string sParams) {
                      svar_.Set(name, Svar::toString(svar_.Get<int>(sParams)));
                    },
                    _1, _2);
    RegisterCommand("GetDouble",
                    [&](std::string name, std::string sParams) {
                      svar_.Set(name,
                                Svar::toString(svar_.Get<double>(sParams)));
                    },
                    _1, _2);
    RegisterCommand("system",
                    [&](std::string sParams) {
                      svar_.Set("System.Result",
                                Svar::toString(system(sParams.c_str())));
                    },
                    _2);

    RegisterCommand(".", &SvarLanguage::collect_line, this, _1, _2);
    RegisterCommand("function", &SvarLanguage::function, this, _1, _2);
    RegisterCommand("endfunction", &SvarLanguage::endfunction, this, _1, _2);
    RegisterCommand("if", &SvarLanguage::gui_if_equal, this, _1, _2);
    RegisterCommand("else", &SvarLanguage::gui_if_else, this, _1, _2);
    RegisterCommand("endif", &SvarLanguage::gui_endif, this, _1, _2);
  }

  template <typename Func, typename... Args>
  inline void RegisterCommand(std::string sCommandName, Func&& func,
                              Args&&... args) {
    data.insert(sCommandName, std::bind(std::forward<Func>(func),
                                        std::forward<Args>(args)...));
  }

  inline void UnRegisterCommand(std::string sCommandName) {
    data.erase(sCommandName);
  }

  bool Call(std::string sCommand, std::string sParams) {
    if (!data.exist(sCommand)) {
      return false;
    }

    CallbackProc& calls = data[sCommand];
    calls(sCommand, sParams);
    return true;
  }

  /** split the command and paraments from a string
   * eg:Call("shell ls"); equal Call("shell","ls");
   */
  bool Call(const std::string& sCommand) {
    size_t found = sCommand.find_first_of(" ");
    if (found < sCommand.size())
      return Call(sCommand.substr(0, found), sCommand.substr(found + 1));
    else
      return Call(sCommand, "");
  }

  virtual void clear() {
    data.clear();
    language.reset();
  }

  virtual std::string getStatsAsText(const size_t column_width = 80) {
    return data.getStatsAsText(column_width);
  }

  inline bool ParseLine(std::string s, bool bSilentFailure = false) {
    using namespace std;
    if (s == "") return 0;
    if (collectFlag) {
      istringstream ist(s);
      string sCommand;
      ist >> sCommand;
      if (sCommand == "endif" || sCommand == "fi") Call("endif");
      if (sCommand == "else")
        Call("else");
      else if (sCommand == "endfunction")
        Call("endfunction");
      else if (sCommand == ".")
        Call(".", ist.str());
      else
        Call(".", s);
      return 0;
    }
    s = UncommentString(expandVal(s, '{'));
    s = UncommentString(expandVal(s, '('));
    if (s == "") return 0;

    // Old ParseLine code follows, here no braces can be left (unless in arg.)
    istringstream ist(s);

    string sCommand;
    string sParams;

    // Get the first token (the command name)
    ist >> sCommand;
    if (sCommand == "") return 0;

    // Get everything else (the arguments)...

    // Remove any whitespace
    ist >> ws;
    getline(ist, sParams);

    // Attempt to execute command
    if (Call(sCommand, sParams)) return true;

    if (setvar(s)) return 1;

    if (!bSilentFailure)
      cerr << "? GUI_impl::ParseLine: Unknown command \"" << sCommand
           << "\" or invalid assignment." << endl;
    return 0;
  }

  inline bool ParseStream(std::istream& is) {
    using namespace std;
    svar_.Set("Svar.ParsingPath", Svar::getFolderPath(parsingFile), true);
    svar_.Set("Svar.ParsingName", Svar::getBaseName(parsingFile), true);
    svar_.Set("Svar.ParsingFile", parsingFile, true);
    string buffer;
    int& shouldParse = svar_.GetInt("Svar.NoReturn", 1);
    while (getline(is, buffer) && shouldParse) {
      // Lines ending with '\' are taken as continuing on the next line.
      while (!buffer.empty() && buffer[buffer.length() - 1] == '\\') {
        string buffer2;
        if (!getline(is, buffer2)) break;
        buffer = buffer.substr(0, buffer.length() - 1) + buffer2;
      }
      ParseLine(buffer);
    }
    shouldParse = 1;
    return true;
  }

  inline bool ParseFile(std::string sFileName) {
    using namespace std;

    if (!Svar::fileExists(sFileName)) return false;

    auto idx = sFileName.find_last_of('.');
    if (idx == std::string::npos) return false;
    std::string ext = sFileName.substr(idx + 1);
    if (ext != "cfg") {
      typedef std::function<void(std::string file, Svar var)> SvarLoadFunc;
      if (svar.as<SvarLoadFunc>().exist(ext)) {
        svar.Get<SvarLoadFunc>(ext)(sFileName, svar_);
      }
      else std::cerr<<"Svar does't have format \""<<ext<<"\" support."
       " Please include format support header such as <GSLAM/core/XML.h>.\n";
    }
    ifstream ifs(sFileName.c_str());

    if (!ifs.is_open()) {
      return false;
    }

    std::string current_tid = svar_.toString(std::this_thread::get_id());
    std::string& parsing_tid = svar_.GetString("Svar.ParsingThreadId");
    assert(current_tid == parsing_tid || parsing_tid.empty());

    fileQueue.push_back(sFileName);
    parsingFile = sFileName;

    bool ret = ParseStream(ifs);
    ifs.close();

    //    cout<<"Finished parsing "<<fileQueue.back();
    fileQueue.pop_back();
    if (fileQueue.size()) {
      parsingFile = fileQueue.back();
      svar_.Set("Svar.ParsingPath", Svar::getFolderPath(parsingFile), true);
      svar_.Set("Svar.ParsingName", Svar::getFileName(parsingFile), true);
      svar_.Set("Svar.ParsingFile", parsingFile, true);
    } else {
      svar_.erase("Svar.ParsingName");
      svar_.erase("Svar.ParsingPath");
      svar_.erase("Svar.ParsingFile");
      parsingFile.clear();
      parsing_tid.clear();
    }
    return ret;
  }

  /**
   = overwrite
  ?= don't overwrite
  */
  inline bool setvar(std::string s) {
    using namespace std;
    // Execution failed. Maybe its an assignment.
    string::size_type n;
    n = s.find("=");
    bool shouldOverwrite = true;

    if (n != string::npos) {
      string var = s.substr(0, n);
      string val = s.substr(n + 1);

      // Strip whitespace from around var;
      string::size_type s = 0, e = var.length() - 1;
      if ('?' == var[e]) {
        e--;
        shouldOverwrite = false;
      }
      for (; isspace(var[s]) && s < var.length(); s++) {
      }
      if (s == var.length())  // All whitespace before the `='?
        return false;
      for (; isspace(var[e]); e--) {
      }
      if (e >= s) {
        var = var.substr(s, e - s + 1);

        // Strip whitespace from around val;
        s = 0, e = val.length() - 1;
        for (; isspace(val[s]) && s < val.length(); s++) {
        }
        if (s < val.length()) {
          for (; isspace(val[e]); e--) {
          }
          val = val.substr(s, e - s + 1);
        } else
          val = "";

        svar_.Set(var, val, shouldOverwrite);
        return true;
      }
    }

    return false;
  }

 private:
  inline std::string expandVal(std::string val, char flag) {
    using namespace std;
    string expanded = val;

    while (true) {
      const char* brace = FirstOpenBrace(expanded.c_str(), flag);
      if (brace) {
        const char* endbrace = MatchingEndBrace(brace, flag);
        if (endbrace) {
          ostringstream oss;
          oss << std::string(expanded.c_str(), brace - 1);

          const string inexpand =
              expandVal(std::string(brace + 1, endbrace), flag);
          if (svar_.exist(inexpand)) {
            oss << svar_.Get<std::string>(inexpand);
          } else {
            printf(
                "Unabled to expand: [%s].\nMake sure it is defined and "
                "terminated with a semi-colon.\n",
                inexpand.c_str());
            oss << "#";
          }

          oss << std::string(endbrace + 1,
                             expanded.c_str() + expanded.length());
          expanded = oss.str();
          continue;
        }
      }
      break;
    }

    return expanded;
  }

  static std::string Trim(const std::string& str,
                          const std::string& delimiters) {
    const size_t f = str.find_first_not_of(delimiters);
    return f == std::string::npos
               ? ""
               : str.substr(f, str.find_last_not_of(delimiters) + 1);
  }

  // Find the open brace preceeded by '$'
  static const char* FirstOpenBrace(const char* str, char flag) {
    bool symbol = false;

    for (; *str != '\0'; ++str) {
      if (*str == '$') {
        symbol = true;
      } else {
        if (symbol) {
          if (*str == flag) {
            return str;
          } else {
            symbol = false;
          }
        }
      }
    }
    return 0;
  }

  // Find the first matching end brace. str includes open brace
  static const char* MatchingEndBrace(const char* str, char flag) {
    char endflag = '}';
    if (flag == '(')
      endflag = ')';
    else if (flag == '[')
      endflag = ']';
    int b = 0;
    for (; *str != '\0'; ++str) {
      if (*str == flag) {
        ++b;
      } else if (*str == endflag) {
        --b;
        if (b == 0) {
          return str;
        }
      }
    }
    return 0;
  }

  static std::vector<std::string> ChopAndUnquoteString(std::string s) {
    using namespace std;
    vector<string> v;
    string::size_type nPos = 0;
    string::size_type nLength = s.length();
    while (1) {
      string sTarget;
      char cDelim;
      // Get rid of leading whitespace:
      while ((nPos < nLength) && (s[nPos] == ' ')) nPos++;
      if (nPos == nLength) return v;

      // First non-whitespace char...
      if (s[nPos] != '\"')
        cDelim = ' ';
      else {
        cDelim = '\"';
        nPos++;
      }
      for (; nPos < nLength; ++nPos) {
        char c = s[nPos];
        if (c == cDelim) break;
        if (cDelim == '"' && nPos + 1 < nLength && c == '\\') {
          char escaped = s[++nPos];
          switch (escaped) {
            case 'n':
              c = '\n';
              break;
            case 'r':
              c = '\r';
              break;
            case 't':
              c = '\t';
              break;
            default:
              c = escaped;
              break;
          }
        }
        sTarget += c;
      }
      v.push_back(sTarget);

      if (cDelim == '\"') nPos++;
    }
  }

  static std::string::size_type FindCloseBrace(const std::string& s,
                                               std::string::size_type start,
                                               char op, char cl) {
    using namespace std;
    string::size_type open = 1;
    string::size_type i;

    for (i = start + 1; i < s.size(); i++) {
      if (s[i] == op)
        open++;
      else if (s[i] == cl)
        open--;

      if (open == 0) break;
    }

    if (i == s.size()) i = s.npos;
    return i;
  }

  inline static std::string UncommentString(std::string s) {
    using namespace std;
    int q = 0;

    for (string::size_type n = 0; n < s.size(); n++) {
      if (s[n] == '"') q = !q;

      if (s[n] == '/' && !q) {
        if (n < s.size() - 1 && s[n + 1] == '/') return s.substr(0, n);
      }
    }

    return s;
  }

  void collect_line(std::string name, std::string args) {
    (void)name;
    collection.push_back(args);
  }

  void function(std::string name, std::string args) {
    using namespace std;

    collectFlag++;
    vector<string> vs = ChopAndUnquoteString(args);
    if (vs.size() != 1) {
      cerr << "Error: " << name << " takes 1 argument: " << name << " name\n";
      return;
    }

    current_function = vs[0];
    collection.clear();
  }

  void endfunction(std::string name, std::string args) {
    using namespace std::placeholders;
    using namespace std;
    collectFlag--;
    if (current_function == "") {
      cerr << "Error: " << name << ": no current function.\n";
      return;
    }

    vector<string> vs = ChopAndUnquoteString(args);
    if (vs.size() != 0) cerr << "Warning: " << name << " takes 0 arguments.\n";

    functions.insert(current_function, collection, true);

    RegisterCommand(current_function, &SvarLanguage::runfunction, this, _1, _2);

    current_function.clear();
    collection.clear();
  }

  void runfunction(std::string name, std::string args) {
    (void)args;
    using namespace std;
    vector<string>& v = *functions.get_ptr(name, vector<string>());
    for (unsigned int i = 0; i < v.size(); i++) svar_.ParseLine(v[i]);
  }

  void gui_if_equal(std::string name, std::string args) {
    (void)name;
    using namespace std;
    string& s = args;
    collectFlag++;
    bool is_equal = false;
    string::size_type n;
    n = s.find("=");
    if (n != string::npos) {
      string left = s.substr(0, n);
      string right = s.substr(n + 1);
      // Strip whitespace from around left;
      string::size_type s = 0, e = left.length() - 1;
      if ('!' == left[e]) {
        //                        cout<<"Found !"<<endl;
        e--;
        is_equal = true;
      }
      for (; isspace(left[s]) && s < left.length(); s++) {
      }
      if (s == left.length())  // All whitespace before the `='?
        left = "";
      else
        for (; isspace(left[e]); e--) {
        }
      if (e >= s) {
        left = left.substr(s, e - s + 1);
      } else
        left = "";

      // Strip whitespace from around val;
      s = 0, e = right.length() - 1;
      for (; isspace(right[s]) && s < right.length(); s++) {
      }
      if (s < right.length()) {
        for (; isspace(right[e]); e--) {
        }
        right = right.substr(s, e - s + 1);
      } else
        right = "";

      //                    cout<<"Found
      //                    =,Left:-"<<left<<"-,Right:-"<<right<<"-\n";

      if (left == right) is_equal = !is_equal;
    } else if (s != "") {
      is_equal = true;
    }

    collection.clear();
    if (is_equal)
      if_gvar = "";
    else
      if_gvar = "n";
    if_string = "";
  }

  void gui_if_else(std::string name, std::string args) {
    (void)name;
    (void)args;
    using namespace std;
    ifbit = collection;
    if (ifbit.empty()) ifbit.push_back("");
    collection.clear();
  }

  void gui_endif(std::string name, std::string args) {
    (void)name;
    (void)args;
    using namespace std;
    collectFlag--;
    if (ifbit.empty())
      ifbit = collection;
    else
      elsebit = collection;

    collection.clear();

    // Save a copy, since it canget trashed
    vector<string> ib = ifbit, eb = elsebit;
    string gv = if_gvar, st = if_string;

    ifbit.clear();
    elsebit.clear();
    if_gvar.clear();
    if_string.clear();
    //                cout<<"SvarName="<<gv<<",Value="<<svar.GetString(gv,"")<<",Test="<<st<<endl;
    if (gv == st)
      for (unsigned int i = 0; i < ib.size(); i++) svar_.ParseLine(ib[i]);
    else
      for (unsigned int i = 0; i < eb.size(); i++) svar_.ParseLine(eb[i]);
  }

 protected:
  Svar svar_;
  SvarMapHolder<CallbackProc> data;
  std::shared_ptr<SvarLanguage> language;

 private:
  std::string current_function;
  std::string if_gvar, if_string;
  std::vector<std::string> collection, ifbit, elsebit;
  SvarMapHolder<std::vector<std::string>> functions;

  std::string parsingFile;
  std::deque<std::string> fileQueue;
  int collectFlag = 0;
};

inline SvarLanguage& Svar::language() {
  auto& hd = (*holders)[typeid(SvarLanguage).name()];
  if (!hd) hd = std::shared_ptr<SvarLanguage>(new SvarLanguage(*this));
  return *std::dynamic_pointer_cast<SvarLanguage>(hd);
}

inline bool Svar::ParseLine(std::string s, bool bSilentFailure) {
  return language().ParseLine(s, bSilentFailure);
}

inline bool Svar::ParseStream(std::istream& is) {
  return language().ParseStream(is);
}

inline bool Svar::ParseFile(std::string sFileName) {
  return language().ParseFile(sFileName);
}

template <typename T>
T& Svar::Arg(const std::string& name, T def, const std::string& help) {
  std::string str = toString<T>(def);
  if (!str.empty()) Set(name, str, false);
  auto& args = as<ArgumentInfo>();
  ArgumentInfo& argInfo = args[name];
  argInfo.introduction = help;
  argInfo.type = typeid(T).name();
  argInfo.def = str;
  return Get<T>(name, def);
}

inline std::vector<std::string> Svar::ParseMain(int argc, char** argv) {
  using namespace std;
  // save main cmd things
  GetInt("argc") = argc;
  GetPointer("argv", NULL) = argv;
  // SvarWithType<char**>::instance()["argv"] = argv;

  // parse main cmd
  std::vector<std::string> unParsed;
  int beginIdx = 1;
  for (int i = beginIdx; i < argc; i++) {
    string str = argv[i];
    bool foundPrefix = false;
    size_t j = 0;
    for (; j < 2 && j < str.size() && str.at(j) == '-'; j++) foundPrefix = true;

    if (!foundPrefix) {
      if (!language().setvar(str)) unParsed.push_back(str);
      continue;
    }

    str = str.substr(j);
    if (str.find('=') != string::npos) {
      language().setvar(str);
      continue;
    }

    if (i + 1 >= argc) {
      Set(str, "true", true);
      continue;
    }
    string str2 = argv[i + 1];
    if (str2.front() == '-') {
      Set(str, "true", true);
      continue;
    }

    i++;
    Set<std::string>(str, argv[i]);
    continue;
  }

  // parse default config file
  string argv0 = argv[0];
  Set("argv0", argv0);
  Set("ProgramPath", getFolderPath(argv0));
  Set("ProgramName", getFileName(argv0));

  if (fileExists(argv0 + ".cfg")) ParseFile(argv0 + ".cfg");

  argv0 = GetString("conf", "./Default.cfg");
  if (fileExists(argv0)) ParseFile(argv0);

  return unParsed;
}
}

#endif
