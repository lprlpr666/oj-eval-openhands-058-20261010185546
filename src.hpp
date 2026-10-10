#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <set>
#include <cctype>

// Basic exception hierarchy
class BasicException {
protected:
    const char *message;
public:
    explicit BasicException(const char *_message) : message(_message) {}
    virtual const char *what() const { return message; }
    virtual ~BasicException() = default;
};

class ArgumentException : public BasicException {
public:
    explicit ArgumentException(const char *msg) : BasicException(msg) {}
};

class IteratorException : public BasicException {
public:
    explicit IteratorException(const char *msg) : BasicException(msg) {}
};

struct Pokemon {
    std::string name;
    int id;
    std::vector<std::string> types;
};

class Pokedex {
private:
    std::string filename;
    // store by id for quick lookup, also maintain order by id
    std::map<int, Pokemon> data; // sorted by id ascending
    // helper to split types string
    static std::vector<std::string> splitTypes(const std::string &s) {
        std::vector<std::string> res;
        std::stringstream ss(s);
        std::string item;
        while (std::getline(ss, item, '#')) {
            if (!item.empty()) res.push_back(item);
        }
        return res;
    }
    static std::string joinTypes(const std::vector<std::string> &v) {
        std::string out;
        for (size_t i = 0; i < v.size(); ++i) {
            out += v[i];
            if (i + 1 < v.size()) out += '#';
        }
        return out;
    }
    // type effectiveness table for the 7 types used in this problem
    // attacker -> defender -> multiplier
    static double typeEffect(const std::string &atk, const std::string &def) {
        static const std::unordered_map<std::string, std::unordered_map<std::string, double>> table = {
            {"water", {{"fire", 2.0}, {"water", 0.5}, {"grass", 0.5}, {"electric", 1.0}, {"ground", 2.0}, {"flying", 1.0}, {"dragon", 1.0}}},
            {"fire", {{"grass", 2.0}, {"water", 0.5}, {"fire", 0.5}, {"electric", 1.0}, {"ground", 1.0}, {"flying", 1.0}, {"dragon", 0.5}}},
            {"grass", {{"water", 2.0}, {"fire", 0.5}, {"grass", 0.5}, {"electric", 1.0}, {"ground", 2.0}, {"flying", 0.5}, {"dragon", 0.5}}},
            {"electric", {{"water", 2.0}, {"grass", 0.5}, {"electric", 0.5}, {"ground", 0.0}, {"flying", 2.0}, {"fire", 1.0}, {"dragon", 1.0}}},
            {"ground", {{"electric", 2.0}, {"fire", 2.0}, {"grass", 0.5}, {"water", 1.0}, {"ground", 1.0}, {"flying", 0.0}, {"dragon", 1.0}}},
            {"flying", {{"grass", 2.0}, {"electric", 0.5}, {"ground", 1.0}, {"water", 1.0}, {"fire", 1.0}, {"flying", 0.5}, {"dragon", 1.0}}},
            {"dragon", {{"dragon", 2.0}, {"water", 1.0}, {"fire", 1.0}, {"grass", 1.0}, {"electric", 1.0}, {"ground", 1.0}, {"flying", 1.0}}}
        };
        auto itA = table.find(atk);
        if (itA != table.end()) {
            auto itD = itA->second.find(def);
            if (itD != itA->second.end()) return itD->second;
        }
        return 1.0; // default neutral
    }
    // validate name: only letters, length <=10, non-empty
    static void validateName(const std::string &name) {
        if (name.empty() || name.size() > 10) {
            throw ArgumentException((std::string("Argument Error: PM Name Invalid (") + name + ")").c_str());
        }
        for (char c : name) {
            if (!std::isalpha(static_cast<unsigned char>(c))) {
                throw ArgumentException((std::string("Argument Error: PM Name Invalid (") + name + ")").c_str());
            }
        }
    }
    // validate types string: each type only letters, length <=10, at least one, at most 7
    static std::vector<std::string> validateAndParseTypes(const std::string &typesStr) {
        auto vec = splitTypes(typesStr);
        if (vec.empty() || vec.size() > 7) {
            throw ArgumentException((std::string("Argument Error: PM Type Invalid (") + (vec.empty()? "" : vec[0]) + ")").c_str());
        }
        for (const auto &t : vec) {
            if (t.empty() || t.size() > 10) {
                throw ArgumentException((std::string("Argument Error: PM Type Invalid (") + t + ")").c_str());
            }
            for (char c : t) {
                if (!std::isalpha(static_cast<unsigned char>(c))) {
                    throw ArgumentException((std::string("Argument Error: PM Type Invalid (") + t + ")").c_str());
                }
            }
        }
        return vec;
    }
    // load from file
    void load() {
        std::ifstream fin(filename);
        if (!fin) return; // file may not exist
        std::string line;
        while (std::getline(fin, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string name; int id; std::string typesStr;
            ss >> name >> id >> typesStr;
            if (ss.fail()) continue;
            Pokemon p; p.name = name; p.id = id; p.types = splitTypes(typesStr);
            data[id] = std::move(p);
        }
    }
    // save to file
    void save() const {
        std::ofstream fout(filename, std::ios::trunc);
        for (const auto &kv : data) {
            const Pokemon &p = kv.second;
            fout << p.name << ' ' << p.id << ' ' << joinTypes(p.types) << '\n';
        }
    }
public:
    explicit Pokedex(const char *_fileName) : filename(_fileName) {
        load();
    }
    ~Pokedex() {
        save();
    }
    bool pokeAdd(const char *name_c, int id, const char *types_c) {
        std::string name(name_c);
        std::string typesStr(types_c);
        // validation
        try {
            validateName(name);
            auto typesVec = validateAndParseTypes(typesStr);
            // check duplicates
            if (data.count(id) || std::any_of(data.begin(), data.end(), [&](const auto &kv){return kv.second.name == name;})) {
                return false;
            }
            Pokemon p; p.name = name; p.id = id; p.types = std::move(typesVec);
            data[id] = std::move(p);
            return true;
        } catch (const ArgumentException &) {
            throw; // propagate
        }
    }
    bool pokeDel(int id) {
        auto it = data.find(id);
        if (it == data.end()) return false;
        data.erase(it);
        return true;
    }
    std::string pokeFind(int id) const {
        auto it = data.find(id);
        if (it == data.end()) return "None";
        return it->second.name;
    }
    std::string typeFind(const char *types_c) const {
        std::string typesStr(types_c);
        std::vector<std::string> queryTypes;
        try {
            queryTypes = validateAndParseTypes(typesStr);
        } catch (const ArgumentException &) {
            throw;
        }
        std::vector<const Pokemon*> matches;
        for (const auto &kv : data) {
            const Pokemon &p = kv.second;
            bool ok = true;
            for (const auto &qt : queryTypes) {
                if (std::find(p.types.begin(), p.types.end(), qt) == p.types.end()) { ok = false; break; }
            }
            if (ok) matches.push_back(&p);
        }
        if (matches.empty()) return "None";
        // sort by id ascending (data already sorted, but ensure)
        std::sort(matches.begin(), matches.end(), [](const Pokemon *a, const Pokemon *b){return a->id < b->id;});
        std::ostringstream out;
        out << matches.size() << '\n';
        for (size_t i = 0; i < matches.size(); ++i) {
            out << matches[i]->name;
            if (i + 1 < matches.size()) out << '\n';
        }
        return out.str();
    }
    float attack(const char *type_c, int id) const {
        std::string atk(type_c);
        auto it = data.find(id);
        if (it == data.end()) return -1.0f;
        const Pokemon &def = it->second;
        double mult = 1.0;
        for (const auto &defType : def.types) {
            mult *= typeEffect(atk, defType);
        }
        return static_cast<float>(mult);
    }
    int catchTry() const {
        if (data.empty()) return 0;
        // start with smallest id pokemon owned
        std::set<int> ownedIds;
        auto itStart = data.begin();
        ownedIds.insert(itStart->first);
        bool progress = true;
        while (progress) {
            progress = false;
            for (const auto &kv : data) {
                int pid = kv.first;
                if (ownedIds.count(pid)) continue;
                const Pokemon &target = kv.second;
                // check if any owned pokemon can deal >=2x
                bool can = false;
                for (int oid : ownedIds) {
                    const Pokemon &owner = data.at(oid);
                    // owner can use any of its types as attack
                    for (const auto &atkType : owner.types) {
                        double mult = 1.0;
                        for (const auto &defType : target.types) {
                            mult *= typeEffect(atkType, defType);
                        }
                        if (mult >= 2.0 - 1e-9) { can = true; break; }
                    }
                    if (can) break;
                }
                if (can) {
                    ownedIds.insert(pid);
                    progress = true;
                }
            }
        }
        return static_cast<int>(ownedIds.size());
    }
    // iterator implementation
    class iterator {
    private:
        const Pokedex *parent;
        std::map<int, Pokemon>::const_iterator it;
    public:
        iterator(const Pokedex *p, std::map<int, Pokemon>::const_iterator i) : parent(p), it(i) {}
        iterator &operator++() { ++it; return *this; }
        iterator operator++(int) { iterator tmp = *this; ++it; return tmp; }
        iterator &operator--() { --it; return *this; }
        iterator operator--(int) { iterator tmp = *this; --it; return tmp; }
        iterator &operator=(const iterator &rhs) { parent = rhs.parent; it = rhs.it; return *this; }
        bool operator==(const iterator &rhs) const { return it == rhs.it; }
        bool operator!=(const iterator &rhs) const { return it != rhs.it; }
        Pokemon & operator*() const { return const_cast<Pokemon&>(it->second); }
        Pokemon * operator->() const { return const_cast<Pokemon*>(&it->second); }
    };
    iterator begin() { return iterator(this, data.begin()); }
    iterator end() { return iterator(this, data.end()); }
};
